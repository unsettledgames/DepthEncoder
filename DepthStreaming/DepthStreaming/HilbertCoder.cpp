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

        // Init tables
        MortonCoder m(q, curveBits);
        uint32_t side = 1 << (curveBits + m_SegmentBits);
        m_Table = new uint16_t**[side];
        for (uint32_t i=0; i<side; i++)
        {
            m_Table[i] = new uint16_t*[side];
            for (uint32_t j=0; j<side; j++)
            {
                m_Table[i][j] = new uint16_t[side];
                for (uint32_t k=0; k<side; k++)
                {
                    Color c = {(uint8_t)i, (uint8_t)j, (uint8_t)k};
                    std::swap(c[0], c[2]);
                    TransposeFromHilbertCoords(c);
                    m_Table[i][j][k] = m.ColorToValue(c);
                }
            }
        }

        m_XErrors.resize(side-1);
        // Init error vectors
        for (uint32_t k=0; k<side-1; k++)
        {
            uint16_t max = 0;
            for (uint32_t i=0; i<side; i++)
            {
                for (uint32_t j=0; j<side; j++)
                {
                    max = std::max<uint16_t>(max, abs(m_Table[k][i][j] - m_Table[k+1][i][j]));
                }
            }
            m_XErrors[k] = max;
        }
/*
        for (uint32_t k=0; k<side-1; k++)
        {
            uint16_t max = 0;
            for (uint32_t i=0; i<side; i++)
            {
                for (uint32_t j=0; j<side; j++)
                {
                    max = std::max<uint16_t>(max, abs(m_Table[i][k][j] - m_Table[i][k+1][j]));
                }
            }
            m_YErrors.push_back(max);
        }

        for (uint32_t k=0; k<side-1; k++)
        {
            uint16_t max = 0;
            for (uint32_t i=0; i<side; i++)
            {
                for (uint32_t j=0; j<side; j++)
                {
                    max = std::max<uint16_t>(max, abs(m_Table[i][j][k] - m_Table[i][j][k+1]));
                }
            }
            m_ZErrors.push_back(max);
        }
*/

        for (uint16_t i=0; i<side; i++)
        {
            for (uint16_t j=0; j<side; j++)
                delete[] m_Table[i][j];
            delete[] m_Table[i];
        }

        uint32_t count=0;
        uint32_t errorSum = 0;

        std::vector<uint16_t> scaledValues;
        scaledValues.push_back(0);

        for (uint32_t i=0; i<m_XErrors.size(); i++)
            errorSum += m_XErrors[i];
        std::vector<uint16_t> advances;
        for (uint32_t i=0; i<m_XErrors.size(); i++)
            advances.push_back(std::round(((float)m_XErrors[i] / errorSum) * 256.0f));

        m_EnlargeTable.push_back(0);

        for (uint32_t i=0; i<m_XErrors.size(); i++)
        {
            float advance = advances[i];

            for (uint32_t j=0; j<advance/2; j++)
            {
                m_ShrinkTable.push_back(i);
                count++;
            }
            m_EnlargeTable.push_back(count);
            for (uint32_t j=0; j<advance/2; j++)
            {
                m_ShrinkTable.push_back(i+1);
                count++;
            }
/*
            if (i == m_XErrors.size()/2)
            {
                m_ShrinkTable.push_back(count);
                if (m_EnlargeTable[m_EnlargeTable.size()-1] != count)
                    m_EnlargeTable.push_back(count);

                for (uint32_t j=i+1; j<newSize/2; j++)
                {
                    m_ShrinkTable.push_back(i);
                    count++;
                }

                count = newSize - (i+1);
                m_EnlargeTable.push_back(count);
                for (uint32_t j=newSize/2; j<newSize-(i+1); j++)
                    m_ShrinkTable.push_back(i+1);
            }
            else
            {
                m_ShrinkTable.push_back(i);
                if (m_EnlargeTable.size() == 0 || m_EnlargeTable[m_EnlargeTable.size()-1] != count)
                    m_EnlargeTable.push_back(count);
                count++;
            }*/
        }

        std::cout << "End of hilbert initialization" << std::endl;
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
            col[k] = m_EnlargeTable[col[k]];
        return col;
    }

    Color HilbertCoder::Shrink(Color col)
    {
        for(int k = 0; k < 3; k++)
            col[k] = m_ShrinkTable[col[k]];
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

    uint16_t HilbertCoder::ColorToValue(const Color& col)
    {
        int side = 1<<m_SegmentBits;
        MortonCoder m(m_Quantization, m_CurveBits);
        Color col1 = Shrink(col);
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
        for(int i = 0; i < 3; i++) {
            v1 += fract[i]*sgn(nextCol[i] - prevCol[i]);
        }

        v1 <<= 16 - m_Quantization;
        return v1;
    }
}
