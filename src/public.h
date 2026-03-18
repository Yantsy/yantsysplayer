#pragma once

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <queue>
#include <ranges>
extern "C" {
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}
#include "resampler.h"
#include <QMetaType>
#include <QObject>
namespace target {
enum : uint8_t { NONE, VIDEO, AUDIO, SUBTITLE };
}

template <typename T>
struct CtxFree;
template <>
struct CtxFree<AVFormatContext> {
    auto operator()(AVFormatContext* ctx) const noexcept { avformat_close_input(&ctx); };
};
template <>
struct CtxFree<AVCodecContext> {
    auto operator()(AVCodecContext* ctx) const noexcept { avcodec_free_context(&ctx); }
};
template <>
struct CtxFree<SwrContext> {
    auto operator()(SwrContext* ctx) const noexcept { swr_free(&ctx); }
};
template <>
struct CtxFree<AVFrame> {
    auto operator()(AVFrame* ctx) const noexcept {
        if (ctx != nullptr) av_frame_free(&ctx);
    }
};
template <>
struct CtxFree<AVPacket> {
    auto operator()(AVPacket* ctx) const noexcept { av_packet_free(&ctx); }
};

using FmtCtxPtr = std::unique_ptr<AVFormatContext, CtxFree<AVFormatContext>>;
using DecCtxPtr = std::unique_ptr<AVCodecContext, CtxFree<AVCodecContext>>;
using SwrCtxPtr = std::unique_ptr<SwrContext, CtxFree<SwrContext>>;
using FrmPtr    = std::unique_ptr<AVFrame, CtxFree<AVFrame>>;
using PktPtr    = std::unique_ptr<AVPacket, CtxFree<AVPacket>>;
using rt        = std::chrono::steady_clock;

struct AudioInfo {
    int asIndex;
    char* adecoderName;
    double atimeBase;
    AVRational timeBase;
    int samples { 1024 };
    int silence;
    int splRate;
    int splDepth;
    float aduration;
    AVSampleFormat sampleFmt;
    char* splFmtName;
    int channels;
    AVChannelLayout channelLayout;
    char channelLayoutName[64];
};
Q_DECLARE_METATYPE(AudioInfo);

struct VideoInfo {
    int vsIndex;
    char* vdecoderName;
    int resolution[2];
    double vtimeBase;
    AVRational timeBase;
    float vduration;
    AVPixelFormat pxFmt;
    char* pxFmtName;
    int pxFmtDpth;
};
Q_DECLARE_METATYPE(VideoInfo);
struct MediaInfo {
    char* filePath;
    AudioInfo audioInfo;
    VideoInfo videoInfo;
};

struct AudioChunk {
    std::vector<uint8_t> pcm;
    int audioBufferSize { 0 };
    int64_t pts;
};
Q_DECLARE_METATYPE(std::shared_ptr<AudioChunk>);
struct VideoFrame0 {
    int width;
    int height;
    int64_t pts;
    std::array<int, 3> linesize;
    std::vector<uint8_t> y;
    std::vector<uint8_t> u;
    std::vector<uint8_t> v;
    VideoFrame0(AVFrame* pframe)
        : width(pframe->width)
        , height(pframe->height)
        , pts(pframe->pts) {
        std::ranges::transform(std::views::iota(0, 3), linesize.begin(),
            [pframe](int i) { return std::abs(pframe->linesize[i]); });
        auto yH = ((height & 1) == 1) ? height + 1 : height;
        auto uH { yH / 2 }, vH { yH / 2 };
        y.resize(linesize[0] * height);
        y.assign(pframe->data[0], pframe->data[0] + linesize[0] * height);
        u.resize(linesize[1] * uH);
        u.assign(pframe->data[1], pframe->data[1] + linesize[1] * uH);
        v.resize(linesize[2] * vH);
        v.assign(pframe->data[2], pframe->data[2] + linesize[2] * vH);
    }
};
struct VideoFrame {
    int width;
    int height;
    int64_t pts;
    std::array<int, 3> linesize;
    FrmPtr frame { nullptr };
    VideoFrame(AVFrame* pframe)
        : width(pframe->width)
        , height(pframe->height)
        , pts(pframe->pts) {
        std::ranges::transform(std::views::iota(0, 3), linesize.begin(),
            [pframe](int i) { return std::abs(pframe->linesize[i]); });
        AVFrame* f = av_frame_alloc();
        av_frame_ref(f, pframe);
        frame.reset(f);
    }
    ~VideoFrame() { };
};
Q_DECLARE_METATYPE(std::shared_ptr<VideoFrame>);
struct ChunkQueue {
    std::queue<AudioChunk> bufferredPcm;
};
struct FrameQueue {
    std::queue<VideoFrame> bufferredFrm;
};

struct Clock {
    int init = -1;
    bool skip { false };
    int64_t base { 0 };
    rt::time_point start, latest;
    int64_t update { 0 };
    double timebase { 0.0 };
    double diff { 0.0 }, prog { 0.0 }, wclk { 0.0 }, rclk { 0.0 };
    auto pushwclk() { wclk = update * timebase * 1000000; };
    auto progressed() { prog = (update - base) * timebase * 1000000; }
    auto pushrclk() {
        rclk = std::chrono::duration<double, std::micro>(latest - start).count();
        if (prog != 0) {
            rclk += prog;
        }
    };
    auto pushdiff() { diff = wclk - rclk; };
};
struct PlayerState {
    // vars changed by different files
    FmtCtxPtr formatCtx { nullptr };
    DecCtxPtr videoDecCtx { nullptr };
    DecCtxPtr audioDecCtx { nullptr };
    SwrCtxPtr audioSwrCtx { nullptr };
    PktPtr packet { nullptr };
    MediaInfo mediaInfo;
    // vars changed by different frames
    FrmPtr decAudioFrm { nullptr };
    FrmPtr myAudioFrm { nullptr };
    std::array<uint8_t*, 1> audioBuffer { }; // container of the pointer to audio buffer
    int audioBufferSize { 0 };
    int audioBufferRemains { 0 };
    size_t readPos { 0 };
    FrmPtr decVideoFrm { nullptr };
    FrmPtr myVideoFrm { nullptr };
    std::array<uint8_t*, 3> videoBufferHead { }; // container of the pointer to video buffer
    std::array<int, 3> videoBufferSize { };
    int videoBufferRemains { 0 };
    Clock audioClock, videoClock, exClock;
    std::queue<AudioChunk> chunks;
    ChunkQueue chunkQueue;
    FrameQueue frameQueue;
    int serial { 0 };
    int64_t estAudioPTS { 0 };
    int64_t audioPTS { 0 };
    int64_t estVideoPTS { 0 };
    int64_t videoPTS { 0 };
    // double currentProgress { 0.0 };
    bool progressChanged { false };
    bool apc { false };
    bool isplay { true };
    bool topause { false };
    std::mutex pasueMutex;
    std::condition_variable pausecv;
    bool aflush { false };
    bool vflush { false };
    bool toquit { false };
    auto flusha() { aflush = true; };
    auto flushv() { vflush = true; };

    auto pause() { topause = true; };
    auto play() {
        topause = false;
        pausecv.notify_all();
    };
    auto quit() { toquit = true; }
    auto reset() { };
    auto update() {
        if (toquit) {
            return -1;
        };
        return 0;
    };
    auto audioUpdate() {
        if (aflush == true) return -1;
        return 0;
    };
    auto videoUpdate() {
        if (vflush == true) return -1;
        return 0;
    }
};

using PlayerStatePtr = std::shared_ptr<PlayerState>;
Q_DECLARE_METATYPE(PlayerStatePtr);

/*
       //for ai chat
       QObject::connect(send, &QPushButton::clicked, [&]() {
           auto a = texeditor->text();
           auto b = a.toStdString();
           robot.recv(b);
           texeditor->clear();
       });
        QObject::connect(&robot, &Robot::get, [&]() {
           texeditor->clear();
           user->clear();
           user->append(robot.words);
       });
       //for audio push mode
       QObject::connect(
           demuxer, &DemuxerPlusDecoder::chunkReady, audioWidget, &MyAudioWidget::chunkIn);*/
// robot.moveToThread(thread);