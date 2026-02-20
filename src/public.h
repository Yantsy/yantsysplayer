#pragma once

#include <cstdint>
#include <memory>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
namespace target {
enum : uint8_t { NONE, VIDEO, AUDIO, SUBTITLE };
}

template <typename T> struct CtxFree;
template <> struct CtxFree<AVFormatContext> {
  auto operator()(AVFormatContext *ctx) const noexcept {
    avformat_close_input(&ctx);
  };
};
template <> struct CtxFree<AVCodecContext> {
  auto operator()(AVCodecContext *ctx) const noexcept {
    avcodec_free_context(&ctx);
  }
};
template <> struct CtxFree<SwrContext> {
  auto operator()(SwrContext *ctx) const noexcept { swr_free(&ctx); }
};
template <> struct CtxFree<AVFrame> {
  auto operator()(AVFrame *ctx) const noexcept { av_frame_free(&ctx); }
};
template <> struct CtxFree<AVPacket> {
  auto operator()(AVPacket *ctx) const noexcept { av_packet_free(&ctx); }
};

using FmtCtxPtr = std::unique_ptr<AVFormatContext, CtxFree<AVFormatContext>>;
using DecCtxPtr = std::unique_ptr<AVCodecContext, CtxFree<AVCodecContext>>;
using SwrCtxPtr = std::unique_ptr<SwrContext, CtxFree<SwrContext>>;
using FrmPtr = std::unique_ptr<AVFrame, CtxFree<AVFrame>>;
using PktPtr = std::unique_ptr<AVPacket, CtxFree<AVPacket>>;

struct MediaInfo {
  char *filePath;
  int vsIndex;
  char *vdecoderName;
  int resolution[2];
  double vtimeBase;
  float vduration;
  AVPixelFormat pxFmt;
  char *pxFmtName;
  int pxFmtDpth;
  int asIndex;
  char *adecoderName;
  double atimeBase;
  int samples{1024};
  int silence;
  int splRate;
  float aduration;
  AVSampleFormat sampleFmt;
  char *splFmtName;
  int channels;
  AVChannelLayout channelLayout;
  char channelLayoutName[64];
  int ssIndex;
};

struct PtrSet {
  FmtCtxPtr formatCtx{nullptr};
  DecCtxPtr videoDecCtx{nullptr};
  DecCtxPtr audioDecCtx{nullptr};
  SwrCtxPtr videoRsplCtx{nullptr};
  SwrCtxPtr audioRsplCtx{nullptr};
  PktPtr paket{nullptr};
  FrmPtr vdecFrm{nullptr};
  FrmPtr vmyFrm{nullptr};
  FrmPtr adecFrm{nullptr};
  FrmPtr amyFrm{nullptr};
};