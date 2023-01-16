CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32:LIBS += \
    $$PWD/../Deps/libjpeg-turbo-2.0.6/bin/jpeg62.dll
win32:INCLUDEPATH += \
    $$PWD/../Deps/libjpeg-turbo-2.0.6/include

SOURCES += \
        HilbertCoder.cpp \
        MortonCoder.cpp \
        PackedCoder.cpp \
        Parser.cpp \
        PhaseCoder.cpp \
        SplitCoder.cpp \
        TriangleCoder.cpp \
        Writer.cpp \
        jpeg_decoder.cpp \
        jpeg_encoder.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Algorithm.h \
    HilbertCoder.h \
    MortonCoder.h \
    PackedCoder.h \
    Parser.h \
    PhaseCoder.h \
    SplitCoder.h \
    TriangleCoder.h \
    Vec3.h \
    Writer.h \
    jpeg_decoder.h \
    jpeg_encoder.h

