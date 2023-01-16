#ifndef PACKEDCODER_H
#define PACKEDCODER_H

#include <Algorithm.h>
#include <Vec3.h>

namespace DStream
{
    class PackedCoder : public Algorithm
    {
    public:
        PackedCoder(int q);

        void Encode(uint16_t* values, uint8_t* dest, uint32_t count);
        void Decode(uint8_t* values, uint16_t* dest, uint32_t count);

        Color ValueToColor(uint16_t val);
        uint16_t ColorToValue(const Color& col);
    };
}


#endif // PACKEDCODER_H
