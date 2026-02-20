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
  auto open(MediaInfo &mediaInfo) {
    MyResampler myResampler;
    audioQueue.format = myResampler.fmtNameTrans(mediaInfo.sampleFmt);
    wantedSpec.freq = mediaInfo.splRate;
    wantedSpec.format = audioQueue.format;
    wantedSpec.channels = mediaInfo.channels;
    wantedSpec.samples = mediaInfo.samples;
    wantedSpec.silence = mediaInfo.silence;
    wantedSpec.callback = callBack;
    wantedSpec.userdata = &audioQueue;

    audioDevice = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &obtainedSpec, 0);
    if (audioDevice == 0) {
      SDL_Quit();
    }
    SDL_PauseAudioDevice(audioDevice, 0);
  };

  static void callBack(void *puserData, uint8_t *pstream, int plen) {
    auto *audioState = static_cast<AudioQueue *>(puserData);
    SDL_memset(pstream, 0, plen);

    int remaining = plen;
    uint8_t *out = pstream;

    std::lock_guard<std::mutex> lock(audioState->mutex);
    while (remaining > 0) {
      if (audioState->currentIndex >= audioState->currentBuffer.size()) {
        if (audioState->packets.empty()) {
          return;
        }
        audioState->currentBuffer = std::move(audioState->packets.front());
        audioState->packets.pop();
        audioState->currentIndex = 0;
      }
      size_t available =
          audioState->currentBuffer.size() - audioState->currentIndex;
      size_t tocopy = std::min(available, (size_t)remaining);

      SDL_MixAudioFormat(
          out, audioState->currentBuffer.data() + audioState->currentIndex,
          audioState->format, tocopy, SDL_MIX_MAXVOLUME);
      audioState->currentIndex += tocopy;
      out += tocopy;
      remaining -= tocopy;
    }
  };

  auto buffer(int pchannels, int psamples, AVSampleFormat psprFmt) {
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
    std::queue<std::vector<uint8_t>> packets;
    std::mutex mutex;
    size_t currentIndex = 0;
    std::vector<uint8_t> currentBuffer;
  } audioQueue;

  SDL_AudioSpec wantedSpec{}, obtainedSpec{};
  SDL_AudioDeviceID audioDevice;
  auto bufferManage(Uint8 *pstream, int plen) {}
};
