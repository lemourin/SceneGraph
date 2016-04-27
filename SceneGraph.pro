QT = core gui quick
CONFIG += c++14
CONFIG -= debug_and_release
TARGET = SceneGraph
TEMPLATE = lib
OBJECTS_DIR = .obj
MOC_DIR = .moc
android: QMAKE_CXXFLAGS += -std=c++14

SOURCES += \
    BaseObject.cpp \
    Camera.cpp \
    DefaultRenderer.cpp \
    Geometry.cpp \
    Item.cpp \
    Material.cpp \
    Node.cpp \
    Renderer.cpp \
    Shader.cpp \
    Window.cpp \
    ShaderSource.cpp

HEADERS += \
    BaseObject.hpp \
    Camera.hpp \
    Geometry.hpp \
    Item.hpp \
    Material.hpp \
    Node.hpp \
    Shader.hpp \
    Window.hpp \
    ShaderSource.hpp \
    DefaultRenderer.hpp \
    Renderer.hpp

!android {
  unix {
    QT += x11extras
    LIBS += -lX11
  }
}
