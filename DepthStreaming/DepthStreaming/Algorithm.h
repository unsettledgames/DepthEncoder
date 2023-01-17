#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <cstdint>

namespace DStream
{
    class Algorithm
    {
    public:
        Algorithm(int quantization) : m_Quantization(quantization) {}

        inline void SetQuantization(int q) {m_Quantization = q;}
        inline int GetQuantization() {return m_Quantization;}
        inline uint16_t Quantize(uint16_t val)
        {
            uint8_t shift = 16 - m_Quantization;
            uint16_t lowPart = val % 256;
            uint16_t highPart = ((val >> 8) >> shift) << shift;

            return (highPart << 8) + lowPart;
        }

    protected:
        uint32_t m_Quantization;
    };
}

#endif // ALGORITHM_H
