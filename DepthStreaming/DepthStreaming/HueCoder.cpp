#include <HueCoder.h>
#include <cmath>
#include <algorithm>

namespace DStream
{
    HueCoder::HueCoder(int q) : Algorithm(q) {}

    void HueCoder::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(values[i]);
            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }
    }

    void HueCoder::Decode(uint8_t* values, uint16_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};
            dest[i] = ColorToValue(currColor);
        }
    }

    Color HueCoder::ValueToColor(uint16_t val)
    {
        Color ret;
        uint16_t d = std::floor(((float)val / ((1 << 16)-1)) * 1529.0f);

        if ((d <= 255) || (1275 < d && d <= 1529))
            ret.x = 255;
        else if (255 < d && d <= 510)
            ret.x = 255 - d;
        else if (510 < d && d <= 1020)
            ret.x = 0;
        else if (1020 < d && d <= 1275)
            ret.x = d - 1020;

        if (d <= 255)
            ret.y = d;
        else if (255 < d && d <= 765)
            ret.y = 255;
        else if (765 < d && d <= 1020)
            ret.y = 255 - (d - 765);
        else
            ret.y = 0;

        if (d < 510)
            ret.z = 0;
        else if (510 < d && d <= 765)
            ret.z = d;
        else if (765 < d && d <= 1275)
            ret.z = 255;
        else if (1275 < d && d <= 1529)
            ret.z = 1275 - d;

        return ret;
    }

    uint16_t HueCoder::ColorToValue(const Color& col)
    {
        uint8_t r = col.x, g = col.y, b = col.z;
        uint16_t ret = 0;

        if (b + g + r < 255)
            ret = 0;
        else if (r >= g && r >= b)
        {
            if (g >= b)
                ret = g - b;
            else
                ret = (g - b) + 1529;
        }
        else if (g >= r && g >= b)
            ret = b - r + 510;
        else if (b >= g && b >= r)
            ret = r - g + 1020;

        float q = std::round(((float)ret / 1529.0f) * (1 << m_Quantization));
        q = q * (1 << (16 - m_Quantization));

        return q;
    }
}
