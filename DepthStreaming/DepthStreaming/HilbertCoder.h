#ifndef HILBERTCODER_H
#define HILBERTCODER_H

#include <Algorithm.h>
#include <Vec3.h>

#include <vector>

// Credits for Hilbert convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DStream
{
    class HilbertCoder : public Algorithm
    {
    public:
        HilbertCoder(uint32_t q, uint32_t curveBits, bool optimizeSpacing = false);
        void Encode(uint16_t* values, uint8_t* dest, uint32_t count);
        void Decode(uint8_t* values, uint16_t* dest, uint32_t count);

        Color ValueToColor(uint16_t val);
        uint16_t ColorToValue(const Color& col);

    private:
        // Hilbert transpose
        void TransposeFromHilbertCoords(Color& col);
        void TransposeToHilbertCoords(Color& col);

        Color Enlarge(Color col);
        Color Shrink(Color col);

    private:
        uint32_t m_CurveBits;
        uint32_t m_SegmentBits;
        bool m_OptimizeSpacing;

        uint16_t*** m_Table;
        std::vector<uint16_t> m_XErrors;
        std::vector<uint16_t> m_YErrors;
        std::vector<uint16_t> m_ZErrors;

        std::vector<uint16_t> m_EnlargeTable;
        std::vector<uint16_t> m_ShrinkTable;
    };
}

#endif // HILBERTCODER_H
