#pragma once

// ArcEngine overrides to keep the static lib's ABI fully resolved at build time
// (same trick as the upstream code-gen for double precision).
#ifndef BOX2D_FLOAT32_EPSILON
    #define BOX2D_FLOAT32_EPSILON 0.000001f
#endif

#ifndef BOX2D_DOUBLE_EPSILON
    #define BOX2D_DOUBLE_EPSILON 0.0000000001
#endif

// Force float precision + the fast b2_math path for both app and lib.
#define BOX2D_FLOAT32
#ifdef BOX2D_DOUBLE_PRECISION
    #undef BOX2D_DOUBLE_PRECISION
#endif

// Square root: fast approximation keeps the lib and user code bit-identical.
#define b2Sqrt(x) __builtin_sqrtf(x)

// Contact solver: use the 4-wide SIMD path by default (matches the float build).
#define B2_USE_CONTACT_SOLVER_W4
