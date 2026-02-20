#pragma once

#include <chrono>
#include <memory>

#include <thread>

#include "audiowidget.h"
#include "decoder.h"
#include "glwidget.h"
#include "public.h"
// resource management

class MediaPlayer {
private:
  // media
  MediaInfo mediaInfo;
  PtrSet ptrSet;
  MyResampler myResampler;
  MyAudioWidget audioWidget;

  auto demux() {}

  auto openFile(const char *pfilePath) {
    auto pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, pfilePath, nullptr, nullptr) != 0) {
      std::cerr << "Can't open file\n" << std::flush;
    }
    if (avformat_find_stream_info(pFormatCtx, nullptr) != 0) {
      std::cerr << "Can't find video stream info\n" << std::flush;
    }
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
      const auto cdcPar = pFormatCtx->streams[i]->codecpar;
      const auto *decoder = avcodec_find_decoder(cdcPar->codec_id);
      if (decoder == nullptr) {
        std::cerr << "Can't find supported decoder\n" << std::flush;
      }
      auto containerDuration = (float)pFormatCtx->duration;

      auto base = pFormatCtx->streams[i]->time_base.den;

      auto duration = (float)pFormatCtx->streams[i]->duration / base;

      if (duration <= 0.0f) {
        duration = containerDuration / 1000 / base;
      };
      printf("4\n");
      if (cdcPar->codec_type == AVMEDIA_TYPE_AUDIO) {
        printf("5\n");
        auto decCtx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(decCtx, cdcPar);
        avcodec_open2(decCtx, decoder, nullptr);
        ptrSet.audioDecCtx.reset(decCtx);
        printf("6\n");
        mediaInfo.asIndex = i;
        mediaInfo.adecoderName = const_cast<char *>(decoder->name);
        mediaInfo.atimeBase = av_q2d(pFormatCtx->streams[i]->time_base);
        mediaInfo.aduration = duration;
        mediaInfo.sampleFmt = ptrSet.audioDecCtx->sample_fmt;
        mediaInfo.splFmtName =
            const_cast<char *>(av_get_sample_fmt_name(mediaInfo.sampleFmt));
        mediaInfo.splRate = (float)cdcPar->sample_rate / 1000;
        mediaInfo.channelLayout = cdcPar->ch_layout;
        mediaInfo.channels = cdcPar->ch_layout.nb_channels;
        av_channel_layout_describe(&cdcPar->ch_layout,
                                   mediaInfo.channelLayoutName,
                                   sizeof(mediaInfo.channelLayoutName));
        std::cout << "Audio Stream Info Get\n";
      } else if (cdcPar->codec_type == AVMEDIA_TYPE_VIDEO) {
        auto decCtx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(decCtx, cdcPar);
        avcodec_open2(decCtx, decoder, nullptr);
        ptrSet.videoDecCtx.reset(decCtx);
        mediaInfo.vsIndex = i;
        mediaInfo.vdecoderName = const_cast<char *>(decoder->name);
        mediaInfo.vtimeBase = av_q2d(pFormatCtx->streams[i]->time_base);
        mediaInfo.resolution[0] = cdcPar->width;
        mediaInfo.resolution[1] = cdcPar->height;
        mediaInfo.vduration = duration;
        mediaInfo.pxFmt = ptrSet.videoDecCtx->pix_fmt;
        printf("8\n");
        mediaInfo.pxFmtName =
            const_cast<char *>(av_get_pix_fmt_name(mediaInfo.pxFmt));
        const AVPixFmtDescriptor *pDesc = av_pix_fmt_desc_get(mediaInfo.pxFmt);
        printf("9\n");
        mediaInfo.pxFmtDpth = pDesc->comp[1].depth;
        printf("10\n");
        std::cout << "Video Stream Info Get\n";
      }
    }
    mediaInfo.filePath = const_cast<char *>(pfilePath);
    ptrSet.formatCtx.reset(pFormatCtx);
    auto pvdecFrm = av_frame_alloc();
    auto padecFrm = av_frame_alloc();
    auto pvmyFrm = av_frame_alloc();
    auto pamyFrm = av_frame_alloc();
    auto ppaket = av_packet_alloc();
    ptrSet.vdecFrm.reset(pvdecFrm);
    ptrSet.adecFrm.reset(padecFrm);
    ptrSet.vmyFrm.reset(pvmyFrm);
    ptrSet.amyFrm.reset(pamyFrm);
    ptrSet.paket.reset(ppaket);
    std::cout << "Contexts allocated\n";
  };

  const auto outputInfo(MediaInfo &pmediaInfo) const noexcept {
    std::cout << "File Path: " << pmediaInfo.filePath << "\n"
              << "Video Decoder: " << pmediaInfo.vdecoderName << "\n"
              << "Video Resolution: " << pmediaInfo.resolution[0] << "x"
              << pmediaInfo.resolution[1] << "\n"
              << "Video Duration: " << pmediaInfo.vduration << "s\n"
              << "Video Time Base: " << pmediaInfo.vtimeBase << "\n"
              << "Video Pixel Format: " << pmediaInfo.pxFmtName
              << "(depth:" << pmediaInfo.pxFmtDpth << ")\n"
              << "Audio Decoder: " << pmediaInfo.adecoderName << "\n"
              << "Audio Sample Rate: " << pmediaInfo.splRate * 1000 << "Hz\n"
              << "Audio Channel Layout: "
              << (pmediaInfo.channelLayoutName[0] != '\0'
                      ? pmediaInfo.channelLayoutName
                      : std::to_string(pmediaInfo.channels).c_str())
              << "\n"
              << "Audio Sample Format: " << pmediaInfo.splFmtName << "\n"
              << "Audio Duration: " << pmediaInfo.aduration << "s\n";
  };

  auto decodeLoop(MediaInfo &pmediaInfo, PtrSet &pptrSet) {
    MyDecoder myDecoder;
    while (myDecoder.axptPkt(ptrSet.formatCtx.get(), ptrSet.paket.get())) {
      if (ptrSet.paket->stream_index == mediaInfo.vsIndex) {
        myDecoder.decPkt(ptrSet.videoDecCtx.get(), ptrSet.paket.get());
        while (
            myDecoder.rcvFrm(ptrSet.videoDecCtx.get(), ptrSet.vdecFrm.get())) {
          myDecoder.cpyFrm(ptrSet.vdecFrm.get(), ptrSet.vmyFrm.get());
          auto startTime = std::chrono::steady_clock::now();
          int64_t pts = ptrSet.vmyFrm->pts;
          int64_t vfirstPts = AV_NOPTS_VALUE;
          if (pts == AV_NOPTS_VALUE) {
            pts = ptrSet.vmyFrm->best_effort_timestamp;
          }
          if (vfirstPts == AV_NOPTS_VALUE) {
            vfirstPts = pts;
          }
          double frameTimeSeconds = (pts - vfirstPts) * mediaInfo.vtimeBase;
          auto yPlane = ptrSet.vmyFrm->data[0];
          auto uPlane = ptrSet.vmyFrm->data[1];
          auto vPlane = ptrSet.vmyFrm->data[2];
          // myFrame->format = deCtx->pix_fmt;

          auto frmWidth = ptrSet.vmyFrm->width;
          auto frmHeight = ptrSet.vmyFrm->height;
          auto lineSize0 = ptrSet.vmyFrm->linesize[0];
          auto lineSize1 = ptrSet.vmyFrm->linesize[1];
          if (mediaInfo.pxFmtDpth == 8) {
            videowidget.renderWithOpenGL8(yPlane, uPlane, vPlane, frmWidth,
                                          frmHeight, lineSize0, lineSize1,
                                          mediaInfo.pxFmt);
          } else if (mediaInfo.pxFmtDpth == 10) {
            videowidget.renderWithOpenGL10(yPlane, uPlane, vPlane, frmWidth,
                                           frmHeight, lineSize0, lineSize1,
                                           mediaInfo.pxFmt);
          }

          auto now = std::chrono::steady_clock::now();
          double elapsed =
              std::chrono::duration<double>(now - startTime).count();

          double waitTime = frameTimeSeconds - elapsed;
          if (waitTime > 0) {
            std::this_thread::sleep_for(
                std::chrono::duration<double>(waitTime));
          }
          av_frame_unref(ptrSet.vmyFrm.get());
        }
        av_frame_unref(ptrSet.vdecFrm.get());
      } else if (ptrSet.paket->stream_index == mediaInfo.asIndex) {
        myDecoder.decPkt(ptrSet.audioDecCtx.get(), ptrSet.paket.get());
        while (
            myDecoder.rcvFrm(ptrSet.audioDecCtx.get(), ptrSet.adecFrm.get())) {
          myDecoder.cpyFrm(ptrSet.adecFrm.get(), ptrSet.amyFrm.get());
          int outSamples =
              ptrSet.amyFrm
                  ->nb_samples; // 一个声道一次填入缓冲区的样本数，用来控制音频采样的延迟

          int outBufferSize =
              audioWidget.buffer(mediaInfo.channels, outSamples,
                                 mediaInfo.sampleFmt); // 缓冲区大小
          std::vector<uint8_t> buffer(outBufferSize);
          const auto data = ptrSet.adecFrm->data;
          // 判断是否是plannar数据，如果data[1]为空，肯定不是，否则需要转packed数据
          if (data[1] == nullptr) {
            buffer.assign(data[0], data[0] + outBufferSize);
          } else {
            uint8_t *bufPtr = buffer.data();
            swr_convert(ptrSet.audioRsplCtx.get(), &bufPtr, outSamples,
                        (const uint8_t **)ptrSet.adecFrm->data,
                        ptrSet.adecFrm->nb_samples);
          }
          {
            std::lock_guard<std::mutex> lock(audioWidget.audioQueue.mutex);
            audioWidget.audioQueue.packets.push(std::move(buffer));
          }

          av_frame_unref(ptrSet.amyFrm.get());
        }
        av_frame_unref(ptrSet.adecFrm.get());
      }
    }
  }

public:
  MyGLWidget videowidget;
  MediaPlayer() {

  };
  ~MediaPlayer() {};
  void open(const char *pfilePath) {
    openFile(pfilePath);
    outputInfo(mediaInfo);
    audioWidget.open(mediaInfo);
    ptrSet.audioRsplCtx.reset(
        myResampler.alcSwrCtx(mediaInfo.channelLayout, mediaInfo.splRate,
                              mediaInfo.sampleFmt, mediaInfo.sampleFmt));
    decodeLoop(mediaInfo, ptrSet);
  }
};