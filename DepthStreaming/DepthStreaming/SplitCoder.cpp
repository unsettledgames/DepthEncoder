#include <SplitCoder.h>
#include <cmath>
// Credits for Morton convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DStream
{
    SplitCoder::SplitCoder(int q) : Algorithm(q){}

    void SplitCoder::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(values[i]);
            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }
    }

    void SplitCoder::Decode(uint8_t* values, uint16_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};
            dest[i] = ColorToValue(currColor);
        }
    }

    Color SplitCoder::ValueToColor(uint16_t val)
    {
        Color ret;
        uint8_t highPart, lowPart;

        highPart = val >> 8;
        if (highPart % 2 == 0) lowPart = val % 256;
        else lowPart = 255 -  (val % 256);

        ret[0] = highPart; ret[1] = lowPart; ret[2] = 0.0f;
        return ret;
    }

    uint16_t SplitCoder::ColorToValue(const Color& col)
    {
        uint16_t highPart = col.x;
        uint16_t lowPart = col.y;
        uint16_t delta = 0;

        int m = highPart % 2;

        switch (m) {
        case 0:
            delta = lowPart;
            break;
        case 1:
            delta = 255 - lowPart;
            break;
        }

        return highPart * 256 + delta;
    }
}

