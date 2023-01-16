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

    protected:
        uint32_t m_Quantization;
    };
}

#endif // ALGORITHM_H
