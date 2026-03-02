#include "QFileDialog"
#include "mediaplayer.h"
#include <QApplication>
#include <QObject>
#include <QThread>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    qRegisterMetaType<AudioInfo>("AudioInfo&");
    qRegisterMetaType<VideoInfo>("VideoInfo&");
    qRegisterMetaType<std::shared_ptr<VideoFrame>>("std::shared_ptr<VideoFrame>");
    qRegisterMetaType<std::shared_ptr<AudioChunk>>("std::shared_ptr<AudioChunk>");
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Open Media File", "");
    if (filePath.isEmpty()) {
        return -1;
    }
    auto file = filePath.toStdString();
    MyGLWidget videoWidget;
    MyAudioWidget audioWidget;
    videoWidget.show();
    QThread thread;
    auto processor = new DemuxerPlusDecoder(file);
    processor->moveToThread(&thread);

    QObject::connect(&thread, &QThread::started, processor, &DemuxerPlusDecoder::processStart);

    QObject::connect(
        processor, &DemuxerPlusDecoder::sendVideoInfo, &videoWidget, &MyGLWidget::getInfo);

    QObject::connect(
        processor, &DemuxerPlusDecoder::sendAudioInfo, &audioWidget, &MyAudioWidget::getInfo);
    QObject::connect(
        processor, &DemuxerPlusDecoder::frameReady, &videoWidget, &MyGLWidget::frameIn);
    QObject::connect(
        processor, &DemuxerPlusDecoder::chunkReady, &audioWidget, &MyAudioWidget::chunkIn);
    QObject::connect(processor, &DemuxerPlusDecoder::finished, &thread, &QThread::quit);
    QObject::connect(processor, &DemuxerPlusDecoder::finished, processor, &QObject::deleteLater);
    QObject::connect(
        &audioWidget, &MyAudioWidget::returnTime, processor, &DemuxerPlusDecoder::getAudioTime);

    thread.start();

    int ret = a.exec();
    thread.quit();
    thread.wait();
    return ret;
}