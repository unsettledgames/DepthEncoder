#include <Morton.h>
// Credits for Morton convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DStream
{
    Morton::Morton(int q, int nbits) : Algorithm(q), m_NBits(nbits){}

    void Morton::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(values[i]);
            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }
    }

    void Morton::Decode(uint8_t* values, uint16_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};
            dest[i] = ColorToValue(currColor);
        }
    }

    Color Morton::ValueToColor(uint16_t val)
    {
        Color ret;
        ret[0] = 0; ret[1] = 0; ret[2] = 0;

        for (unsigned int i = 0; i <= m_NBits; ++i) {
            uint8_t selector = 1;
            unsigned int shift_selector = 3 * i;
            unsigned int shiftback = 2 * i;

            ret[0] |= (val & (selector << shift_selector)) >> (shiftback);
            ret[1] |= (val & (selector << (shift_selector + 1))) >> (shiftback + 1);
            ret[2] |= (val & (selector << (shift_selector + 2))) >> (shiftback + 2);
        }
        return ret;
    }

    uint16_t Morton::ColorToValue(const Color& col)
    {
        int codex = 0, codey = 0, codez = 0;

        const int nbits2 = 2 * m_NBits;

        for (int i = 0, andbit = 1; i < nbits2; i += 2, andbit <<= 1) {
            codex |= (int)(col.x & andbit) << i;
            codey |= (int)(col.y & andbit) << i;
            codez |= (int)(col.z & andbit) << i;
        }

        return ((codez << 2) | (codey << 1) | codex);
    }
}

