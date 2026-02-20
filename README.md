## 如何实现一个基于OpenGL+ffmpeg的本地视频播放器？

要回答这个问题，我们首先需要知道，什么是视频？视频可以简单的看作连续播放的图片，但是我们日常所观看的视频并不是只有画面。 

以经典动画猫和老鼠为例，制作一集猫和老鼠的视频，我们需要一张张图片来画出猫和老鼠的动作、故事等等，还需要打击音效，背景音乐等音频文件配合图片播放。正是这些连续播放的图片（视频流）、声音（音频流）等等，共同构成了我们通常所认知的视频文件（容器）。因此，严格来说，我们所写的并非是一个视频播放器，而是一个多媒体播放器。下文中提到的视频都指代视频流。

要播放该视频，第一步得先打开容器，然后从中取出我们所需要的流，这个过程就叫做\[解封装（demux）\]。

但是播放器并不一定读得懂被取出的流，原因在于，直接存储在容器中的流是被压缩过的，如果不能对它执行解压的操作，那么在播放器看来你直接传给它的流就是一堆乱码，而这个解压的过程就是\[解码\]（decode）。

你可能会好奇，为什么容器中的流都是被压缩过的呢？原因在于，完整的音频、图片、视频文件所需的存储空间空间是很庞大的。以一张图片上的一个像素为例，假如我用RGB色彩空间（用（R，G，B）值描述颜色）对它进行采样，每一个像素需要3字节的存储空间，一张1920*1080的图片就需要1920*1080*3字节（5.933MB）的存储空间，如果我们每秒钟播放25张这样的图片，那么一秒钟1920*1080的视频则需148.325MB的存储空间。这既不方便存储，也不方便传输。  

为此，工程师们想到了利用YUV色彩空间对颜色进行采样。YUV又被称作YCbCr,分别表示像素的亮度，以及蓝色值和红色值与亮度的差值。人眼难以直接看出RGB值的变化，但是对于亮度的变化却很敏感，基于此，我们可以在保持像素的亮度Y不变的情况下，减少对色度UV的采样，从而实现在减少存储空间的情况下，让图片至少看起来依然保留了大部分信息。人们通常用每四个像素中YUV的采样比例，如4：4：4,4：2：2等命名采样格式。其中，最常用的格式是YUV420，即YUV420P，在水平每两像素和垂直每两像素中，我们取4个像素的Y值，一个像素的U值，一个像素的V值。四个像素一共需要六个字节，平均每个像素1.5字节，相比基于RGB色彩空间进行采样节省了一半的存储空间。在Y420P格式中，我们先存储Y值，然后存储U值，最后存储V值。在安卓常用的NV12格式中，U值和V值则是交替存储的。此外，由于显卡是基于RGB色彩空间绘制图像的，尽管图像基于YUV色彩空间采样，我们仍需在绘制图像前将它转化为RGB色彩空间下的图像。

讲完了图片，便可以聊聊视频了。虽然我们可以将视频看作连续播放的图片，但是并不是它们中的每一张都重要，也不是每一张图的所有信息都重要，决定哪些图片的哪些部分重要，并将重要的信息存储起来，这就是视频\[编码\](encode)的工作，也是编码标准（如H.264）发挥作用的领域。H.264将视频划分成一个个GOP（group of pictures）,GOP中的帧被划分为I帧，B帧和P帧。I帧即关键帧，P帧对前面的帧的差异，B帧则对前后帧的差异进行编码。借助这几类帧，即可实现对视频的压缩。

在了解完关于流的基本知识后，我们便可以概括出播放一个视频所需的基本流程了，即：解封装->解码->绘制一帧图像->在GUI界面上呈现出来，其中解封装和解码可以借助ffmpeg实现，绘制则可以通过SDL，OpenGL，Vulkan等图形学API实现。至于GUI的实现，也有很多方案可选，SDL，GLFW，QT等都行。笔者选择的是QT，因为它可以自动管理进程和资源，且功能强大。

### ffmpeg相关


### OpenGL相关

#### 1.图形学渲染管线

#### 2.状态机

#### 3.顶点着色器与变换矩阵

三角形的顶点是顶点（一个三维坐标）着色器处理的最小单元，两个三角形拼在一起就是一个矩阵——这正是我们播放视频所需要的。你可以创建两个数组分别存储它们，也可以创建一个索引数组用来标记三角形的顶点，通过读取索引数组，获取对应的顶点，这样我们只需要创建一个数组从右上角的顶点开始按照顺时针方向存储矩形的四个顶点，两个三角形重合的点索引两次即可。

接着我们需要创建vbo(vertex buffer object)，将顶点传入顶点着色器中，通过设置顶点属性（从哪里开始读，读的是什么类型的数据，间隔多少个字节读一次等等），顶点着色器便知道如何读取这些顶点。编译着色器，经过顶点着色器的处理，所有顶点的（x,y）都会被转化为NDC（\[-1,1\]*\[-1,1\]）上的坐标。先将视频的顶点组成的矩形完全填满，那么我们便可以将NDC看作窗口，同时将视频的每一帧画面看作一个2*2的中心对称矩形，这将是我们调节窗口尺寸，以及对视频画面进行各种变换的起点。

#### 4.VAO与ShaderProgram

#### 4.片元着色器与纹理

### GUI界面

#### 1.initializeGL()

#### 2.resizeGL()

#### 3.paintGL()
此处值得注意的点是，那就是有必要区分QOpenGLTexture的两种创建方式，一是直接创建一个有target，但是没有绑定context，且也不包含数据的对象：

此时，你并没有上传texture到内存，也没有将它拷贝到GPU中，也没有绑定context，此时painGL无法执行，强硬执行会导致整个程序崩溃，所以才有必要检测texture是否被创建；此外，此时也无法对texture进行任何format,filter和存储空间的设置。

二是创建一个有target，同时也包含数据的对象。

### 线程管理

#### 1.链接解码循环与渲染管线

#### 2.时间管理

### 数据流的管理

#### 1.拷贝AVCodecParameters至AVCodecContext

#### 2.linesize

详情见 [Image Stride](https://learn.microsoft.com/en-us/windows/win32/medfound/image-stride)。

首先需要知道，窗口中的图像是由GPU一行行地读取图像文件然后绘制出来的，那么GPU每行应该读取多少个字节呢？以YUV420P、8位深的Y平面数据为例，你可能会想当然地认为，既然每个像素的Y值都要采样，那么自然每行应该读取的字节数就是每一行的像素数，也就是图像的width，然后你就看到了如下图像：

<img src="readmeimg/截图 2026-02-01 09-40-11.png " title="渲染错误的图像")



#### 3.位深，10bit与8bit有何不同，如何处理？什么是归一化？

首先我们需要知道，texture实际存储在本地或者其他什么地方，要将它上传到GPU的VRAM中，就必须经过RAM->CPU->PCI->VRAM的过程。

那么在这个过程中，我们需要让GPU知道我们将要上传的texture data是如何存储和读取的，而这就是[glPixelStorei](https://registry.khronos.org/OpenGL-Refpages/gl4/)的工作。

>void glPixelStorei(GLenum pname,GLint param);

其中pname的值影响着将data从GPu pack到CPU中，以及将data从CPU unpack到GPU中的方式，而param则是具体的参数。GL_PACK_ALIGNMENT决定着texture data是如何对齐的，默认是4,而对于8bit的图像，我们取1即可(1byte=8bits)。10bit的图像在内存中只能按照16bit的方式存储，也就是每个像素至少要占用2个字节，那么其alignment的值就得取2。

接着便是通过[glTexImage2D](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml)上传纹理到GPU中。

>void glTexImage2D(	GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void * data);

其中值得注意的参数是type，它规定了pixel data的数据类型，及每个分量值占据的存储空间。由于我们的YUV分量是分别上传的，所以我们只需单独考虑每个分量，以及每个像素的每个分量值占据的字节数，对于8bit的图像，就是1字节，那么取GL_UNSIGENED_BYTE即可，而对于10bit的图像，则需取GL_UNSIGNED_SHORT。


#### 4.如何读取mkv文件的duration?


#### 2.YUV数据的存储

### 内存管理

#### 1.未及时更新texture

若你看到如下报错：

>QOpenGLTexture::setFormat(): Cannot change format once storage has been allocatedCannot resize a texture that already has storage allocated.To do so destroy() the texture and then create() and setSize()
>
>Cannot set mip levels on a texture that already has storage allocated.To do so, destroy() the texture and then create() and setMipLevels()

导致该报错的原因是，你将从AVPacket中读出的AVFrame data 存入纹理中，但是却忘记了及时销毁上一帧纹理，并为当前帧创建纹理，当前帧无法被存储，也无法进行设置filter等操作。

为了看懂该报错，我们需要了解纹理是如何被读取，创建和存储的。

### SDL音频处理

#### audioQueue与callback函数

SDL处理音频的机制大概相当于，你给它音频参数，如每次callback的样本数，采样频率，采样格式，callback函数和pcm数据，那么它就会驱动声卡。









