#include "yslider.h"
yslider::yslider(QWidget* parent)
    : QAbstractSlider(parent) {
    this->setOrientation(Qt::Horizontal);
}
void yslider::getpara(int pxr, int pyr) {

    xr = pxr;
    yr = pyr;
}

void yslider::setgroovecolor(QColor color) { groovecolor = color; }

void yslider::settracecolor(QColor color) { tracecolor = color; }

void yslider::sethandelcolor(QColor color) { handelcolor = color; }

yslider::~yslider() { }

bool yslider::isdragging() { return dragging; }

// bool yslider::under_control(bool control) { return dragging = control; }

void yslider::paintEvent(QPaintEvent* event) {
    QPainter groove(this);
    double ratio = (double)value() / (double)maximum();

    groove.setRenderHint(QPainter::Antialiasing);
    groove.setPen(Qt::NoPen);
    groove.setBrush(groovecolor.rgb());
    groove.drawRoundedRect(this->rect(), xr, yr);

    groove.setRenderHint(QPainter::Antialiasing);
    groove.setPen(Qt::NoPen);
    groove.setBrush(tracecolor.rgb());

    groove.drawRoundedRect(
        this->rect().x(), this->rect().y() - 2, width() * ratio, height() - 4, xr, yr);

    groove.setRenderHint(QPainter::Antialiasing);
    groove.setPen(Qt::NoPen);
    groove.setBrush(handelcolor.rgb());

    if (this->rect().x() + width() * ratio < this->rect().width() - 6) {
        groove.drawRoundedRect(
            this->rect().x() + width() * ratio, this->rect().y() + 2, 6, 12, xr, yr);
    } else {
        groove.drawRoundedRect(this->rect().width() - 6, this->rect().y() + 2, 6, 12, xr, yr);
    }

    // update();
}

void yslider::mousePressEvent(QMouseEvent* event) {
    dragging = true;

    emit dragStarted();

    double tratio = (double)event->position().x() / (double)this->width();
    tvalue        = tratio * maximum();
    setValue(tvalue);

    update();
}

void yslider::mouseMoveEvent(QMouseEvent* event) {

    if (event->buttons() & Qt::LeftButton) {

        double tratio = (double)event->position().x() / (double)this->width();
        tvalue        = tratio * maximum();

        setValue(tratio * maximum());

        update();
    }
}

void yslider::mouseReleaseEvent(QMouseEvent* event) {
    dragging = false;
    emit dragFinished();
    // QAbstractSlider::mouseReleaseEvent(event);
    double tratio = (double)event->position().x() / (double)this->width();
    tvalue        = tratio * maximum();

    setValue(tratio * maximum());

    update();
}

void yslider::forward() {
    if (value() < maximum()) {
        setValue(value() + 1);
        update();
    }
}

void yslider::backward() {
    if (value() > minimum()) {
        setValue(value() - 1);
        update();
    }
}
/*struct MonitorComponent : public QVideoWidget {
  QMediaPlayer *player;
  QAudioOutput *output;

  bool under_control = false;

  explicit MonitorComponent() noexcept
      : player{new QMediaPlayer{this}}, output{new QAudioOutput{this}} {

    output->setVolume(0.);

    constexpr auto location =
        "/home/creeper/workspace/alliance/robomaster/autoaim/detection.mp4";
    player->setAudioOutput(output);
    player->setVideoOutput(this);
    player->setSource(QUrl::fromLocalFile(location));

    player->play();
  }

  auto connect(creeper::Slider &slider) noexcept {
    QObject::connect(
        player, &QMediaPlayer::positionChanged, [&](qint64 position) {
          if (!under_control) {
            const auto duration = player->duration();
            const auto progress = static_cast<double>(position) / duration;
            slider.set_progress(progress);
          }
        });
  }

  auto set_under_control(bool control) noexcept { under_control = control; }

  auto set_progress(double progress) noexcept {
    progress = std::clamp(progress, 0., 1.);

    const auto duration = player->duration();
    const auto position = duration * progress;

    player->setPosition(position);
  }
};

// ......

auto monitor_component = new MonitorComponent{};
auto monitor_slider = new Slider{
    slider::pro::ThemeManager{manager},
    slider::pro::Measurements{Slider::Measurements::Xs()},
    slider::pro::FixedHeight{Slider::Measurements::Xs().minimum_height()},
    slider::pro::OnValueChange{[=](double value) {
      monitor_component->set_under_control(true);
      monitor_component->set_progress(value);
    }},
    slider::pro::OnValueChangeFinished{
        [=](double) { monitor_component->set_under_control(false); }},
};
monitor_component->connect(*monitor_slider);*/