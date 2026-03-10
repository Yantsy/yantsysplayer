#include "videoWidget_gl.h"
#include <iostream>
#include <thread>

MyGLWidget::MyGLWidget(QWidget* parent) noexcept
    : QOpenGLWidget(parent) {
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
    emit close();
    for (const auto e : { "至", "是", "工", "程", "已", "毕", "，", "于", "斯", "合", "题" }) {
        std::cout << e << std::flush;
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
    std::cout << "\n";
}

void MyGLWidget::renderWithOpenGL(uint8_t* Y, uint8_t* U, uint8_t* V, int& w, int& h, int& strideY,
    int& strideU, int& strideV, const char& pxFmt, int depth) noexcept { };

void MyGLWidget::renderWithOpenGL8(uint8_t* Y, uint8_t* U, uint8_t* V, int& w, int& h, int& strideY,
    int& strideU, int& strideV, const char& pxFmt) noexcept {

    // isTenbit = false;
    renderCount       = renderCount + 1;
    imageWidth        = w;
    imageHeight       = h;
    bool needRecreate = !m_textureY || !m_textureY->isCreated() || m_textureY->width() != w
        || m_textureY->height() != h;
    switch (pxFmt) {
    case AV_PIX_FMT_YUV420P: {
        makeCurrent();
        // update textures
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
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, strideY);

        m_textureY->bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, Y);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, strideU);

        m_textureU->bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, U);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, strideV);

        m_textureV->bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, V);

        // 重置
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        doneCurrent();
    } break;
    case AV_PIX_FMT_YUV420P10LE: {

    } break;
    }

    repaint();
}
void MyGLWidget::renderWithOpenGL10(uint8_t* Y, uint8_t* U, uint8_t* V, int& w, int& h,
    int& strideY, int& strideU, int& strideV, const char& pxFmt) noexcept {
    isTenbit          = true;
    renderCount       = renderCount + 1;
    imageWidth        = w;
    imageHeight       = h;
    bool needRecreate = !m_textureY || !m_textureY->isCreated() || m_textureY->width() != w
        || m_textureY->height() != h;
    switch (pxFmt) {
    case AV_PIX_FMT_YUV420P10LE: {
        makeCurrent();
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
        // dealing with linesize,ignorance of this step leads to strange pictures;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, strideY / 2);

        m_textureY->bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_SHORT, Y);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, strideU / 2);

        m_textureU->bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_SHORT, U);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, strideV / 2);

        m_textureV->bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_SHORT, V);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        doneCurrent();
    } break;
    }
    repaint();
}
void MyGLWidget::getInfo(VideoInfo videoInfo) {
    pxFmt    = videoInfo.pxFmt;
    depth    = videoInfo.pxFmtDpth;
    timeBase = videoInfo.vtimeBase;
};
void MyGLWidget::frameIn(std::shared_ptr<VideoFrame> videoFrame) {
    latest = (videoFrame->pts) * timeBase;
    emit progressChanged(latest);
    if (depth == 8) {
        renderWithOpenGL8(videoFrame->y.data(), videoFrame->u.data(), videoFrame->v.data(),
            videoFrame->width, videoFrame->height, videoFrame->linesize[0], videoFrame->linesize[1],
            videoFrame->linesize[2], pxFmt);
    } else if (depth == 10) {
        renderWithOpenGL10(videoFrame->y.data(), videoFrame->u.data(), videoFrame->v.data(),
            videoFrame->width, videoFrame->height, videoFrame->linesize[0], videoFrame->linesize[1],
            videoFrame->linesize[2], pxFmt);
    }
};
void MyGLWidget::initializeGL() {

    const float m_vertices[] = { 1.0f, 1, 0.0f, 1.0f, 1.0f, 1.0f, -1, 0.0f, 1.0f, 0.0f, -1.0f, -1,
        0.0f, 0.0f, 0.0f, -1.0f, 1, 0.0f, 0.0f, 1.0f };

    const unsigned int m_indices[] = { 0, 1, 2, 0, 2, 3 };

    const auto m_vertexShaderSource   = R"(
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

    auto m_ebo0 = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    m_ebo0->create();
    m_ebo0->bind();
    m_ebo0->allocate(m_indices, sizeof(m_indices));

    const auto m_vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    m_vertexShader->compileSourceCode(m_vertexShaderSource);
    const auto m_fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    m_fragmentShader->compileSourceCode(m_fragmentShaderSource);

    m_shaderProgram0 = new QOpenGLShaderProgram(this);
    m_shaderProgram0->addShader(m_vertexShader);
    m_shaderProgram0->addShader(m_fragmentShader);
    m_shaderProgram0->link();
    m_shaderProgram0->bind();
    m_shaderProgram0->setAttributeBuffer(0, GL_FLOAT, 0, 3, 5 * sizeof(float));
    m_shaderProgram0->enableAttributeArray(0);
    m_shaderProgram0->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    m_shaderProgram0->enableAttributeArray(1);

    m_vao0->release();
    m_shaderProgram0->release();
}
void MyGLWidget::resizeGL(const int w, const int h) { glViewport(0, 0, w, h); }
void MyGLWidget::paintGL() {
    paintCount = paintCount + 1;
    if (!m_textureY || !m_textureY->isCreated() || !m_textureU || !m_textureU->isCreated()
        || !m_textureV || !m_textureV->isCreated()) {
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
    QMatrix4x4 transform = transformMatrix(width(), height(), imageWidth, imageHeight);
    m_shaderProgram0->setUniformValue("transform", transform);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // m_texture0->release();
    m_textureY->release();
    m_textureU->release();
    m_textureV->release();
    m_vao0->release();
    m_shaderProgram0->release();
}

QMatrix4x4 MyGLWidget::transformMatrix(
    const float ww, const float wh, const float iw, const float ih) {
    QMatrix4x4 transformMatrix;

    if (ww <= 0 || wh <= 0 || iw <= 0 || ih <= 0) {
        return QMatrix4x4();
    }
    auto imageRatio  = iw / ih;
    auto windowRatio = ww / wh;
    auto scaleX      = 0.0f;
    auto scaleY      = 0.0f;

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