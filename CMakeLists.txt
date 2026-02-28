cmake_minimum_required(VERSION 3.28)

project(test)


# Qt相关
set(CMAKE_AUTOUIC ON)
# Qt's Meta-Object Compiler
set(CMAKE_AUTOMOC ON)
# Qt资源系统
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 COMPONENTS Widgets Core  OpenGL OpenGLWidgets REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2MIXER REQUIRED SDL2_mixer)


set(FFMPEG_INCLUDE_DIR "/usr/local/ffmpeg/include")
set(FFMPEG_LIB_DIR "/usr/local/ffmpeg/lib")

include_directories(${FFMPEG_INCLUDE_DIR})
link_directories(${FFMPEG_LIB_DIR})

qt_standard_project_setup(REQUIRES 6.5)


set(QT_QML_GENERATE_QMLS_INIT ON)

qt_add_executable(${PROJECT_NAME}
../src/test.cc
../src/videoWidget_gl.h
../src/videoWidget_gl.cc
../src/audioWidget_sdl2.h
../src/resampler.h
../src/public.h
)
qt_add_resources({${PROJECT_NAME} ../resources.qrc})

target_sources(${PROJECT_NAME} PRIVATE ../resources.qrc)

    
target_link_libraries(${PROJECT_NAME}  PUBLIC Qt6::Core Qt6::Widgets  Qt6::OpenGL Qt6::OpenGLWidgets OpenGL::GL OpenGL::GLU)

target_link_libraries(${PROJECT_NAME} PUBLIC avformat avcodec avutil swscale swresample)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2MIXER_LIBRARIES}
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


