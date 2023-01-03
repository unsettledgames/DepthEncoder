#ifndef DEFS_H
#define DEFS_H

#include <cmath>

namespace DepthEncoder
{
    enum EncodingMode {NONE = 0, TRIANGLE = 1, MORTON = 2, HILBERT = 3};

    template <typename intcode_t> constexpr int CurveOrder() { return std::ceil((sizeof(intcode_t) * 8) / 3.0f); }
}


#endif // DEFS_H
