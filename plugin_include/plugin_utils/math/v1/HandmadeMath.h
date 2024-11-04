// This file is the Handmademath.h (https://github.com/HandmadeMath/HandmadeMath/blob/master/HandmadeMath.h)
// math library, but with types renamed to use the Hatch vector types.
// This is the quick and dirty solution for a math lib and is good enough for now.

/*
  HandmadeMath.h v2.0.0

  This is a single header file with a bunch of useful types and functions for
  games and graphics. Consider it a lightweight alternative to GLM that works
  both C and C++.

  =============================================================================
  CONFIG
  =============================================================================

  By default, all angles in Handmade Math are specified in radians. However, it
  can be configured to use degrees or turns instead. Use one of the following
  defines to specify the default unit for angles:

    #define HANDMADE_MATH_USE_RADIANS
    #define HANDMADE_MATH_USE_DEGREES
    #define HANDMADE_MATH_USE_TURNS

  Regardless of the default angle, you can use the following functions to
  specify an angle in a particular unit:

    M_AngleRad(radians)
    M_AngleDeg(degrees)
    M_AngleTurn(turns)

  The definitions of these functions change depending on the default unit.

  -----------------------------------------------------------------------------

  Handmade Math ships with SSE (SIMD) implementations of several common
  operations. To disable the use of SSE intrinsics, you must define
  HANDMADE_MATH_NO_SSE before including this file:

    #define HANDMADE_MATH_NO_SSE
    #include "HandmadeMath.h"

  -----------------------------------------------------------------------------

  To use Handmade Math without the C runtime library, you must provide your own
  implementations of basic math functions. Otherwise, HandmadeMath.h will use
  the runtime library implementation of these functions.

  Define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS and provide your own
  implementations of M_SINF, M_COSF, M_TANF, M_ACOSF, and M_SQRTF
  before including HandmadeMath.h, like so:

    #define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS
    #define M_SINF MySinF
    #define M_COSF MyCosF
    #define M_TANF MyTanF
    #define M_ACOSF MyACosF
    #define M_SQRTF MySqrtF
    #include "HandmadeMath.h"

  By default, it is assumed that your math functions take radians. To use
  different units, you must define M_ANGLE_USER_TO_INTERNAL and
  M_ANGLE_INTERNAL_TO_USER. For example, if you want to use degrees in your
  code but your math functions use turns:

    #define M_ANGLE_USER_TO_INTERNAL(a) ((a)*M_DegToTurn)
    #define M_ANGLE_INTERNAL_TO_USER(a) ((a)*M_TurnToDeg)

  =============================================================================

  LICENSE

  This software is in the public domain. Where that dedication is not
  recognized, you are granted a perpetual, irrevocable license to copy,
  distribute, and modify this file as you see fit.

  =============================================================================

  CREDITS

  Originally written by Zakary Strange.

  Functionality:
   Zakary Strange (strangezak@protonmail.com && @strangezak)
   Matt Mascarenhas (@miblo_)
   Aleph
   FieryDrake (@fierydrake)
   Gingerbill (@TheGingerBill)
   Ben Visness (@bvisness)
   Trinton Bullard (@Peliex_Dev)
   @AntonDan
   Logan Forman (@dev_dwarf)

  Fixes:
   Jeroen van Rijn (@J_vanRijn)
   Kiljacken (@Kiljacken)
   Insofaras (@insofaras)
   Daniel Gibson (@DanielGibson)
*/

#pragma once
#define MATH_INCLUDED

// Dummy macros for when test framework is not present.
#ifndef COVERAGE
# define COVERAGE(a, b)
#endif

#ifndef ASSERT_COVERED
# define ASSERT_COVERED(a)
#endif

// TODO: SIMD math
#if 0

/* let's figure out if SSE is really available (unless disabled anyway)
   (it isn't on non-x86/x86_64 platforms or even x86 without explicit SSE support)
   => only use "#ifdef HANDMADE_MATH__USE_SSE" to check for SSE support below this block! */
#ifndef HANDMADE_MATH_NO_SIMD
# ifdef _MSC_VER /* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#  if defined(_M_AMD64) || ( defined(_M_IX86_FP) && _M_IX86_FP >= 1 )
#   define HANDMADE_MATH__USE_SSE 1
#  endif
# else /* not MSVC, probably GCC, clang, icc or something that doesn't support SSE anyway */
#  ifdef __SSE__ /* they #define __SSE__ if it's supported */
#   define HANDMADE_MATH__USE_SSE 1
#  endif /*  __SSE__ */
# endif /* not _MSC_VER */
# ifdef __ARM_NEON
#  define HANDMADE_MATH__USE_NEON 1
# endif /* NEON Supported */
#endif /* #ifndef HANDMADE_MATH_NO_SIMD */

#if (!defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
# define HANDMADE_MATH__USE_C11_GENERICS 1
#endif

#endif

#ifdef HANDMADE_MATH__USE_SSE
# include <xmmintrin.h>
#endif

#ifdef HANDMADE_MATH__USE_NEON
# include <arm_neon.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif

#if defined(__GNUC__) || defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wfloat-equal"
# pragma GCC diagnostic ignored "-Wmissing-braces"
# ifdef __clang__
#  pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# endif
#endif

#if defined(__GNUC__) || defined(__clang__)
# define M_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
# define M_DEPRECATED(msg) __declspec(deprecated(msg))
#else
# define M_DEPRECATED(msg)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(HANDMADE_MATH_USE_DEGREES) \
    && !defined(HANDMADE_MATH_USE_TURNS) \
    && !defined(HANDMADE_MATH_USE_RADIANS)
# define HANDMADE_MATH_USE_RADIANS
#endif

#define M_PI 3.14159265358979323846
#define M_PI32 3.14159265359f
#define M_DEG180 180.0
#define M_DEG18032 180.0f
#define M_TURNHALF 0.5
#define M_TURNHALF32 0.5f
#define M_RadToDeg ((float)(M_DEG180/M_PI))
#define M_RadToTurn ((float)(M_TURNHALF/M_PI))
#define M_DegToRad ((float)(M_PI/M_DEG180))
#define M_DegToTurn ((float)(M_TURNHALF/M_DEG180))
#define M_TurnToRad ((float)(M_PI/M_TURNHALF))
#define M_TurnToDeg ((float)(M_DEG180/M_TURNHALF))

#if defined(HANDMADE_MATH_USE_RADIANS)
# define M_AngleRad(a) (a)
# define M_AngleDeg(a) ((a)*M_DegToRad)
# define M_AngleTurn(a) ((a)*M_TurnToRad)
#elif defined(HANDMADE_MATH_USE_DEGREES)
# define M_AngleRad(a) ((a)*M_RadToDeg)
# define M_AngleDeg(a) (a)
# define M_AngleTurn(a) ((a)*M_TurnToDeg)
#elif defined(HANDMADE_MATH_USE_TURNS)
# define M_AngleRad(a) ((a)*M_RadToTurn)
# define M_AngleDeg(a) ((a)*M_DegToTurn)
# define M_AngleTurn(a) (a)
#endif

#if !defined(HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS)
# include <math.h>
# define M_SINF sinf
# define M_COSF cosf
# define M_TANF tanf
# define M_SQRTF sqrtf
# define M_ACOSF acosf
#endif

#if !defined(M_ANGLE_USER_TO_INTERNAL)
# define M_ANGLE_USER_TO_INTERNAL(a) (M_ToRad(a))
#endif

#if !defined(M_ANGLE_INTERNAL_TO_USER)
# if defined(HANDMADE_MATH_USE_RADIANS)
#  define M_ANGLE_INTERNAL_TO_USER(a) (a)
# elif defined(HANDMADE_MATH_USE_DEGREES)
#  define M_ANGLE_INTERNAL_TO_USER(a) ((a)*M_RadToDeg)
# elif defined(HANDMADE_MATH_USE_TURNS)
#  define M_ANGLE_INTERNAL_TO_USER(a) ((a)*M_RadToTurn)
# endif
#endif

#define M_MIN(a, b) ((a) > (b) ? (b) : (a))
#define M_MAX(a, b) ((a) < (b) ? (b) : (a))
#define M_ABS(a) ((a) > 0 ? (a) : -(a))
#define M_MOD(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))
#define M_SQUARE(x) ((x) * (x))

typedef union mat2
{
    float _[2][2];
    vec2 Columns[2];

#ifdef __cplusplus
    inline vec2 &operator[](int Index) { return Columns[Index]; }
    inline const vec2 &operator[](int Index) const { return Columns[Index]; }
#endif
} mat2;

typedef union mat3
{
    float _[3][3];
    vec3 Columns[3];

#ifdef __cplusplus
    inline vec3 &operator[](int Index) { return Columns[Index]; }
    inline const vec3 &operator[](int Index) const { return Columns[Index]; }
#endif
} mat3;

typedef union mat4
{
    float _[4][4];
    vec4 Columns[4];

#ifdef __cplusplus
    inline vec4 &operator[](int Index) { return Columns[Index]; }
    inline const vec4 &operator[](int Index) const { return Columns[Index]; }
#endif
} mat4;

typedef union quat
{
    struct
    {
        union
        {
            vec3 xyz;
            struct
            {
                float x, y, z;
            };
        };

        float w;
    };

    float _[4];

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSE;
#endif
#ifdef HANDMADE_MATH__USE_NEON
    float32x4_t NEON;
#endif
} quat;

typedef signed int M_Bool;

/*
 * Angle unit conversion functions
 */
static inline float M_ToRad(float Angle)
{
#if defined(HANDMADE_MATH_USE_RADIANS)
    float Result = Angle;
#elif defined(HANDMADE_MATH_USE_DEGREES)
    float Result = Angle * M_DegToRad;
#elif defined(HANDMADE_MATH_USE_TURNS)
    float Result = Angle * M_TurnToRad;
#endif

    return Result;
}

static inline float M_ToDeg(float Angle)
{
#if defined(HANDMADE_MATH_USE_RADIANS)
    float Result = Angle * M_RadToDeg;
#elif defined(HANDMADE_MATH_USE_DEGREES)
    float Result = Angle;
#elif defined(HANDMADE_MATH_USE_TURNS)
    float Result = Angle * M_TurnToDeg;
#endif

    return Result;
}

static inline float M_ToTurn(float Angle)
{
#if defined(HANDMADE_MATH_USE_RADIANS)
    float Result = Angle * M_RadToTurn;
#elif defined(HANDMADE_MATH_USE_DEGREES)
    float Result = Angle * M_DegToTurn;
#elif defined(HANDMADE_MATH_USE_TURNS)
    float Result = Angle;
#endif

    return Result;
}

/*
 * Floating-point math functions
 */

COVERAGE(M_SinF, 1)
static inline float M_SinF(float Angle)
{
    ASSERT_COVERED(M_SinF);
    return M_SINF(M_ANGLE_USER_TO_INTERNAL(Angle));
}

COVERAGE(M_CosF, 1)
static inline float M_CosF(float Angle)
{
    ASSERT_COVERED(M_CosF);
    return M_COSF(M_ANGLE_USER_TO_INTERNAL(Angle));
}

COVERAGE(M_TanF, 1)
static inline float M_TanF(float Angle)
{
    ASSERT_COVERED(M_TanF);
    return M_TANF(M_ANGLE_USER_TO_INTERNAL(Angle));
}

COVERAGE(M_ACosF, 1)
static inline float M_ACosF(float Arg)
{
    ASSERT_COVERED(M_ACosF);
    return M_ANGLE_INTERNAL_TO_USER(M_ACOSF(Arg));
}

COVERAGE(M_SqrtF, 1)
static inline float M_SqrtF(float Float)
{
    ASSERT_COVERED(M_SqrtF);

    float Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 In = _mm_set_ss(Float);
    __m128 Out = _mm_sqrt_ss(In);
    Result = _mm_cvtss_f32(Out);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t In = vdupq_n_f32(Float);
    float32x4_t Out = vsqrtq_f32(In);
    Result = vgetq_lane_f32(Out, 0);
#else
    Result = M_SQRTF(Float);
#endif

    return Result;
}

COVERAGE(M_InvSqrtF, 1)
static inline float M_InvSqrtF(float Float)
{
    ASSERT_COVERED(M_InvSqrtF);

    float Result;

    Result = 1.0f/M_SqrtF(Float);

    return Result;
}


/*
 * Utility functions
 */

COVERAGE(M_Lerp, 1)
static inline float M_Lerp(float A, float Time, float B)
{
    ASSERT_COVERED(M_Lerp);
    return (1.0f - Time) * A + Time * B;
}

COVERAGE(M_Clamp, 1)
static inline float M_Clamp(float Min, float Value, float Max)
{
    ASSERT_COVERED(M_Clamp);

    float Result = Value;

    if (Result < Min)
    {
        Result = Min;
    }

    if (Result > Max)
    {
        Result = Max;
    }

    return Result;
}


/*
 * Vector initialization
 */

COVERAGE(M_V2, 1)
static inline vec2 M_V2(float x, float y)
{
    ASSERT_COVERED(M_V2);

    vec2 Result;
    Result.x = x;
    Result.y = y;

    return Result;
}

COVERAGE(M_V3, 1)
static inline vec3 M_V3(float x, float y, float z)
{
    ASSERT_COVERED(M_V3);

    vec3 Result;
    Result.x = x;
    Result.y = y;
    Result.z = z;

    return Result;
}

COVERAGE(M_V4, 1)
static inline vec4 M_V4(float x, float y, float z, float w)
{
    ASSERT_COVERED(M_V4);

    vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_setr_ps(x, y, z, w);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t v = {x, y, z, w};
    Result.NEON = v;
#else
    Result.x = x;
    Result.y = y;
    Result.z = z;
    Result.w = w;
#endif

    return Result;
}

COVERAGE(M_V4V, 1)
static inline vec4 M_V4V(vec3 Vector, float w)
{
    ASSERT_COVERED(M_V4V);

    vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_setr_ps(Vector.x, Vector.y, Vector.z, w);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t v = {Vector.x, Vector.y, Vector.z, w};
    Result.NEON = v;
#else
    Result.xyz = Vector;
    Result.w = w;
#endif

    return Result;
}


/*
 * Binary vector operations
 */

COVERAGE(M_AddV2, 1)
static inline vec2 M_AddV2(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_AddV2);

    vec2 Result;
    Result.x = Left.x + Right.x;
    Result.y = Left.y + Right.y;

    return Result;
}

COVERAGE(M_AddV3, 1)
static inline vec3 M_AddV3(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_AddV3);

    vec3 Result;
    Result.x = Left.x + Right.x;
    Result.y = Left.y + Right.y;
    Result.z = Left.z + Right.z;

    return Result;
}

COVERAGE(M_AddV4, 1)
static inline vec4 M_AddV4(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_AddV4);

    vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_add_ps(Left.SSE, Right.SSE);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vaddq_f32(Left.NEON, Right.NEON);
#else
    Result.x = Left.x + Right.x;
    Result.y = Left.y + Right.y;
    Result.z = Left.z + Right.z;
    Result.w = Left.w + Right.w;
#endif

    return Result;
}

COVERAGE(M_SubV2, 1)
static inline vec2 M_SubV2(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_SubV2);

    vec2 Result;
    Result.x = Left.x - Right.x;
    Result.y = Left.y - Right.y;

    return Result;
}

COVERAGE(M_SubV3, 1)
static inline vec3 M_SubV3(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_SubV3);

    vec3 Result;
    Result.x = Left.x - Right.x;
    Result.y = Left.y - Right.y;
    Result.z = Left.z - Right.z;

    return Result;
}

COVERAGE(M_SubV4, 1)
static inline vec4 M_SubV4(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_SubV4);

    vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_sub_ps(Left.SSE, Right.SSE);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vsubq_f32(Left.NEON, Right.NEON);
#else
    Result.x = Left.x - Right.x;
    Result.y = Left.y - Right.y;
    Result.z = Left.z - Right.z;
    Result.w = Left.w - Right.w;
#endif

    return Result;
}

COVERAGE(M_MulV2, 1)
static inline vec2 M_MulV2(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_MulV2);

    vec2 Result;
    Result.x = Left.x * Right.x;
    Result.y = Left.y * Right.y;

    return Result;
}

COVERAGE(M_MulV2F, 1)
static inline vec2 M_MulV2F(vec2 Left, float Right)
{
    ASSERT_COVERED(M_MulV2F);

    vec2 Result;
    Result.x = Left.x * Right;
    Result.y = Left.y * Right;

    return Result;
}

COVERAGE(M_MulV3, 1)
static inline vec3 M_MulV3(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_MulV3);

    vec3 Result;
    Result.x = Left.x * Right.x;
    Result.y = Left.y * Right.y;
    Result.z = Left.z * Right.z;

    return Result;
}

COVERAGE(M_MulV3F, 1)
static inline vec3 M_MulV3F(vec3 Left, float Right)
{
    ASSERT_COVERED(M_MulV3F);

    vec3 Result;
    Result.x = Left.x * Right;
    Result.y = Left.y * Right;
    Result.z = Left.z * Right;

    return Result;
}

COVERAGE(M_MulV4, 1)
static inline vec4 M_MulV4(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_MulV4);

    vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_mul_ps(Left.SSE, Right.SSE);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vmulq_f32(Left.NEON, Right.NEON);
#else
    Result.x = Left.x * Right.x;
    Result.y = Left.y * Right.y;
    Result.z = Left.z * Right.z;
    Result.w = Left.w * Right.w;
#endif

    return Result;
}

COVERAGE(M_MulV4F, 1)
static inline vec4 M_MulV4F(vec4 Left, float Right)
{
    ASSERT_COVERED(M_MulV4F);

    vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Right);
    Result.SSE = _mm_mul_ps(Left.SSE, Scalar);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vmulq_n_f32(Left.NEON, Right);
#else
    Result.x = Left.x * Right;
    Result.y = Left.y * Right;
    Result.z = Left.z * Right;
    Result.w = Left.w * Right;
#endif

    return Result;
}

COVERAGE(M_DivV2, 1)
static inline vec2 M_DivV2(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_DivV2);

    vec2 Result;
    Result.x = Left.x / Right.x;
    Result.y = Left.y / Right.y;

    return Result;
}

COVERAGE(M_DivV2F, 1)
static inline vec2 M_DivV2F(vec2 Left, float Right)
{
    ASSERT_COVERED(M_DivV2F);

    vec2 Result;
    Result.x = Left.x / Right;
    Result.y = Left.y / Right;

    return Result;
}

COVERAGE(M_DivV3, 1)
static inline vec3 M_DivV3(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_DivV3);

    vec3 Result;
    Result.x = Left.x / Right.x;
    Result.y = Left.y / Right.y;
    Result.z = Left.z / Right.z;

    return Result;
}

COVERAGE(M_DivV3F, 1)
static inline vec3 M_DivV3F(vec3 Left, float Right)
{
    ASSERT_COVERED(M_DivV3F);

    vec3 Result;
    Result.x = Left.x / Right;
    Result.y = Left.y / Right;
    Result.z = Left.z / Right;

    return Result;
}

COVERAGE(M_DivV4, 1)
static inline vec4 M_DivV4(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_DivV4);

    vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_div_ps(Left.SSE, Right.SSE);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vdivq_f32(Left.NEON, Right.NEON);
#else
    Result.x = Left.x / Right.x;
    Result.y = Left.y / Right.y;
    Result.z = Left.z / Right.z;
    Result.w = Left.w / Right.w;
#endif

    return Result;
}

COVERAGE(M_DivV4F, 1)
static inline vec4 M_DivV4F(vec4 Left, float Right)
{
    ASSERT_COVERED(M_DivV4F);

    vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Right);
    Result.SSE = _mm_div_ps(Left.SSE, Scalar);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t Scalar = vdupq_n_f32(Right);
    Result.NEON = vdivq_f32(Left.NEON, Scalar);
#else
    Result.x = Left.x / Right;
    Result.y = Left.y / Right;
    Result.z = Left.z / Right;
    Result.w = Left.w / Right;
#endif

    return Result;
}

COVERAGE(M_EqV2, 1)
static inline M_Bool M_EqV2(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_EqV2);
    return Left.x == Right.x && Left.y == Right.y;
}

COVERAGE(M_EqV3, 1)
static inline M_Bool M_EqV3(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_EqV3);
    return Left.x == Right.x && Left.y == Right.y && Left.z == Right.z;
}

COVERAGE(M_EqV4, 1)
static inline M_Bool M_EqV4(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_EqV4);
    return Left.x == Right.x && Left.y == Right.y && Left.z == Right.z && Left.w == Right.w;
}

COVERAGE(M_DotV2, 1)
static inline float M_DotV2(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_DotV2);
    return (Left.x * Right.x) + (Left.y * Right.y);
}

COVERAGE(M_DotV3, 1)
static inline float M_DotV3(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_DotV3);
    return (Left.x * Right.x) + (Left.y * Right.y) + (Left.z * Right.z);
}

COVERAGE(M_DotV4, 1)
static inline float M_DotV4(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_DotV4);

    float Result;

    // NOTE(zak): IN the future if we wanna check what version SSE is support
    // we can use _mm_dp_ps (4.3) but for now we will use the old way.
    // Or a r = _mm_mul_ps(v1, v2), r = _mm_hadd_ps(r, r), r = _mm_hadd_ps(r, r) for SSE3
#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_mul_ps(Left.SSE, Right.SSE);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&Result, SSEResultOne);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t NEONMultiplyResult = vmulq_f32(Left.NEON, Right.NEON);
    float32x4_t NEONHalfAdd = vpaddq_f32(NEONMultiplyResult, NEONMultiplyResult);
    float32x4_t NEONFullAdd = vpaddq_f32(NEONHalfAdd, NEONHalfAdd);
    Result = vgetq_lane_f32(NEONFullAdd, 0);
#else
    Result = ((Left.x * Right.x) + (Left.z * Right.z)) + ((Left.y * Right.y) + (Left.w * Right.w));
#endif

    return Result;
}

COVERAGE(M_Cross, 1)
static inline vec3 M_Cross(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_Cross);

    vec3 Result;
    Result.x = (Left.y * Right.z) - (Left.z * Right.y);
    Result.y = (Left.z * Right.x) - (Left.x * Right.z);
    Result.z = (Left.x * Right.y) - (Left.y * Right.x);

    return Result;
}


/*
 * Unary vector operations
 */

COVERAGE(M_LenSqrV2, 1)
static inline float M_LenSqrV2(vec2 A)
{
    ASSERT_COVERED(M_LenSqrV2);
    return M_DotV2(A, A);
}

COVERAGE(M_LenSqrV3, 1)
static inline float M_LenSqrV3(vec3 A)
{
    ASSERT_COVERED(M_LenSqrV3);
    return M_DotV3(A, A);
}

COVERAGE(M_LenSqrV4, 1)
static inline float M_LenSqrV4(vec4 A)
{
    ASSERT_COVERED(M_LenSqrV4);
    return M_DotV4(A, A);
}

COVERAGE(M_LenV2, 1)
static inline float M_LenV2(vec2 A)
{
    ASSERT_COVERED(M_LenV2);
    return M_SqrtF(M_LenSqrV2(A));
}

COVERAGE(M_LenV3, 1)
static inline float M_LenV3(vec3 A)
{
    ASSERT_COVERED(M_LenV3);
    return M_SqrtF(M_LenSqrV3(A));
}

COVERAGE(M_LenV4, 1)
static inline float M_LenV4(vec4 A)
{
    ASSERT_COVERED(M_LenV4);
    return M_SqrtF(M_LenSqrV4(A));
}

COVERAGE(M_NormV2, 1)
static inline vec2 M_NormV2(vec2 A)
{
    ASSERT_COVERED(M_NormV2);
    return M_MulV2F(A, M_InvSqrtF(M_DotV2(A, A)));
}

COVERAGE(M_NormV3, 1)
static inline vec3 M_NormV3(vec3 A)
{
    ASSERT_COVERED(M_NormV3);
    return M_MulV3F(A, M_InvSqrtF(M_DotV3(A, A)));
}

COVERAGE(M_NormV4, 1)
static inline vec4 M_NormV4(vec4 A)
{
    ASSERT_COVERED(M_NormV4);
    return M_MulV4F(A, M_InvSqrtF(M_DotV4(A, A)));
}

/*
 * Utility vector functions
 */

COVERAGE(M_LerpV2, 1)
static inline vec2 M_LerpV2(vec2 A, float Time, vec2 B)
{
    ASSERT_COVERED(M_LerpV2);
    return M_AddV2(M_MulV2F(A, 1.0f - Time), M_MulV2F(B, Time));
}

COVERAGE(M_LerpV3, 1)
static inline vec3 M_LerpV3(vec3 A, float Time, vec3 B)
{
    ASSERT_COVERED(M_LerpV3);
    return M_AddV3(M_MulV3F(A, 1.0f - Time), M_MulV3F(B, Time));
}

COVERAGE(M_LerpV4, 1)
static inline vec4 M_LerpV4(vec4 A, float Time, vec4 B)
{
    ASSERT_COVERED(M_LerpV4);
    return M_AddV4(M_MulV4F(A, 1.0f - Time), M_MulV4F(B, Time));
}

/*
 * SSE stuff
 */

COVERAGE(M_LinearCombineV4M4, 1)
static inline vec4 M_LinearCombineV4M4(vec4 Left, mat4 Right)
{
    ASSERT_COVERED(M_LinearCombineV4M4);

    vec4 Result;
#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0x00), Right.Columns[0].SSE);
    Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0x55), Right.Columns[1].SSE));
    Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0xaa), Right.Columns[2].SSE));
    Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0xff), Right.Columns[3].SSE));
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vmulq_laneq_f32(Right.Columns[0].NEON, Left.NEON, 0);
    Result.NEON = vfmaq_laneq_f32(Result.NEON, Right.Columns[1].NEON, Left.NEON, 1);
    Result.NEON = vfmaq_laneq_f32(Result.NEON, Right.Columns[2].NEON, Left.NEON, 2);
    Result.NEON = vfmaq_laneq_f32(Result.NEON, Right.Columns[3].NEON, Left.NEON, 3);
#else
    Result.x = Left._[0] * Right.Columns[0].x;
    Result.y = Left._[0] * Right.Columns[0].y;
    Result.z = Left._[0] * Right.Columns[0].z;
    Result.w = Left._[0] * Right.Columns[0].w;

    Result.x += Left._[1] * Right.Columns[1].x;
    Result.y += Left._[1] * Right.Columns[1].y;
    Result.z += Left._[1] * Right.Columns[1].z;
    Result.w += Left._[1] * Right.Columns[1].w;

    Result.x += Left._[2] * Right.Columns[2].x;
    Result.y += Left._[2] * Right.Columns[2].y;
    Result.z += Left._[2] * Right.Columns[2].z;
    Result.w += Left._[2] * Right.Columns[2].w;

    Result.x += Left._[3] * Right.Columns[3].x;
    Result.y += Left._[3] * Right.Columns[3].y;
    Result.z += Left._[3] * Right.Columns[3].z;
    Result.w += Left._[3] * Right.Columns[3].w;
#endif

    return Result;
}

/*
 * 2x2 Matrices
 */

COVERAGE(M_M2, 1)
static inline mat2 M_M2(void)
{
    ASSERT_COVERED(M_M2);
    mat2 Result = {0};
    return Result;
}

COVERAGE(M_M2D, 1)
static inline mat2 M_M2D(float Diagonal)
{
    ASSERT_COVERED(M_M2D);

    mat2 Result = {0};
    Result._[0][0] = Diagonal;
    Result._[1][1] = Diagonal;

    return Result;
}

COVERAGE(M_TransposeM2, 1)
static inline mat2 M_TransposeM2(mat2 Matrix)
{
    ASSERT_COVERED(M_TransposeM2);

    mat2 Result = Matrix;

    Result._[0][1] = Matrix._[1][0];
    Result._[1][0] = Matrix._[0][1];

    return Result;
}

COVERAGE(M_AddM2, 1)
static inline mat2 M_AddM2(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_AddM2);

    mat2 Result;

    Result._[0][0] = Left._[0][0] + Right._[0][0];
    Result._[0][1] = Left._[0][1] + Right._[0][1];
    Result._[1][0] = Left._[1][0] + Right._[1][0];
    Result._[1][1] = Left._[1][1] + Right._[1][1];

    return Result;
}

COVERAGE(M_SubM2, 1)
static inline mat2 M_SubM2(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_SubM2);

    mat2 Result;

    Result._[0][0] = Left._[0][0] - Right._[0][0];
    Result._[0][1] = Left._[0][1] - Right._[0][1];
    Result._[1][0] = Left._[1][0] - Right._[1][0];
    Result._[1][1] = Left._[1][1] - Right._[1][1];

    return Result;
}

COVERAGE(M_MulM2V2, 1)
static inline vec2 M_MulM2V2(mat2 Matrix, vec2 Vector)
{
    ASSERT_COVERED(M_MulM2V2);

    vec2 Result;

    Result.x = Vector._[0] * Matrix.Columns[0].x;
    Result.y = Vector._[0] * Matrix.Columns[0].y;

    Result.x += Vector._[1] * Matrix.Columns[1].x;
    Result.y += Vector._[1] * Matrix.Columns[1].y;

    return Result;
}

COVERAGE(M_MulM2, 1)
static inline mat2 M_MulM2(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_MulM2);

    mat2 Result;
    Result.Columns[0] = M_MulM2V2(Left, Right.Columns[0]);
    Result.Columns[1] = M_MulM2V2(Left, Right.Columns[1]);

    return Result;
}

COVERAGE(M_MulM2F, 1)
static inline mat2 M_MulM2F(mat2 Matrix, float Scalar)
{
    ASSERT_COVERED(M_MulM2F);

    mat2 Result;

    Result._[0][0] = Matrix._[0][0] * Scalar;
    Result._[0][1] = Matrix._[0][1] * Scalar;
    Result._[1][0] = Matrix._[1][0] * Scalar;
    Result._[1][1] = Matrix._[1][1] * Scalar;

    return Result;
}

COVERAGE(M_DivM2F, 1)
static inline mat2 M_DivM2F(mat2 Matrix, float Scalar)
{
    ASSERT_COVERED(M_DivM2F);

    mat2 Result;

    Result._[0][0] = Matrix._[0][0] / Scalar;
    Result._[0][1] = Matrix._[0][1] / Scalar;
    Result._[1][0] = Matrix._[1][0] / Scalar;
    Result._[1][1] = Matrix._[1][1] / Scalar;

    return Result;
}

COVERAGE(M_DeterminantM2, 1)
static inline float M_DeterminantM2(mat2 Matrix)
{
    ASSERT_COVERED(M_DeterminantM2);
    return Matrix._[0][0]*Matrix._[1][1] - Matrix._[0][1]*Matrix._[1][0];
}


COVERAGE(M_InvGeneralM2, 1)
static inline mat2 M_InvGeneralM2(mat2 Matrix)
{
    ASSERT_COVERED(M_InvGeneralM2);

    mat2 Result;
    float InvDeterminant = 1.0f / M_DeterminantM2(Matrix);
    Result._[0][0] = InvDeterminant * +Matrix._[1][1];
    Result._[1][1] = InvDeterminant * +Matrix._[0][0];
    Result._[0][1] = InvDeterminant * -Matrix._[0][1];
    Result._[1][0] = InvDeterminant * -Matrix._[1][0];

    return Result;
}

/*
 * 3x3 Matrices
 */

COVERAGE(M_M3, 1)
static inline mat3 M_M3(void)
{
    ASSERT_COVERED(M_M3);
    mat3 Result = {0};
    return Result;
}

COVERAGE(M_M3D, 1)
static inline mat3 M_M3D(float Diagonal)
{
    ASSERT_COVERED(M_M3D);

    mat3 Result = {0};
    Result._[0][0] = Diagonal;
    Result._[1][1] = Diagonal;
    Result._[2][2] = Diagonal;

    return Result;
}

COVERAGE(M_TransposeM3, 1)
static inline mat3 M_TransposeM3(mat3 Matrix)
{
    ASSERT_COVERED(M_TransposeM3);

    mat3 Result = Matrix;

    Result._[0][1] = Matrix._[1][0];
    Result._[0][2] = Matrix._[2][0];
    Result._[1][0] = Matrix._[0][1];
    Result._[1][2] = Matrix._[2][1];
    Result._[2][1] = Matrix._[1][2];
    Result._[2][0] = Matrix._[0][2];

    return Result;
}

COVERAGE(M_AddM3, 1)
static inline mat3 M_AddM3(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_AddM3);

    mat3 Result;

    Result._[0][0] = Left._[0][0] + Right._[0][0];
    Result._[0][1] = Left._[0][1] + Right._[0][1];
    Result._[0][2] = Left._[0][2] + Right._[0][2];
    Result._[1][0] = Left._[1][0] + Right._[1][0];
    Result._[1][1] = Left._[1][1] + Right._[1][1];
    Result._[1][2] = Left._[1][2] + Right._[1][2];
    Result._[2][0] = Left._[2][0] + Right._[2][0];
    Result._[2][1] = Left._[2][1] + Right._[2][1];
    Result._[2][2] = Left._[2][2] + Right._[2][2];

    return Result;
}

COVERAGE(M_SubM3, 1)
static inline mat3 M_SubM3(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_SubM3);

    mat3 Result;

    Result._[0][0] = Left._[0][0] - Right._[0][0];
    Result._[0][1] = Left._[0][1] - Right._[0][1];
    Result._[0][2] = Left._[0][2] - Right._[0][2];
    Result._[1][0] = Left._[1][0] - Right._[1][0];
    Result._[1][1] = Left._[1][1] - Right._[1][1];
    Result._[1][2] = Left._[1][2] - Right._[1][2];
    Result._[2][0] = Left._[2][0] - Right._[2][0];
    Result._[2][1] = Left._[2][1] - Right._[2][1];
    Result._[2][2] = Left._[2][2] - Right._[2][2];

    return Result;
}

COVERAGE(M_MulM3V3, 1)
static inline vec3 M_MulM3V3(mat3 Matrix, vec3 Vector)
{
    ASSERT_COVERED(M_MulM3V3);

    vec3 Result;

    Result.x = Vector._[0] * Matrix.Columns[0].x;
    Result.y = Vector._[0] * Matrix.Columns[0].y;
    Result.z = Vector._[0] * Matrix.Columns[0].z;

    Result.x += Vector._[1] * Matrix.Columns[1].x;
    Result.y += Vector._[1] * Matrix.Columns[1].y;
    Result.z += Vector._[1] * Matrix.Columns[1].z;

    Result.x += Vector._[2] * Matrix.Columns[2].x;
    Result.y += Vector._[2] * Matrix.Columns[2].y;
    Result.z += Vector._[2] * Matrix.Columns[2].z;

    return Result;
}

COVERAGE(M_MulM3, 1)
static inline mat3 M_MulM3(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_MulM3);

    mat3 Result;
    Result.Columns[0] = M_MulM3V3(Left, Right.Columns[0]);
    Result.Columns[1] = M_MulM3V3(Left, Right.Columns[1]);
    Result.Columns[2] = M_MulM3V3(Left, Right.Columns[2]);

    return Result;
}

COVERAGE(M_MulM3F, 1)
static inline mat3 M_MulM3F(mat3 Matrix, float Scalar)
{
    ASSERT_COVERED(M_MulM3F);

    mat3 Result;

    Result._[0][0] = Matrix._[0][0] * Scalar;
    Result._[0][1] = Matrix._[0][1] * Scalar;
    Result._[0][2] = Matrix._[0][2] * Scalar;
    Result._[1][0] = Matrix._[1][0] * Scalar;
    Result._[1][1] = Matrix._[1][1] * Scalar;
    Result._[1][2] = Matrix._[1][2] * Scalar;
    Result._[2][0] = Matrix._[2][0] * Scalar;
    Result._[2][1] = Matrix._[2][1] * Scalar;
    Result._[2][2] = Matrix._[2][2] * Scalar;

    return Result;
}

COVERAGE(M_DivM3, 1)
static inline mat3 M_DivM3F(mat3 Matrix, float Scalar)
{
    ASSERT_COVERED(M_DivM3);

    mat3 Result;

    Result._[0][0] = Matrix._[0][0] / Scalar;
    Result._[0][1] = Matrix._[0][1] / Scalar;
    Result._[0][2] = Matrix._[0][2] / Scalar;
    Result._[1][0] = Matrix._[1][0] / Scalar;
    Result._[1][1] = Matrix._[1][1] / Scalar;
    Result._[1][2] = Matrix._[1][2] / Scalar;
    Result._[2][0] = Matrix._[2][0] / Scalar;
    Result._[2][1] = Matrix._[2][1] / Scalar;
    Result._[2][2] = Matrix._[2][2] / Scalar;

    return Result;
}

COVERAGE(M_DeterminantM3, 1)
static inline float M_DeterminantM3(mat3 Matrix)
{
    ASSERT_COVERED(M_DeterminantM3);

    mat3 Cross;
    Cross.Columns[0] = M_Cross(Matrix.Columns[1], Matrix.Columns[2]);
    Cross.Columns[1] = M_Cross(Matrix.Columns[2], Matrix.Columns[0]);
    Cross.Columns[2] = M_Cross(Matrix.Columns[0], Matrix.Columns[1]);

    return M_DotV3(Cross.Columns[2], Matrix.Columns[2]);
}

COVERAGE(M_InvGeneralM3, 1)
static inline mat3 M_InvGeneralM3(mat3 Matrix)
{
    ASSERT_COVERED(M_InvGeneralM3);

    mat3 Cross;
    Cross.Columns[0] = M_Cross(Matrix.Columns[1], Matrix.Columns[2]);
    Cross.Columns[1] = M_Cross(Matrix.Columns[2], Matrix.Columns[0]);
    Cross.Columns[2] = M_Cross(Matrix.Columns[0], Matrix.Columns[1]);

    float InvDeterminant = 1.0f / M_DotV3(Cross.Columns[2], Matrix.Columns[2]);

    mat3 Result;
    Result.Columns[0] = M_MulV3F(Cross.Columns[0], InvDeterminant);
    Result.Columns[1] = M_MulV3F(Cross.Columns[1], InvDeterminant);
    Result.Columns[2] = M_MulV3F(Cross.Columns[2], InvDeterminant);

    return M_TransposeM3(Result);
}

/*
 * 4x4 Matrices
 */

COVERAGE(M_M4, 1)
static inline mat4 M_M4(void)
{
    ASSERT_COVERED(M_M4);
    mat4 Result = {0};
    return Result;
}

COVERAGE(M_M4D, 1)
static inline mat4 M_M4D(float Diagonal)
{
    ASSERT_COVERED(M_M4D);

    mat4 Result = {0};
    Result._[0][0] = Diagonal;
    Result._[1][1] = Diagonal;
    Result._[2][2] = Diagonal;
    Result._[3][3] = Diagonal;

    return Result;
}

COVERAGE(M_TransposeM4, 1)
static inline mat4 M_TransposeM4(mat4 Matrix)
{
    ASSERT_COVERED(M_TransposeM4);

    mat4 Result;
#ifdef HANDMADE_MATH__USE_SSE
    Result = Matrix;
    _MM_TRANSPOSE4_PS(Result.Columns[0].SSE, Result.Columns[1].SSE, Result.Columns[2].SSE, Result.Columns[3].SSE);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4x4_t Transposed = vld4q_f32((float*)Matrix.Columns);
    Result.Columns[0].NEON = Transposed.val[0];
    Result.Columns[1].NEON = Transposed.val[1];
    Result.Columns[2].NEON = Transposed.val[2];
    Result.Columns[3].NEON = Transposed.val[3];
#else
    Result._[0][0] = Matrix._[0][0];
    Result._[0][1] = Matrix._[1][0];
    Result._[0][2] = Matrix._[2][0];
    Result._[0][3] = Matrix._[3][0];
    Result._[1][0] = Matrix._[0][1];
    Result._[1][1] = Matrix._[1][1];
    Result._[1][2] = Matrix._[2][1];
    Result._[1][3] = Matrix._[3][1];
    Result._[2][0] = Matrix._[0][2];
    Result._[2][1] = Matrix._[1][2];
    Result._[2][2] = Matrix._[2][2];
    Result._[2][3] = Matrix._[3][2];
    Result._[3][0] = Matrix._[0][3];
    Result._[3][1] = Matrix._[1][3];
    Result._[3][2] = Matrix._[2][3];
    Result._[3][3] = Matrix._[3][3];
#endif

    return Result;
}

COVERAGE(M_AddM4, 1)
static inline mat4 M_AddM4(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_AddM4);

    mat4 Result;

    Result.Columns[0] = M_AddV4(Left.Columns[0], Right.Columns[0]);
    Result.Columns[1] = M_AddV4(Left.Columns[1], Right.Columns[1]);
    Result.Columns[2] = M_AddV4(Left.Columns[2], Right.Columns[2]);
    Result.Columns[3] = M_AddV4(Left.Columns[3], Right.Columns[3]);

    return Result;
}

COVERAGE(M_SubM4, 1)
static inline mat4 M_SubM4(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_SubM4);

    mat4 Result;

    Result.Columns[0] = M_SubV4(Left.Columns[0], Right.Columns[0]);
    Result.Columns[1] = M_SubV4(Left.Columns[1], Right.Columns[1]);
    Result.Columns[2] = M_SubV4(Left.Columns[2], Right.Columns[2]);
    Result.Columns[3] = M_SubV4(Left.Columns[3], Right.Columns[3]);

    return Result;
}

COVERAGE(M_MulM4, 1)
static inline mat4 M_MulM4(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_MulM4);

    mat4 Result;
    Result.Columns[0] = M_LinearCombineV4M4(Right.Columns[0], Left);
    Result.Columns[1] = M_LinearCombineV4M4(Right.Columns[1], Left);
    Result.Columns[2] = M_LinearCombineV4M4(Right.Columns[2], Left);
    Result.Columns[3] = M_LinearCombineV4M4(Right.Columns[3], Left);

    return Result;
}

COVERAGE(M_MulM4F, 1)
static inline mat4 M_MulM4F(mat4 Matrix, float Scalar)
{
    ASSERT_COVERED(M_MulM4F);

    mat4 Result;


#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEScalar = _mm_set1_ps(Scalar);
    Result.Columns[0].SSE = _mm_mul_ps(Matrix.Columns[0].SSE, SSEScalar);
    Result.Columns[1].SSE = _mm_mul_ps(Matrix.Columns[1].SSE, SSEScalar);
    Result.Columns[2].SSE = _mm_mul_ps(Matrix.Columns[2].SSE, SSEScalar);
    Result.Columns[3].SSE = _mm_mul_ps(Matrix.Columns[3].SSE, SSEScalar);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.Columns[0].NEON = vmulq_n_f32(Matrix.Columns[0].NEON, Scalar);
    Result.Columns[1].NEON = vmulq_n_f32(Matrix.Columns[1].NEON, Scalar);
    Result.Columns[2].NEON = vmulq_n_f32(Matrix.Columns[2].NEON, Scalar);
    Result.Columns[3].NEON = vmulq_n_f32(Matrix.Columns[3].NEON, Scalar);
#else
    Result._[0][0] = Matrix._[0][0] * Scalar;
    Result._[0][1] = Matrix._[0][1] * Scalar;
    Result._[0][2] = Matrix._[0][2] * Scalar;
    Result._[0][3] = Matrix._[0][3] * Scalar;
    Result._[1][0] = Matrix._[1][0] * Scalar;
    Result._[1][1] = Matrix._[1][1] * Scalar;
    Result._[1][2] = Matrix._[1][2] * Scalar;
    Result._[1][3] = Matrix._[1][3] * Scalar;
    Result._[2][0] = Matrix._[2][0] * Scalar;
    Result._[2][1] = Matrix._[2][1] * Scalar;
    Result._[2][2] = Matrix._[2][2] * Scalar;
    Result._[2][3] = Matrix._[2][3] * Scalar;
    Result._[3][0] = Matrix._[3][0] * Scalar;
    Result._[3][1] = Matrix._[3][1] * Scalar;
    Result._[3][2] = Matrix._[3][2] * Scalar;
    Result._[3][3] = Matrix._[3][3] * Scalar;
#endif

    return Result;
}

COVERAGE(M_MulM4V4, 1)
static inline vec4 M_MulM4V4(mat4 Matrix, vec4 Vector)
{
    ASSERT_COVERED(M_MulM4V4);
    return M_LinearCombineV4M4(Vector, Matrix);
}

COVERAGE(M_DivM4F, 1)
static inline mat4 M_DivM4F(mat4 Matrix, float Scalar)
{
    ASSERT_COVERED(M_DivM4F);

    mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEScalar = _mm_set1_ps(Scalar);
    Result.Columns[0].SSE = _mm_div_ps(Matrix.Columns[0].SSE, SSEScalar);
    Result.Columns[1].SSE = _mm_div_ps(Matrix.Columns[1].SSE, SSEScalar);
    Result.Columns[2].SSE = _mm_div_ps(Matrix.Columns[2].SSE, SSEScalar);
    Result.Columns[3].SSE = _mm_div_ps(Matrix.Columns[3].SSE, SSEScalar);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t NEONScalar = vdupq_n_f32(Scalar);
    Result.Columns[0].NEON = vdivq_f32(Matrix.Columns[0].NEON, NEONScalar);
    Result.Columns[1].NEON = vdivq_f32(Matrix.Columns[1].NEON, NEONScalar);
    Result.Columns[2].NEON = vdivq_f32(Matrix.Columns[2].NEON, NEONScalar);
    Result.Columns[3].NEON = vdivq_f32(Matrix.Columns[3].NEON, NEONScalar);
#else
    Result._[0][0] = Matrix._[0][0] / Scalar;
    Result._[0][1] = Matrix._[0][1] / Scalar;
    Result._[0][2] = Matrix._[0][2] / Scalar;
    Result._[0][3] = Matrix._[0][3] / Scalar;
    Result._[1][0] = Matrix._[1][0] / Scalar;
    Result._[1][1] = Matrix._[1][1] / Scalar;
    Result._[1][2] = Matrix._[1][2] / Scalar;
    Result._[1][3] = Matrix._[1][3] / Scalar;
    Result._[2][0] = Matrix._[2][0] / Scalar;
    Result._[2][1] = Matrix._[2][1] / Scalar;
    Result._[2][2] = Matrix._[2][2] / Scalar;
    Result._[2][3] = Matrix._[2][3] / Scalar;
    Result._[3][0] = Matrix._[3][0] / Scalar;
    Result._[3][1] = Matrix._[3][1] / Scalar;
    Result._[3][2] = Matrix._[3][2] / Scalar;
    Result._[3][3] = Matrix._[3][3] / Scalar;
#endif

    return Result;
}

COVERAGE(M_DeterminantM4, 1)
static inline float M_DeterminantM4(mat4 Matrix)
{
    ASSERT_COVERED(M_DeterminantM4);

    vec3 C01 = M_Cross(Matrix.Columns[0].xyz, Matrix.Columns[1].xyz);
    vec3 C23 = M_Cross(Matrix.Columns[2].xyz, Matrix.Columns[3].xyz);
    vec3 B10 = M_SubV3(M_MulV3F(Matrix.Columns[0].xyz, Matrix.Columns[1].w), M_MulV3F(Matrix.Columns[1].xyz, Matrix.Columns[0].w));
    vec3 B32 = M_SubV3(M_MulV3F(Matrix.Columns[2].xyz, Matrix.Columns[3].w), M_MulV3F(Matrix.Columns[3].xyz, Matrix.Columns[2].w));

    return M_DotV3(C01, B32) + M_DotV3(C23, B10);
}

COVERAGE(M_InvGeneralM4, 1)
// Returns a general-purpose inverse of an mat4. Note that special-purpose inverses of many transformations
// are available and will be more efficient.
static inline mat4 M_InvGeneralM4(mat4 Matrix)
{
    ASSERT_COVERED(M_InvGeneralM4);

    vec3 C01 = M_Cross(Matrix.Columns[0].xyz, Matrix.Columns[1].xyz);
    vec3 C23 = M_Cross(Matrix.Columns[2].xyz, Matrix.Columns[3].xyz);
    vec3 B10 = M_SubV3(M_MulV3F(Matrix.Columns[0].xyz, Matrix.Columns[1].w), M_MulV3F(Matrix.Columns[1].xyz, Matrix.Columns[0].w));
    vec3 B32 = M_SubV3(M_MulV3F(Matrix.Columns[2].xyz, Matrix.Columns[3].w), M_MulV3F(Matrix.Columns[3].xyz, Matrix.Columns[2].w));

    float InvDeterminant = 1.0f / (M_DotV3(C01, B32) + M_DotV3(C23, B10));
    C01 = M_MulV3F(C01, InvDeterminant);
    C23 = M_MulV3F(C23, InvDeterminant);
    B10 = M_MulV3F(B10, InvDeterminant);
    B32 = M_MulV3F(B32, InvDeterminant);

    mat4 Result;
    Result.Columns[0] = M_V4V(M_AddV3(M_Cross(Matrix.Columns[1].xyz, B32), M_MulV3F(C23, Matrix.Columns[1].w)), -M_DotV3(Matrix.Columns[1].xyz, C23));
    Result.Columns[1] = M_V4V(M_SubV3(M_Cross(B32, Matrix.Columns[0].xyz), M_MulV3F(C23, Matrix.Columns[0].w)), +M_DotV3(Matrix.Columns[0].xyz, C23));
    Result.Columns[2] = M_V4V(M_AddV3(M_Cross(Matrix.Columns[3].xyz, B10), M_MulV3F(C01, Matrix.Columns[3].w)), -M_DotV3(Matrix.Columns[3].xyz, C01));
    Result.Columns[3] = M_V4V(M_SubV3(M_Cross(B10, Matrix.Columns[2].xyz), M_MulV3F(C01, Matrix.Columns[2].w)), +M_DotV3(Matrix.Columns[2].xyz, C01));

    return M_TransposeM4(Result);
}

/*
 * Common graphics transformations
 */

COVERAGE(M_Orthographic_RH_NO, 1)
// Produces a right-handed orthographic projection matrix with z ranging from -1 to 1 (the GL convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
static inline mat4 M_Orthographic_RH_NO(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    ASSERT_COVERED(M_Orthographic_RH_NO);

    mat4 Result = {0};

    Result._[0][0] = 2.0f / (Right - Left);
    Result._[1][1] = 2.0f / (Top - Bottom);
    Result._[2][2] = 2.0f / (Near - Far);
    Result._[3][3] = 1.0f;

    Result._[3][0] = (Left + Right) / (Left - Right);
    Result._[3][1] = (Bottom + Top) / (Bottom - Top);
    Result._[3][2] = (Near + Far) / (Near - Far);

    return Result;
}

COVERAGE(M_Orthographic_RH_ZO, 1)
// Produces a right-handed orthographic projection matrix with z ranging from 0 to 1 (the DirectX convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
static inline mat4 M_Orthographic_RH_ZO(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    ASSERT_COVERED(M_Orthographic_RH_ZO);

    mat4 Result = {0};

    Result._[0][0] = 2.0f / (Right - Left);
    Result._[1][1] = 2.0f / (Top - Bottom);
    Result._[2][2] = 1.0f / (Near - Far);
    Result._[3][3] = 1.0f;

    Result._[3][0] = (Left + Right) / (Left - Right);
    Result._[3][1] = (Bottom + Top) / (Bottom - Top);
    Result._[3][2] = (Near) / (Near - Far);

    return Result;
}

COVERAGE(M_Orthographic_LH_NO, 1)
// Produces a left-handed orthographic projection matrix with z ranging from -1 to 1 (the GL convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
static inline mat4 M_Orthographic_LH_NO(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    ASSERT_COVERED(M_Orthographic_LH_NO);

    mat4 Result = M_Orthographic_RH_NO(Left, Right, Bottom, Top, Near, Far);
    Result._[2][2] = -Result._[2][2];

    return Result;
}

COVERAGE(M_Orthographic_LH_ZO, 1)
// Produces a left-handed orthographic projection matrix with z ranging from 0 to 1 (the DirectX convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
static inline mat4 M_Orthographic_LH_ZO(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    ASSERT_COVERED(M_Orthographic_LH_ZO);

    mat4 Result = M_Orthographic_RH_ZO(Left, Right, Bottom, Top, Near, Far);
    Result._[2][2] = -Result._[2][2];

    return Result;
}

COVERAGE(M_InvOrthographic, 1)
// Returns an inverse for the given orthographic projection matrix. Works for all orthographic
// projection matrices, regardless of handedness or NDC convention.
static inline mat4 M_InvOrthographic(mat4 OrthoMatrix)
{
    ASSERT_COVERED(M_InvOrthographic);

    mat4 Result = {0};
    Result._[0][0] = 1.0f / OrthoMatrix._[0][0];
    Result._[1][1] = 1.0f / OrthoMatrix._[1][1];
    Result._[2][2] = 1.0f / OrthoMatrix._[2][2];
    Result._[3][3] = 1.0f;

    Result._[3][0] = -OrthoMatrix._[3][0] * Result._[0][0];
    Result._[3][1] = -OrthoMatrix._[3][1] * Result._[1][1];
    Result._[3][2] = -OrthoMatrix._[3][2] * Result._[2][2];

    return Result;
}

COVERAGE(M_Perspective_RH_NO, 1)
static inline mat4 M_Perspective_RH_NO(float FOV, float AspectRatio, float Near, float Far)
{
    ASSERT_COVERED(M_Perspective_RH_NO);

    mat4 Result = {0};

    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

    float Cotangent = 1.0f / M_TanF(FOV / 2.0f);
    Result._[0][0] = Cotangent / AspectRatio;
    Result._[1][1] = Cotangent;
    Result._[2][3] = -1.0f;

    Result._[2][2] = (Near + Far) / (Near - Far);
    Result._[3][2] = (2.0f * Near * Far) / (Near - Far);

    return Result;
}

COVERAGE(M_Perspective_RH_ZO, 1)
static inline mat4 M_Perspective_RH_ZO(float FOV, float AspectRatio, float Near, float Far)
{
    ASSERT_COVERED(M_Perspective_RH_ZO);

    mat4 Result = {0};

    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

    float Cotangent = 1.0f / M_TanF(FOV / 2.0f);
    Result._[0][0] = Cotangent / AspectRatio;
    Result._[1][1] = Cotangent;
    Result._[2][3] = -1.0f;

    Result._[2][2] = (Far) / (Near - Far);
    Result._[3][2] = (Near * Far) / (Near - Far);

    return Result;
}

COVERAGE(M_Perspective_LH_NO, 1)
static inline mat4 M_Perspective_LH_NO(float FOV, float AspectRatio, float Near, float Far)
{
    ASSERT_COVERED(M_Perspective_LH_NO);

    mat4 Result = M_Perspective_RH_NO(FOV, AspectRatio, Near, Far);
    Result._[2][2] = -Result._[2][2];
    Result._[2][3] = -Result._[2][3];

    return Result;
}

COVERAGE(M_Perspective_LH_ZO, 1)
static inline mat4 M_Perspective_LH_ZO(float FOV, float AspectRatio, float Near, float Far)
{
    ASSERT_COVERED(M_Perspective_LH_ZO);

    mat4 Result = M_Perspective_RH_ZO(FOV, AspectRatio, Near, Far);
    Result._[2][2] = -Result._[2][2];
    Result._[2][3] = -Result._[2][3];

    return Result;
}

COVERAGE(M_InvPerspective_RH, 1)
static inline mat4 M_InvPerspective_RH(mat4 PerspectiveMatrix)
{
    ASSERT_COVERED(M_InvPerspective_RH);

    mat4 Result = {0};
    Result._[0][0] = 1.0f / PerspectiveMatrix._[0][0];
    Result._[1][1] = 1.0f / PerspectiveMatrix._[1][1];
    Result._[2][2] = 0.0f;

    Result._[2][3] = 1.0f / PerspectiveMatrix._[3][2];
    Result._[3][3] = PerspectiveMatrix._[2][2] * Result._[2][3];
    Result._[3][2] = PerspectiveMatrix._[2][3];

    return Result;
}

COVERAGE(M_InvPerspective_LH, 1)
static inline mat4 M_InvPerspective_LH(mat4 PerspectiveMatrix)
{
    ASSERT_COVERED(M_InvPerspective_LH);

    mat4 Result = {0};
    Result._[0][0] = 1.0f / PerspectiveMatrix._[0][0];
    Result._[1][1] = 1.0f / PerspectiveMatrix._[1][1];
    Result._[2][2] = 0.0f;

    Result._[2][3] = 1.0f / PerspectiveMatrix._[3][2];
    Result._[3][3] = PerspectiveMatrix._[2][2] * -Result._[2][3];
    Result._[3][2] = PerspectiveMatrix._[2][3];

    return Result;
}

COVERAGE(M_Translate, 1)
static inline mat4 M_Translate(vec3 Translation)
{
    ASSERT_COVERED(M_Translate);

    mat4 Result = M_M4D(1.0f);
    Result._[3][0] = Translation.x;
    Result._[3][1] = Translation.y;
    Result._[3][2] = Translation.z;

    return Result;
}

COVERAGE(M_InvTranslate, 1)
static inline mat4 M_InvTranslate(mat4 TranslationMatrix)
{
    ASSERT_COVERED(M_InvTranslate);

    mat4 Result = TranslationMatrix;
    Result._[3][0] = -Result._[3][0];
    Result._[3][1] = -Result._[3][1];
    Result._[3][2] = -Result._[3][2];

    return Result;
}

COVERAGE(M_Rotate_RH, 1)
static inline mat4 M_Rotate_RH(float Angle, vec3 Axis)
{
    ASSERT_COVERED(M_Rotate_RH);

    mat4 Result = M_M4D(1.0f);

    Axis = M_NormV3(Axis);

    float SinTheta = M_SinF(Angle);
    float CosTheta = M_CosF(Angle);
    float CosValue = 1.0f - CosTheta;

    Result._[0][0] = (Axis.x * Axis.x * CosValue) + CosTheta;
    Result._[0][1] = (Axis.x * Axis.y * CosValue) + (Axis.z * SinTheta);
    Result._[0][2] = (Axis.x * Axis.z * CosValue) - (Axis.y * SinTheta);

    Result._[1][0] = (Axis.y * Axis.x * CosValue) - (Axis.z * SinTheta);
    Result._[1][1] = (Axis.y * Axis.y * CosValue) + CosTheta;
    Result._[1][2] = (Axis.y * Axis.z * CosValue) + (Axis.x * SinTheta);

    Result._[2][0] = (Axis.z * Axis.x * CosValue) + (Axis.y * SinTheta);
    Result._[2][1] = (Axis.z * Axis.y * CosValue) - (Axis.x * SinTheta);
    Result._[2][2] = (Axis.z * Axis.z * CosValue) + CosTheta;

    return Result;
}

COVERAGE(M_Rotate_LH, 1)
static inline mat4 M_Rotate_LH(float Angle, vec3 Axis)
{
    ASSERT_COVERED(M_Rotate_LH);
    /* NOTE(lcf): Matrix will be inverse/transpose of RH. */
    return M_Rotate_RH(-Angle, Axis);
}

COVERAGE(M_InvRotate, 1)
static inline mat4 M_InvRotate(mat4 RotationMatrix)
{
    ASSERT_COVERED(M_InvRotate);
    return M_TransposeM4(RotationMatrix);
}

COVERAGE(M_Scale, 1)
static inline mat4 M_Scale(vec3 Scale)
{
    ASSERT_COVERED(M_Scale);

    mat4 Result = M_M4D(1.0f);
    Result._[0][0] = Scale.x;
    Result._[1][1] = Scale.y;
    Result._[2][2] = Scale.z;

    return Result;
}

COVERAGE(M_InvScale, 1)
static inline mat4 M_InvScale(mat4 ScaleMatrix)
{
    ASSERT_COVERED(M_InvScale);

    mat4 Result = ScaleMatrix;
    Result._[0][0] = 1.0f / Result._[0][0];
    Result._[1][1] = 1.0f / Result._[1][1];
    Result._[2][2] = 1.0f / Result._[2][2];

    return Result;
}

static inline mat4 _M_LookAt(vec3 F,  vec3 S, vec3 U,  vec3 Eye)
{
    mat4 Result;

    Result._[0][0] = S.x;
    Result._[0][1] = U.x;
    Result._[0][2] = -F.x;
    Result._[0][3] = 0.0f;

    Result._[1][0] = S.y;
    Result._[1][1] = U.y;
    Result._[1][2] = -F.y;
    Result._[1][3] = 0.0f;

    Result._[2][0] = S.z;
    Result._[2][1] = U.z;
    Result._[2][2] = -F.z;
    Result._[2][3] = 0.0f;

    Result._[3][0] = -M_DotV3(S, Eye);
    Result._[3][1] = -M_DotV3(U, Eye);
    Result._[3][2] = M_DotV3(F, Eye);
    Result._[3][3] = 1.0f;

    return Result;
}

COVERAGE(M_LookAt_RH, 1)
static inline mat4 M_LookAt_RH(vec3 Eye, vec3 Center, vec3 Up)
{
    ASSERT_COVERED(M_LookAt_RH);

    vec3 F = M_NormV3(M_SubV3(Center, Eye));
    vec3 S = M_NormV3(M_Cross(F, Up));
    vec3 U = M_Cross(S, F);

    return _M_LookAt(F, S, U, Eye);
}

COVERAGE(M_LookAt_LH, 1)
static inline mat4 M_LookAt_LH(vec3 Eye, vec3 Center, vec3 Up)
{
    ASSERT_COVERED(M_LookAt_LH);

    vec3 F = M_NormV3(M_SubV3(Eye, Center));
    vec3 S = M_NormV3(M_Cross(F, Up));
    vec3 U = M_Cross(S, F);

    return _M_LookAt(F, S, U, Eye);
}

COVERAGE(M_InvLookAt, 1)
static inline mat4 M_InvLookAt(mat4 Matrix)
{
    ASSERT_COVERED(M_InvLookAt);
    mat4 Result;

    mat3 Rotation = {0};
    Rotation.Columns[0] = Matrix.Columns[0].xyz;
    Rotation.Columns[1] = Matrix.Columns[1].xyz;
    Rotation.Columns[2] = Matrix.Columns[2].xyz;
    Rotation = M_TransposeM3(Rotation);

    Result.Columns[0] = M_V4V(Rotation.Columns[0], 0.0f);
    Result.Columns[1] = M_V4V(Rotation.Columns[1], 0.0f);
    Result.Columns[2] = M_V4V(Rotation.Columns[2], 0.0f);
    Result.Columns[3] = M_MulV4F(Matrix.Columns[3], -1.0f);
    Result._[3][0] = -1.0f * Matrix._[3][0] /
        (Rotation._[0][0] + Rotation._[0][1] + Rotation._[0][2]);
    Result._[3][1] = -1.0f * Matrix._[3][1] /
        (Rotation._[1][0] + Rotation._[1][1] + Rotation._[1][2]);
    Result._[3][2] = -1.0f * Matrix._[3][2] /
        (Rotation._[2][0] + Rotation._[2][1] + Rotation._[2][2]);
    Result._[3][3] = 1.0f;

    return Result;
}

/*
 * Quaternion operations
 */

COVERAGE(M_Q, 1)
static inline quat M_Q(float x, float y, float z, float w)
{
    ASSERT_COVERED(M_Q);

    quat Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_setr_ps(x, y, z, w);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t v = { x, y, z, w };
    Result.NEON = v;
#else
    Result.x = x;
    Result.y = y;
    Result.z = z;
    Result.w = w;
#endif

    return Result;
}

COVERAGE(M_QV4, 1)
static inline quat M_QV4(vec4 Vector)
{
    ASSERT_COVERED(M_QV4);

    quat Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = Vector.SSE;
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = Vector.NEON;
#else
    Result.x = Vector.x;
    Result.y = Vector.y;
    Result.z = Vector.z;
    Result.w = Vector.w;
#endif

    return Result;
}

COVERAGE(M_AddQ, 1)
static inline quat M_AddQ(quat Left, quat Right)
{
    ASSERT_COVERED(M_AddQ);

    quat Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_add_ps(Left.SSE, Right.SSE);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vaddq_f32(Left.NEON, Right.NEON);
#else

    Result.x = Left.x + Right.x;
    Result.y = Left.y + Right.y;
    Result.z = Left.z + Right.z;
    Result.w = Left.w + Right.w;
#endif

    return Result;
}

COVERAGE(M_SubQ, 1)
static inline quat M_SubQ(quat Left, quat Right)
{
    ASSERT_COVERED(M_SubQ);

    quat Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_sub_ps(Left.SSE, Right.SSE);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vsubq_f32(Left.NEON, Right.NEON);
#else
    Result.x = Left.x - Right.x;
    Result.y = Left.y - Right.y;
    Result.z = Left.z - Right.z;
    Result.w = Left.w - Right.w;
#endif

    return Result;
}

COVERAGE(M_MulQ, 1)
static inline quat M_MulQ(quat Left, quat Right)
{
    ASSERT_COVERED(M_MulQ);

    quat Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.f, -0.f, 0.f, -0.f));
    __m128 SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(0, 1, 2, 3));
    __m128 SSEResultThree = _mm_mul_ps(SSEResultTwo, SSEResultOne);

    SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(1, 1, 1, 1)) , _mm_setr_ps(0.f, 0.f, -0.f, -0.f));
    SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(1, 0, 3, 2));
    SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.f, 0.f, 0.f, -0.f));
    SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    SSEResultOne = _mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(3, 3, 3, 3));
    SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(3, 2, 1, 0));
    Result.SSE = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t Right1032 = vrev64q_f32(Right.NEON);
    float32x4_t Right3210 = vcombine_f32(vget_high_f32(Right1032), vget_low_f32(Right1032));
    float32x4_t Right2301 = vrev64q_f32(Right3210);

    float32x4_t FirstSign = {1.0f, -1.0f, 1.0f, -1.0f};
    Result.NEON = vmulq_f32(Right3210, vmulq_f32(vdupq_laneq_f32(Left.NEON, 0), FirstSign));
    float32x4_t SecondSign = {1.0f, 1.0f, -1.0f, -1.0f};
    Result.NEON = vfmaq_f32(Result.NEON, Right2301, vmulq_f32(vdupq_laneq_f32(Left.NEON, 1), SecondSign));
    float32x4_t ThirdSign = {-1.0f, 1.0f, 1.0f, -1.0f};
    Result.NEON = vfmaq_f32(Result.NEON, Right1032, vmulq_f32(vdupq_laneq_f32(Left.NEON, 2), ThirdSign));
    Result.NEON = vfmaq_laneq_f32(Result.NEON, Right.NEON, Left.NEON, 3);

#else
    Result.x =  Right._[3] * +Left._[0];
    Result.y =  Right._[2] * -Left._[0];
    Result.z =  Right._[1] * +Left._[0];
    Result.w =  Right._[0] * -Left._[0];

    Result.x += Right._[2] * +Left._[1];
    Result.y += Right._[3] * +Left._[1];
    Result.z += Right._[0] * -Left._[1];
    Result.w += Right._[1] * -Left._[1];

    Result.x += Right._[1] * -Left._[2];
    Result.y += Right._[0] * +Left._[2];
    Result.z += Right._[3] * +Left._[2];
    Result.w += Right._[2] * -Left._[2];

    Result.x += Right._[0] * +Left._[3];
    Result.y += Right._[1] * +Left._[3];
    Result.z += Right._[2] * +Left._[3];
    Result.w += Right._[3] * +Left._[3];
#endif

    return Result;
}

COVERAGE(M_MulQF, 1)
static inline quat M_MulQF(quat Left, float Multiplicative)
{
    ASSERT_COVERED(M_MulQF);

    quat Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Multiplicative);
    Result.SSE = _mm_mul_ps(Left.SSE, Scalar);
#elif defined(HANDMADE_MATH__USE_NEON)
    Result.NEON = vmulq_n_f32(Left.NEON, Multiplicative);
#else
    Result.x = Left.x * Multiplicative;
    Result.y = Left.y * Multiplicative;
    Result.z = Left.z * Multiplicative;
    Result.w = Left.w * Multiplicative;
#endif

    return Result;
}

COVERAGE(M_DivQF, 1)
static inline quat M_DivQF(quat Left, float Divnd)
{
    ASSERT_COVERED(M_DivQF);

    quat Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Divnd);
    Result.SSE = _mm_div_ps(Left.SSE, Scalar);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t Scalar = vdupq_n_f32(Divnd);
    Result.NEON = vdivq_f32(Left.NEON, Scalar);
#else
    Result.x = Left.x / Divnd;
    Result.y = Left.y / Divnd;
    Result.z = Left.z / Divnd;
    Result.w = Left.w / Divnd;
#endif

    return Result;
}

COVERAGE(M_DotQ, 1)
static inline float M_DotQ(quat Left, quat Right)
{
    ASSERT_COVERED(M_DotQ);

    float Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_mul_ps(Left.SSE, Right.SSE);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&Result, SSEResultOne);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t NEONMultiplyResult = vmulq_f32(Left.NEON, Right.NEON);
    float32x4_t NEONHalfAdd = vpaddq_f32(NEONMultiplyResult, NEONMultiplyResult);
    float32x4_t NEONFullAdd = vpaddq_f32(NEONHalfAdd, NEONHalfAdd);
    Result = vgetq_lane_f32(NEONFullAdd, 0);
#else
    Result = ((Left.x * Right.x) + (Left.z * Right.z)) + ((Left.y * Right.y) + (Left.w * Right.w));
#endif

    return Result;
}

COVERAGE(M_InvQ, 1)
static inline quat M_InvQ(quat Left)
{
    ASSERT_COVERED(M_InvQ);

    quat Result;
    Result.x = -Left.x;
    Result.y = -Left.y;
    Result.z = -Left.z;
    Result.w = Left.w;

    return M_DivQF(Result, (M_DotQ(Left, Left)));
}

COVERAGE(M_NormQ, 1)
static inline quat M_NormQ(quat Quat)
{
    ASSERT_COVERED(M_NormQ);

    /* NOTE(lcf): Take advantage of SSE implementation in M_NormV4 */
    vec4 Vec = {Quat.x, Quat.y, Quat.z, Quat.w};
    Vec = M_NormV4(Vec);
    quat Result = {Vec.x, Vec.y, Vec.z, Vec.w};

    return Result;
}

static inline quat _M_MixQ(quat Left, float MixLeft, quat Right, float MixRight) {
    quat Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 ScalarLeft = _mm_set1_ps(MixLeft);
    __m128 ScalarRight = _mm_set1_ps(MixRight);
    __m128 SSEResultOne = _mm_mul_ps(Left.SSE, ScalarLeft);
    __m128 SSEResultTwo = _mm_mul_ps(Right.SSE, ScalarRight);
    Result.SSE = _mm_add_ps(SSEResultOne, SSEResultTwo);
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t ScaledLeft = vmulq_n_f32(Left.NEON, MixLeft);
    float32x4_t ScaledRight = vmulq_n_f32(Right.NEON, MixRight);
    Result.NEON = vaddq_f32(ScaledLeft, ScaledRight);
#else
    Result.x = Left.x*MixLeft + Right.x*MixRight;
    Result.y = Left.y*MixLeft + Right.y*MixRight;
    Result.z = Left.z*MixLeft + Right.z*MixRight;
    Result.w = Left.w*MixLeft + Right.w*MixRight;
#endif

    return Result;
}

COVERAGE(M_NLerp, 1)
static inline quat M_NLerp(quat Left, float Time, quat Right)
{
    ASSERT_COVERED(M_NLerp);

    quat Result = _M_MixQ(Left, 1.0f-Time, Right, Time);
    Result = M_NormQ(Result);

    return Result;
}

COVERAGE(M_SLerp, 1)
static inline quat M_SLerp(quat Left, float Time, quat Right)
{
    ASSERT_COVERED(M_SLerp);

    quat Result;

    float Cos_Theta = M_DotQ(Left, Right);

    if (Cos_Theta < 0.0f) { /* NOTE(lcf): Take shortest path on Hyper-sphere */
        Cos_Theta = -Cos_Theta;
        Right = M_Q(-Right.x, -Right.y, -Right.z, -Right.w);
    }

    /* NOTE(lcf): Use Normalized Linear interpolation when vectors are roughly not L.I. */
    if (Cos_Theta > 0.9995f) {
        Result = M_NLerp(Left, Time, Right);
    } else {
        float Angle = M_ACosF(Cos_Theta);
        float MixLeft = M_SinF((1.0f - Time) * Angle);
        float MixRight = M_SinF(Time * Angle);

        Result = _M_MixQ(Left, MixLeft, Right, MixRight);
        Result = M_NormQ(Result);
    }

    return Result;
}

COVERAGE(M_QToM4, 1)
static inline mat4 M_QToM4(quat Left)
{
    ASSERT_COVERED(M_QToM4);

    mat4 Result;

    quat NormalizedQ = M_NormQ(Left);

    float XX, YY, ZZ,
          XY, XZ, YZ,
          WX, WY, WZ;

    XX = NormalizedQ.x * NormalizedQ.x;
    YY = NormalizedQ.y * NormalizedQ.y;
    ZZ = NormalizedQ.z * NormalizedQ.z;
    XY = NormalizedQ.x * NormalizedQ.y;
    XZ = NormalizedQ.x * NormalizedQ.z;
    YZ = NormalizedQ.y * NormalizedQ.z;
    WX = NormalizedQ.w * NormalizedQ.x;
    WY = NormalizedQ.w * NormalizedQ.y;
    WZ = NormalizedQ.w * NormalizedQ.z;

    Result._[0][0] = 1.0f - 2.0f * (YY + ZZ);
    Result._[0][1] = 2.0f * (XY + WZ);
    Result._[0][2] = 2.0f * (XZ - WY);
    Result._[0][3] = 0.0f;

    Result._[1][0] = 2.0f * (XY - WZ);
    Result._[1][1] = 1.0f - 2.0f * (XX + ZZ);
    Result._[1][2] = 2.0f * (YZ + WX);
    Result._[1][3] = 0.0f;

    Result._[2][0] = 2.0f * (XZ + WY);
    Result._[2][1] = 2.0f * (YZ - WX);
    Result._[2][2] = 1.0f - 2.0f * (XX + YY);
    Result._[2][3] = 0.0f;

    Result._[3][0] = 0.0f;
    Result._[3][1] = 0.0f;
    Result._[3][2] = 0.0f;
    Result._[3][3] = 1.0f;

    return Result;
}

// This method taken from Mike Day at Insomniac Games.
// https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
//
// Note that as mentioned at the top of the paper, the paper assumes the matrix
// would be *post*-multiplied to a vector to rotate it, meaning the matrix is
// the transpose of what we're dealing with. But, because our matrices are
// stored in column-major order, the indices *appear* to match the paper.
//
// For example, m12 in the paper is row 1, column 2. We need to transpose it to
// row 2, column 1. But, because the column comes first when referencing
// elements, it looks like M._[1][2].
//
// Don't be confused! Or if you must be confused, at least trust this
// comment. :)
COVERAGE(M_M4ToQ_RH, 4)
static inline quat M_M4ToQ_RH(mat4 M)
{
    float T;
    quat Q;

    if (M._[2][2] < 0.0f) {
        if (M._[0][0] > M._[1][1]) {
            ASSERT_COVERED(M_M4ToQ_RH);

            T = 1 + M._[0][0] - M._[1][1] - M._[2][2];
            Q = M_Q(
                T,
                M._[0][1] + M._[1][0],
                M._[2][0] + M._[0][2],
                M._[1][2] - M._[2][1]
            );
        } else {
            ASSERT_COVERED(M_M4ToQ_RH);

            T = 1 - M._[0][0] + M._[1][1] - M._[2][2];
            Q = M_Q(
                M._[0][1] + M._[1][0],
                T,
                M._[1][2] + M._[2][1],
                M._[2][0] - M._[0][2]
            );
        }
    } else {
        if (M._[0][0] < -M._[1][1]) {
            ASSERT_COVERED(M_M4ToQ_RH);

            T = 1 - M._[0][0] - M._[1][1] + M._[2][2];
            Q = M_Q(
                M._[2][0] + M._[0][2],
                M._[1][2] + M._[2][1],
                T,
                M._[0][1] - M._[1][0]
            );
        } else {
            ASSERT_COVERED(M_M4ToQ_RH);

            T = 1 + M._[0][0] + M._[1][1] + M._[2][2];
            Q = M_Q(
                M._[1][2] - M._[2][1],
                M._[2][0] - M._[0][2],
                M._[0][1] - M._[1][0],
                T
            );
        }
    }

    Q = M_MulQF(Q, 0.5f / M_SqrtF(T));

    return Q;
}

COVERAGE(M_M4ToQ_LH, 4)
static inline quat M_M4ToQ_LH(mat4 M)
{
    float T;
    quat Q;

    if (M._[2][2] < 0.0f) {
        if (M._[0][0] > M._[1][1]) {
            ASSERT_COVERED(M_M4ToQ_LH);

            T = 1 + M._[0][0] - M._[1][1] - M._[2][2];
            Q = M_Q(
                T,
                M._[0][1] + M._[1][0],
                M._[2][0] + M._[0][2],
                M._[2][1] - M._[1][2]
            );
        } else {
            ASSERT_COVERED(M_M4ToQ_LH);

            T = 1 - M._[0][0] + M._[1][1] - M._[2][2];
            Q = M_Q(
                M._[0][1] + M._[1][0],
                T,
                M._[1][2] + M._[2][1],
                M._[0][2] - M._[2][0]
            );
        }
    } else {
        if (M._[0][0] < -M._[1][1]) {
            ASSERT_COVERED(M_M4ToQ_LH);

            T = 1 - M._[0][0] - M._[1][1] + M._[2][2];
            Q = M_Q(
                M._[2][0] + M._[0][2],
                M._[1][2] + M._[2][1],
                T,
                M._[1][0] - M._[0][1]
            );
        } else {
            ASSERT_COVERED(M_M4ToQ_LH);

            T = 1 + M._[0][0] + M._[1][1] + M._[2][2];
            Q = M_Q(
                M._[2][1] - M._[1][2],
                M._[0][2] - M._[2][0],
                M._[1][0] - M._[0][2],
                T
            );
        }
    }

    Q = M_MulQF(Q, 0.5f / M_SqrtF(T));

    return Q;
}


COVERAGE(M_QFromAxisAngle_RH, 1)
static inline quat M_QFromAxisAngle_RH(vec3 Axis, float Angle)
{
    ASSERT_COVERED(M_QFromAxisAngle_RH);

    quat Result;

    vec3 AxisNormalized = M_NormV3(Axis);
    float SineOfRotation = M_SinF(Angle / 2.0f);

    Result.xyz = M_MulV3F(AxisNormalized, SineOfRotation);
    Result.w = M_CosF(Angle / 2.0f);

    return Result;
}

COVERAGE(M_QFromAxisAngle_LH, 1)
static inline quat M_QFromAxisAngle_LH(vec3 Axis, float Angle)
{
    ASSERT_COVERED(M_QFromAxisAngle_LH);

    return M_QFromAxisAngle_RH(Axis, -Angle);
}

COVERAGE(M_QFromNormPair, 1)
static inline quat M_QFromNormPair(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_QFromNormPair);

    quat Result;

    Result.xyz = M_Cross(Left, Right);
    Result.w = 1.0f + M_DotV3(Left, Right);

    return M_NormQ(Result);
}

COVERAGE(M_QFromVecPair, 1)
static inline quat M_QFromVecPair(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_QFromVecPair);

    return M_QFromNormPair(M_NormV3(Left), M_NormV3(Right));
}

COVERAGE(M_RotateV2, 1)
static inline vec2 M_RotateV2(vec2 V, float Angle)
{
    ASSERT_COVERED(M_RotateV2)

    float sinA = M_SinF(Angle);
    float cosA = M_CosF(Angle);

    return M_V2(V.x * cosA - V.y * sinA, V.x * sinA + V.y * cosA);
}

// implementation from
// https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/
COVERAGE(M_RotateV3Q, 1)
static inline vec3 M_RotateV3Q(vec3 V, quat Q)
{
    ASSERT_COVERED(M_RotateV3Q);

    vec3 t = M_MulV3F(M_Cross(Q.xyz, V), 2);
    return M_AddV3(V, M_AddV3(M_MulV3F(t, Q.w), M_Cross(Q.xyz, t)));
}

COVERAGE(M_RotateV3AxisAngle_LH, 1)
static inline vec3 M_RotateV3AxisAngle_LH(vec3 V, vec3 Axis, float Angle) {
    ASSERT_COVERED(M_RotateV3AxisAngle_LH);

    return M_RotateV3Q(V, M_QFromAxisAngle_LH(Axis, Angle));
}

COVERAGE(M_RotateV3AxisAngle_RH, 1)
static inline vec3 M_RotateV3AxisAngle_RH(vec3 V, vec3 Axis, float Angle) {
    ASSERT_COVERED(M_RotateV3AxisAngle_RH);

    return M_RotateV3Q(V, M_QFromAxisAngle_RH(Axis, Angle));
}


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

COVERAGE(M_LenV2CPP, 1)
static inline float M_Len(vec2 A)
{
    ASSERT_COVERED(M_LenV2CPP);
    return M_LenV2(A);
}

COVERAGE(M_LenV3CPP, 1)
static inline float M_Len(vec3 A)
{
    ASSERT_COVERED(M_LenV3CPP);
    return M_LenV3(A);
}

COVERAGE(M_LenV4CPP, 1)
static inline float M_Len(vec4 A)
{
    ASSERT_COVERED(M_LenV4CPP);
    return M_LenV4(A);
}

COVERAGE(M_LenSqrV2CPP, 1)
static inline float M_LenSqr(vec2 A)
{
    ASSERT_COVERED(M_LenSqrV2CPP);
    return M_LenSqrV2(A);
}

COVERAGE(M_LenSqrV3CPP, 1)
static inline float M_LenSqr(vec3 A)
{
    ASSERT_COVERED(M_LenSqrV3CPP);
    return M_LenSqrV3(A);
}

COVERAGE(M_LenSqrV4CPP, 1)
static inline float M_LenSqr(vec4 A)
{
    ASSERT_COVERED(M_LenSqrV4CPP);
    return M_LenSqrV4(A);
}

COVERAGE(M_NormV2CPP, 1)
static inline vec2 M_Norm(vec2 A)
{
    ASSERT_COVERED(M_NormV2CPP);
    return M_NormV2(A);
}

COVERAGE(M_NormV3CPP, 1)
static inline vec3 M_Norm(vec3 A)
{
    ASSERT_COVERED(M_NormV3CPP);
    return M_NormV3(A);
}

COVERAGE(M_NormV4CPP, 1)
static inline vec4 M_Norm(vec4 A)
{
    ASSERT_COVERED(M_NormV4CPP);
    return M_NormV4(A);
}

COVERAGE(M_NormQCPP, 1)
static inline quat M_Norm(quat A)
{
    ASSERT_COVERED(M_NormQCPP);
    return M_NormQ(A);
}

COVERAGE(M_DotV2CPP, 1)
static inline float M_Dot(vec2 Left, vec2 VecTwo)
{
    ASSERT_COVERED(M_DotV2CPP);
    return M_DotV2(Left, VecTwo);
}

COVERAGE(M_DotV3CPP, 1)
static inline float M_Dot(vec3 Left, vec3 VecTwo)
{
    ASSERT_COVERED(M_DotV3CPP);
    return M_DotV3(Left, VecTwo);
}

COVERAGE(M_DotV4CPP, 1)
static inline float M_Dot(vec4 Left, vec4 VecTwo)
{
    ASSERT_COVERED(M_DotV4CPP);
    return M_DotV4(Left, VecTwo);
}

COVERAGE(M_LerpV2CPP, 1)
static inline vec2 M_Lerp(vec2 Left, float Time, vec2 Right)
{
    ASSERT_COVERED(M_LerpV2CPP);
    return M_LerpV2(Left, Time, Right);
}

COVERAGE(M_LerpV3CPP, 1)
static inline vec3 M_Lerp(vec3 Left, float Time, vec3 Right)
{
    ASSERT_COVERED(M_LerpV3CPP);
    return M_LerpV3(Left, Time, Right);
}

COVERAGE(M_LerpV4CPP, 1)
static inline vec4 M_Lerp(vec4 Left, float Time, vec4 Right)
{
    ASSERT_COVERED(M_LerpV4CPP);
    return M_LerpV4(Left, Time, Right);
}

COVERAGE(M_TransposeM2CPP, 1)
static inline mat2 M_Transpose(mat2 Matrix)
{
    ASSERT_COVERED(M_TransposeM2CPP);
    return M_TransposeM2(Matrix);
}

COVERAGE(M_TransposeM3CPP, 1)
static inline mat3 M_Transpose(mat3 Matrix)
{
    ASSERT_COVERED(M_TransposeM3CPP);
    return M_TransposeM3(Matrix);
}

COVERAGE(M_TransposeM4CPP, 1)
static inline mat4 M_Transpose(mat4 Matrix)
{
    ASSERT_COVERED(M_TransposeM4CPP);
    return M_TransposeM4(Matrix);
}

COVERAGE(M_DeterminantM2CPP, 1)
static inline float M_Determinant(mat2 Matrix)
{
    ASSERT_COVERED(M_DeterminantM2CPP);
    return M_DeterminantM2(Matrix);
}

COVERAGE(M_DeterminantM3CPP, 1)
static inline float M_Determinant(mat3 Matrix)
{
    ASSERT_COVERED(M_DeterminantM3CPP);
    return M_DeterminantM3(Matrix);
}

COVERAGE(M_DeterminantM4CPP, 1)
static inline float M_Determinant(mat4 Matrix)
{
    ASSERT_COVERED(M_DeterminantM4CPP);
    return M_DeterminantM4(Matrix);
}

COVERAGE(M_InvGeneralM2CPP, 1)
static inline mat2 M_InvGeneral(mat2 Matrix)
{
    ASSERT_COVERED(M_InvGeneralM2CPP);
    return M_InvGeneralM2(Matrix);
}

COVERAGE(M_InvGeneralM3CPP, 1)
static inline mat3 M_InvGeneral(mat3 Matrix)
{
    ASSERT_COVERED(M_InvGeneralM3CPP);
    return M_InvGeneralM3(Matrix);
}

COVERAGE(M_InvGeneralM4CPP, 1)
static inline mat4 M_InvGeneral(mat4 Matrix)
{
    ASSERT_COVERED(M_InvGeneralM4CPP);
    return M_InvGeneralM4(Matrix);
}

COVERAGE(M_DotQCPP, 1)
static inline float M_Dot(quat QuatOne, quat QuatTwo)
{
    ASSERT_COVERED(M_DotQCPP);
    return M_DotQ(QuatOne, QuatTwo);
}

COVERAGE(M_AddV2CPP, 1)
static inline vec2 M_Add(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_AddV2CPP);
    return M_AddV2(Left, Right);
}

COVERAGE(M_AddV3CPP, 1)
static inline vec3 M_Add(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_AddV3CPP);
    return M_AddV3(Left, Right);
}

COVERAGE(M_AddV4CPP, 1)
static inline vec4 M_Add(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_AddV4CPP);
    return M_AddV4(Left, Right);
}

COVERAGE(M_AddM2CPP, 1)
static inline mat2 M_Add(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_AddM2CPP);
    return M_AddM2(Left, Right);
}

COVERAGE(M_AddM3CPP, 1)
static inline mat3 M_Add(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_AddM3CPP);
    return M_AddM3(Left, Right);
}

COVERAGE(M_AddM4CPP, 1)
static inline mat4 M_Add(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_AddM4CPP);
    return M_AddM4(Left, Right);
}

COVERAGE(M_AddQCPP, 1)
static inline quat M_Add(quat Left, quat Right)
{
    ASSERT_COVERED(M_AddQCPP);
    return M_AddQ(Left, Right);
}

COVERAGE(M_SubV2CPP, 1)
static inline vec2 M_Sub(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_SubV2CPP);
    return M_SubV2(Left, Right);
}

COVERAGE(M_SubV3CPP, 1)
static inline vec3 M_Sub(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_SubV3CPP);
    return M_SubV3(Left, Right);
}

COVERAGE(M_SubV4CPP, 1)
static inline vec4 M_Sub(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_SubV4CPP);
    return M_SubV4(Left, Right);
}

COVERAGE(M_SubM2CPP, 1)
static inline mat2 M_Sub(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_SubM2CPP);
    return M_SubM2(Left, Right);
}

COVERAGE(M_SubM3CPP, 1)
static inline mat3 M_Sub(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_SubM3CPP);
    return M_SubM3(Left, Right);
}

COVERAGE(M_SubM4CPP, 1)
static inline mat4 M_Sub(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_SubM4CPP);
    return M_SubM4(Left, Right);
}

COVERAGE(M_SubQCPP, 1)
static inline quat M_Sub(quat Left, quat Right)
{
    ASSERT_COVERED(M_SubQCPP);
    return M_SubQ(Left, Right);
}

COVERAGE(M_MulV2CPP, 1)
static inline vec2 M_Mul(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_MulV2CPP);
    return M_MulV2(Left, Right);
}

COVERAGE(M_MulV2FCPP, 1)
static inline vec2 M_Mul(vec2 Left, float Right)
{
    ASSERT_COVERED(M_MulV2FCPP);
    return M_MulV2F(Left, Right);
}

COVERAGE(M_MulV3CPP, 1)
static inline vec3 M_Mul(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_MulV3CPP);
    return M_MulV3(Left, Right);
}

COVERAGE(M_MulV3FCPP, 1)
static inline vec3 M_Mul(vec3 Left, float Right)
{
    ASSERT_COVERED(M_MulV3FCPP);
    return M_MulV3F(Left, Right);
}

COVERAGE(M_MulV4CPP, 1)
static inline vec4 M_Mul(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_MulV4CPP);
    return M_MulV4(Left, Right);
}

COVERAGE(M_MulV4FCPP, 1)
static inline vec4 M_Mul(vec4 Left, float Right)
{
    ASSERT_COVERED(M_MulV4FCPP);
    return M_MulV4F(Left, Right);
}

COVERAGE(M_MulM2CPP, 1)
static inline mat2 M_Mul(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_MulM2CPP);
    return M_MulM2(Left, Right);
}

COVERAGE(M_MulM3CPP, 1)
static inline mat3 M_Mul(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_MulM3CPP);
    return M_MulM3(Left, Right);
}

COVERAGE(M_MulM4CPP, 1)
static inline mat4 M_Mul(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_MulM4CPP);
    return M_MulM4(Left, Right);
}

COVERAGE(M_MulM2FCPP, 1)
static inline mat2 M_Mul(mat2 Left, float Right)
{
    ASSERT_COVERED(M_MulM2FCPP);
    return M_MulM2F(Left, Right);
}

COVERAGE(M_MulM3FCPP, 1)
static inline mat3 M_Mul(mat3 Left, float Right)
{
    ASSERT_COVERED(M_MulM3FCPP);
    return M_MulM3F(Left, Right);
}

COVERAGE(M_MulM4FCPP, 1)
static inline mat4 M_Mul(mat4 Left, float Right)
{
    ASSERT_COVERED(M_MulM4FCPP);
    return M_MulM4F(Left, Right);
}

COVERAGE(M_MulM2V2CPP, 1)
static inline vec2 M_Mul(mat2 Matrix, vec2 Vector)
{
    ASSERT_COVERED(M_MulM2V2CPP);
    return M_MulM2V2(Matrix, Vector);
}

COVERAGE(M_MulM3V3CPP, 1)
static inline vec3 M_Mul(mat3 Matrix, vec3 Vector)
{
    ASSERT_COVERED(M_MulM3V3CPP);
    return M_MulM3V3(Matrix, Vector);
}

COVERAGE(M_MulM4V4CPP, 1)
static inline vec4 M_Mul(mat4 Matrix, vec4 Vector)
{
    ASSERT_COVERED(M_MulM4V4CPP);
    return M_MulM4V4(Matrix, Vector);
}

COVERAGE(M_MulQCPP, 1)
static inline quat M_Mul(quat Left, quat Right)
{
    ASSERT_COVERED(M_MulQCPP);
    return M_MulQ(Left, Right);
}

COVERAGE(M_MulQFCPP, 1)
static inline quat M_Mul(quat Left, float Right)
{
    ASSERT_COVERED(M_MulQFCPP);
    return M_MulQF(Left, Right);
}

COVERAGE(M_DivV2CPP, 1)
static inline vec2 M_Div(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_DivV2CPP);
    return M_DivV2(Left, Right);
}

COVERAGE(M_DivV2FCPP, 1)
static inline vec2 M_Div(vec2 Left, float Right)
{
    ASSERT_COVERED(M_DivV2FCPP);
    return M_DivV2F(Left, Right);
}

COVERAGE(M_DivV3CPP, 1)
static inline vec3 M_Div(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_DivV3CPP);
    return M_DivV3(Left, Right);
}

COVERAGE(M_DivV3FCPP, 1)
static inline vec3 M_Div(vec3 Left, float Right)
{
    ASSERT_COVERED(M_DivV3FCPP);
    return M_DivV3F(Left, Right);
}

COVERAGE(M_DivV4CPP, 1)
static inline vec4 M_Div(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_DivV4CPP);
    return M_DivV4(Left, Right);
}

COVERAGE(M_DivV4FCPP, 1)
static inline vec4 M_Div(vec4 Left, float Right)
{
    ASSERT_COVERED(M_DivV4FCPP);
    return M_DivV4F(Left, Right);
}

COVERAGE(M_DivM2FCPP, 1)
static inline mat2 M_Div(mat2 Left, float Right)
{
    ASSERT_COVERED(M_DivM2FCPP);
    return M_DivM2F(Left, Right);
}

COVERAGE(M_DivM3FCPP, 1)
static inline mat3 M_Div(mat3 Left, float Right)
{
    ASSERT_COVERED(M_DivM3FCPP);
    return M_DivM3F(Left, Right);
}

COVERAGE(M_DivM4FCPP, 1)
static inline mat4 M_Div(mat4 Left, float Right)
{
    ASSERT_COVERED(M_DivM4FCPP);
    return M_DivM4F(Left, Right);
}

COVERAGE(M_DivQFCPP, 1)
static inline quat M_Div(quat Left, float Right)
{
    ASSERT_COVERED(M_DivQFCPP);
    return M_DivQF(Left, Right);
}

COVERAGE(M_EqV2CPP, 1)
static inline M_Bool M_Eq(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_EqV2CPP);
    return M_EqV2(Left, Right);
}

COVERAGE(M_EqV3CPP, 1)
static inline M_Bool M_Eq(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_EqV3CPP);
    return M_EqV3(Left, Right);
}

COVERAGE(M_EqV4CPP, 1)
static inline M_Bool M_Eq(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_EqV4CPP);
    return M_EqV4(Left, Right);
}

COVERAGE(M_AddV2Op, 1)
static inline vec2 operator+(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_AddV2Op);
    return M_AddV2(Left, Right);
}

COVERAGE(M_AddV3Op, 1)
static inline vec3 operator+(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_AddV3Op);
    return M_AddV3(Left, Right);
}

COVERAGE(M_AddV4Op, 1)
static inline vec4 operator+(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_AddV4Op);
    return M_AddV4(Left, Right);
}

COVERAGE(M_AddM2Op, 1)
static inline mat2 operator+(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_AddM2Op);
    return M_AddM2(Left, Right);
}

COVERAGE(M_AddM3Op, 1)
static inline mat3 operator+(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_AddM3Op);
    return M_AddM3(Left, Right);
}

COVERAGE(M_AddM4Op, 1)
static inline mat4 operator+(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_AddM4Op);
    return M_AddM4(Left, Right);
}

COVERAGE(M_AddQOp, 1)
static inline quat operator+(quat Left, quat Right)
{
    ASSERT_COVERED(M_AddQOp);
    return M_AddQ(Left, Right);
}

COVERAGE(M_SubV2Op, 1)
static inline vec2 operator-(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_SubV2Op);
    return M_SubV2(Left, Right);
}

COVERAGE(M_SubV3Op, 1)
static inline vec3 operator-(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_SubV3Op);
    return M_SubV3(Left, Right);
}

COVERAGE(M_SubV4Op, 1)
static inline vec4 operator-(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_SubV4Op);
    return M_SubV4(Left, Right);
}

COVERAGE(M_SubM2Op, 1)
static inline mat2 operator-(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_SubM2Op);
    return M_SubM2(Left, Right);
}

COVERAGE(M_SubM3Op, 1)
static inline mat3 operator-(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_SubM3Op);
    return M_SubM3(Left, Right);
}

COVERAGE(M_SubM4Op, 1)
static inline mat4 operator-(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_SubM4Op);
    return M_SubM4(Left, Right);
}

COVERAGE(M_SubQOp, 1)
static inline quat operator-(quat Left, quat Right)
{
    ASSERT_COVERED(M_SubQOp);
    return M_SubQ(Left, Right);
}

COVERAGE(M_MulV2Op, 1)
static inline vec2 operator*(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_MulV2Op);
    return M_MulV2(Left, Right);
}

COVERAGE(M_MulV3Op, 1)
static inline vec3 operator*(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_MulV3Op);
    return M_MulV3(Left, Right);
}

COVERAGE(M_MulV4Op, 1)
static inline vec4 operator*(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_MulV4Op);
    return M_MulV4(Left, Right);
}

COVERAGE(M_MulM2Op, 1)
static inline mat2 operator*(mat2 Left, mat2 Right)
{
    ASSERT_COVERED(M_MulM2Op);
    return M_MulM2(Left, Right);
}

COVERAGE(M_MulM3Op, 1)
static inline mat3 operator*(mat3 Left, mat3 Right)
{
    ASSERT_COVERED(M_MulM3Op);
    return M_MulM3(Left, Right);
}

COVERAGE(M_MulM4Op, 1)
static inline mat4 operator*(mat4 Left, mat4 Right)
{
    ASSERT_COVERED(M_MulM4Op);
    return M_MulM4(Left, Right);
}

COVERAGE(M_MulQOp, 1)
static inline quat operator*(quat Left, quat Right)
{
    ASSERT_COVERED(M_MulQOp);
    return M_MulQ(Left, Right);
}

COVERAGE(M_MulV2FOp, 1)
static inline vec2 operator*(vec2 Left, float Right)
{
    ASSERT_COVERED(M_MulV2FOp);
    return M_MulV2F(Left, Right);
}

COVERAGE(M_MulV3FOp, 1)
static inline vec3 operator*(vec3 Left, float Right)
{
    ASSERT_COVERED(M_MulV3FOp);
    return M_MulV3F(Left, Right);
}

COVERAGE(M_MulV4FOp, 1)
static inline vec4 operator*(vec4 Left, float Right)
{
    ASSERT_COVERED(M_MulV4FOp);
    return M_MulV4F(Left, Right);
}

COVERAGE(M_MulM2FOp, 1)
static inline mat2 operator*(mat2 Left, float Right)
{
    ASSERT_COVERED(M_MulM2FOp);
    return M_MulM2F(Left, Right);
}

COVERAGE(M_MulM3FOp, 1)
static inline mat3 operator*(mat3 Left, float Right)
{
    ASSERT_COVERED(M_MulM3FOp);
    return M_MulM3F(Left, Right);
}

COVERAGE(M_MulM4FOp, 1)
static inline mat4 operator*(mat4 Left, float Right)
{
    ASSERT_COVERED(M_MulM4FOp);
    return M_MulM4F(Left, Right);
}

COVERAGE(M_MulQFOp, 1)
static inline quat operator*(quat Left, float Right)
{
    ASSERT_COVERED(M_MulQFOp);
    return M_MulQF(Left, Right);
}

COVERAGE(M_MulV2FOpLeft, 1)
static inline vec2 operator*(float Left, vec2 Right)
{
    ASSERT_COVERED(M_MulV2FOpLeft);
    return M_MulV2F(Right, Left);
}

COVERAGE(M_MulV3FOpLeft, 1)
static inline vec3 operator*(float Left, vec3 Right)
{
    ASSERT_COVERED(M_MulV3FOpLeft);
    return M_MulV3F(Right, Left);
}

COVERAGE(M_MulV4FOpLeft, 1)
static inline vec4 operator*(float Left, vec4 Right)
{
    ASSERT_COVERED(M_MulV4FOpLeft);
    return M_MulV4F(Right, Left);
}

COVERAGE(M_MulM2FOpLeft, 1)
static inline mat2 operator*(float Left, mat2 Right)
{
    ASSERT_COVERED(M_MulM2FOpLeft);
    return M_MulM2F(Right, Left);
}

COVERAGE(M_MulM3FOpLeft, 1)
static inline mat3 operator*(float Left, mat3 Right)
{
    ASSERT_COVERED(M_MulM3FOpLeft);
    return M_MulM3F(Right, Left);
}

COVERAGE(M_MulM4FOpLeft, 1)
static inline mat4 operator*(float Left, mat4 Right)
{
    ASSERT_COVERED(M_MulM4FOpLeft);
    return M_MulM4F(Right, Left);
}

COVERAGE(M_MulQFOpLeft, 1)
static inline quat operator*(float Left, quat Right)
{
    ASSERT_COVERED(M_MulQFOpLeft);
    return M_MulQF(Right, Left);
}

COVERAGE(M_MulM2V2Op, 1)
static inline vec2 operator*(mat2 Matrix, vec2 Vector)
{
    ASSERT_COVERED(M_MulM2V2Op);
    return M_MulM2V2(Matrix, Vector);
}

COVERAGE(M_MulM3V3Op, 1)
static inline vec3 operator*(mat3 Matrix, vec3 Vector)
{
    ASSERT_COVERED(M_MulM3V3Op);
    return M_MulM3V3(Matrix, Vector);
}

COVERAGE(M_MulM4V4Op, 1)
static inline vec4 operator*(mat4 Matrix, vec4 Vector)
{
    ASSERT_COVERED(M_MulM4V4Op);
    return M_MulM4V4(Matrix, Vector);
}

COVERAGE(M_DivV2Op, 1)
static inline vec2 operator/(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_DivV2Op);
    return M_DivV2(Left, Right);
}

COVERAGE(M_DivV3Op, 1)
static inline vec3 operator/(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_DivV3Op);
    return M_DivV3(Left, Right);
}

COVERAGE(M_DivV4Op, 1)
static inline vec4 operator/(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_DivV4Op);
    return M_DivV4(Left, Right);
}

COVERAGE(M_DivV2FOp, 1)
static inline vec2 operator/(vec2 Left, float Right)
{
    ASSERT_COVERED(M_DivV2FOp);
    return M_DivV2F(Left, Right);
}

COVERAGE(M_DivV3FOp, 1)
static inline vec3 operator/(vec3 Left, float Right)
{
    ASSERT_COVERED(M_DivV3FOp);
    return M_DivV3F(Left, Right);
}

COVERAGE(M_DivV4FOp, 1)
static inline vec4 operator/(vec4 Left, float Right)
{
    ASSERT_COVERED(M_DivV4FOp);
    return M_DivV4F(Left, Right);
}

COVERAGE(M_DivM4FOp, 1)
static inline mat4 operator/(mat4 Left, float Right)
{
    ASSERT_COVERED(M_DivM4FOp);
    return M_DivM4F(Left, Right);
}

COVERAGE(M_DivM3FOp, 1)
static inline mat3 operator/(mat3 Left, float Right)
{
    ASSERT_COVERED(M_DivM3FOp);
    return M_DivM3F(Left, Right);
}

COVERAGE(M_DivM2FOp, 1)
static inline mat2 operator/(mat2 Left, float Right)
{
    ASSERT_COVERED(M_DivM2FOp);
    return M_DivM2F(Left, Right);
}

COVERAGE(M_DivQFOp, 1)
static inline quat operator/(quat Left, float Right)
{
    ASSERT_COVERED(M_DivQFOp);
    return M_DivQF(Left, Right);
}

COVERAGE(M_AddV2Assign, 1)
static inline vec2 &operator+=(vec2 &Left, vec2 Right)
{
    ASSERT_COVERED(M_AddV2Assign);
    return Left = Left + Right;
}

COVERAGE(M_AddV3Assign, 1)
static inline vec3 &operator+=(vec3 &Left, vec3 Right)
{
    ASSERT_COVERED(M_AddV3Assign);
    return Left = Left + Right;
}

COVERAGE(M_AddV4Assign, 1)
static inline vec4 &operator+=(vec4 &Left, vec4 Right)
{
    ASSERT_COVERED(M_AddV4Assign);
    return Left = Left + Right;
}

COVERAGE(M_AddM2Assign, 1)
static inline mat2 &operator+=(mat2 &Left, mat2 Right)
{
    ASSERT_COVERED(M_AddM2Assign);
    return Left = Left + Right;
}

COVERAGE(M_AddM3Assign, 1)
static inline mat3 &operator+=(mat3 &Left, mat3 Right)
{
    ASSERT_COVERED(M_AddM3Assign);
    return Left = Left + Right;
}

COVERAGE(M_AddM4Assign, 1)
static inline mat4 &operator+=(mat4 &Left, mat4 Right)
{
    ASSERT_COVERED(M_AddM4Assign);
    return Left = Left + Right;
}

COVERAGE(M_AddQAssign, 1)
static inline quat &operator+=(quat &Left, quat Right)
{
    ASSERT_COVERED(M_AddQAssign);
    return Left = Left + Right;
}

COVERAGE(M_SubV2Assign, 1)
static inline vec2 &operator-=(vec2 &Left, vec2 Right)
{
    ASSERT_COVERED(M_SubV2Assign);
    return Left = Left - Right;
}

COVERAGE(M_SubV3Assign, 1)
static inline vec3 &operator-=(vec3 &Left, vec3 Right)
{
    ASSERT_COVERED(M_SubV3Assign);
    return Left = Left - Right;
}

COVERAGE(M_SubV4Assign, 1)
static inline vec4 &operator-=(vec4 &Left, vec4 Right)
{
    ASSERT_COVERED(M_SubV4Assign);
    return Left = Left - Right;
}

COVERAGE(M_SubM2Assign, 1)
static inline mat2 &operator-=(mat2 &Left, mat2 Right)
{
    ASSERT_COVERED(M_SubM2Assign);
    return Left = Left - Right;
}

COVERAGE(M_SubM3Assign, 1)
static inline mat3 &operator-=(mat3 &Left, mat3 Right)
{
    ASSERT_COVERED(M_SubM3Assign);
    return Left = Left - Right;
}

COVERAGE(M_SubM4Assign, 1)
static inline mat4 &operator-=(mat4 &Left, mat4 Right)
{
    ASSERT_COVERED(M_SubM4Assign);
    return Left = Left - Right;
}

COVERAGE(M_SubQAssign, 1)
static inline quat &operator-=(quat &Left, quat Right)
{
    ASSERT_COVERED(M_SubQAssign);
    return Left = Left - Right;
}

COVERAGE(M_MulV2Assign, 1)
static inline vec2 &operator*=(vec2 &Left, vec2 Right)
{
    ASSERT_COVERED(M_MulV2Assign);
    return Left = Left * Right;
}

COVERAGE(M_MulV3Assign, 1)
static inline vec3 &operator*=(vec3 &Left, vec3 Right)
{
    ASSERT_COVERED(M_MulV3Assign);
    return Left = Left * Right;
}

COVERAGE(M_MulV4Assign, 1)
static inline vec4 &operator*=(vec4 &Left, vec4 Right)
{
    ASSERT_COVERED(M_MulV4Assign);
    return Left = Left * Right;
}

COVERAGE(M_MulV2FAssign, 1)
static inline vec2 &operator*=(vec2 &Left, float Right)
{
    ASSERT_COVERED(M_MulV2FAssign);
    return Left = Left * Right;
}

COVERAGE(M_MulV3FAssign, 1)
static inline vec3 &operator*=(vec3 &Left, float Right)
{
    ASSERT_COVERED(M_MulV3FAssign);
    return Left = Left * Right;
}

COVERAGE(M_MulV4FAssign, 1)
static inline vec4 &operator*=(vec4 &Left, float Right)
{
    ASSERT_COVERED(M_MulV4FAssign);
    return Left = Left * Right;
}

COVERAGE(M_MulM2FAssign, 1)
static inline mat2 &operator*=(mat2 &Left, float Right)
{
    ASSERT_COVERED(M_MulM2FAssign);
    return Left = Left * Right;
}

COVERAGE(M_MulM3FAssign, 1)
static inline mat3 &operator*=(mat3 &Left, float Right)
{
    ASSERT_COVERED(M_MulM3FAssign);
    return Left = Left * Right;
}

COVERAGE(M_MulM4FAssign, 1)
static inline mat4 &operator*=(mat4 &Left, float Right)
{
    ASSERT_COVERED(M_MulM4FAssign);
    return Left = Left * Right;
}

COVERAGE(M_MulQFAssign, 1)
static inline quat &operator*=(quat &Left, float Right)
{
    ASSERT_COVERED(M_MulQFAssign);
    return Left = Left * Right;
}

COVERAGE(M_DivV2Assign, 1)
static inline vec2 &operator/=(vec2 &Left, vec2 Right)
{
    ASSERT_COVERED(M_DivV2Assign);
    return Left = Left / Right;
}

COVERAGE(M_DivV3Assign, 1)
static inline vec3 &operator/=(vec3 &Left, vec3 Right)
{
    ASSERT_COVERED(M_DivV3Assign);
    return Left = Left / Right;
}

COVERAGE(M_DivV4Assign, 1)
static inline vec4 &operator/=(vec4 &Left, vec4 Right)
{
    ASSERT_COVERED(M_DivV4Assign);
    return Left = Left / Right;
}

COVERAGE(M_DivV2FAssign, 1)
static inline vec2 &operator/=(vec2 &Left, float Right)
{
    ASSERT_COVERED(M_DivV2FAssign);
    return Left = Left / Right;
}

COVERAGE(M_DivV3FAssign, 1)
static inline vec3 &operator/=(vec3 &Left, float Right)
{
    ASSERT_COVERED(M_DivV3FAssign);
    return Left = Left / Right;
}

COVERAGE(M_DivV4FAssign, 1)
static inline vec4 &operator/=(vec4 &Left, float Right)
{
    ASSERT_COVERED(M_DivV4FAssign);
    return Left = Left / Right;
}

COVERAGE(M_DivM4FAssign, 1)
static inline mat4 &operator/=(mat4 &Left, float Right)
{
    ASSERT_COVERED(M_DivM4FAssign);
    return Left = Left / Right;
}

COVERAGE(M_DivQFAssign, 1)
static inline quat &operator/=(quat &Left, float Right)
{
    ASSERT_COVERED(M_DivQFAssign);
    return Left = Left / Right;
}

COVERAGE(M_EqV2Op, 1)
static inline M_Bool operator==(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_EqV2Op);
    return M_EqV2(Left, Right);
}

COVERAGE(M_EqV3Op, 1)
static inline M_Bool operator==(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_EqV3Op);
    return M_EqV3(Left, Right);
}

COVERAGE(M_EqV4Op, 1)
static inline M_Bool operator==(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_EqV4Op);
    return M_EqV4(Left, Right);
}

COVERAGE(M_EqV2OpNot, 1)
static inline M_Bool operator!=(vec2 Left, vec2 Right)
{
    ASSERT_COVERED(M_EqV2OpNot);
    return !M_EqV2(Left, Right);
}

COVERAGE(M_EqV3OpNot, 1)
static inline M_Bool operator!=(vec3 Left, vec3 Right)
{
    ASSERT_COVERED(M_EqV3OpNot);
    return !M_EqV3(Left, Right);
}

COVERAGE(M_EqV4OpNot, 1)
static inline M_Bool operator!=(vec4 Left, vec4 Right)
{
    ASSERT_COVERED(M_EqV4OpNot);
    return !M_EqV4(Left, Right);
}

COVERAGE(M_UnaryMinusV2, 1)
static inline vec2 operator-(vec2 In)
{
    ASSERT_COVERED(M_UnaryMinusV2);

    vec2 Result;
    Result.x = -In.x;
    Result.y = -In.y;

    return Result;
}

COVERAGE(M_UnaryMinusV3, 1)
static inline vec3 operator-(vec3 In)
{
    ASSERT_COVERED(M_UnaryMinusV3);

    vec3 Result;
    Result.x = -In.x;
    Result.y = -In.y;
    Result.z = -In.z;

    return Result;
}

COVERAGE(M_UnaryMinusV4, 1)
static inline vec4 operator-(vec4 In)
{
    ASSERT_COVERED(M_UnaryMinusV4);

    vec4 Result;
#if HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_xor_ps(In.SSE, _mm_set1_ps(-0.0f));
#elif defined(HANDMADE_MATH__USE_NEON)
    float32x4_t Zero = vdupq_n_f32(0.0f);
    Result.NEON = vsubq_f32(Zero, In.NEON);
#else
    Result.x = -In.x;
    Result.y = -In.y;
    Result.z = -In.z;
    Result.w = -In.w;
#endif

    return Result;
}

#endif /* __cplusplus*/

#ifdef HANDMADE_MATH__USE_C11_GENERICS
#define M_Add(A, B) _Generic((A), \
        vec2: M_AddV2, \
        vec3: M_AddV3, \
        vec4: M_AddV4, \
        mat2: M_AddM2, \
        mat3: M_AddM3, \
        mat4: M_AddM4, \
        quat: M_AddQ \
)(A, B)

#define M_Sub(A, B) _Generic((A), \
        vec2: M_SubV2, \
        vec3: M_SubV3, \
        vec4: M_SubV4, \
        mat2: M_SubM2, \
        mat3: M_SubM3, \
        mat4: M_SubM4, \
        quat: M_SubQ \
)(A, B)

#define M_Mul(A, B) _Generic((B), \
     float: _Generic((A), \
        vec2: M_MulV2F, \
        vec3: M_MulV3F, \
        vec4: M_MulV4F, \
        mat2: M_MulM2F, \
        mat3: M_MulM3F, \
        mat4: M_MulM4F, \
        quat: M_MulQF \
     ), \
     mat2: M_MulM2, \
     mat3: M_MulM3, \
     mat4: M_MulM4, \
     quat: M_MulQ, \
     default: _Generic((A), \
        vec2: M_MulV2, \
        vec3: M_MulV3, \
        vec4: M_MulV4, \
        mat2: M_MulM2V2, \
        mat3: M_MulM3V3, \
        mat4: M_MulM4V4 \
    ) \
)(A, B)

#define M_Div(A, B) _Generic((B), \
     float: _Generic((A), \
        mat2: M_DivM2F, \
        mat3: M_DivM3F, \
        mat4: M_DivM4F, \
        vec2: M_DivV2F, \
        vec3: M_DivV3F, \
        vec4: M_DivV4F, \
        quat: M_DivQF  \
     ), \
     mat2: M_DivM2, \
     mat3: M_DivM3, \
     mat4: M_DivM4, \
     quat: M_DivQ, \
     default: _Generic((A), \
        vec2: M_DivV2, \
        vec3: M_DivV3, \
        vec4: M_DivV4  \
    ) \
)(A, B)

#define M_Len(A) _Generic((A), \
        vec2: M_LenV2, \
        vec3: M_LenV3, \
        vec4: M_LenV4  \
)(A)

#define M_LenSqr(A) _Generic((A), \
        vec2: M_LenSqrV2, \
        vec3: M_LenSqrV3, \
        vec4: M_LenSqrV4  \
)(A)

#define M_Norm(A) _Generic((A), \
        vec2: M_NormV2, \
        vec3: M_NormV3, \
        vec4: M_NormV4  \
)(A)

#define M_Dot(A, B) _Generic((A), \
        vec2: M_DotV2, \
        vec3: M_DotV3, \
        vec4: M_DotV4  \
)(A, B)

#define M_Lerp(A, T, B) _Generic((A), \
        float: M_Lerp, \
        vec2: M_LerpV2, \
        vec3: M_LerpV3, \
        vec4: M_LerpV4 \
)(A, T, B)

#define M_Eq(A, B) _Generic((A), \
        vec2: M_EqV2, \
        vec3: M_EqV3, \
        vec4: M_EqV4  \
)(A, B)

#define M_Transpose(M) _Generic((M), \
        mat2: M_TransposeM2, \
        mat3: M_TransposeM3, \
        mat4: M_TransposeM4  \
)(M)

#define M_Determinant(M) _Generic((M), \
        mat2: M_DeterminantM2, \
        mat3: M_DeterminantM3, \
        mat4: M_DeterminantM4  \
)(M)

#define M_InvGeneral(M) _Generic((M), \
        mat2: M_InvGeneralM2, \
        mat3: M_InvGeneralM3, \
        mat4: M_InvGeneralM4  \
)(M)

#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
