#ifndef PHASECODER_H
#define PHASECODER_H

#include <Vec3.h>
#include <Algorithm.h>

#define M_PI 3.14159265359

namespace DStream
{
    class PhaseCoder : public Algorithm
    {
    public:
        PhaseCoder(int q);

        void Encode(uint16_t* values, uint8_t* dest, uint32_t count);
        void Decode(uint8_t* values, uint16_t* dest, uint32_t count);

        Color ValueToColor(uint16_t val);
        uint16_t ColorToValue(const Color& col);
    };
}

#endif // PHASECODER_H
