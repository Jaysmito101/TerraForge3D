#pragma once

/*
* The data tpe to be used for math functions
* 
* NOTE: This is not currently being used. Now TerraForge3D Math works only with floats
*/
#ifdef TF3D_HIGHP_MATH
#define TF3D_MATH_VALUE_TYPE double
#else
#define TF3D_MATH_VALUE_TYPE float
#endif

#include <cmath>
#include <cstdint>

namespace TerraForge3D
{

    /*
    * The well known Fast Inverse Square Root Algorithm
    * 
    * Reference: https://en.wikipedia.org/wiki/Fast_inverse_square_root
    */
    inline float FastInverseSqrt(float number) {
        union {
            float f;
            uint32_t i;
        } conv;

        float x2;
        const float threehalfs = 1.5F;

        x2 = number * 0.5F;
        conv.f = number;
        conv.i = 0x5f3759df - (conv.i >> 1);
        conv.f = conv.f * (threehalfs - (x2 * conv.f * conv.f));
        return conv.f;
    }

}