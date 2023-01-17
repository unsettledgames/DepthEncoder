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
            Color encoded = ValueToColor(Quantize(values[i]));
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

        float Ld, Ha;

        float mod = fmod(val / 256.0f, 2.0f);
        if (mod <= 1)
            Ha = mod;
        else
            Ha = 2 - mod;

        Ld = val >> 8; Ha *= 255.0f;

        if (mod <= 1)
            Ha = std::ceil(Ha);
        else
            Ha = std::floor(Ha);

        ret[0] = Ld; ret[1] = Ha; ret[2] = 0.0f;

        return ret;
    }

    uint16_t SplitCoder::ColorToValue(const Color& col)
    {
        float Ld = col.x;
        float Ha = col.y;
        int m = (int)col.x % 2;
        float delta = 0;

        switch (m) {
        case 0:
            delta = Ha;
            break;
        case 1:
            delta = 255.0f - Ha;
            break;
        }

        return Ld * 256.0f + delta;
    }
}

