#include <HilbertCoder.h>
#include <MortonCoder.h>

#include <cmath>
#include <vector>
#include <assert.h>
#include <iostream>

using namespace std;

namespace DStream
{
    template <typename T>
    static inline int sgn(T val)
    {
        return (T(0) < val) - (val < T(0));
    }

    HilbertCoder::HilbertCoder(uint32_t q, uint32_t curveBits, bool optimizeSpacing/* = false*/) : Algorithm(q)
    {
        assert(curveBits * 3 < q);

        m_CurveBits = curveBits;
        m_SegmentBits = q - 3 * m_CurveBits;

        assert(m_CurveBits + m_SegmentBits <= 8);
        m_OptimizeSpacing = optimizeSpacing;
    }

    void HilbertCoder::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(values[i]);
            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }
    }

    void HilbertCoder::Decode(uint8_t* values, uint16_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};
            dest[i] = ColorToValue(currColor);
        }
    }

    void HilbertCoder::TransposeFromHilbertCoords(Color& col)
    {
        int X[3] = {col.x, col.y, col.z};
        uint32_t N = 2 << (m_CurveBits - 1), P, Q, t;

        // Gray decode by H ^ (H/2)
        t = X[3 - 1] >> 1;
        // Corrected error in Skilling's paper on the following line. The appendix had i >= 0 leading to negative array index.
        for (int i = 3 - 1; i > 0; i--) X[i] ^= X[i - 1];
        X[0] ^= t;

        // Undo excess work
        for (Q = 2; Q != N; Q <<= 1) {
            P = Q - 1;
            for (int i = 3 - 1; i >= 0; i--)
                if (X[i] & Q) // Invert
                    X[0] ^= P;
                else { // Exchange
                    t = (X[0] ^ X[i]) & P;
                    X[0] ^= t;
                    X[i] ^= t;
                }
        }

        col.x = X[0]; col.y = X[1]; col.z = X[2];
    }

    void HilbertCoder::TransposeToHilbertCoords(Color& col)
    {
        int X[3] = {col.x, col.y, col.z};
        uint32_t M = 1 << (m_CurveBits - 1), P, Q, t;

        // Inverse undo

        for (Q = M; Q > 1; Q >>= 1) {
            P = Q - 1;
            for (int i = 0; i < 3; i++)
                if (X[i] & Q) // Invert
                    X[0] ^= P;
                else { // Exchange
                    t = (X[0] ^ X[i]) & P;
                    X[0] ^= t;
                    X[i] ^= t;
                }
        }

        // Gray encode
        for (int i = 1; i < 3; i++) X[i] ^= X[i - 1];
        t = 0;
        for (Q = M; Q > 1; Q >>= 1)
            if (X[3 - 1] & Q) t ^= Q - 1;
        for (int i = 0; i < 3; i++) X[i] ^= t;

        col.x = X[0]; col.y = X[1]; col.z = X[2];
    }

    Color HilbertCoder::Enlarge(Color col)
    {
        Color ret = col;
        static std::vector<uint8_t> occupancy;
        static std::vector<uint8_t> remap;
        if(occupancy.size() == 0) {
            occupancy.push_back(1);
            int gap = 1;
            while(gap < 64) {
                int end = occupancy.size();
                for(int i = 0; i < gap; i++)
                    occupancy.push_back(0);
                for(int i = 0; i < end; i++)
                    occupancy.push_back(occupancy[i]);
                gap *= 2;
            }

            for(size_t i = 0; i < occupancy.size(); i++) {
                if(occupancy[i])
                    remap.push_back(i);
            }
        }
        for(int k = 0; k < 3; k++)
            ret[k] = remap[ret[k]];
        return ret;
    }

    Color HilbertCoder::Shrink(Color col)
    {
        static std::vector<uint8_t> occupancy;
        static std::vector<uint8_t> remap;
        Color ret = col;

        if(occupancy.size() == 0) {
            occupancy.push_back(0);
            int gap = 1;
            while(gap < 64) {
                int end = occupancy.size();
                int last = occupancy.back();
                for(int i = 0; i < gap; i++) {
                    if(i <= gap/2)
                        occupancy.push_back(last);
                    else
                        occupancy.push_back(last+1);
                }
                for(int i = 0; i < end; i++)
                    occupancy.push_back(occupancy[i] + last+1);
                gap *= 2;
            }
        }
        for(int k = 0; k < 3; k++)
            ret[k] = occupancy[ret[k]];

        return ret;
    }

    Color HilbertCoder::ValueToColor(uint16_t val)
    {
        val >>= 16 - m_Quantization;
        int frac = val & ((1 << m_SegmentBits) - 1);
        val >>= m_SegmentBits;

        MortonCoder m(m_Quantization, m_CurveBits);
        Color v = m.ValueToColor(val);
        std::swap(v[0], v[2]);
        TransposeFromHilbertCoords(v);

        // Handle curve overflow
        if ((val+1) >= (1 << (3 * m_CurveBits)))
        {
            // Compute previous to see the direction of the curve
            Color v2 = m.ValueToColor(val + 1);
            std::swap(v2[0], v2[2]);
            TransposeFromHilbertCoords(v2);

            for (uint32_t i=0; i<3; i++)
            {
                // Add frac in that direction
                if (v2[i] != v[i])
                {
                    v[i] = (v[i] << m_SegmentBits) + sgn(max(v2[i], v[i]) - min(v2[i], v[i])) * frac/2;
                    for (uint32_t j=0; j<3; j++)
                    {
                        if (i != j)
                            v[j] <<= m_SegmentBits;
                        if (m_CurveBits != 5)
                            v[j] <<= (8 - m_CurveBits - m_SegmentBits);
                    }

                    if (m_CurveBits == 5)
                        return Enlarge(v);
                    return v;
                }
            }
        }

        Color v2 = m.ValueToColor(val + 1);
        std::swap(v2[0], v2[2]);
        TransposeFromHilbertCoords(v2);

        // Divide in segments
        for (uint32_t i=0; i<3; i++)
        {
            int mult = sgn(v2[i] - v[i]) * frac;
            v[i] = ((v[i] << m_SegmentBits) + mult);

            if (m_CurveBits != 5)
                v[i] <<= (8 - m_CurveBits - m_SegmentBits);
        }

        if (m_CurveBits == 5)
            return Enlarge(v);
        return v;
    }

    uint16_t HilbertCoder::ColorToValue(const Color& col)
    {
        int side = 1<<m_SegmentBits;
        MortonCoder m(m_Quantization, m_CurveBits);
        Color col1 = m_CurveBits == 5 ? Shrink(col) : col;
        Color currColor;

        if (m_CurveBits != 5)
            for (uint32_t i=0; i<3; i++)
                col1[i] >>= (8 - m_CurveBits - m_SegmentBits);

        int fract[3];
        for (uint32_t i=0; i<3; i++) {
            fract[i] = col1[i] & ((1 << m_SegmentBits)-1);
            if (fract[i] >= side/2) {
                fract[i] -= side;
                col1[i] += side/2; //round to the closest one
            }
        }

        for (uint32_t i=0; i<3; i++)
            col1[i] >>= m_SegmentBits;

        currColor = col1;
        TransposeToHilbertCoords(col1);
        std::swap(col1[0], col1[2]);
        uint16_t v1 = m.ColorToValue(col1);

        uint16_t v2 = std::min(v1 + 1, 1<<m_Quantization);
        Color nextCol = m.ValueToColor(v2);
        std::swap(nextCol[0], nextCol[2]);
        TransposeFromHilbertCoords(nextCol);

        uint16_t v3 = std::max(v1 - 1, 0);

        Color prevCol = m.ValueToColor(v3);
        std::swap(prevCol[0], prevCol[2]);
        TransposeFromHilbertCoords(prevCol);

        v1 <<=  m_SegmentBits;
        for(int i = 0; i < 3; i++) {
            v1 += fract[i]*sgn(nextCol[i] - prevCol[i]);
        }

        v1 <<= 16 - m_Quantization;
        return v1;
    }
}
