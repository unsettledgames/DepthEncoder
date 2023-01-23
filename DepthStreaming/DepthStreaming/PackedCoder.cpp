#include "PackedCoder.h"

#include <cmath>

namespace DStream
{
    PackedCoder::PackedCoder(int q, uint32_t left) : Algorithm(q), m_LeftBits(left){}

    void PackedCoder::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(values[i]);
            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }
    }

    void PackedCoder::Decode(uint8_t* values, uint16_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};
            dest[i] = ColorToValue(currColor);
        }
    }

    Color PackedCoder::ValueToColor(uint16_t val)
    {
        Color ret;
        uint32_t right = m_Quantization - m_LeftBits;

        ret.x = (val >> (16 - m_LeftBits)) << (8 - m_LeftBits);
        ret.y = ((val >> (16 - m_Quantization)) & ((1 << right) - 1)) << (8 - right);

        return ret;
    }

    uint16_t PackedCoder::ColorToValue(const Color& col)
    {
        Color copy = col;
        uint16_t highPart, lowPart;
        uint32_t right = m_Quantization - m_LeftBits;

        copy.x >>= (8 - m_LeftBits);
        copy.y >>= (8 - right);

        highPart = copy.x << (m_Quantization - m_LeftBits);
        lowPart = copy.y;

        return (highPart + lowPart) << (16 - m_Quantization);
    }
}
