#include <HilbertCoder.h>
#include <MortonCoder.h>

#include <vector>
#include <iostream>

namespace DStream
{
    HilbertCoder::HilbertCoder(uint32_t q, uint32_t curveBits, bool optimizeSpacing/* = false*/) : Algorithm(q),
        m_CurveBits(curveBits), m_SegmentBits(curveBits >= 6 ? 0 : 16 - 3*curveBits), m_OptimizeSpacing(optimizeSpacing){}

    void HilbertCoder::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(Quantize(values[i]));
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

            for(int i = 0; i < occupancy.size(); i++) {
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
        int frac = val & ((1 << m_SegmentBits) - 1);
        val >>= m_SegmentBits;

        MortonCoder m(m_Quantization, m_CurveBits);
        Color v = m.ValueToColor(val);
        Color v2 = m.ValueToColor(val + 1);

        std::swap(v[0], v[2]);
        std::swap(v2[0], v2[2]);

        TransposeFromHilbertCoords(v);
        TransposeFromHilbertCoords(v2);

        // Divide in segments
        for (uint32_t i=0; i<3; i++)
            v[i] = ((v[i] << m_SegmentBits) + (v2[i] - v[i]) * frac);

        return Enlarge(v); // TODO: Enlarge here
    }

    uint16_t HilbertCoder::ColorToValue(const Color& col)
    {
        MortonCoder m(m_Quantization, m_CurveBits);
        Color col1 = Shrink(col);
        int fract = 0;
        for (uint32_t i=0; i<3; i++)
            fract |= col1[i] & ~(~0 << m_SegmentBits);

        for (uint32_t i=0; i<3; i++)
            col1[i] >>= m_SegmentBits;

        TransposeToHilbertCoords(col1);
        std::swap(col1[0], col1[2]);
        uint16_t v1 = m.ColorToValue(col1);

        // Add back fractional part
        v1 <<= m_SegmentBits;
        v1 += fract;

        return v1;
    }
}

