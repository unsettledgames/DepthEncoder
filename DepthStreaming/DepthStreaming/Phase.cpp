#include <Phase.h>
#include <cmath>
// Credits for Morton convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DStream
{
    Phase::Phase(int q) : Algorithm(q){}

    void Phase::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(values[i]);
            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }
    }

    void Phase::Decode(uint8_t* values, uint16_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};
            dest[i] = ColorToValue(currColor);
        }
    }

    Color Phase::ValueToColor(uint16_t val)
    {
        Color ret;
        const float P = 16384.0f;
        const float w = 65535.0f;

        ret[0] = 255 * (0.5f + 0.5f * std::cos(M_PI * 2.0f * (val / P)));
        ret[1] = 255 * (val / w);
        ret[2] = 0;

        return ret;
    }

    uint16_t Phase::ColorToValue(const Color& col)
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

