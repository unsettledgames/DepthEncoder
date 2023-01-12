#ifndef VEC3_H
#define VEC3_H

#include <stdint.h>

namespace DStream
{
    template<typename T>
    struct _vec3
    {
        T x;
        T y;
        T z;

        inline T& operator [](int idx)
        {
            switch (idx)
            {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            }
        }
    };

    typedef _vec3<uint8_t>   Color;
    typedef _vec3<float>     Vec3;
    typedef _vec3<int>       IntVec3;
}

#endif // VEC3_H
