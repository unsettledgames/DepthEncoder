#ifndef MORTON_H
#define MORTON_H

#include <Vec3.h>
#include <Algorithm.h>

namespace DStream
{
    class Morton : public Algorithm
    {
    public:
        Morton(int q, int nbits);
        void Encode(uint16_t* values, uint8_t* dest, uint32_t count);
        void Decode(uint8_t* values, uint16_t* dest, uint32_t count);

        Color ValueToColor(uint16_t val);
        uint16_t ColorToValue(const Color& col);

    private:
        int m_NBits;
    };
}

#endif // MORTON_H
