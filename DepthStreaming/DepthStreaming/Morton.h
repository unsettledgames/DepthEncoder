#ifndef MORTON_H
#define MORTON_H

#include <Vec3.h>

#include <cmath>
#include <vector>

// Credits for Morton convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DStream
{
    template <typename intcode_t> inline constexpr int CurveOrder() { return std::ceil((sizeof(intcode_t) * 8) / 3.0f); }

    static Color MortonToColor(uint16_t val, int nbits)
    {
        Color ret;
        ret[0] = 0; ret[1] = 0; ret[2] = 0;

        for (unsigned int i = 0; i <= nbits; ++i) {
            uint8_t selector = 1;
            unsigned int shift_selector = 3 * i;
            unsigned int shiftback = 2 * i;

            ret[0] |= (val & (selector << shift_selector)) >> (shiftback);
            ret[1] |= (val & (selector << (shift_selector + 1))) >> (shiftback + 1);
            ret[2] |= (val & (selector << (shift_selector + 2))) >> (shiftback + 2);
        }
        return ret;
    }

    static uint16_t ColorToMorton(const Color& col, int nbits)
    {
        int codex = 0, codey = 0, codez = 0;

        const int nbits2 = 2 * nbits;

        for (int i = 0, andbit = 1; i < nbits2; i += 2, andbit <<= 1) {
            codex |= (int)(col.x & andbit) << i;
            codey |= (int)(col.y & andbit) << i;
            codez |= (int)(col.z & andbit) << i;
        }

        return ((codez << 2) | (codey << 1) | codex);
    }
}

#endif // MORTON_H
