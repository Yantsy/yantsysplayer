#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}
#include "public.h"
#include "resampler.h"
class MyDecoder {
public:
  auto findPxFmt(const AVCodecContext *pcdCtx) const noexcept {
    const auto *ppixFmt = av_get_pix_fmt_name(pcdCtx->pix_fmt);
    if (ppixFmt == nullptr) {
      std::cerr << "Can't find supported pixel format\n";
      // exit(-1);
    }
    std::cout << "__pixel format:" << ppixFmt << "\n";
    return pcdCtx->pix_fmt;
  }
  auto findSprFmt(const AVCodecContext *pcdCtx) const noexcept {
    const auto *psprFmt = av_get_sample_fmt_name(pcdCtx->sample_fmt);
    auto a = pcdCtx->sample_fmt;
    if (psprFmt == nullptr) {
      std::cerr << "Can't find supported sample format\n";
      // exit(-1);
    }
    std::cout << "__sample format:" << psprFmt << "\n";
    return pcdCtx->sample_fmt;
  }
  auto findASInfo(const AVFormatContext *pFormatCtx,
                  const AVCodecContext *pcdCtx,
                  const int &pastreamIndex) const noexcept {
    SDL_AudioSpec pSpec;
    MyResampler pResampler;
    const auto cdcPar = pFormatCtx->streams[pastreamIndex]->codecpar;
    char layout_name[64];
    const auto *psprFmt = av_get_sample_fmt_name(pcdCtx->sample_fmt);
    auto a = pcdCtx->sample_fmt;
    if (psprFmt == nullptr) {
      std::cerr << "Can't find supported sample format\n";
      // exit(-1);
    }
    auto b = cdcPar->ch_layout;
    av_channel_layout_describe(&cdcPar->ch_layout, layout_name,
                               sizeof(layout_name));
    auto containerDuration = (float)pFormatCtx->duration;
    auto base = pFormatCtx->streams[pastreamIndex]->time_base.den;
    auto duration = (float)pFormatCtx->streams[pastreamIndex]->duration / base;
    if (duration <= 0) {
      duration = containerDuration / 1000 / base;
    };

    std::cout << "__sample rate:" << float(cdcPar->sample_rate / 1000.0f)
              << "KHz\n"
              << "__channel layout: " << layout_name << "\n"
              << "__channels:" << cdcPar->ch_layout.nb_channels << "\n"
              << "__duration:" << duration << "s\n"
              << "__sample format:" << psprFmt << "\n";
    pSpec.freq = cdcPar->sample_rate;
    pSpec.format = pResampler.fmtNameTrans(pcdCtx->sample_fmt);
    pSpec.channels = cdcPar->ch_layout.nb_channels;
    pSpec.samples = 1024;
    pSpec.silence = 0;
    return pSpec;
  }
  auto findDec(AVFormatContext *pFormatCtx, const int &pstreamIndex,
               const uint8_t &tgt) const noexcept {
    const auto cdcPar = pFormatCtx->streams[pstreamIndex]->codecpar;
    const auto *pdecoder = avcodec_find_decoder(cdcPar->codec_id);
    if (pdecoder == nullptr) {
      std::cerr << "Can't find supported decoder\n" << std::endl;
      // exit(-1);
    } else {
      switch (tgt) {
      case target::VIDEO: {
        std::cout << "__decoder:" << pdecoder->name << "\n";
        break;
      }
      case target::AUDIO: {
        std::cout << "__decoder:" << pdecoder->name << "\n";
        break;
      }
      }
    }
    return pdecoder;
  }

  auto alcCtx(const AVCodec *pdecoder, const AVFormatContext *pFormatCtx,
              const int &pstreamIndex) const noexcept {
    AVCodecContext *cdCtx = avcodec_alloc_context3(pdecoder);
    const auto cdcPar = pFormatCtx->streams[pstreamIndex]->codecpar;
    // ignorance of this step makes the codec fail to find the start code,
    // and causes error splitting the input into NAL units
    if (avcodec_parameters_to_context(cdCtx, cdcPar) != 0) {
      std::cerr << "Can't copy codec parameters\n";
      // exit(-1);
    }
    if (avcodec_open2(cdCtx, pdecoder, nullptr) != 0) {
      std::cerr << "Can't open codec\n";
      // exit(-1);
    } else {
      // std::cout << "Open codec successfully\n";
    }

    return cdCtx;
  }
  auto findPxDpth(const AVPixelFormat &ppixFmt, const int &out) const noexcept {
    // AVPixFmtDescriptor:Descriptor that unambiguously describes how the bits
    // of a pixel are stored in the up to 4 data planes of an image.
    const AVPixFmtDescriptor *pDesc = av_pix_fmt_desc_get(ppixFmt);
    if (pDesc) {
      if (out == 1) {
        std::cout << "__pixel depth:" << pDesc->comp[1].depth << "\n";
      }
      return pDesc->comp[1].depth;
    }
    return 8;
  }
  auto alcFrm() const noexcept {
    AVFrame *frame = av_frame_alloc();
    if (frame == nullptr) {
      std::cerr << "Can't allocate frame\n";
    } else {
      std::cout << "Allocate frame successfully\n";
    }
    return frame;
  }
  // needed when you want to encode a frame while decoding

  auto alcPkt() const noexcept {
    AVPacket *ppkt = av_packet_alloc();
    if (ppkt == nullptr) {
      std::cerr << "Can't allocate packet\n";
    } else {
      std::cout << "Allocate packet successfully\n";
    }
    return ppkt;
  }

  bool axptPkt(AVFormatContext *pFormatCtx, AVPacket *ppkt) const noexcept {
    return av_read_frame(pFormatCtx, ppkt) >= 0;
  }

  auto decPkt(AVCodecContext *pcdCtx, AVPacket *ppkt) const noexcept {
    avcodec_send_packet(pcdCtx, ppkt);
    av_packet_unref(ppkt);
  }

  bool rcvFrm(AVCodecContext *pcdCtx, AVFrame *pdecFrame) const noexcept {
    return avcodec_receive_frame(pcdCtx, pdecFrame) >= 0;
  }

  auto cpyFrm(AVFrame *pdecFrame, AVFrame *pmyFrame) const noexcept {
    av_frame_ref(pmyFrame, pdecFrame);
    // av_frame_unref(decFrame);
  };
  auto free(AVCodecContext *pcdCtx) const noexcept {
    avcodec_free_context(&pcdCtx);
    return 0;
  }

  auto free(AVFrame *pFrame) const noexcept {
    av_frame_free(&pFrame);
    return 0;
  }

  auto free(AVPacket *ppkt) const noexcept {
    av_packet_free(&ppkt);
    return 0;
  }
};