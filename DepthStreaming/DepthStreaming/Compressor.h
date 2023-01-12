#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <Vec3.h>

namespace DStream
{
    // Enum per il tipo di encoding, funzioni definite in h
    enum EncodingType {TRIANGLE = 0, MORTON, HILBERT, PHASE, SPLIT};

    class Compressor
    {
    public:
        Compressor(uint32_t width, uint32_t height);

        bool Encode(uint16_t* values, uint8_t* dest, uint32_t count, EncodingType type);
        bool Decode(uint8_t* values, uint16_t* dest, uint32_t count, EncodingType type);


    protected:
        uint32_t m_Width;
        uint32_t m_Height;
    };
}

#endif // COMPRESSOR_H
