TEMPLATE = lib
CONFIG += plugin \
    release
TARGET = glesdriver
LIBS += -I/usr/include \
    -I/usr/include/GLES \
    -L/usr/lib \
    -lGLES_CM \
    -lgf
QT += core
HEADERS += glesscreen.h
SOURCES += glesscreen.cpp \
    glesplugin.cpp

# HEADERS += glesscreen.h
# SOURCES += glesscreen.cpp \
# glesplugin.cpp
FORMS += 
RESOURCES += 
