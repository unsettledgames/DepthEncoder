#ifndef MORTONCODER_H
#define MORTONCODER_H

#include <Vec3.h>
#include <Algorithm.h>

namespace DStream
{
    class MortonCoder : public Algorithm
    {
    public:
        MortonCoder(uint32_t q, uint32_t curveBits);
        void Encode(uint16_t* values, uint8_t* dest, uint32_t count);
        void Decode(uint8_t* values, uint16_t* dest, uint32_t count);

        Color ValueToColor(uint16_t val);
        uint16_t ColorToValue(const Color& col);

    private:
        uint32_t m_CurveBits;
    };
}

#endif // MORTONCODER_H
