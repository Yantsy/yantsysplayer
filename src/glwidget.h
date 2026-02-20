#pragma once
#include <QElapsedTimer>
#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPixmap>
#include <QTimer>

extern "C" {
#include <libavutil/imgutils.h>
}

class MyGLWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
  Q_OBJECT
public:
  explicit MyGLWidget(QWidget *parent = nullptr) noexcept;
  ~MyGLWidget();
  // void render(uint8_t *Y, uint8_t *U, uint8_t *V, char *ppxFmt) noexcept;
  void renderWithOpenGL8(uint8_t *Y, uint8_t *U, uint8_t *V, int &w, int &h,
                         int &strideY, int &strideUV,
                         const char &ppxFmt) noexcept;
  void renderWithOpenGL10(uint8_t *Y, uint8_t *U, uint8_t *V, int &w, int &h,
                          int &strideY, int &strideUV,
                          const char &ppxFmt) noexcept;

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
  // void resizeEvent(QResizeEvent *event) override;

private:
  QOpenGLVertexArrayObject *m_vao0 = nullptr, *m_vao1 = nullptr;
  // QOpenGLBuffer *m_vbo0 = nullptr, *m_vbo1 = nullptr;
  QOpenGLShaderProgram *m_shaderProgram0 = nullptr, *m_shaderProgram1 = nullptr;
  // QOpenGLShader *m_vertexShader = nullptr, *m_fragmentShader = nullptr;
  QOpenGLTexture *m_texture0 = nullptr, *m_texture1 = nullptr,
                 *m_textureY = nullptr, *m_textureU = nullptr,
                 *m_textureV = nullptr;

  // QOpenGLBuffer *m_vbo0 = nullptr, *m_ebo0 = nullptr;
  QMatrix4x4 transform;
  const float screenAspectRatio = 16.0f / 9.0f;
  const float screenVerseRatio = 9.0f / 16.0f;
  int paintCount = 0;
  int renderCount = 0;

  float imageWidth = 0.0f;
  float imageHeight = 0.0f;
  float windowWidth = 0.0f;
  float windowHeight = 0.0f;
  bool isTenbit = false;
  QMatrix4x4 imageScaleMatrix(float imgWidth, float imgHeight);
  QMatrix4x4 windowScaleMatrix(float winWidth, float winHeight);
  QMatrix4x4 transformMatrix(float ww, float wh, float iw, float ih);
  QMatrix4x4 transformMatrix2(float ww, float wh, float iw, float ih);
  QElapsedTimer *timer = nullptr;

private slots:
};