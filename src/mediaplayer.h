#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "audioWidget_sdl2.h"
#include "public.h"
#include "videoWidget_gl.h"
// resource management

struct AudioChunk {
    std::vector<uint8_t> pcm;
    size_t offset { 0 };
};
struct VideoFrame {
    int width { 0 };
    int height { 0 };
    int linesizeY { 0 };
    int linesizeU { 0 };
    int linesizeV { 0 };
    std::vector<uint8_t> y;
    std::vector<uint8_t> u;
    std::vector<uint8_t> v;
};

struct AudioBufferQueue {
    std::queue<AudioChunk> bufferredPcm;
    SDL_AudioDeviceID device;
};
struct VideoBufferQueue {
    std::queue<VideoFrame> bufferredFrm;
};

class MediaPlayer {
private:
    // media
    MediaInfo mediaInfo;
    PtrSet ptrSet;
    MyResampler myResampler;
    MyAudioWidget audioWidget;
    MyGLWidget videowidget;
    AudioBufferQueue chunkQueue;

    int frames { 0 };
    auto outputInfo(MediaInfo& pmediaInfo) noexcept {
        std::cout << "File Path: " << pmediaInfo.filePath << "\n"
                  << "Stream [" << pmediaInfo.vsIndex << "] Video\n"
                  << "____resolution: " << pmediaInfo.resolution[0] << "x"
                  << pmediaInfo.resolution[1] << "\n"
                  << "____duration: " << pmediaInfo.vduration << "s\n"
                  << "____decoder: " << pmediaInfo.vdecoderName << "\n"
                  << "____pixel_format: " << pmediaInfo.pxFmtName
                  << "(depth:" << pmediaInfo.pxFmtDpth << ")\n"
                  << "Stream [" << pmediaInfo.asIndex << "] Audio\n"
                  << "____samplerate: " << (float)pmediaInfo.splRate / 1000 << "kHz\n"
                  << "____duration: " << pmediaInfo.aduration << "s\n"
                  << "____decoder: " << pmediaInfo.adecoderName << "\n"
                  << "____channel_layout: "
                  << (pmediaInfo.channelLayoutName[0] != '\0'
                             ? pmediaInfo.channelLayoutName
                             : std::to_string(pmediaInfo.channels).c_str())
                  << "\n"
                  << "____sample_format: " << pmediaInfo.splFmtName << "\n"
                  << std::flush;
    };
    auto demux(const char* pfilePath) {
        auto pFormatCtx = avformat_alloc_context();
        if (avformat_open_input(&pFormatCtx, pfilePath, nullptr, nullptr) != 0) {
            std::cerr << "Can't open file\n" << std::flush;
        }
        if (avformat_find_stream_info(pFormatCtx, nullptr) != 0) {
            std::cerr << "Can't find video stream info\n" << std::flush;
        }
        for (int i = 0; i < pFormatCtx->nb_streams; i++) {
            const auto cdcPar   = pFormatCtx->streams[i]->codecpar;
            const auto* decoder = avcodec_find_decoder(cdcPar->codec_id);
            if (decoder == nullptr) {
                std::cerr << "Can't find supported decoder\n" << std::flush;
            }
            auto containerDuration = (float)pFormatCtx->duration;

            auto base = pFormatCtx->streams[i]->time_base.den;

            auto duration = (float)pFormatCtx->streams[i]->duration / base;

            if (duration <= 0.0f) {
                duration = containerDuration / 1000 / base;
            };
            if (cdcPar->codec_type == AVMEDIA_TYPE_AUDIO) {
                auto decCtx = avcodec_alloc_context3(decoder);
                avcodec_parameters_to_context(decCtx, cdcPar);
                avcodec_open2(decCtx, decoder, nullptr);
                ptrSet.audioDecCtx.reset(decCtx);
                mediaInfo.asIndex      = i;
                mediaInfo.adecoderName = const_cast<char*>(decoder->name);
                mediaInfo.atimeBase    = av_q2d(pFormatCtx->streams[i]->time_base);
                mediaInfo.aduration    = duration;
                mediaInfo.sampleFmt    = ptrSet.audioDecCtx->sample_fmt;
                mediaInfo.splDepth     = av_get_bytes_per_sample(mediaInfo.sampleFmt);
                mediaInfo.splFmtName =
                    const_cast<char*>(av_get_sample_fmt_name(mediaInfo.sampleFmt));
                mediaInfo.splRate       = (float)cdcPar->sample_rate;
                mediaInfo.channelLayout = cdcPar->ch_layout;
                mediaInfo.channels      = cdcPar->ch_layout.nb_channels;
                av_channel_layout_describe(&cdcPar->ch_layout, mediaInfo.channelLayoutName,
                    sizeof(mediaInfo.channelLayoutName));
                std::cout << "Audio Stream Info Get\n";
            } else if (cdcPar->codec_type == AVMEDIA_TYPE_VIDEO) {
                auto decCtx = avcodec_alloc_context3(decoder);
                avcodec_parameters_to_context(decCtx, cdcPar);
                avcodec_open2(decCtx, decoder, nullptr);
                ptrSet.videoDecCtx.reset(decCtx);
                mediaInfo.vsIndex       = i;
                mediaInfo.vdecoderName  = const_cast<char*>(decoder->name);
                mediaInfo.vtimeBase     = av_q2d(pFormatCtx->streams[i]->time_base);
                mediaInfo.resolution[0] = cdcPar->width;
                mediaInfo.resolution[1] = cdcPar->height;
                mediaInfo.vduration     = duration;
                mediaInfo.pxFmt         = ptrSet.videoDecCtx->pix_fmt;
                mediaInfo.pxFmtName     = const_cast<char*>(av_get_pix_fmt_name(mediaInfo.pxFmt));
                const AVPixFmtDescriptor* pDesc = av_pix_fmt_desc_get(mediaInfo.pxFmt);
                mediaInfo.pxFmtDpth             = pDesc->comp[1].depth;
                std::cout << "Video Stream Info Get\n";
            }
        }
        mediaInfo.filePath = const_cast<char*>(pfilePath);
        ptrSet.formatCtx.reset(pFormatCtx);
        audioWidget.copyParameters(mediaInfo);
        auto swrCtx = myResampler.alcSwrCtx(mediaInfo.channelLayout, mediaInfo.splRate,
            mediaInfo.sampleFmt, myResampler.toPackedFmt(mediaInfo.sampleFmt));
        ptrSet.audioRsplCtx.reset(swrCtx);
        auto pvdecFrm = av_frame_alloc();
        auto padecFrm = av_frame_alloc();
        auto pvmyFrm  = av_frame_alloc();
        auto pamyFrm  = av_frame_alloc();
        auto ppaket   = av_packet_alloc();
        ptrSet.vdecFrm.reset(pvdecFrm);
        ptrSet.vmyFrm.reset(pvmyFrm);
        ptrSet.adecFrm.reset(padecFrm);
        ptrSet.amyFrm.reset(pamyFrm);
        ptrSet.paket.reset(ppaket);
        std::cout << "Contexts allocated\n";
        outputInfo(mediaInfo);
    };
    bool rdFrm(AVFormatContext* pFormatCtx, AVPacket* ppkt) const noexcept {
        return av_read_frame(pFormatCtx, ppkt) >= 0;
    }

    auto decodePkt(AVCodecContext* pcdCtx, AVPacket* ppkt) const noexcept {
        avcodec_send_packet(pcdCtx, ppkt);
        av_packet_unref(ppkt);
    }

    bool rcvFrm(AVCodecContext* pcdCtx, AVFrame* pdecFrame) const noexcept {
        return avcodec_receive_frame(pcdCtx, pdecFrame) >= 0;
    }

    auto cpyFrm(AVFrame* pdecFrame, AVFrame* pmyFrame) const noexcept {
        av_frame_ref(pmyFrame, pdecFrame);
        // av_frame_unref(decFrame);
    }

    auto decodeLoop(MediaInfo& pmediaInfo, PtrSet& pptrSet) {
        auto swrCtx = ptrSet.audioRsplCtx.get();
        swr_init(swrCtx);
        while (rdFrm(pptrSet.formatCtx.get(), pptrSet.paket.get())) {
            if (pptrSet.paket->stream_index == pmediaInfo.vsIndex) {
                decodePkt(pptrSet.videoDecCtx.get(), pptrSet.paket.get());
                while (rcvFrm(pptrSet.videoDecCtx.get(), pptrSet.vdecFrm.get())) {
                    cpyFrm(pptrSet.vdecFrm.get(), pptrSet.vmyFrm.get());
                    auto y = pptrSet.vmyFrm.get()->data[0];
                    auto u = pptrSet.vmyFrm.get()->data[1];
                    auto v = pptrSet.vmyFrm.get()->data[2];
                }
            }
            if (pptrSet.paket->stream_index == pmediaInfo.asIndex) {
                decodePkt(pptrSet.audioDecCtx.get(), pptrSet.paket.get());
                while (rcvFrm(pptrSet.audioDecCtx.get(), pptrSet.adecFrm.get())) {
                    cpyFrm(pptrSet.adecFrm.get(), pptrSet.amyFrm.get());
                    AudioChunk chunk { };
                    auto frame      = pptrSet.amyFrm.get();
                    auto outBytes   = av_samples_get_buffer_size(nullptr, mediaInfo.channels,
                        frame->nb_samples, myResampler.toPackedFmt(mediaInfo.sampleFmt), 1);
                    auto outSamples = swr_get_delay(swrCtx, mediaInfo.splRate) + frame->nb_samples;
                    chunk.pcm.resize(outBytes);
                    uint8_t* out[1] = { chunk.pcm.data() };
                    if (pptrSet.amyFrm.get()->data[1] != nullptr) {
                        auto convertedSamples = swr_convert(swrCtx, out, outSamples,
                            (const uint8_t**)pptrSet.amyFrm->data, frame->nb_samples);
                        auto convertedBytes =
                            av_samples_get_buffer_size(nullptr, mediaInfo.channels,
                                convertedSamples, myResampler.toPackedFmt(mediaInfo.sampleFmt), 1);
                        outBytes = convertedBytes;
                    } else {
                        chunk.pcm.insert(
                            chunk.pcm.end(), chunk.pcm.data(), chunk.pcm.data() + outBytes);
                    }

                    chunkQueue.bufferredPcm.push(std::move(chunk));
                }
            }
            ++frames;
        }
        std::cout << "Decoding Completed\n";
        std::cout << "Buffer stored:" << chunkQueue.bufferredPcm.size() << "chunks\n";
    }
    auto playLoop(AudioBufferQueue& pqueue) {
        auto& pdevice = pqueue.device;
        pdevice       = std::move(audioWidget.audioDevice);
        if (pdevice < 0) {
            std::cerr << "SDL_AudioDevice Initialization Failed" << std::flush;
            return;
        }
        while (!pqueue.bufferredPcm.empty()) {
            auto& outFrame = pqueue.bufferredPcm.front();
            SDL_QueueAudio(pdevice, outFrame.pcm.data(), (Uint32)outFrame.pcm.size());
            pqueue.bufferredPcm.pop();
        }
        while (SDL_GetQueuedAudioSize(pdevice) > 0)
            SDL_Delay(10);
    }

    auto threadManage() { }

public:
    MediaPlayer() { videowidget.show(); };
    ~MediaPlayer() { std::cout << "Total Frames:" << frames << std::endl; };
    void open(const char* pfile) {
        demux(pfile);
        decodeLoop(mediaInfo, ptrSet);
        playLoop(chunkQueue);
    }
};