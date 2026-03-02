#pragma once

#include <memory>
#include <queue>

#include "audioWidget_sdl2.h"
#include "buffer.h"

#include "demuxer.h"
#include "public.h"
#include "videoWidget_gl.h"

// resource management
class MediaPlayer : public QObject {
    Q_OBJECT
private:
public:
    MediaPlayer() { };
    ~MediaPlayer() { };
    auto play(const char* pFile) { }
};