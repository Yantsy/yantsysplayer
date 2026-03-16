#pragma once
#include "public.h"
#include "resampler.h"
#include <thread>
class DemuxerPlusDecoder : public QObject {
    Q_OBJECT;
signals:
    void frameReady(std::shared_ptr<VideoFrame> videoFrame);
    void chunkReady(std::shared_ptr<AudioChunk> audioChunk);
    void sendVideoInfo(PlayerStatePtr is);
    void sendAudioInfo(PlayerStatePtr is);
    void durationChanged(double duration);
    void videoDecodingDone();
    void audioDecodingDone();
    void finished();

private:
    std::string file;
    Clock audioClk, videoClk, exClk;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point update;
    PlayerStatePtr is = std::make_shared<PlayerState>();
    int count { 0 };
    bool jump { false };
    double newTime { 0.0 };
    MediaInfo mediaInfo;
    ChunkQueue chunks;
    MyResampler myResampler;
    // tool functions
    auto outputInfo(MediaInfo& mediaInfo) {
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
                  << "____sample_format: " << mediaInfo.audioInfo.splFmtName
                  << "(depth:" << mediaInfo.audioInfo.splDepth * 8 << ")\n"
                  << std::flush;
        std::cout << " " << std::endl;
    };

    auto seekFrame(PlayerStatePtr is) {
        /*
        int64_t seekTarget = av_rescale_q(is->estAudioPTS,
            is->formatCtx->streams[is->mediaInfo.audioInfo.asIndex]->time_base, AV_TIME_BASE_Q);
*/
        av_seek_frame(is->formatCtx.get(), is->mediaInfo.videoInfo.vsIndex, is->estVideoPTS,
            AVSEEK_FLAG_BACKWARD);
        count = 0;
        start = std::chrono::steady_clock::now();
        avcodec_flush_buffers(is->audioDecCtx.get());
        avcodec_flush_buffers(is->videoDecCtx.get());
    }

    // core functions
    auto demux(std::string& filePath) {
        auto pFormatCtx = avformat_alloc_context();
        auto file       = filePath.c_str();
        if (avformat_open_input(&pFormatCtx, file, nullptr, nullptr) != 0) {
            std::cerr << "Can't open file\n" << std::flush;
        }
        if (avformat_find_stream_info(pFormatCtx, nullptr) != 0) {
            std::cerr << "Can't find video stream info\n" << std::flush;
        }
        auto asi = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        auto vsi = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        auto ssi = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0);
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
            if (i == asi) {
                auto decCtx = avcodec_alloc_context3(decoder);
                avcodec_parameters_to_context(decCtx, cdcPar);
                avcodec_open2(decCtx, decoder, nullptr);
                is->audioDecCtx.reset(decCtx);
                mediaInfo.audioInfo.asIndex      = i;
                mediaInfo.audioInfo.adecoderName = const_cast<char*>(decoder->name);
                mediaInfo.audioInfo.atimeBase    = av_q2d(pFormatCtx->streams[i]->time_base);
                mediaInfo.audioInfo.timeBase     = pFormatCtx->streams[i]->time_base;
                mediaInfo.audioInfo.aduration    = duration;
                mediaInfo.audioInfo.sampleFmt    = is->audioDecCtx->sample_fmt;
                mediaInfo.audioInfo.splDepth =
                    av_get_bytes_per_sample(mediaInfo.audioInfo.sampleFmt);
                mediaInfo.audioInfo.splFmtName =
                    const_cast<char*>(av_get_sample_fmt_name(mediaInfo.audioInfo.sampleFmt));
                mediaInfo.audioInfo.splRate       = (float)cdcPar->sample_rate;
                mediaInfo.audioInfo.channelLayout = cdcPar->ch_layout;
                mediaInfo.audioInfo.channels      = cdcPar->ch_layout.nb_channels;
                av_channel_layout_describe(&cdcPar->ch_layout,
                    mediaInfo.audioInfo.channelLayoutName,
                    sizeof(mediaInfo.audioInfo.channelLayoutName));
                std::cout << "Audio Stream Info Get\n";

            } else if (i == vsi) {
                auto decCtx = avcodec_alloc_context3(decoder);
                avcodec_parameters_to_context(decCtx, cdcPar);
                avcodec_open2(decCtx, decoder, nullptr);
                is->videoDecCtx.reset(decCtx);
                mediaInfo.videoInfo.vsIndex       = i;
                mediaInfo.videoInfo.vdecoderName  = const_cast<char*>(decoder->name);
                mediaInfo.videoInfo.vtimeBase     = av_q2d(pFormatCtx->streams[i]->time_base);
                mediaInfo.videoInfo.resolution[0] = cdcPar->width;
                mediaInfo.videoInfo.resolution[1] = cdcPar->height;
                mediaInfo.videoInfo.vduration     = duration;
                emit durationChanged(duration);
                mediaInfo.videoInfo.pxFmt = is->videoDecCtx->pix_fmt;
                mediaInfo.videoInfo.pxFmtName =
                    const_cast<char*>(av_get_pix_fmt_name(mediaInfo.videoInfo.pxFmt));
                const AVPixFmtDescriptor* pDesc = av_pix_fmt_desc_get(mediaInfo.videoInfo.pxFmt);
                mediaInfo.videoInfo.pxFmtDpth   = pDesc->comp[0].depth;
                std::cout << "Video Stream Info Get\n";
            }
        }
        mediaInfo.filePath = const_cast<char*>(file);
        is->mediaInfo      = std::move(mediaInfo);
        is->formatCtx.reset(pFormatCtx);
        auto swrCtx = myResampler.alcSwrCtx(mediaInfo.audioInfo.channelLayout,
            mediaInfo.audioInfo.splRate, mediaInfo.audioInfo.sampleFmt,
            myResampler.toPackedFmt(mediaInfo.audioInfo.sampleFmt));
        swr_init(swrCtx);
        is->audioSwrCtx.reset(swrCtx);
        auto decVideoFrm = av_frame_alloc();
        is->decVideoFrm.reset(decVideoFrm);
        auto myVideoFrm = av_frame_alloc();
        is->myVideoFrm.reset(myVideoFrm);
        auto decAudioFrm = av_frame_alloc();
        is->decAudioFrm.reset(decAudioFrm);
        auto myAudioFrm = av_frame_alloc();
        is->myAudioFrm.reset(myAudioFrm);
        auto packet = av_packet_alloc();
        is->packet.reset(packet);
        std::cout << "Contexts allocated\n";
        outputInfo(is->mediaInfo);
        emit sendAudioInfo(is);
        emit sendVideoInfo(is);
    }

    auto decodeVideo(PlayerStatePtr is) {
        auto decCtx = is->videoDecCtx.get();
        auto packet = is->packet.get();
        avcodec_send_packet(decCtx, packet);
        av_packet_unref(packet);
        auto decodedFrm = is->decVideoFrm.get();
        // auto myFrm      = is->myVideoFrm.get();
        while (is->videoUpdate() == 0) {

            int ret = avcodec_receive_frame(decCtx, decodedFrm);
            if (ret == AVERROR(EAGAIN)) {
                break;
            }
            if (ret == AVERROR_EOF) {
                is->flushv();
                break;
            }
            auto pts = decodedFrm->pts;
            // av_frame_ref(myFrm, decodedFrm);

            auto frame = std::make_shared<VideoFrame>(decodedFrm);
            av_frame_unref(decodedFrm);
            if (count == 0) {
                start = std::chrono::steady_clock::now();
                count += 1;
            }
            update        = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration<double>(update - start).count();
            auto vTime { pts * (is->mediaInfo.videoInfo.vtimeBase) };
            auto diff = vTime - duration;
            if (diff > 0.01) {
                std::this_thread::sleep_for(std::chrono::duration<double>(diff));
            } else if (diff < -0.05) {
                continue;
            }
            // av_frame_unref(myFrm);
            if (!is->topause) emit frameReady(frame);
        }
    };
    auto decodeAudio(PlayerStatePtr is) {

        auto decCtx = is->audioDecCtx.get();
        auto packet = is->packet.get();
        avcodec_send_packet(decCtx, packet);
        av_packet_unref(packet);
        auto swrCtx     = is->audioSwrCtx.get();
        auto decodedFrm = is->decAudioFrm.get();
        auto myFrm      = is->myAudioFrm.get();
        while (is->audioUpdate() == 0) {
            // if (is->progressChanged || is->topause) return;
            int ret = avcodec_receive_frame(decCtx, decodedFrm);
            if (ret == AVERROR(EAGAIN)) {
                break;
            }
            if (ret == AVERROR_EOF) {
                is->flusha();
                break;
            }

            av_frame_ref(myFrm, decodedFrm);
            av_frame_unref(decodedFrm);
            // auto chunk    = std::make_shared<AudioChunk>();
            AudioChunk chunk;
            chunk.pts     = myFrm->pts;
            auto outBytes = av_samples_get_buffer_size(nullptr, mediaInfo.audioInfo.channels,
                myFrm->nb_samples, myResampler.toPackedFmt(mediaInfo.audioInfo.sampleFmt), 1);
            auto outSamples =
                swr_get_delay(swrCtx, mediaInfo.audioInfo.splRate) + myFrm->nb_samples;
            chunk.pcm.resize(outBytes);
            std::array<uint8_t*, 1> outBuffer { chunk.pcm.data() };

            if (myFrm->data[1] != nullptr) {
                auto convertedSamples = swr_convert(swrCtx, &outBuffer[0], outSamples,
                    (const uint8_t**)myFrm->data, myFrm->nb_samples);
                auto convertedBytes   = av_samples_get_buffer_size(nullptr,
                    mediaInfo.audioInfo.channels, convertedSamples,
                    myResampler.toPackedFmt(mediaInfo.audioInfo.sampleFmt), 1);
                outBytes              = convertedBytes;
            } else {
                chunk.pcm.assign(myFrm->data[0], myFrm->data[0] + outBytes);
            }
            chunk.audioBufferSize = outBytes;
            is->chunks.push(std::move(chunk));
            av_frame_unref(myFrm);
            /*
            pause for the push mode
            if (!is->topause) {
                emit chunkReady(chunk);
            } else {
                count = 0;
            };*/
        }
    };
    auto decode(PlayerStatePtr is) {
        auto packets { is->packet.get() };
        while (is->update() == 0) {
            if (is->progressChanged) {
                is->progressChanged = false;
                seekFrame(is);
            }

            int ret = av_read_frame(is->formatCtx.get(), packets);
            if (ret == AVERROR_EOF) {
                is->quit();
                break;
            };

            if (packets->stream_index == is->mediaInfo.audioInfo.asIndex) {
                decodeAudio(is);
            } else if (packets->stream_index == is->mediaInfo.videoInfo.vsIndex) {
                decodeVideo(is);
            }
        }
        av_packet_unref(packets);
    }

public slots:
    void processStart() {
        demux(file);
        decode(is);
        emit finished();
    }
    void progressChange(double newProgress) {
        is->progressChanged = true;
        is->estAudioPTS     = (int64_t)(newProgress / (is->mediaInfo.audioInfo.atimeBase));
        is->audioPTS =
            av_rescale_q(is->estAudioPTS, is->mediaInfo.audioInfo.timeBase, AV_TIME_BASE_Q);
        is->estVideoPTS = (int64_t)(newProgress / (is->mediaInfo.videoInfo.vtimeBase));
        is->videoPTS =
            av_rescale_q(is->estVideoPTS, is->mediaInfo.videoInfo.timeBase, AV_TIME_BASE_Q);
    }
    void pause() { is->pause(); };
    void play() { is->play(); };
    void quit() {
        is->flusha();
        is->flushv();
        is->quit();
    }

public:
    DemuxerPlusDecoder() { };
    ~DemuxerPlusDecoder() { };
    void open(std::string filePath) { file = std::move(filePath); };
};
