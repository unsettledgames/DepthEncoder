#ifndef PHASE_H
#define PHASE_H

#include <Vec3.h>
#include <cmath>

#define M_PI 3.14159265359

namespace DStream
{
    static Color PhaseToColor(uint16_t val)
    {
        Color ret;
        const float P = 16384.0f;
        const float w = 65535.0f;

        ret[0] = 255 * (0.5f + 0.5f * std::cos(M_PI * 2.0f * (val / P)));
        ret[1] = 255 * (val / w);
        ret[2] = 0;

        return ret;
    }

    static uint16_t ColorToPhase(const Color& col)
    {
        const float w = 65535.0f;
        const float P = 16384.0f;
        const float beta = P / 2.0f;
        float gamma, phi, PHI, K, Z;
        float i1 = col.x / 255.0f, i2 = col.y / 255.0f;

        phi = std::fabs(std::acos(2.0f * i1 - 1.0f));
        gamma = std::floor((i2 * w) / beta);

        if (((int)gamma) % 2)
            phi *= -1;

        K = std::round((i2 * w) / P);
        PHI = phi + 2 * M_PI * K;

        Z = PHI * (P / (M_PI * 2.0f));
        return Z;
    }
}

#endif // PHASE_H
