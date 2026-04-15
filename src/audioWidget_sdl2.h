#pragma once
#include <iostream>
extern "C" {

#include <libswresample/swresample.h>
}
#include "public.h"
#include "resampler.h"
class MyAudioWidget : public QObject {
    Q_OBJECT
signals:
    void returnTime(rt::time_point now);

private:
    SDL_AudioSpec wantedSpec { }, obtainedSpec { };
    SDL_AudioDeviceID audioDevice;
    int bytesPerSample { 0 }, bytesPerSecond { 0 };
    double timebase { 0.0 };
    bool pullmode { true }; // set pull mode the default and set it false for the push mode
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point update;
    // pull mode
    static void sdlCallBack(void* userdata, uint8_t* stream, int len) {
        // fill len bytes 0 in to stream to make sure that it is always fully filled
        SDL_memset(stream, 0, len);
        if (userdata == nullptr) return;
        auto is = static_cast<PlayerState*>(userdata);
        // the job of callback function is to send data of len bytes from src of of audioBufferSize
        // bytes to dst whose size is len bytes,too,and the key is that you have to always fill the
        // stream and control how much you can fill it in one cycle

        auto dst       = stream;
        auto remaining = len; // remaining of dst

        while (remaining > 0 && is != nullptr) {
            if (is->chunks.empty()) break;

            auto& src           = is->chunks.front().pcm;
            int audioBufferSize = is->chunks.front().audioBufferSize;
            // the most you can get from src
            int available = audioBufferSize - is->readPos;
            // the amount you copy from src
            // if readPos(0)+len=remaining<available,then remaining-=tocopy=0,the cycle ends
            // else readPos(0)+len=remaing>available,then remaining -=tocopy>0,you pop current chunk
            // and enter the next cycle to get a new chunk,and send data from the new src to the new
            // dst
            int tocopy = std::min(available, remaining);
            SDL_memcpy(dst, src.data() + is->readPos, tocopy);
            is->readPos += tocopy;
            dst += tocopy;
            remaining -= tocopy;
            if (is->readPos >= audioBufferSize) {
                is->chunks.pop();
                is->readPos = 0;
            };
        };
        // is->videoClock.masterclock = is->chunks.front().pts;
    };
    //
    auto initialize(PlayerStatePtr is) {
        MyResampler myResampler;
        auto audioInfo      = is->mediaInfo.audioInfo;
        wantedSpec.freq     = audioInfo.splRate;
        wantedSpec.format   = myResampler.toSDLAudioFmt(audioInfo.sampleFmt);
        wantedSpec.channels = audioInfo.channels;
        wantedSpec.samples  = audioInfo.samples;
        wantedSpec.silence  = audioInfo.silence;
        wantedSpec.callback = sdlCallBack; // set nullptr if in push mode
        wantedSpec.userdata = is.get();
        if (!pullmode) {
            wantedSpec.callback = nullptr; // set nullptr if in push mode
            wantedSpec.userdata = nullptr;
        }
        bytesPerSample = av_get_bytes_per_sample(audioInfo.sampleFmt);
        bytesPerSecond = wantedSpec.freq * wantedSpec.channels * bytesPerSample;
        timebase       = audioInfo.atimeBase;
        audioDevice    = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &obtainedSpec, 0);
        if (audioDevice == 0) {
            SDL_Quit();
        }
        SDL_PauseAudioDevice(audioDevice, 0);
    };
    // push mode
    auto playLoop(std::shared_ptr<AudioChunk> audioChunk) {
        SDL_QueueAudio(
            audioDevice, audioChunk.get()->pcm.data(), (Uint32)audioChunk.get()->pcm.size());
        // emit returnTime(latestDuration);
    }; //
public slots:
    void chunkIn(std::shared_ptr<AudioChunk> audioChunk) { playLoop(audioChunk); };
    void getInfo(PlayerStatePtr is) { initialize(is); };

public:
    MyAudioWidget() {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Can't init SDL\n";
        }
        std::cout << "Init SDL_Audio success\n";
    };

    ~MyAudioWidget() { SDL_Quit(); };
    void pause() { SDL_PauseAudioDevice(audioDevice, 1); }
    void play() { SDL_PauseAudioDevice(audioDevice, 0); }
    void quit() {
        SDL_PauseAudioDevice(audioDevice, 1);
        SDL_CloseAudioDevice(audioDevice);
    }
};

/*
pushmode:feed data to SDL_QueuedAudio,so you don't have to know whether there are data in the
queue,you only need to control the decodeAudio and the chunkReady signal

pullmode:the device asks for data ,so you don't need to know whether the decodeAudio is working now
as long as the queue is not empty,instead you control the state of device.
*/