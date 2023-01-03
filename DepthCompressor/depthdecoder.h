#ifndef DEPTHDECODER_H
#define DEPTHDECODER_H

#include <Defs.h>

#include <QString>
#include <QImage>

namespace DepthEncoder
{
    class Decoder
    {
    public:
        Decoder(QString filePath);
        void Decode(QString outPath, EncodingMode mode);

    private:
        QImage DecodeNone();
        QImage DecodeMorton();
        QImage DecodeTriangle();
        QImage DecodeHilbert();

    private:
        std::vector<uint8_t> m_Data;
        uint32_t m_Width;
        uint32_t m_Height;
    };
}

#endif // DEPTHDECODER_H
