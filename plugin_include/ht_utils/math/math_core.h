#pragma once
#define MATH_CORE_INCLUDED

#include <math.h>

#define M_PI 3.14159265358979323846f
#define M_TAU 6.28318530717958647693f

#define M_DegToRad (M_PI / 180.f)
#define M_RadToDeg (180.f / M_PI)

static inline float M_Min(float a, float b) {
	return a < b ? a : b;
}

static inline float M_Max(float a, float b) {
	return a > b ? a : b;
}

static inline float M_Clamp(float x, float min, float max) {
	if (x < min) x = min;
	if (x > max) x = max;
	return x;
}

static inline float M_Lerp(float a, float b, float t) {
	return (1.f-t)*a + b*t;
}

// -- vec2 -------------------------------------------------

static inline vec2 operator+(vec2 a, vec2 b) {
	return {a.x + b.x, a.y + b.y};
}

static inline vec2 operator-(vec2 a, vec2 b) {
	return {a.x - b.x, a.y - b.y};
}

static inline vec2 operator*(vec2 a, vec2 b) {
	return {a.x * b.x, a.y * b.y};
}

static inline vec2 operator/(vec2 a, vec2 b) {
	return {a.x / b.x, a.y / b.y};
}

static inline vec2 operator*(float a, vec2 b) {
	return {a * b.x, a * b.y};
}

static inline vec2 operator*(vec2 a, float b) {
	return {a.x * b, a.y * b};
}

static inline vec2 operator/(vec2 a, float b) {
	float b_inv = 1.f / b;
	return {a.x * b_inv, a.y * b_inv};
}

static inline float M_Dot2(vec2 a, vec2 b) {
	return a.x * b.x + a.y * b.y;
}

static inline float M_Len2(vec2 v) {
	return sqrtf(v.x*v.x + v.y*v.y);
}

static inline float M_LenSquared2(vec2 v) {
	return v.x*v.x + v.y*v.y;
}

// Returns {1, 0} if v is zero.
static inline vec2 M_Norm2(vec2 v) {
	float len_squared = v.x*v.x + v.y*v.y;
	return len_squared != 0.f ? v / sqrtf(len_squared) : vec2{1.f, 0.f};
}

static inline float M_Cross2(vec2 a, vec2 b) {
	return a.x * b.y - a.y * b.x;
}

static inline vec2 M_Lerp2(vec2 a, vec2 b, float t) {
	return (1.f-t)*a + b*t;
}

// -- vec3 -------------------------------------------------

static inline vec3 operator+(vec3 a, vec3 b) {
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline vec3 operator-(vec3 a, vec3 b) {
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline vec3 operator*(vec3 a, vec3 b) {
	return {a.x * b.x, a.y * b.y, a.z * b.z};
}

static inline vec3 operator/(vec3 a, vec3 b) {
	return {a.x / b.x, a.y / b.y, a.z / b.z};
}

static inline vec3 operator*(float a, vec3 b) {
	return {a * b.x, a * b.y, a * b.z};
}

static inline vec3 operator*(vec3 a, float b) {
	return {a.x * b, a.y * b, a.z * b};
}

static inline vec3 operator/(vec3 a, float b) {
	float b_inv = 1.f / b;
	return {a.x * b_inv, a.y * b_inv, a.z * b_inv};
}

static inline float M_Dot3(vec3 a, vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float M_Len3(vec3 v) {
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline float M_LenSquared3(vec3 v) {
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

// Returns {1, 0, 0} if v is zero.
static inline vec3 M_Norm3(vec3 v) {
	float len_squared = v.x*v.x + v.y*v.y + v.z*v.z;
	return len_squared != 0.f ? v / sqrtf(len_squared) : vec3{1.f, 0.f, 0.f};
}

static inline vec3 M_Cross3(vec3 a, vec3 b) {
	return {
		a.y*b.z - a.z*b.y,
		a.z*b.x - a.x*b.z,
		a.x*b.y - a.y*b.x,
	};
}

static inline vec3 M_Lerp3(vec3 a, vec3 b, float t) {
	return (1.f-t)*a + b*t;
}

// -- vec4 -------------------------------------------------

static inline vec4 operator+(vec4 a, vec4 b) {
	return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

static inline vec4 operator-(vec4 a, vec4 b) {
	return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

static inline vec4 operator*(vec4 a, vec4 b) {
	return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

static inline vec4 operator/(vec4 a, vec4 b) {
	return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
}

static inline vec4 operator*(float a, vec4 b) {
	return {a * b.x, a * b.y, a * b.z, a * b.w};
}

static inline vec4 operator*(vec4 a, float b) {
	return {a.x * b, a.y * b, a.z * b, a.w * b};
}

static inline vec4 operator/(vec4 a, float b) {
	float b_inv = 1.f / b;
	return {a.x * b_inv, a.y * b_inv, a.z * b_inv, a.w * b_inv};
}

static inline float M_Dot4(vec4 a, vec4 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline float M_Len4(vec4 v) {
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
}

static inline float M_LenSquared4(vec4 v) {
	return v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w;
}

// Returns {1, 0, 0, 0} if v is zero.
static inline vec4 M_Norm4(vec4 v) {
	float len_squared = v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w;
	return len_squared != 0.f ? v / sqrtf(len_squared) : vec4{1.f, 0.f, 0.f, 0.f};
}

static inline vec4 M_Lerp4(vec4 a, vec4 b, float t) {
	return (1.f-t)*a + b*t;
}

// -- mat4 -------------------------------------------------

// Row-major matrix.
// The reason for picking row-major is so that we can write matrix literals like the following:
//   mat4 M = {
//      1, 0, 0, 0,
//      0, 1, 0, 0,
//      0, 0, 1, 0,
//      0, 0, 0, 1,
//   };
//
// It also makes indexing elements through the `_` member similar to the mathematical notation:
//    M._[row][column]
//
// When using row-major matrices, it's best to do matrix-vector multiplication
// in the order of (row_vector * matrix) as it's more cache-efficient compared to (matrix * column_vector).
// Had we chosen to use column-major matrices, it would be the other way around.
// 
union mat4 {
	float coef[16];
	float _[4][4];
	__m128 SSE_row[4];
};

static inline mat4 operator*(const mat4& a, const mat4& b) {
	// https://gist.github.com/rygorous/4172889
	
	__m128 a0 = a.SSE_row[0];
	__m128 r0 = _mm_mul_ps(_mm_shuffle_ps(a0, a0, 0x00), b.SSE_row[0]);
	r0 = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, 0x55), b.SSE_row[1]));
	r0 = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, 0xaa), b.SSE_row[2]));
	r0 = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, 0xff), b.SSE_row[3]));
	
	__m128 a1 = a.SSE_row[1];
	__m128 r1 = _mm_mul_ps(_mm_shuffle_ps(a1, a1, 0x00), b.SSE_row[0]);
	r1 = _mm_add_ps(r1, _mm_mul_ps(_mm_shuffle_ps(a1, a1, 0x55), b.SSE_row[1]));
	r1 = _mm_add_ps(r1, _mm_mul_ps(_mm_shuffle_ps(a1, a1, 0xaa), b.SSE_row[2]));
	r1 = _mm_add_ps(r1, _mm_mul_ps(_mm_shuffle_ps(a1, a1, 0xff), b.SSE_row[3]));
	
	__m128 a2 = a.SSE_row[2];
	__m128 r2 = _mm_mul_ps(_mm_shuffle_ps(a2, a2, 0x00), b.SSE_row[0]);
	r2 = _mm_add_ps(r2, _mm_mul_ps(_mm_shuffle_ps(a2, a2, 0x55), b.SSE_row[1]));
	r2 = _mm_add_ps(r2, _mm_mul_ps(_mm_shuffle_ps(a2, a2, 0xaa), b.SSE_row[2]));
	r2 = _mm_add_ps(r2, _mm_mul_ps(_mm_shuffle_ps(a2, a2, 0xff), b.SSE_row[3]));
	
	__m128 a3 = a.SSE_row[3];
	__m128 r3 = _mm_mul_ps(_mm_shuffle_ps(a3, a3, 0x00), b.SSE_row[0]);
	r3 = _mm_add_ps(r3, _mm_mul_ps(_mm_shuffle_ps(a3, a3, 0x55), b.SSE_row[1]));
	r3 = _mm_add_ps(r3, _mm_mul_ps(_mm_shuffle_ps(a3, a3, 0xaa), b.SSE_row[2]));
	r3 = _mm_add_ps(r3, _mm_mul_ps(_mm_shuffle_ps(a3, a3, 0xff), b.SSE_row[3]));
	
	mat4 result;
	result.SSE_row[0] = r0;
	result.SSE_row[1] = r1;
	result.SSE_row[2] = r2;
	result.SSE_row[3] = r3;
	return result;
}

// `a` is interpreted as a row vector
static inline vec4 operator*(const vec4& a, const mat4& b) {
	__m128 a0 = a.SSE;
	__m128 r0 = _mm_mul_ps(_mm_shuffle_ps(a0, a0, 0x00), b.SSE_row[0]);
	r0 = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, 0x55), b.SSE_row[1]));
	r0 = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, 0xaa), b.SSE_row[2]));
	r0 = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, 0xff), b.SSE_row[3]));
	
	vec4 result;
	result.SSE = r0;
	return result;
}

// ---------------------------------------------------------

// Assumes row-vectors and right-handed rotation
static mat4 M_MatRotateX(float angle) {
	__debugbreak();
	return {};
}

// Assumes row-vectors and right-handed rotation
static mat4 M_MatRotateY(float angle) {
	__debugbreak();
	return {};
}

// Assumes row-vectors and right-handed rotation
static mat4 M_MatRotateZ(float angle) {
	float cos_a = cosf(angle);
	float sin_a = sinf(angle);
	mat4 result = {
		 cos_a, sin_a,  0.f,  0.f,
		-sin_a, cos_a,  0.f,  0.f,
		   0.f,   0.f,  1.f,  0.f,
		   0.f,   0.f,  0.f,  1.f,
	};
	return result;
}

// Assumes row-vectors
static mat4 M_MatTranslate(vec3 v) {
	mat4 result = {
		1.f,  0.f,  0.f,  0.f,
		0.f,  1.f,  0.f,  0.f,
		0.f,  0.f,  1.f,  0.f,
		v.x,  v.y,  v.z,  1.f,
	};
	return result;
}

// Assumes row-vectors
static mat4 M_MatScale(vec3 s) {
	mat4 result = {
		s.x,  0.f,  0.f,  0.f,
		0.f,  s.y,  0.f,  0.f,
		0.f,  0.f,  s.z,  0.f,
		0.f,  0.f,  0.f,  1.f,
	};
	return result;
}