#pragma once

#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
}

class MyDemuxer {
public:
  auto alcFmtCtx() noexcept {
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    return pFormatCtx;
  }

  auto open(const char *filename, AVFormatContext *pFormatCtx) const noexcept {

    if (avformat_open_input(&pFormatCtx, filename, nullptr, nullptr) != 0) {
      std::cout << "Can't open file\n";
      return -1;
    } else {
      std::cout << "Open file success\n"
                << "Filepath: " << filename << "\n";
    };
    return 0;
  }

  auto findSInfo(AVFormatContext *pFormatCtx) const noexcept {
    if (avformat_find_stream_info(pFormatCtx, nullptr) != 0) {
      std::cout << "Can't find video stream info\n";
      return -1;
    }
    return 0;
  }

  auto findVSInfo(AVFormatContext *pFormatCtx) const noexcept {

    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
      const auto cdcPar = pFormatCtx->streams[i]->codecpar;

      auto containerDuration = (float)pFormatCtx->duration;
      auto base = pFormatCtx->streams[i]->time_base.den;
      auto duration = (float)pFormatCtx->streams[i]->duration / base;
      if (duration <= 0.0f) {
        duration = containerDuration / 1000 / base;
      };
      if (cdcPar->codec_type == AVMEDIA_TYPE_VIDEO) {
        std::cout << "Stream[" << i << "]:Video\n"
                  << "__resolution:" << cdcPar->width << "x" << cdcPar->height
                  << "\n"
                  << "__duration:" << duration << "s\n";
        return i;
      }
    }
    return 0;
  }
  auto findASInfo(AVFormatContext *pFormatCtx) const noexcept {

    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
      const auto cdcPar = pFormatCtx->streams[i]->codecpar;

      if (cdcPar->codec_type == AVMEDIA_TYPE_AUDIO) {
        std::cout << "Stream[" << i << "]:Audio\n";
        return i;
      }
    }
    return 0;
  }

  auto close(AVFormatContext *pFormatCtx) noexcept {
    avformat_close_input(&pFormatCtx);
    return 0;
  }

  auto free(AVCodecContext *pCodecContext) noexcept {
    avcodec_free_context(&pCodecContext);
    return 0;
  }
};