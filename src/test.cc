#include "QFileDialog"
#include "mediaplayer.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Open Media File", "");
    if (filePath.isEmpty()) {
        return -1;
    }
    MediaPlayer mediaPlayer;
    mediaPlayer.open(filePath.toStdString().c_str());
    return a.exec();
}