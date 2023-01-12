CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32:LIBS += \
    $$PWD/../Deps/libjpeg-turbo-2.0.6/lib/jpeg-static.lib

win32:INCLUDEPATH += \
    $$PWD/../Deps/libjpeg-turbo-2.0.6/include

SOURCES += \
        Compressor.cpp \
        Parser.cpp \
        Writer.cpp \
        jpeg_decoder.cpp \
        jpeg_encoder.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Algorithms.h \
    Compressor.h \
    Hilbert.h \
    Morton.h \
    Parser.h \
    Phase.h \
    Split.h \
    Triangle.h \
    Vec3.h \
    Writer.h \
    jpeg_decoder.h \
    jpeg_encoder.h

