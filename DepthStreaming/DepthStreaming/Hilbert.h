#ifndef HILBERT_H
#define HILBERT_H

#include <Vec3.h>
#include <Morton.h>

#include <iostream>
#include <vector>

/** POSSIBLE ISSUES:
 *  - TansposeTo/FromHilbertCoords requires an int
 *
 */

// Credits for Hilbert convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DStream
{

    // Hilbert transpose
    static void TransposeFromHilbertCoords(Color& col, int nbits, int dim)
    {
        int X[3] = {col.x, col.y, col.z};
        uint32_t N = 2 << (nbits - 1), P, Q, t;

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

    static void TransposeToHilbertCoords(Color& col, int nbits, int dim)
    {
        int X[3] = {col.x, col.y, col.z};
        uint32_t M = 1 << (nbits - 1), P, Q, t;

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

    static Color HilbertEnlarge(Color col) {
        /*col[0] *= 4;
        col[1] *= 4;
        col[2] *= 4;
        return col; */
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

    static Color HilbertShrink(Color col) {
        /*col[0] /= 4;
        col[1] /= 4;
        col[2] /= 4;
        return col; */
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

        //std::cout << "Occupancy size: " << occupancy.size() << std::endl;
        return ret;
    }

    static Color HilbertToColor(uint16_t val, int nbits)
    {
        Color v = MortonToColor(val, nbits);
        std::swap(v[0], v[2]);
        TransposeFromHilbertCoords(v, nbits, 3);

        return HilbertEnlarge(v);
    }

    static uint16_t ColorToHilbert(const Color& col, int nbits)
    {
        Color v;
        v = HilbertShrink(col);
        TransposeToHilbertCoords(v, nbits, 3);
        std::swap(v[0], v[2]);
        return ColorToMorton(v, nbits);
    }
}

#endif // HILBERT_H
