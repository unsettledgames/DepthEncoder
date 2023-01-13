#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <Vec3.h>
#include <Algorithm.h>


namespace DStream
{
    class Triangle : public Algorithm
    {
    public:
        Triangle(int q);

        void Encode(uint16_t* values, uint8_t* dest, uint32_t count);
        void Decode(uint8_t* values, uint16_t* dest, uint32_t count);

        Color ValueToColor(uint16_t val);
        uint16_t ColorToValue(const Color& col);
    };
}

#endif // TRIANGLE_H
