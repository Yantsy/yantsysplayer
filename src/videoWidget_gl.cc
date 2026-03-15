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
    emit close();
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
    for (const auto& e : { "至", "是", "工", "程", "已", "毕", "，", "于", "斯", "合", "题" }) {
        std::cout << e << std::flush;
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
    std::cout << "\n";
}

void MyGLWidget::renderWithOpenGL(uint8_t* Y, uint8_t* U, uint8_t* V, int& w, int& h, int& strideY,
    int& strideU, int& strideV, const char& pxFmt, int& newdepth) noexcept {
    float diff[2] = { w - imageWidth, h - imageHeight };
    imageWidth += diff[0];
    imageHeight += diff[1];
    GLint alignment { 1 };
    GLenum type { GL_UNSIGNED_BYTE };
    auto format = QOpenGLTexture::R8_UNorm;
    if (newdepth > 8) {
        oneByte = false;
        format  = QOpenGLTexture::R16_UNorm;
        alignment += 1;
        type   = GL_UNSIGNED_SHORT;
        adjust = adjustC(newdepth, 16);
    } else {
        oneByte = true;
        adjust  = 0;
    };
    makeCurrent();
    if (!oneByte || diff[0] != 0 || diff[1] != 0 || !m_textureY || !m_textureY->isCreated()) {
        updateTexture(m_textureY, imageWidth, imageHeight, format);
        updateTexture(m_textureU, imageWidth / 2, imageHeight / 2, format);
        updateTexture(m_textureV, imageWidth / 2, imageHeight / 2, format);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, strideY / alignment);
    m_textureY->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight, GL_RED, type, Y);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, strideU / alignment);
    m_textureU->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth / 2, imageHeight / 2, GL_RED, type, U);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, strideV / alignment);
    m_textureV->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth / 2, imageHeight / 2, GL_RED, type, V);

    doneCurrent();
    repaint();
};

bool MyGLWidget::needupdate() {
    bool update;
    return update;
};
float MyGLWidget::adjustC(int orig, int inbuffer) {
    float a = (1 << orig) - 1;
    float b = (1 << inbuffer) - 1;
    return b / a;
};

void MyGLWidget::updateTexture(
    QOpenGLTexture*& texture, float w, float h, QOpenGLTexture::TextureFormat format) {
    if (texture) {
        texture->destroy();
    } else {
        texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    }
    texture->setSize(w, h);
    texture->setFormat(format);
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->allocateStorage();
};

void MyGLWidget::sendTexture(QOpenGLTexture* texture) { texture->bind(); }

void MyGLWidget::getInfo(PlayerStatePtr is) {
    auto videoInfo = is->mediaInfo.videoInfo;
    pxFmt          = videoInfo.pxFmt;
    depth          = videoInfo.pxFmtDpth;
    timeBase       = videoInfo.vtimeBase;
};
void MyGLWidget::frameIn(std::shared_ptr<VideoFrame> videoFrame) {
    // compared to the audio playing process,this widget calls paint faster and has less delay,so I
    // have the slider chase the video progress instead of the audio's.
    latest     = (videoFrame->pts) * timeBase;
    auto frame = videoFrame->frame.get();
    emit progressChanged(latest);
    renderWithOpenGL(frame->data[0], frame->data[1], frame->data[2], videoFrame->width,
        videoFrame->height, videoFrame->linesize[0], videoFrame->linesize[1],
        videoFrame->linesize[2], pxFmt, depth);
    /*
if (depth == 8) {
    renderWithOpenGL8(frame->data[0], frame->data[1], frame->data[2], videoFrame->width,
        videoFrame->height, videoFrame->linesize[0], videoFrame->linesize[1],
        videoFrame->linesize[2], pxFmt);
} else if (depth == 10) {
    renderWithOpenGL10(frame->data[0], frame->data[1], frame->data[2], videoFrame->width,
        videoFrame->height, videoFrame->linesize[0], videoFrame->linesize[1],
        videoFrame->linesize[2], pxFmt);
}*/
};

void MyGLWidget::quit() {
    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.06275f, 0.06275f, 0.06275f, 1.0f);
    if (m_textureY) m_textureY->destroy();
    if (m_textureU) m_textureU->destroy();
    if (m_textureV) m_textureV->destroy();
    doneCurrent();
}
void MyGLWidget::initializeGL() {

    const float m_vertices[] = { 1.0f, 1, 0.0f, 1.0f, 1.0f, 1.0f, -1, 0.0f, 1.0f, 0.0f, -1.0f, -1,
        0.0f, 0.0f, 0.0f, -1.0f, 1, 0.0f, 0.0f, 1.0f };

    const unsigned int m_indices[] = { 0, 1, 2, 0, 2, 3 };

    constexpr auto m_vertexShaderSource   = R"(
  #version 330 core 
  layout (location =0) in vec3 aPos;
  layout (location =1) in vec2 aTexCoord;
  out vec2 TexCoord;
  uniform mat4 transform;
  void main(){
  gl_Position =transform*vec4(aPos,1.0);
  TexCoord =aTexCoord;})";
    constexpr auto m_fragmentShaderSource = R"(
  #version 330 core
  out vec4 FragColor;
  in vec2 TexCoord;
  uniform sampler2D textureY;
  uniform sampler2D textureU;
  uniform sampler2D textureV;
  uniform bool oneByte;
  uniform int adjust;
  void main(){
  float y = texture(textureY, TexCoord).r;
  float u =texture(textureU, TexCoord).r;
  float v =texture(textureV, TexCoord).r;
  if(!oneByte){
  y=y*adjust;
  u=u*adjust;
  v=v*adjust;}
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
    m_shaderProgram0->setUniformValue("oneByte", oneByte);
    m_shaderProgram0->setUniformValue("adjust", adjust);
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