CONFIG += c++11 console
CONFIG -= app_bundle

INCLUDEPATH += ../Deps/libjpeg-turbo-2.0.6/include
LIBS += ../Deps/libjpeg-turbo-2.0.6/bin/jpeg62.dll

#unix:LIBS += -ljpeg -liomp5
#
#mac:INCLUDEPATH += /usr/local/Cellar/jpeg-turbo/2.0.6/include
#mac:LIBS += -L/usr/local/Cellar/jpeg-turbo/2.0.6/lib/ -ljpeg

SOURCES += \
        depthdecoder.cpp \
        depthencoder.cpp \
        main.cpp \
        jpeg_decoder.cpp \
        jpeg_encoder.cpp

HEADERS += \
        conversions.h \
        depthdecoder.h \
        depthencoder.h \
        jpeg_decoder.h \
        jpeg_encoder.h \
        types.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
