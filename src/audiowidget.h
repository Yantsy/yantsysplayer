#pragma once
#include <iostream>
#include <mutex>
#include <queue>
extern "C" {
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <libswresample/swresample.h>
}
#include "public.h"
#include "resampler.h"
class MyAudioWidget {
public:
    MyAudioWidget() {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Can't init SDL\n";
        }
        std::cout << "Init SDL_Audio success\n";
    };

    ~MyAudioWidget() { SDL_Quit(); };
    auto open(MediaInfo& mediaInfo) {
        MyResampler myResampler;
        audioQueue.format   = myResampler.fmtNameTrans(mediaInfo.sampleFmt);
        wantedSpec.freq     = mediaInfo.splRate;
        wantedSpec.format   = audioQueue.format;
        wantedSpec.channels = mediaInfo.channels;
        wantedSpec.samples  = mediaInfo.samples;
        wantedSpec.silence  = mediaInfo.silence;
        wantedSpec.callback = nullptr;
        wantedSpec.userdata = &audioQueue;

        audioDevice = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &obtainedSpec, 0);
        if (audioDevice == 0) {
            SDL_Quit();
        }
        SDL_PauseAudioDevice(audioDevice, 0);
    };

    static void callBack(void* puserData, uint8_t* pstream, int plen) {

    };

    auto bufferSize(int pchannels, int psamples, AVSampleFormat psprFmt) {
        auto pBufSize = psamples * pchannels;
        switch (psprFmt) {
        case AV_SAMPLE_FMT_S16: {
            pBufSize *= 2;
        } break;
        case AV_SAMPLE_FMT_S32: {
            pBufSize *= 4;
        } break;
        case AV_SAMPLE_FMT_FLT: {
            pBufSize *= 4;
        } break;
        default:
            pBufSize *= 1;
            break;
        }
        return pBufSize;
    };

    struct AudioQueue {
        SDL_AudioFormat format;
    } audioQueue;

    SDL_AudioSpec wantedSpec { }, obtainedSpec { };
    SDL_AudioDeviceID audioDevice;
};
