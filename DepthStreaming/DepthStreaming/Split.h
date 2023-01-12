#ifndef SPLIT_H
#define SPLIT_H

#include <cmath>
#include <Vec3.h>

namespace DStream
{
    static Color SplitToColor(uint16_t val)
    {
        Color ret;

        float Ld, Ha;

        float mod = fmod(val / 256.0f, 2.0f);
        if (mod <= 1)
            Ha = mod;
        else
            Ha =2 - mod;

        Ld = ret.x >> 8; Ha *= 255.0f;

        if (mod <= 1)
            Ha = std::ceil(Ha);
        else
            Ha = std::floor(Ha);

        ret[0] = Ld; ret[1] = Ha; ret[2] = 0.0f;

        return ret;
    }

    static uint16_t ColorToSplit(const Color& col)
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

#endif // SPLIT_H
