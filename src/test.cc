#include "QFileDialog"

#include "demuxer.h"
#include "public.h"
#include "videoWidget_gl.h"
#include "yslider.h"
#include <QApplication>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

#include "audioWidget_sdl2.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    // regeister user-defined objects;
    qRegisterMetaType<AudioInfo>("AudioInfo&");
    qRegisterMetaType<VideoInfo>("VideoInfo&");
    qRegisterMetaType<std::shared_ptr<VideoFrame>>("std::shared_ptr<VideoFrame>");
    qRegisterMetaType<std::shared_ptr<AudioChunk>>("std::shared_ptr<AudioChunk>");
    qRegisterMetaType<PlayerStatePtr>("PlayerStatePtr");
    // ui
    QWidget window;
    window.setWindowTitle("Yantsys Player");
    auto* layout      = new QVBoxLayout(&window);
    auto* pickFileBtn = new QPushButton("获取文件");
    auto* play        = new QPushButton("Play");
    play->setFixedWidth(30);
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
    layout->addWidget(play, 1);
    window.resize(960, 640);
    window.show();

    bool isPlaying                       = false;
    QPointer<QThread> thread             = nullptr;
    QPointer<DemuxerPlusDecoder> demuxer = nullptr;
    QPointer<MyAudioWidget> audioWidget  = nullptr;

    QObject::connect(pickFileBtn, &QPushButton::clicked, &window, [&]() {
        // clear previous resources
        if (isPlaying) {
            isPlaying = false;
            audioWidget->quit();
            videoWidget.quit();
            demuxer->quit();
            demuxer->deleteLater();
            thread->quit();
            thread->wait();
            QObject::disconnect(play, &QPushButton::clicked, nullptr, nullptr);
            QObject::disconnect(&slider, &yslider::dragFinished, nullptr, nullptr);
        };
        const QString filePath = QFileDialog::getOpenFileName(&window, "Open Media File", "");
        if (filePath.isEmpty()) {
            return;
        }

        isPlaying = true;
        // pickFileBtn->setEnabled(false);
        pickFileBtn->setText("播放中...");

        thread      = new QThread(&window);
        audioWidget = new MyAudioWidget();
        demuxer     = new DemuxerPlusDecoder();
        demuxer->open(filePath.toStdString());
        // add demuxer to main thread before the video is played
        demuxer->moveToThread(thread);
        // link signals with slots to start the two threads
        // while the video is playing
        QObject::connect(thread, &QThread::started, demuxer, &DemuxerPlusDecoder::processStart);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::durationChanged, &slider, &yslider::setMaximum);
        QObject::connect(
            &slider, &yslider::dragFinished, [&]() { demuxer->progressChange(slider.tvalue); });
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::sendVideoInfo, &videoWidget, &MyGLWidget::getInfo);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::sendAudioInfo, audioWidget, &MyAudioWidget::getInfo);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::frameReady, &videoWidget, &MyGLWidget::frameIn);
        QObject::connect(&videoWidget, &MyGLWidget::progressChanged, &slider, &yslider::setValue);
        /*
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::chunkReady, audioWidget, &MyAudioWidget::chunkIn);*/
        //  clear resources of the main thread when the sub thread is finished
        //  after the video is played
        QObject::connect(demuxer, &DemuxerPlusDecoder::finished, thread, &QThread::quit);
        QObject::connect(demuxer, &DemuxerPlusDecoder::finished, demuxer, &QObject::deleteLater);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::finished, audioWidget, &QObject::deleteLater);
        // reset the button when the thread is finished(or say when the presentation of a video is
        // finished),then you can choose a new video to restart the whole cycle
        QObject::connect(thread, &QThread::finished, &window, [&]() {
            isPlaying = false;
            pickFileBtn->setEnabled(true);
            pickFileBtn->setText("获取文件");
            thread = nullptr;
        });
        QObject::connect(play, &QPushButton::clicked, [&]() {
            if (isPlaying) {
                isPlaying = false;
                demuxer->pause();
                audioWidget->pause();
            } else {
                isPlaying = true;
                demuxer->play();
                audioWidget->play();
            }
        });
        QObject::connect(&window, &QWidget::destroyed, [&]() {
            if (demuxer && thread && thread->isRunning()) {
                demuxer->quit();
                thread->quit();
                thread->wait(3000); // 最多等待3秒
            }
        });
        thread->start();
    });
    // when the application is finished,or say when you close the main window,quit the main thread
    QObject::connect(&a, &QApplication::aboutToQuit, &window, [&]() {
        if (thread && thread->isRunning()) {
            thread->quit();
            thread->wait();
        }
    });

    return a.exec();
}
