#ifndef HILBERT_H
#define HILBERT_H

#include <Algorithm.h>
#include <Vec3.h>

// Credits for Hilbert convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DStream
{
    class Hilbert : public Algorithm
    {
    public:
        Hilbert(int q, int nbits, bool optimizeSpacing = false);
        void Encode(uint16_t* values, uint8_t* dest, uint32_t count);
        void Decode(uint8_t* values, uint16_t* dest, uint32_t count);

        Color ValueToColor(uint16_t val);
        uint16_t ColorToValue(const Color& col);

    private:
        // Hilbert transpose
        void TransposeFromHilbertCoords(Color& col, int dim);
        void TransposeToHilbertCoords(Color& col, int dim);

        Color Enlarge(Color col);
        Color Shrink(Color col);

    private:
        int m_NBits;
        bool m_OptimizeSpacing;
    };
}

#endif // HILBERT_H
