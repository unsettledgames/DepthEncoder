#ifndef SPLITCODER_H
#define SPLITCODER_H

#include <Vec3.h>
#include <Algorithm.h>

namespace DStream
{
    class SplitCoder : public Algorithm
    {
    public:
        SplitCoder(int q);

        void Encode(uint16_t* values, uint8_t* dest, uint32_t count);
        void Decode(uint8_t* values, uint16_t* dest, uint32_t count);

        Color ValueToColor(uint16_t val);
        uint16_t ColorToValue(const Color& col);
    };
}

#endif // SPLITCODER_H
