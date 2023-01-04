#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#define _USE_MATH_DEFINES
#define PI 3.141592653589793238463
#define PI2 PI* 2

#include <cmath>
#include <cstdint>
#include <vector>


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

    static int GetPhaseCode(int r, int g, int b);
    static std::vector<uint8_t> PhaseToVec(uint16_t p);

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

    // To Hilbert and backwards
    static int GetHilbertCode(int r, int g, int b, int nbits)
    {
        int col[3] = {r,g,b};
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

        return v;
    }

    // To Phase encoding and backwards
    static int GetPhaseCode(int r, int g, int b)
    {
        const float w = 65535.0f;
        const float P = 16384.0f;
        const float beta = P / 2.0f;
        float gamma, phi, PHI, K, Z;
        float i1 = r / 255.0f, i2 = g / 255.0f;

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
}

#endif // CONVERSIONS_H
