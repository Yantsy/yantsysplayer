#pragma once
#include <iostream>
extern "C" {
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <libswresample/swresample.h>
}
#include "public.h"
#include "resampler.h"
struct MyAudioWidget {

    MyAudioWidget() {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Can't init SDL\n";
        }
        std::cout << "Init SDL_Audio success\n";
    };

    ~MyAudioWidget() { SDL_Quit(); };
    auto copyParameters(MediaInfo& mediaInfo) {
        MyResampler myResampler;
        wantedSpec.freq     = mediaInfo.splRate;
        wantedSpec.format   = myResampler.toSDLAudioFmt(mediaInfo.sampleFmt);
        wantedSpec.channels = mediaInfo.channels;
        wantedSpec.samples  = mediaInfo.samples;
        wantedSpec.silence  = mediaInfo.silence;
        wantedSpec.callback = nullptr;

        audioDevice = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &obtainedSpec, 0);
        if (audioDevice == 0) {
            SDL_Quit();
        }
        SDL_PauseAudioDevice(audioDevice, 0);
    };

    SDL_AudioSpec wantedSpec { }, obtainedSpec { };
    SDL_AudioDeviceID audioDevice;
};
