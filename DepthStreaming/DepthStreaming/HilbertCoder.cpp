#include <HilbertCoder.h>
#include <MortonCoder.h>

#include <cmath>
#include <vector>
#include <assert.h>
#include <iostream>

namespace DStream
{
    template <typename T>
    static inline int sgn(T val)
    {
        return (T(0) < val) - (val < T(0));
    }

    static void TransposeAdvanceToRange(std::vector<uint16_t>& vec, uint16_t rangeMax)
    {
        uint32_t errorSum = 0;
        // Scale advance vector from 0 to 256
        for (uint32_t i=0; i<vec.size(); i++)
            errorSum += vec[i];

        for (uint32_t i=0; i<vec.size(); i++)
            vec[i] = std::round(((float)vec[i] / errorSum) * rangeMax);
    }

    static void RemoveZerosFromAdvance(std::vector<uint16_t>& advances)
    {
        std::vector<uint16_t> nonZeros;
        uint16_t zeroes = 0;
        for (uint16_t i=0; i<advances.size(); i++)
        {
            if (advances[i] == 0)
            {
                advances[i] = 1;
                zeroes++;
            }
            else
                nonZeros.push_back(advances[i]);
        }

        TransposeAdvanceToRange(nonZeros, 256 - zeroes);

        uint32_t nonzeroIdx = 0;
        for (uint32_t i=0; i<advances.size(); i++)
        {
            if (advances[i] != 1)
            {
                advances[i] = nonZeros[nonzeroIdx];
                nonzeroIdx++;
            }
        }
    }

    HilbertCoder::HilbertCoder(uint32_t q, uint32_t curveBits, bool optimizeSpacing/* = false*/) : Algorithm(q)
    {
        assert(curveBits * 3 < q);

        m_CurveBits = curveBits;
        m_SegmentBits = q - 3 * m_CurveBits;

        assert(m_CurveBits + m_SegmentBits <= 8);
        m_OptimizeSpacing = optimizeSpacing;

        // Init tables
        uint32_t side = 1 << (curveBits + m_SegmentBits);

        // Init table memory
        m_Table = new uint16_t**[side];
        for (uint32_t i=0; i<side; i++)
        {
            m_Table[i] = new uint16_t*[side];
            for (uint32_t j=0; j<side; j++)
                m_Table[i][j] = new uint16_t[side];
        }

        for (uint16_t i=0; i<side; i++)
        {
            for (uint16_t j=0; j<side; j++)
            {
                for (uint16_t k=0; k<side; k++)
                {
                    Color c = {(uint8_t)i, (uint8_t)j, (uint8_t)k};
                    m_Table[i][j][k] = ColorToValue(c, false);
                }
            }
        }

        m_XErrors.resize(side-1);
        // Init error vectors
        for (uint32_t k=0; k<side-1; k++)
        {
            uint16_t max = 0;
            for (uint32_t i=0; i<side; i++)
                for (uint32_t j=0; j<side; j++)
                    max = std::max<uint16_t>(max, abs(m_Table[k][i][j] - m_Table[k+1][i][j]));
            m_XErrors[k] = max;
        }

        m_YErrors.resize(side-1);
        for (uint32_t k=0; k<side-1; k++)
        {
            uint16_t max = 0;
            for (uint32_t i=0; i<side; i++)
                for (uint32_t j=0; j<side; j++)
                    max = std::max<uint16_t>(max, abs(m_Table[i][k][j] - m_Table[i][k+1][j]));
            m_YErrors[k] = max;
        }

        m_ZErrors.resize(side-1);
        for (uint32_t k=0; k<side-1; k++)
        {
            uint16_t max = 0;
            for (uint32_t i=0; i<side; i++)
                for (uint32_t j=0; j<side; j++)
                    max = std::max<uint16_t>(max, abs(m_Table[i][j][k] - m_Table[i][j][k+1]));
            m_ZErrors[k] = max;
        }

        for (uint16_t i=0; i<side; i++)
        {
            for (uint16_t j=0; j<side; j++)
                delete[] m_Table[i][j];
            delete[] m_Table[i];
        }

        std::vector<uint16_t> errors[3] = {m_XErrors, m_YErrors, m_ZErrors};

        for (uint32_t e=0; e<3; e++)
        {
            uint32_t nextNumber=0;

            TransposeAdvanceToRange(errors[e], 256);
            RemoveZerosFromAdvance(errors[e]);

            m_EnlargeTables[e].push_back(0);
            m_ShrinkTables[e].push_back(0);

            for (uint32_t i=1; i<errors[e].size()+1; i++)
            {
                float advance = errors[e][i-1];
                nextNumber += advance;

                if (advance == 1)
                {
                    m_ShrinkTables[e].push_back(i);
                    m_EnlargeTables[e].push_back(nextNumber);
                }
                else
                {
                    m_EnlargeTables[e].push_back(nextNumber);

                    for (uint32_t j=0; j<std::floor(advance/2); j++)
                        m_ShrinkTables[e].push_back(i-1);
                    for (uint32_t j=0; j<std::ceil(advance/2); j++)
                        m_ShrinkTables[e].push_back(i);
                }
            }
        }

        std::cout << "End of hilbert init Q: " << m_Quantization << ", curve: " << m_CurveBits << ", seg: " << m_SegmentBits << std::endl;
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
            for (int i = 0; i < 3; i++) {
                if (X[i] & Q) // Invert
                    X[0] ^= P;
                else { // Exchange
                    t = (X[0] ^ X[i]) & P;
                    X[0] ^= t;
                    X[i] ^= t;
                }
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
        for(int k = 0; k < 3; k++)
            col[k] = m_EnlargeTables[k][col[k]];
        return col;
    }

    Color HilbertCoder::Shrink(Color col)
    {
        for(int k = 0; k < 3; k++)
            col[k] = m_ShrinkTables[k][col[k]];
        return col;
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
/*
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

                    return Enlarge(v);
                }
            }
        }
*/
        Color v2 = m.ValueToColor(val + 1);
        std::swap(v2[0], v2[2]);
        TransposeFromHilbertCoords(v2);

        // Divide in segments
        for (uint32_t i=0; i<3; i++)
        {
            int mult = (v2[i] - v[i]) * frac;
            v[i] = (v[i] << m_SegmentBits) + mult;
        }

        return Enlarge(v);
    }

    uint16_t HilbertCoder::ColorToValue(const Color& col, bool shrink/*=true*/)
    {
        int side = 1<<m_SegmentBits;
        MortonCoder m(m_Quantization, m_CurveBits);
        Color col1;
        if (shrink)
            col1 = Shrink(col);
        else
            col1 = col;
        Color currColor;

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
        for(int i = 0; i < 3; i++)
            v1 += fract[i]*sgn(nextCol[i] - prevCol[i]);

        v1 <<= 16 - m_Quantization;
        return v1;
    }
}
