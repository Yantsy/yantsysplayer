#include "QFileDialog"
#include "mediaplayer.h"
#include "yslider.h"
#include <QApplication>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    // regeister user-defined objects;
    qRegisterMetaType<AudioInfo>("AudioInfo&");
    qRegisterMetaType<VideoInfo>("VideoInfo&");
    qRegisterMetaType<std::shared_ptr<VideoFrame>>("std::shared_ptr<VideoFrame>");
    qRegisterMetaType<std::shared_ptr<AudioChunk>>("std::shared_ptr<AudioChunk>");
    // ui
    QWidget window;
    window.setWindowTitle("Yantsys Player");
    auto* layout      = new QVBoxLayout(&window);
    auto* pickFileBtn = new QPushButton("获取文件");
    MyGLWidget videoWidget;
    yslider slider;
    slider.getpara(8, 8);
    slider.setFixedHeight(10);
    slider.setgroovecolor(QColor(255, 255, 255));
    slider.settracecolor(QColor(51, 232, 219));
    slider.sethandelcolor(QColor(235, 88, 88));
    layout->addWidget(pickFileBtn);
    layout->addWidget(&videoWidget, 1);
    layout->addWidget(&slider, 1);
    window.resize(960, 640);
    window.show();

    bool isPlaying           = false;
    QPointer<QThread> thread = nullptr;

    QObject::connect(pickFileBtn, &QPushButton::clicked, &window, [&]() {
        if (isPlaying || (thread && thread->isRunning())) {
            return;
        }

        const QString filePath = QFileDialog::getOpenFileName(&window, "Open Media File", "");
        if (filePath.isEmpty()) {
            return;
        }

        isPlaying = true;
        pickFileBtn->setEnabled(false);
        pickFileBtn->setText("播放中...");

        auto* audioWidget = new MyAudioWidget();
        thread            = new QThread(&window);
        auto* demuxer     = new DemuxerPlusDecoder(filePath.toStdString());
        demuxer->moveToThread(thread);
        QObject::connect(thread, &QThread::started, demuxer, &DemuxerPlusDecoder::processStart);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::durationChanged, &slider, &yslider::setMaximum);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::sendVideoInfo, &videoWidget, &MyGLWidget::getInfo);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::sendAudioInfo, audioWidget, &MyAudioWidget::getInfo);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::frameReady, &videoWidget, &MyGLWidget::frameIn);
        QObject::connect(&videoWidget, &MyGLWidget::progressChanged, &slider, &yslider::setValue);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::chunkReady, audioWidget, &MyAudioWidget::chunkIn);

        QObject::connect(demuxer, &DemuxerPlusDecoder::finished, thread, &QThread::quit);
        QObject::connect(demuxer, &DemuxerPlusDecoder::finished, demuxer, &QObject::deleteLater);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::finished, audioWidget, &QObject::deleteLater);
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        QObject::connect(thread, &QThread::finished, &window, [&]() {
            isPlaying = false;
            pickFileBtn->setEnabled(true);
            pickFileBtn->setText("获取文件");
            thread = nullptr;
        });

        thread->start();
    });

    QObject::connect(&a, &QApplication::aboutToQuit, &window, [&]() {
        if (thread && thread->isRunning()) {
            thread->quit();
            thread->wait();
        }
    });

    return a.exec();
}
