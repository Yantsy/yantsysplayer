#pragma once
#include "public.h"
#include "resampler.h"
#include <thread>
class DemuxerPlusDecoder : public QObject {
    Q_OBJECT;
signals:
    void frameReady(std::shared_ptr<VideoFrame> videoFrame);
    void chunkReady(std::shared_ptr<AudioChunk> audioChunk);
    void sendVideoInfo(VideoInfo videoInfo);
    void sendAudioInfo(AudioInfo audioInfo, void* buffer);
    void durationChanged(double duration);
    void finished();

private:
    std::string filePath;
    Clock audioClk, videoClk, exClk;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point update;
    int count { 0 };
    bool jump { false };
    double newTime { 0.0 };
    PtrSet ptrSet;
    VPtrSet& vPtrs { ptrSet.vPtrs };
    APtrSet& aPtrs { ptrSet.aPtrs };
    MediaInfo mediaInfo;
    ChunkQueue chunks;
    MyResampler myResampler;
    AudioInfo& audioInfo { mediaInfo.audioInfo };
    VideoInfo& videoInfo { mediaInfo.videoInfo };
    auto outputInfo(MediaInfo mediaInfo) {
        std::cout << "File Path: " << mediaInfo.filePath << "\n"
                  << "Stream [" << mediaInfo.videoInfo.vsIndex << "] Video\n"
                  << "____resolution: " << mediaInfo.videoInfo.resolution[0] << "x"
                  << mediaInfo.videoInfo.resolution[1] << "\n"
                  << "____duration: " << mediaInfo.videoInfo.vduration << "s\n"
                  << "____decoder: " << mediaInfo.videoInfo.vdecoderName << "\n"
                  << "____pixel_format: " << mediaInfo.videoInfo.pxFmtName
                  << "(depth:" << mediaInfo.videoInfo.pxFmtDpth << ")\n"
                  << "Stream [" << mediaInfo.audioInfo.asIndex << "] Audio\n"
                  << "____samplerate: " << (float)mediaInfo.audioInfo.splRate / 1000 << "kHz\n"
                  << "____duration: " << mediaInfo.audioInfo.aduration << "s\n"
                  << "____decoder: " << mediaInfo.audioInfo.adecoderName << "\n"
                  << "____channel_layout: "
                  << (mediaInfo.audioInfo.channelLayoutName[0] != '\0'
                             ? mediaInfo.audioInfo.channelLayoutName
                             : std::to_string(mediaInfo.audioInfo.channels).c_str())
                  << "\n"
                  << "____sample_format: " << mediaInfo.audioInfo.splFmtName << "\n"
                  << std::flush;
        std::cout << " " << std::endl;
    };
    auto demux(std::string filePath) {
        auto pFormatCtx = avformat_alloc_context();
        auto file       = filePath.c_str();
        if (avformat_open_input(&pFormatCtx, file, nullptr, nullptr) != 0) {
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
                aPtrs.decoderCtx.reset(decCtx);
                audioInfo.asIndex      = i;
                audioInfo.adecoderName = const_cast<char*>(decoder->name);
                audioInfo.atimeBase    = av_q2d(pFormatCtx->streams[i]->time_base);
                audioInfo.aduration    = duration;
                audioInfo.sampleFmt    = aPtrs.decoderCtx->sample_fmt;
                audioInfo.splDepth     = av_get_bytes_per_sample(audioInfo.sampleFmt);
                audioInfo.splFmtName =
                    const_cast<char*>(av_get_sample_fmt_name(audioInfo.sampleFmt));
                audioInfo.splRate       = (float)cdcPar->sample_rate;
                audioInfo.channelLayout = cdcPar->ch_layout;
                audioInfo.channels      = cdcPar->ch_layout.nb_channels;
                av_channel_layout_describe(&cdcPar->ch_layout, audioInfo.channelLayoutName,
                    sizeof(audioInfo.channelLayoutName));
                std::cout << "Audio Stream Info Get\n";

            } else if (cdcPar->codec_type == AVMEDIA_TYPE_VIDEO) {
                auto decCtx = avcodec_alloc_context3(decoder);
                avcodec_parameters_to_context(decCtx, cdcPar);
                avcodec_open2(decCtx, decoder, nullptr);
                vPtrs.decoderCtx.reset(decCtx);
                videoInfo.vsIndex       = i;
                videoInfo.vdecoderName  = const_cast<char*>(decoder->name);
                videoInfo.vtimeBase     = av_q2d(pFormatCtx->streams[i]->time_base);
                videoInfo.resolution[0] = cdcPar->width;
                videoInfo.resolution[1] = cdcPar->height;
                videoInfo.vduration     = duration;
                emit durationChanged(duration);
                videoInfo.pxFmt     = vPtrs.decoderCtx->pix_fmt;
                videoInfo.pxFmtName = const_cast<char*>(av_get_pix_fmt_name(videoInfo.pxFmt));
                const AVPixFmtDescriptor* pDesc = av_pix_fmt_desc_get(videoInfo.pxFmt);
                videoInfo.pxFmtDpth             = pDesc->comp[1].depth;
                std::cout << "Video Stream Info Get\n";
            }
        }
        mediaInfo.filePath = const_cast<char*>(file);
        ptrSet.formatCtx.reset(pFormatCtx);
        auto swrCtx = myResampler.alcSwrCtx(audioInfo.channelLayout, audioInfo.splRate,
            audioInfo.sampleFmt, myResampler.toPackedFmt(audioInfo.sampleFmt));
        swr_init(swrCtx);
        aPtrs.resamplerCtx.reset(swrCtx);
        auto pvdecFrm = av_frame_alloc();
        vPtrs.decodedFrm.reset(pvdecFrm);
        auto padecFrm = av_frame_alloc();
        aPtrs.decodedFrm.reset(padecFrm);
        auto pvmyFrm = av_frame_alloc();
        vPtrs.myFrm.reset(pvmyFrm);
        auto pamyFrm = av_frame_alloc();
        aPtrs.myFrm.reset(pamyFrm);
        auto packet = av_packet_alloc();
        ptrSet.packet.reset(packet);
        std::cout << "Contexts allocated\n";
        outputInfo(mediaInfo);
        emit sendAudioInfo(audioInfo, &(ptrSet.aPtrs));
        emit sendVideoInfo(videoInfo);
    }

    auto decodeVideo(AVPacket* packet, VPtrSet* pvPtrs, VideoInfo* videoInfo) {
        avcodec_send_packet(pvPtrs->decoderCtx.get(), packet);
        av_packet_unref(packet);
        while (avcodec_receive_frame(pvPtrs->decoderCtx.get(), pvPtrs->decodedFrm.get()) >= 0) {
            av_frame_ref(pvPtrs->myFrm.get(), pvPtrs->decodedFrm.get());
            av_frame_unref(pvPtrs->decodedFrm.get());
            auto avFrame = pvPtrs->myFrm.get();
            auto frame   = std::make_shared<VideoFrame>(avFrame);

            if (count == 0) {
                start = std::chrono::steady_clock::now();
                count += 1;
            }
            update        = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration<double>(update - start).count();
            auto vTime { avFrame->pts * videoInfo->vtimeBase };
            auto diff = vTime - duration;
            if (diff > 0.01) {
                std::this_thread::sleep_for(std::chrono::duration<double>(diff));
            } else if (diff < -0.05) {
                continue;
            }
            av_frame_unref(avFrame);

            emit frameReady(frame);
        }
    };
    auto decodeAudio(AVPacket* packet, APtrSet* paPtrs, AudioInfo* audioInfo) {
        avcodec_send_packet(paPtrs->decoderCtx.get(), packet);
        av_packet_unref(packet);
        while (avcodec_receive_frame(paPtrs->decoderCtx.get(), paPtrs->decodedFrm.get()) >= 0) {
            av_frame_ref(paPtrs->myFrm.get(), paPtrs->decodedFrm.get());
            av_frame_unref(paPtrs->decodedFrm.get());
            auto avFrame  = paPtrs->myFrm.get();
            auto chunk    = std::make_shared<AudioChunk>();
            chunk->pts    = avFrame->pts;
            auto outBytes = av_samples_get_buffer_size(nullptr, audioInfo->channels,
                avFrame->nb_samples, myResampler.toPackedFmt(audioInfo->sampleFmt), 1);
            auto outSamples =
                swr_get_delay(paPtrs->resamplerCtx.get(), audioInfo->splRate) + avFrame->nb_samples;
            chunk->pcm.resize(outBytes);
            if (paPtrs->myFrm.get()->data[1] != nullptr) {
                uint8_t* out[1]       = { chunk->pcm.data() };
                auto convertedSamples = swr_convert(paPtrs->resamplerCtx.get(), out, outSamples,
                    (const uint8_t**)paPtrs->myFrm->data, avFrame->nb_samples);
                auto convertedBytes   = av_samples_get_buffer_size(nullptr, audioInfo->channels,
                    convertedSamples, myResampler.toPackedFmt(audioInfo->sampleFmt), 1);
                outBytes              = convertedBytes;
            } else {
                chunk->pcm.assign(avFrame->data[0], avFrame->data[0] + outBytes);
            }
            av_frame_unref(avFrame);
            emit chunkReady(chunk);
        }
    };
    auto decode(PtrSet* ptrSet, MediaInfo* mediaInfo) {
        if (jump) {
            jump      = false;
            auto aPts = newTime / (mediaInfo->audioInfo.atimeBase);
            auto vPts = newTime / (mediaInfo->videoInfo.vtimeBase);
            av_seek_frame(
                ptrSet->formatCtx.get(), mediaInfo->audioInfo.asIndex, aPts, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(ptrSet->aPtrs.decoderCtx.get());
            av_seek_frame(
                ptrSet->formatCtx.get(), mediaInfo->videoInfo.vsIndex, vPts, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(ptrSet->vPtrs.decoderCtx.get());
        }
        auto packets { ptrSet->packet.get() };
        while (av_read_frame(ptrSet->formatCtx.get(), packets) >= 0) {

            if (packets->stream_index == mediaInfo->audioInfo.asIndex) {
                decodeAudio(packets, &(ptrSet->aPtrs), &(mediaInfo->audioInfo));
            } else if (ptrSet->packet.get()->stream_index == mediaInfo->videoInfo.vsIndex) {

                decodeVideo(packets, &(ptrSet->vPtrs), &(mediaInfo->videoInfo));
            }
        }
        av_packet_unref(packets);
    }

public slots:
    void processStart() {
        demux(filePath);
        decode(&ptrSet, &mediaInfo);
        emit finished();
    }

public:
    DemuxerPlusDecoder(std::string filePath)
        : filePath(std::move(filePath)) { };
    ~DemuxerPlusDecoder() { };
};
