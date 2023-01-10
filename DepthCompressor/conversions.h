#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#define _USE_MATH_DEFINES
#define PI 3.141592653589793238463
#define PI2 PI* 2

#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>


// Credits for Morton and Hilbert convertions: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h

namespace DepthEncoder
{
    template <typename intcode_t> inline constexpr int CurveOrder() { return std::ceil((sizeof(intcode_t) * 8) / 3.0f); }

    static void TransposeFromHilbertCoords(uint8_t* X, int nBits, int dim);
    static void TransposeToHilbertCoords(int* X, int nbits, int dim);

    static int GetMortonCode(int r, int g, int b, int nbits);
    static std::vector<uint8_t> MortonToVec(uint16_t p);

    static int GetHilbertCode(int r, int g, int b, int nbits);
    static std::vector<uint8_t> HilbertToVec(uint16_t p);
    static std::vector<uint8_t> HilbertEnlarge(std::vector<uint8_t> &col);
    static std::vector<uint8_t> HilbertShrink(std::vector<uint8_t> &col);

    static int GetPhaseCode(int r, int g, int b);
    static std::vector<uint8_t> PhaseToVec(uint16_t p);

    static int GetTriangleCode(int r, int g, int b);
    static std::vector<uint8_t> TriangleToVec(uint16_t p);

    static int GetSplitCode(int r, int g, int b);
    static std::vector<uint8_t> SplitToVec(uint16_t p, int nRemovedBits = 0);

    // Hilbert transpose
    static void TransposeFromHilbertCoords(uint8_t* X, int nbits, int dim)
    {
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
    }

    static void TransposeToHilbertCoords(int* X, int nbits, int dim)
    {
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
    }

    // To morton and backwards
    static int GetMortonCode(int r, int g, int b, int nbits)
    {
        int codex = 0, codey = 0, codez = 0;

        const int nbits2 = 2 * nbits;

        for (int i = 0, andbit = 1; i < nbits2; i += 2, andbit <<= 1) {
            codex |= (int)(r & andbit) << i;
            codey |= (int)(g & andbit) << i;
            codez |= (int)(b & andbit) << i;
        }

        return ((codez << 2) | (codey << 1) | codex);
    }

    static std::vector<uint8_t> MortonToVec(uint16_t p)
    {
        const unsigned int nbits = CurveOrder<uint16_t>();
        std::vector<uint8_t> ret(3);
        ret[0] = 0; ret[1] = 0; ret[2] = 0;

        for (unsigned int i = 0; i <= nbits; ++i) {
            uint8_t selector = 1;
            unsigned int shift_selector = 3 * i;
            unsigned int shiftback = 2 * i;
            ret[0] |= (p & (selector << shift_selector)) >> (shiftback);
            ret[1] |= (p & (selector << (shift_selector + 1))) >> (shiftback + 1);
            ret[2] |= (p & (selector << (shift_selector + 2))) >> (shiftback + 2);
        }
        return ret;
    }

    static std::vector<uint8_t> HilbertEnlarge(std::vector<uint8_t> &col) {
        /*col[0] *= 4;
        col[1] *= 4;
        col[2] *= 4;
        return col; */
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
            col[k] = remap[col[k]];
        return col;
    }

    static std::vector<uint8_t> HilbertShrink(std::vector<uint8_t> &col) {
        /*col[0] /= 4;
        col[1] /= 4;
        col[2] /= 4;
        return col; */
        static std::vector<uint8_t> occupancy;
        static std::vector<uint8_t> remap;
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
            col[k] = occupancy[col[k]];
        return col;
    }

    // To Hilbert and backwards
    static int GetHilbertCode(int r, int g, int b, int nbits)
    {
        std::vector<uint8_t> v(3);
        v[0] = r; v[1] = g; v[2] = b;
        v = HilbertShrink(v);
        int col[3] = {v[0], v[1], v[2]};
        TransposeToHilbertCoords(col, nbits, 3);
        return GetMortonCode(col[2], col[1], col[0], nbits);
    }

    // Credits: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h
    static std::vector<uint8_t> HilbertToVec(uint16_t p)
    {
        const int nbits = CurveOrder<uint16_t>();

        std::vector<uint8_t> v = MortonToVec(p);
        std::swap(v[0], v[2]);
        TransposeFromHilbertCoords(v.data(), nbits, 3);

        return HilbertEnlarge(v);
    }

    // To Phase encoding and backwards
    static int GetPhaseCode(int r, int g, int b)
    {
        const float w = 65535.0f;
        const float P = 16384.0f;
        const float beta = P / 2.0f;
        float gamma, phi, PHI, K, Z;
        float i1 = r / 255.0f, i2 = g / 255.0f;

        if (2.0f * i1 - 1.0f <-1)
        {
            std::cout << "err" << std::endl;
        }
        phi = std::fabs(std::acos(2.0f * i1 - 1.0f));
        gamma = std::floor((i2 * w) / beta);

        if (((int)gamma) % 2)
            phi *= -1;

        K = std::round((i2 * w) / P);
        PHI = phi + 2 * M_PI * K;

        Z = PHI * (P / (M_PI * 2.0f));
        return Z;
    }

    static std::vector<uint8_t> PhaseToVec(uint16_t d)
    {
        std::vector<uint8_t> ret(3);
        const float P = 16384.0f;
        const float w = 65535.0f;

        ret[0] = 255 * (0.5f + 0.5f * std::cos(M_PI * 2.0f * (d / P)));
        ret[1] = 255 * (d / w);
        ret[2] = 0;

        return ret;
    }

    static int GetTriangleCode(int r, int g, int b)
    {
        const int w = 65536;
        // Function data
        int np = 512;
        float p = (float)np / w;
        float Ld = r / 255.0f;
        float Ha = g / 255.0f;
        float Hb = b / 255.0f;
        int m = (int)std::floor(4.0 * (Ld / p) - 0.5f) % 4;
        float L0 = Ld - (fmod( Ld - p/8.0f, p)) + (p/4.0) * m - p/8.0;
        float delta = 0;

        switch (m) {
        case 0:
            delta = (p/2.0f) * Ha;
            break;
        case 1:
            delta = (p/2.0f) * Hb;
            break;
        case 2:
            delta = (p/2.0f) * (1.0f - Ha);
            break;
        case 3:
            delta = (p/2.0f) * (1.0f - Hb);
            break;
        }

        return (L0 + delta) * w;
    }

    static std::vector<uint8_t> TriangleToVec(uint16_t d)
    {
        const float w = 65536.0f;
        const float p = 512.0f / w;
        std::vector<uint8_t> ret(3);

        float Ld, Ha, Hb;
        Ld = (d + 0.5) / w;

        float mod = fmod(Ld / (p/2.0f), 2.0f);
        if (mod <= 1)
            Ha = mod;
        else
            Ha = 2 - mod;

        float mod2 = fmod((Ld - p/4.0f) / (p/2.0f), 2.0f);
        if (mod2 <= 1)
            Hb = mod2;
        else
            Hb = 2 - mod2;

        Ld *= 255; Ha *= 255; Hb *= 255;
        ret[0] = Ld; ret[1] = Ha; ret[2] = Hb;

        return ret;
    }

    static int GetSplitCode(int r, int g, int b)
    {
        float Ld = r;
        float Ha = g;
        int m = (int)r % 2;
        float delta = 0;

        switch (m) {
        case 0:
            delta = Ha;
            break;
        case 1:
            delta = 255.0f - Ha;
            break;
        }

        return Ld * 256.0f + delta;
    }

    static std::vector<uint8_t> SplitToVec(uint16_t d, int nRemovedBits/* = 0*/)
    {
        std::vector<uint8_t> ret(3);

        float Ld, Ha;

        float mod = fmod(d / 256.0f, 2.0f);
        if (mod <= 1)
            Ha = mod;
        else
            Ha =2 - mod;

        Ld = d >> 8; Ha *= 255.0f;

        if (mod <= 1)
            Ha = std::ceil(Ha);
        else
            Ha = std::floor(Ha);

        ret[0] = Ld; ret[1] = Ha; ret[2] = 0.0f;

        return ret;
    }
}

#endif // CONVERSIONS_H
