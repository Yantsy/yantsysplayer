#pragma once
#include "public.h"
#include <mutex>
class Buffer : public QObject {
    Q_OBJECT

signals:
    void hasFrame(VideoFrame& pvideoFrame, std::mutex& pmutex);
    void hasChunk(AudioChunk& paudioChunk, std::mutex& pmutex);

private:
    FrameQueue frameQueue;
    ChunkQueue chunkQueue;
    std::mutex frameQueueMutex;
    std::mutex chunkQueueMutex;
    auto pushFrame(VideoFrame& pFrame) {
        std::lock_guard<std::mutex> lock(frameQueueMutex);
        frameQueue.bufferredFrm.push(std::move(pFrame));
        auto frame = std::move(frameQueue.bufferredFrm.front());
        emit hasFrame(frame, frameQueueMutex);
    };
    auto pushChunk(AudioChunk& pChunk) {
        std::lock_guard<std::mutex> lock(chunkQueueMutex);
        chunkQueue.bufferredPcm.push(std::move(pChunk));
        auto chunk = std::move(chunkQueue.bufferredPcm.front());
        emit hasChunk(chunk, chunkQueueMutex);
    };
public slots:
    void frameIn(VideoFrame& pFrame) { pushFrame(pFrame); };
    void chunkIn(AudioChunk& pChunk) { pushChunk(pChunk); };
    void popFrame() {
        if (!frameQueue.bufferredFrm.empty()) frameQueue.bufferredFrm.pop();
    }
    void popChunk() {
        if (!chunkQueue.bufferredPcm.empty()) chunkQueue.bufferredPcm.pop();
    }

public:
    Buffer() { };
    ~Buffer() { };
};