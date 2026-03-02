#pragma once
#include <iostream>
extern "C" {
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <libswresample/swresample.h>
}

class MyResampler {
public:
    auto alcSwrCtx(const AVChannelLayout& pChnlLyt, const int& pFreq, AVSampleFormat& pinFmt,
        const AVSampleFormat& pOutFmt) const noexcept {
        auto pSwrCtx = swr_alloc();
        swr_alloc_set_opts2(
            &pSwrCtx, &pChnlLyt, pOutFmt, pFreq, &pChnlLyt, pinFmt, pFreq, 0, nullptr);
        swr_init(pSwrCtx);
        return pSwrCtx;
    }

    auto toSDLAudioFmt(const AVSampleFormat& psprFmt) const noexcept {
        switch (psprFmt) {
        case AV_SAMPLE_FMT_U8: {
            return AUDIO_U8;
        } break;
        case AV_SAMPLE_FMT_U8P: {
            return AUDIO_U8;
        } break;
        case AV_SAMPLE_FMT_S16: {
            return AUDIO_S16;
        } break;
        case AV_SAMPLE_FMT_S16P: {
            return AUDIO_S16;
        } break;
        case AV_SAMPLE_FMT_S32: {
            return AUDIO_S32;
        } break;
        case AV_SAMPLE_FMT_S32P: {
            return AUDIO_S32;
        } break;
        default:
            // 对于DBLP，以及64-bit的数据，直接看作F32
            return AUDIO_F32;
        }
    }

    auto toPackedFmt(const AVSampleFormat& psprFmt) const noexcept {
        switch (psprFmt) {
        case AV_SAMPLE_FMT_U8P: {
            return AV_SAMPLE_FMT_U8;
        } break;
        case AV_SAMPLE_FMT_S16P: {
            return AV_SAMPLE_FMT_S16;
        } break;
        case AV_SAMPLE_FMT_S32P: {
            return AV_SAMPLE_FMT_S32;
        } break;
        case AV_SAMPLE_FMT_FLTP: {
            return AV_SAMPLE_FMT_FLT;
        } break;
        default:
            return psprFmt;
        }
    }

private:
    bool isPlannar = false;
};