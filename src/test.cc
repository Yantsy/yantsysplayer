#include "QFileDialog"

#include "demuxer.h"
#include "public.h"
#include "videoWidget_gl.h"
#include "yslider.h"
#include <QApplication>
#include <QLineEdit>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QTextEdit>
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
    window.setWindowTitle("Yantsy's Player");
    auto* layout    = new QVBoxLayout(&window);
    auto* sublayout = new QHBoxLayout();
    auto* texeditor = new QLineEdit("请输入文本：......");
    // auto* user      = new QTextEdit("......");
    // user->setFixedHeight(30);
    auto* pickFileBtn = new QPushButton("获取文件");
    auto* send        = new QPushButton("send");
    auto* play        = new QPushButton("Play");
    play->setFixedWidth(30);

    yslider slider;
    MyGLWidget videoWidget;
    slider.getpara(8, 8);
    slider.setFixedHeight(10);
    slider.setgroovecolor(QColor(255, 255, 255));
    slider.settracecolor(QColor(51, 232, 219));
    slider.sethandelcolor(QColor(235, 88, 88));
    layout->addWidget(&videoWidget);
    layout->addWidget(&slider, 1);
    sublayout->addWidget(pickFileBtn);
    sublayout->addWidget(play, 1);
    sublayout->addWidget(texeditor);
    sublayout->addWidget(send);
    layout->addLayout(sublayout, 1);
    // layout->addWidget(user);
    window.resize(960, 680);
    window.show();

    bool isPlaying                       = false;
    bool hasVideo                        = false;
    QPointer<QThread> thread             = nullptr;
    QPointer<DemuxerPlusDecoder> demuxer = nullptr;
    QPointer<MyAudioWidget> audioWidget  = nullptr;
    // Robot robot;

    QObject::connect(pickFileBtn, &QPushButton::clicked, &window, [&]() {
        // clear previous resources
        if (hasVideo) {
            hasVideo = false;
            audioWidget->quit();
            demuxer->quit();
            videoWidget.quit();
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
        hasVideo  = true;
        isPlaying = true;
        // pickFileBtn->setEnabled(false);
        pickFileBtn->setText("播放中...");

        thread      = new QThread(&window);
        audioWidget = new MyAudioWidget();
        demuxer     = new DemuxerPlusDecoder();

        demuxer->open(filePath.toStdString());
        // add demuxer to main thread before the video is played
        demuxer->moveToThread(thread);

        //  link signals with slots to start the two threads
        //  while the video is playing
        QObject::connect(thread, &QThread::started, demuxer, &DemuxerPlusDecoder::processStart);
        QObject::connect(demuxer, &DemuxerPlusDecoder::durationChanged,
            [&](int64_t duration) { slider.setMaximum(duration); });
        QObject::connect(
            &slider, &yslider::dragFinished, [&]() { demuxer->progressChange(slider.tvalue); });
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::sendVideoInfo, &videoWidget, &MyGLWidget::getInfo);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::sendAudioInfo, audioWidget, &MyAudioWidget::getInfo);
        QObject::connect(
            demuxer, &DemuxerPlusDecoder::frameReady, &videoWidget, &MyGLWidget::frameIn);
        QObject::connect(&videoWidget, &MyGLWidget::progressChanged, &slider, &yslider::setValue);

        QObject::connect(thread, &QThread::finished, &window, [&]() {
            isPlaying = false;
            hasVideo  = false;
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
                if (!hasVideo) return;
                isPlaying = true;
                demuxer->play();
                audioWidget->play();
            }
        });
        thread->start();
    });
    // when the application is finished,or say when you close the main window,quit the main thread
    QObject::connect(&a, &QApplication::aboutToQuit, &window, [&]() {
        if (hasVideo) {
            audioWidget->quit();
            demuxer->quit();
            videoWidget.quit();
            if (demuxer != nullptr) demuxer->deleteLater();
        };

        if (thread && thread->isRunning()) {
            if (thread != nullptr) thread->quit();
            thread->wait();
        }
    });

    return a.exec();
}
