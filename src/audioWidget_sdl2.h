#pragma once
#include <iostream>
extern "C" {

#include <libswresample/swresample.h>
}
#include "buffer.h"
#include "public.h"
#include "resampler.h"
class MyAudioWidget : public QObject {
    Q_OBJECT
signals:
    void returnTime(double realDuration);

private:
    SDL_AudioSpec wantedSpec { }, obtainedSpec { };
    SDL_AudioDeviceID audioDevice;
    int bytesPerSample { 0 }, bytesPerSecond { 0 };
    double timebase { 0.0 };
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point update;
    static void sdlCallBack(void* userdata, uint8_t* stream, int len) {
        APtrSet* buffer = static_cast<APtrSet*>(userdata);
    };
    auto initialize(AudioInfo& paudioInfo, void* buffer) {
        MyResampler myResampler;
        wantedSpec.freq     = paudioInfo.splRate;
        wantedSpec.format   = myResampler.toSDLAudioFmt(paudioInfo.sampleFmt);
        wantedSpec.channels = paudioInfo.channels;
        wantedSpec.samples  = paudioInfo.samples;
        wantedSpec.silence  = paudioInfo.silence;
        wantedSpec.callback = nullptr; // set nullptr for the push mode
        wantedSpec.userdata = buffer;
        bytesPerSample      = av_get_bytes_per_sample(paudioInfo.sampleFmt);
        bytesPerSecond      = wantedSpec.freq * wantedSpec.channels * bytesPerSample;
        timebase            = paudioInfo.atimeBase;

        audioDevice = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &obtainedSpec, 0);
        if (audioDevice == 0) {
            SDL_Quit();
        }
        SDL_PauseAudioDevice(audioDevice, 0);
    };
    // push mode
    auto playLoop(std::shared_ptr<AudioChunk> audioChunk) {
        SDL_QueueAudio(
            audioDevice, audioChunk.get()->pcm.data(), (Uint32)audioChunk.get()->pcm.size());

        auto latestDuration = audioChunk->pts * timebase;
        auto delay          = SDL_GetQueuedAudioSize(audioDevice) / bytesPerSecond;
        latestDuration -= delay;
        // emit returnTime(latestDuration);
    };
public slots:
    void chunkIn(std::shared_ptr<AudioChunk> audioChunk) { playLoop(audioChunk); };
    void getInfo(AudioInfo paudioInfo, void* pbuffer) { initialize(paudioInfo, pbuffer); };

public:
    MyAudioWidget() {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Can't init SDL\n";
        }
        std::cout << "Init SDL_Audio success\n";
    };

    ~MyAudioWidget() { SDL_Quit(); };
};
