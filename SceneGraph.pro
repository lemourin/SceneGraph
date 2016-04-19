QT = core gui quick
CONFIG += c++14
CONFIG -= debug_and_release
TARGET = SceneGraph
TEMPLATE = lib
OBJECTS_DIR = .obj
MOC_DIR = .moc

SOURCES += \
    BaseObject.cpp \
    Camera.cpp \
    Cube.cpp \
    DefaultRenderer.cpp \
    Geometry.cpp \
    Image.cpp \
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
    Cube.hpp \
    Geometry.hpp \
    Image.hpp \
    Item.hpp \
    Material.hpp \
    Node.hpp \
    Shader.hpp \
    Window.hpp \
    ShaderSource.hpp \
    DefaultRenderer.hpp \
    Renderer.hpp
