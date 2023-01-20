#include <MortonCoder.h>
// Credits for Morton convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DStream
{
    MortonCoder::MortonCoder(uint32_t q, uint32_t curveBits) : Algorithm(q), m_CurveBits(curveBits) {}

    void MortonCoder::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(values[i]);
            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }
    }

    void MortonCoder::Decode(uint8_t* values, uint16_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};
            dest[i] = ColorToValue(currColor);
        }
    }

    Color MortonCoder::ValueToColor(uint16_t val)
    {
        Color ret;
        ret[0] = 0; ret[1] = 0; ret[2] = 0;

        for (unsigned int i = 0; i <= m_CurveBits; ++i) {
            uint8_t selector = 1;
            unsigned int shift_selector = 3 * i;
            unsigned int shiftback = 2 * i;

            ret[0] |= (val & (selector << shift_selector)) >> (shiftback);
            ret[1] |= (val & (selector << (shift_selector + 1))) >> (shiftback + 1);
            ret[2] |= (val & (selector << (shift_selector + 2))) >> (shiftback + 2);
        }
        return ret;
    }

    uint16_t MortonCoder::ColorToValue(const Color& col)
    {
        int codex = 0, codey = 0, codez = 0;

        const int nbits2 = 2 * m_CurveBits;

        for (int i = 0, andbit = 1; i < nbits2; i += 2, andbit <<= 1) {
            codex |= (int)(col.x & andbit) << i;
            codey |= (int)(col.y & andbit) << i;
            codez |= (int)(col.z & andbit) << i;
        }

        return ((codez << 2) | (codey << 1) | codex);
    }
}

