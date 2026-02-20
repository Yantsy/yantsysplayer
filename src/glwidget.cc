#include "glwidget.h"
#include <iostream>
#include <thread>

MyGLWidget::MyGLWidget(QWidget *parent) noexcept : QOpenGLWidget(parent) {
  this->setContentsMargins(0, 0, 0, 0);
  this->setMinimumSize(600, 500);
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  // timer->start();
}

MyGLWidget::~MyGLWidget() {
  makeCurrent();
  delete m_vao0;
  delete m_vao1;
  delete m_shaderProgram0;
  delete m_shaderProgram1;
  // delete m_vbo0;
  // delete m_ebo0;
  delete m_texture0;
  delete m_texture1;
  delete m_textureY;
  delete m_textureU;
  delete m_textureV;
  doneCurrent();
  // std::cout << "PaintCount: " << paintCount << "\n";
  // std::cout << "RenderCount: " << renderCount << "\n" << std::endl;
  for (const auto &e :
       {"至", "是", "工", "程", "已", "毕", "，", "于", "斯", "合", "题"}) {
    std::cout << e << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "\n";
}

void MyGLWidget::renderWithOpenGL8(uint8_t *Y, uint8_t *U, uint8_t *V, int &w,
                                   int &h, int &strideY, int &strideUV,
                                   const char &ppxFmt) noexcept {
  // isTenbit = false;
  renderCount = renderCount + 1;
  imageWidth = w;
  imageHeight = h;
  bool needRecreate = !m_textureY || !m_textureY->isCreated() ||
                      m_textureY->width() != w || m_textureY->height() != h;
  switch (ppxFmt) {
  case AV_PIX_FMT_YUV420P: {
    makeCurrent();
    // 更新纹理
    if (needRecreate) {

      if (m_textureY) {
        m_textureY->destroy();
      } else {
        m_textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
      }
      m_textureY->setSize(imageWidth, imageHeight);
      m_textureY->setFormat(QOpenGLTexture::R8_UNorm);
      m_textureY->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
      m_textureY->setMagnificationFilter(QOpenGLTexture::Linear);
      m_textureY->allocateStorage();
      if (m_textureU) {
        m_textureU->destroy();
      } else {
        m_textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
      }
      m_textureU->setSize(imageWidth / 2, imageHeight / 2);
      m_textureU->setFormat(QOpenGLTexture::R8_UNorm);
      m_textureU->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
      m_textureU->setMagnificationFilter(QOpenGLTexture::Linear);
      m_textureU->allocateStorage();
      if (m_textureV) {
        m_textureV->destroy();
      } else {
        m_textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
      }
      m_textureV->setSize(imageWidth / 2, imageHeight / 2);
      m_textureV->setFormat(QOpenGLTexture::R8_UNorm);
      m_textureV->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
      m_textureV->setMagnificationFilter(QOpenGLTexture::Linear);
      m_textureV->allocateStorage();
    }
    // 处理linesize，如果不处理，读取数据的时候会出错,导致渲染出来的画面很奇怪,比如变成一堆竖线或者斜线...
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, strideY); // Y 平面的行长度

    m_textureY->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, Y);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, strideUV); // UV 平面的行长度

    m_textureU->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED,
                    GL_UNSIGNED_BYTE, U);

    m_textureV->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED,
                    GL_UNSIGNED_BYTE, V);

    // 重置
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    doneCurrent();
  } break;
  case AV_PIX_FMT_YUV420P10LE: {

  } break;
  }

  repaint();
  // 使用 repaint() 强制立即重绘，而不是 update()
}
void MyGLWidget::renderWithOpenGL10(uint8_t *Y, uint8_t *U, uint8_t *V, int &w,
                                    int &h, int &strideY, int &strideUV,
                                    const char &ppxFmt) noexcept {
  isTenbit = true;
  renderCount = renderCount + 1;
  imageWidth = w;
  imageHeight = h;
  bool needRecreate = !m_textureY || !m_textureY->isCreated() ||
                      m_textureY->width() != w || m_textureY->height() != h;
  switch (ppxFmt) {
  case AV_PIX_FMT_YUV420P10LE: {
    makeCurrent();
    // 在必要时重新创建纹理并分配内存，一般不会触发，但是就怕万一
    if (needRecreate) {

      if (m_textureY) {
        m_textureY->destroy();
      } else {
        m_textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
      }
      m_textureY->setSize(imageWidth, imageHeight);
      m_textureY->setFormat(QOpenGLTexture::R16_UNorm);
      m_textureY->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
      m_textureY->setMagnificationFilter(QOpenGLTexture::Linear);
      m_textureY->allocateStorage();
      if (m_textureU) {
        m_textureU->destroy();
      } else {
        m_textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
      }
      m_textureU->setSize(imageWidth / 2, imageHeight / 2);
      m_textureU->setFormat(QOpenGLTexture::R16_UNorm);
      m_textureU->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
      m_textureU->setMagnificationFilter(QOpenGLTexture::Linear);
      m_textureU->allocateStorage();
      if (m_textureV) {
        m_textureV->destroy();
      } else {
        m_textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
      }
      m_textureV->setSize(imageWidth / 2, imageHeight / 2);
      m_textureV->setFormat(QOpenGLTexture::R16_UNorm);
      m_textureV->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
      m_textureV->setMagnificationFilter(QOpenGLTexture::Linear);
      m_textureV->allocateStorage();
    }
    // 处理linesize，如果不处理，读取数据的时候会出错,导致渲染出来的画面很奇怪
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, strideY / 2); // Y 平面的行长度

    m_textureY->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_SHORT, Y);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, strideUV / 2); // UV 平面的行长度

    m_textureU->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED,
                    GL_UNSIGNED_SHORT, U);

    m_textureV->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED,
                    GL_UNSIGNED_SHORT, V);

    // 重置
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    doneCurrent();
  } break;
  }
  repaint();
}
void MyGLWidget::initializeGL() {
  // constexpr auto m_imageSource = ":/resources/aichan.jpg";
  //  imageWidth = QImage(m_imageSource).width();
  //  imageHeight = QImage(m_imageSource).height();

  const float m_vertices[] = {1.0f, 1,     0.0f, 1.0f,  1.0f, 1.0f, -1,
                              0.0f, 1.0f,  0.0f, -1.0f, -1,   0.0f, 0.0f,
                              0.0f, -1.0f, 1,    0.0f,  0.0f, 1.0f};

  const unsigned int m_indices[] = {0, 1, 2, 0, 2, 3};

  const auto m_vertexShaderSource = R"(
  #version 330 core 
  layout (location =0) in vec3 aPos;
  layout (location =1) in vec2 aTexCoord;
  out vec2 TexCoord;
  uniform mat4 transform;
  void main(){
  gl_Position =transform*vec4(aPos,1.0);
  TexCoord =aTexCoord;})";
  const auto m_fragmentShaderSource = R"(
  #version 330 core
  out vec4 FragColor;
  in vec2 TexCoord;
  uniform sampler2D textureY;
  uniform sampler2D textureU;
  uniform sampler2D textureV;
  uniform bool is10bit;
  void main(){
  float y = texture(textureY, TexCoord).r;
  float u =texture(textureU, TexCoord).r;
  float v =texture(textureV, TexCoord).r;
  if(is10bit){
  y=y*64;
  u=u*64;
  v=v*64;}
  u=u-0.5;
  v=v-0.5;
  float r=y+1.402*v;
  float g=y-0.34414*u-0.71414*v;
  float b=y+1.772*u;
  FragColor= vec4(r,g,b,1.0);})";

  initializeOpenGLFunctions();
  glClearColor(0.06275f, 0.06275f, 0.06275f, 1.0f);
  m_vao0 = new QOpenGLVertexArrayObject(this);
  m_vao0->create();
  m_vao0->bind();

  auto m_vbo0 = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  m_vbo0->create();
  m_vbo0->bind();
  m_vbo0->allocate(m_vertices, sizeof(m_vertices));

  /*const std::unique_ptr<QOpenGLBuffer> m_ebo0 =
      std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);*/
  auto m_ebo0 = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
  m_ebo0->create();
  m_ebo0->bind();
  m_ebo0->allocate(m_indices, sizeof(m_indices));

  const auto m_vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
  m_vertexShader->compileSourceCode(m_vertexShaderSource);
  const auto m_fragmentShader =
      new QOpenGLShader(QOpenGLShader::Fragment, this);
  m_fragmentShader->compileSourceCode(m_fragmentShaderSource);

  m_shaderProgram0 = new QOpenGLShaderProgram(this);
  m_shaderProgram0->addShader(m_vertexShader);
  m_shaderProgram0->addShader(m_fragmentShader);
  m_shaderProgram0->link();
  m_shaderProgram0->bind();
  m_shaderProgram0->setAttributeBuffer(0, GL_FLOAT, 0, 3, 5 * sizeof(float));
  m_shaderProgram0->enableAttributeArray(0);
  m_shaderProgram0->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2,
                                       5 * sizeof(float));
  m_shaderProgram0->enableAttributeArray(1);
  // 造成内存泄露的罪魁祸首1，此时并没有分配GPU内存
  // m_textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);

  // m_textureU = new QOpenGLTexture(m_imageSource, QOpenGLTexture::Target2D);

  // m_textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);

  m_vao0->release();
  m_shaderProgram0->release();
}
void MyGLWidget::resizeGL(const int w, const int h) { glViewport(0, 0, w, h); }
void MyGLWidget::paintGL() {
  paintCount = paintCount + 1;
  // 造成内存泄露的罪魁祸首2
  if (!m_textureY || !m_textureY->isCreated() || !m_textureU ||
      !m_textureU->isCreated() || !m_textureV || !m_textureV->isCreated()) {
    return;
  }
  glClear(GL_COLOR_BUFFER_BIT);
  m_shaderProgram0->bind();
  m_vao0->bind();
  // m_texture0->bind(0);
  // m_shaderProgram0->setUniformValue("texture0", 0);
  m_textureY->bind(0);
  m_shaderProgram0->setUniformValue("textureY", 0);
  m_textureU->bind(1);
  m_shaderProgram0->setUniformValue("textureU", 1);
  m_textureV->bind(2);
  m_shaderProgram0->setUniformValue("textureV", 2);
  m_shaderProgram0->setUniformValue("is10bit", isTenbit);
  QMatrix4x4 transform =
      transformMatrix2(width(), height(), imageWidth, imageHeight);
  m_shaderProgram0->setUniformValue("transform", transform);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  // m_texture0->release();
  m_textureY->release();
  m_textureU->release();
  m_textureV->release();
  m_vao0->release();
  m_shaderProgram0->release();
}

// 图像原本被完全填充在窗口中，通过mirror进行翻转，通过scale缩放回原来的比例，再通过正交投影消除窗口尺寸的影响(其实也相当于scale，因为vertex
// shader已经把图像压缩到[-1,1]^3空间内了)；
// 此时画面可以尽量完全填充在窗口中，不会出现黑边,但是部分图片可能会被裁剪
QMatrix4x4 MyGLWidget::transformMatrix(const float ww, const float wh,
                                       const float iw, const float ih) {
  if (ih <= 0.0f || wh <= 0.0f) {
    return QMatrix4x4(); // 异常情况返回单位矩阵
  }
  float imageRatio = iw / ih;
  // float imageVerseRatio = 1.0f / imageRatio;
  float windowRatio = ww / wh;
  float windowVerseRatio = 1.0f / windowRatio;
  const float pi = std::acos(-1);
  const float a = std::cos(pi);
  const float b = std::sin(pi);
  // const float windowAspectRatio =
  // static_cast<float>(width()) / static_cast<float>(height());
  // const float windowVerseRatio = 1.0f / windowAspectRatio;
  QMatrix4x4 rotation = {a, -b, 0, 0, b, a, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  QMatrix4x4 mirror = {1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  QMatrix4x4 imgScale = imageScaleMatrix(imageWidth, imageHeight);
  QMatrix4x4 winScale = windowScaleMatrix(width(), height());
  QMatrix4x4 transformMatrix;
  QMatrix4x4 betweenMatrix = winScale * imgScale * mirror;
  QMatrix4x4 adjustMatrix; // adjustMatrix要尽量留在有等号的地方，以包容各种情况
  if (imageRatio > 1.0f) {
    if (windowRatio > 1.0f) {
      transformMatrix = betweenMatrix;

    } else {
      adjustMatrix = {windowRatio, 0, 0, 0, 0, windowRatio, 0, 0,
                      0,           0, 1, 0, 0, 0,           0, 1};

      transformMatrix = adjustMatrix * betweenMatrix;
    }
  } else {
    if (windowRatio >= 1.0f) {
      adjustMatrix = {windowVerseRatio,
                      0,
                      0,
                      0,
                      0,
                      windowVerseRatio,
                      0,
                      0,
                      0,
                      0,
                      1,
                      0,
                      0,
                      0,
                      0,
                      1};

      transformMatrix = adjustMatrix * betweenMatrix;
    } else {
      transformMatrix = betweenMatrix;
    }
  }
  return transformMatrix;
}

QMatrix4x4 MyGLWidget::imageScaleMatrix(const float imgWidth,
                                        const float imgHeight) {
  QMatrix4x4 scaleMatrix;
  const float imgAspectRatio = imgWidth / imgHeight;
  const float imgVerseRatio = 1.0f / imgAspectRatio;
  if (imgAspectRatio >= 1.0f) {
    scaleMatrix = {1, 0, 0, 0, 0, imgVerseRatio, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  } else {
    scaleMatrix = {imgAspectRatio, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  };
  // std::cout << imgWidth << " " << imgHeight << std::endl;
  return scaleMatrix;
}

QMatrix4x4 MyGLWidget::windowScaleMatrix(const float winWidth,
                                         const float winHeight) {
  QMatrix4x4 orthoMatrix;
  const float winAspectRatio = winWidth / winHeight;
  const float winVerseRatio = 1.0f / winAspectRatio;
  // 实际上这一步写错了，所以有后来的adjuset，iw>ih,ww<wh时，其实iw不用变，ih乘上aspectRatio即可，iw<ih,ww>wh时，ih也不用变，iw乘上verseRatio即可，但是我写完adjust才发现，懒得改了
  if (winWidth <= winHeight) {
    orthoMatrix = {winVerseRatio, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0, 0.0,
                   0.0,           0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
  } else {
    orthoMatrix = {1.0, 0.0, 0.0, 0.0, 0.0,  winAspectRatio,
                   0.0, 0.0, 0.0, 0.0, -1.0, 0.0,
                   0.0, 0.0, 0.0, 1.0};
  }
  return orthoMatrix;
}

//
QMatrix4x4 MyGLWidget::transformMatrix2(const float ww, const float wh,
                                        const float iw, const float ih) {
  QMatrix4x4 transformMatrix;

  if (ww <= 0 || wh <= 0 || iw <= 0 || ih <= 0) {
    return QMatrix4x4();
  }
  auto imageRatio = iw / ih;
  auto windowRatio = ww / wh;
  auto scaleX = 0.0f;
  auto scaleY = 0.0f;

  if (imageRatio > windowRatio) {
    scaleX = 1.0f;
    scaleY = windowRatio / imageRatio;
  } else {
    scaleX = imageRatio / windowRatio;
    scaleY = 1.0f;
  }

  transformMatrix.scale(scaleX, -scaleY, 1.0f);

  return transformMatrix;
}