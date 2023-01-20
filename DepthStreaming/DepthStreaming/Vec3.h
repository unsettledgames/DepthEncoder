#ifndef VEC3_H
#define VEC3_H

#include <stdint.h>

namespace DStream
{
    template<typename T>
    struct _vec3
    {
    union {
        T v[3];
        struct {
            T x, y, z;
        };
    };

        inline T& operator [](int idx)
        {
            return v[idx];
        }
    };

    typedef _vec3<uint8_t>   Color;
    typedef _vec3<float>     Vec3;
    typedef _vec3<int>       IntVec3;
}

#endif // VEC3_H
