#include "PackedCoder.h"

namespace DStream
{
    PackedCoder::PackedCoder(int q) : Algorithm(q){}

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

        ret.x = val >> 8;
        ret.y = val & 255;

        return ret;
    }

    uint16_t PackedCoder::ColorToValue(const Color& col)
    {
        return col.y + col.x * 256;
    }
}
