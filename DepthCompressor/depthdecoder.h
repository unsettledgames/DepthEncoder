#ifndef DEPTHDECODER_H
#define DEPTHDECODER_H

#include <types.h>

#include <QString>
#include <QImage>

namespace DepthEncoder
{
    class Decoder
    {
    public:
        Decoder(QString filePath);
        Decoder(const uint8_t* data, uint32_t width, uint32_t height);

        void Decode(QString outPath, EncodingMode mode);
        void Decode(std::vector<uint16_t>& data, EncodingMode mode);

    private:
        std::vector<uint8_t> m_Data;
        uint32_t m_Width;
        uint32_t m_Height;
    };
}

#endif // DEPTHDECODER_H
