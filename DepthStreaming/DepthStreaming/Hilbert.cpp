#include <Hilbert.h>
#include <Morton.h>

#include <vector>

namespace DStream
{
    Hilbert::Hilbert(int q, int nbits, bool optimizeSpacing/* = false*/) : Algorithm(q),
        m_NBits(nbits), m_OptimizeSpacing(optimizeSpacing){}

    void Hilbert::Encode(uint16_t* values, uint8_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded = ValueToColor(values[i]);
            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }
    }

    void Hilbert::Decode(uint8_t* values, uint16_t* dest, uint32_t count)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};
            dest[i] = ColorToValue(currColor);
        }
    }

    void Hilbert::TransposeFromHilbertCoords(Color& col, int dim)
    {
        int X[3] = {col.x, col.y, col.z};
        uint32_t N = 2 << (m_NBits - 1), P, Q, t;

        // Gray decode by H ^ (H/2)
        t = X[dim - 1] >> 1;
        // Corrected error in Skilling's paper on the following line. The appendix had i >= 0 leading to negative array index.
        for (int i = dim - 1; i > 0; i--) X[i] ^= X[i - 1];
        X[0] ^= t;

        // Undo excess work
        for (Q = 2; Q != N; Q <<= 1) {
            P = Q - 1;
            for (int i = dim - 1; i >= 0; i--)
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

    void Hilbert::TransposeToHilbertCoords(Color& col, int dim)
    {
        int X[3] = {col.x, col.y, col.z};
        uint32_t M = 1 << (m_NBits - 1), P, Q, t;

        // Inverse undo

        for (Q = M; Q > 1; Q >>= 1) {
            P = Q - 1;
            for (int i = 0; i < dim; i++)
                if (X[i] & Q) // Invert
                    X[0] ^= P;
                else { // Exchange
                    t = (X[0] ^ X[i]) & P;
                    X[0] ^= t;
                    X[i] ^= t;
                }
        }

        // Gray encode
        for (int i = 1; i < dim; i++) X[i] ^= X[i - 1];
        t = 0;
        for (Q = M; Q > 1; Q >>= 1)
            if (X[dim - 1] & Q) t ^= Q - 1;
        for (int i = 0; i < dim; i++) X[i] ^= t;

        col.x = X[0]; col.y = X[1]; col.z = X[2];
    }

    Color Hilbert::Enlarge(Color col)
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

    Color Hilbert::Shrink(Color col)
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

    Color Hilbert::ValueToColor(uint16_t val)
    {
        // TODO: static? Have it as an attribute?
        Morton m(m_NBits, m_Quantization);
        Color v = m.ValueToColor(val);
        std::swap(v[0], v[2]);
        TransposeFromHilbertCoords(v, 3);

        return Enlarge(v);
    }

    uint16_t Hilbert::ColorToValue(const Color& col)
    {
        Morton m(m_NBits, m_Quantization);
        Color v = Shrink(col);
        TransposeToHilbertCoords(v, 3);
        std::swap(v[0], v[2]);
        return m.ColorToValue(v);
    }
}

