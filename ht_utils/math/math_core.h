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

static inline void operator+=(vec2& a, vec2 b) { a = {a.x + b.x, a.y + b.y}; }
static inline vec2 operator+(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }

static inline void operator-=(vec2& a, vec2 b) { a = {a.x - b.x, a.y - b.y}; }
static inline vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }

static inline void operator*=(vec2& a, vec2 b) { a = {a.x * b.x, a.y * b.y}; }
static inline vec2 operator*(vec2 a, vec2 b) { return {a.x * b.x, a.y * b.y}; }

static inline void operator/=(vec2& a, vec2 b) { a = {a.x / b.x, a.y / b.y}; }
static inline vec2 operator/(vec2 a, vec2 b) { return {a.x / b.x, a.y / b.y}; }

static inline vec2 operator*(float a, vec2 b) { return {a * b.x, a * b.y}; }

static inline void operator*=(vec2& a, float b) { a = {a.x * b, a.y * b}; }
static inline vec2 operator*(vec2 a, float b) { return {a.x * b, a.y * b}; }

static inline vec2 operator/(vec2 a, float b) {
	float b_inv = 1.f / b;
	return {a.x * b_inv, a.y * b_inv};
}
static inline void operator/=(vec2& a, float b) { a = a / b; }

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

static inline void operator+=(vec3& a, vec3 b) { a = {a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline vec3 operator+(vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

static inline void operator-=(vec3& a, vec3 b) { a = {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline vec3 operator-(vec3 a, vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

static inline void operator*=(vec3& a, vec3 b) { a = {a.x * b.x, a.y * b.y, a.z * b.z}; }
static inline vec3 operator*(vec3 a, vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }

static inline void operator/=(vec3& a, vec3 b) { a = {a.x / b.x, a.y / b.y, a.z / b.z}; }
static inline vec3 operator/(vec3 a, vec3 b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }

static inline vec3 operator*(float a, vec3 b) { return {a * b.x, a * b.y, a * b.z}; }

static inline void operator*=(vec3& a, float b) { a = {a.x * b, a.y * b, a.z * b}; }
static inline vec3 operator*(vec3 a, float b) { return {a.x * b, a.y * b, a.z * b}; }

static inline vec3 operator/(vec3 a, float b) {
	float b_inv = 1.f / b;
	return {a.x * b_inv, a.y * b_inv, a.z * b_inv};
}
static inline void operator/=(vec3 a, float b) { a = a / b; }

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

static inline void operator+=(vec4& a, vec4 b) { a = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
static inline vec4 operator+(vec4 a, vec4 b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }

static inline void operator-=(vec4& a, vec4 b) { a = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
static inline vec4 operator-(vec4 a, vec4 b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }

static inline void operator*=(vec4& a, vec4 b) { a = {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }
static inline vec4 operator*(vec4 a, vec4 b) { return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }

static inline void operator/=(vec4& a, vec4 b) { a = {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w}; }
static inline vec4 operator/(vec4 a, vec4 b) { return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w}; }

static inline vec4 operator*(float a, vec4 b) { return {a * b.x, a * b.y, a * b.z, a * b.w}; }

static inline void operator*=(vec4& a, float b) { a = {a.x * b, a.y * b, a.z * b, a.w * b}; }
static inline vec4 operator*(vec4 a, float b) { return {a.x * b, a.y * b, a.z * b, a.w * b}; }

static inline vec4 operator/(vec4 a, float b) {
	float b_inv = 1.f / b;
	return {a.x * b_inv, a.y * b_inv, a.z * b_inv, a.w * b_inv};
}
static inline void operator/=(vec4& a, float b) { a = a / b; }

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
	vec4 row[4];
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
static inline void operator*=(mat4& a, const mat4& b) { a = a * b; }

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
static inline void operator*=(vec4& a, const mat4& b) { a = a * b; }

static inline mat4 M_Transpose4x4(mat4 M) {
	return {
		M.coef[0], M.coef[4], M.coef[8], M.coef[12],
		M.coef[1], M.coef[5], M.coef[9], M.coef[13],
		M.coef[2], M.coef[6], M.coef[10], M.coef[14],
		M.coef[3], M.coef[7], M.coef[11], M.coef[15],
	};
}

static mat4 M_Inverse4x4(mat4 M) { // https://github.com/niswegmann/small-matrix-inverse/blob/master/invert4x4_c.h
	mat4 result;
	float* src = M.coef;
	float* dst = result.coef;

	float det;

	/* Compute adjoint: */

	dst[0] =
		+ src[ 5] * src[10] * src[15]
		- src[ 5] * src[11] * src[14]
		- src[ 9] * src[ 6] * src[15]
		+ src[ 9] * src[ 7] * src[14]
		+ src[13] * src[ 6] * src[11]
		- src[13] * src[ 7] * src[10];

	dst[1] =
		- src[ 1] * src[10] * src[15]
		+ src[ 1] * src[11] * src[14]
		+ src[ 9] * src[ 2] * src[15]
		- src[ 9] * src[ 3] * src[14]
		- src[13] * src[ 2] * src[11]
		+ src[13] * src[ 3] * src[10];

	dst[2] =
		+ src[ 1] * src[ 6] * src[15]
		- src[ 1] * src[ 7] * src[14]
		- src[ 5] * src[ 2] * src[15]
		+ src[ 5] * src[ 3] * src[14]
		+ src[13] * src[ 2] * src[ 7]
		- src[13] * src[ 3] * src[ 6];

	dst[3] =
		- src[ 1] * src[ 6] * src[11]
		+ src[ 1] * src[ 7] * src[10]
		+ src[ 5] * src[ 2] * src[11]
		- src[ 5] * src[ 3] * src[10]
		- src[ 9] * src[ 2] * src[ 7]
		+ src[ 9] * src[ 3] * src[ 6];

	dst[4] =
		- src[ 4] * src[10] * src[15]
		+ src[ 4] * src[11] * src[14]
		+ src[ 8] * src[ 6] * src[15]
		- src[ 8] * src[ 7] * src[14]
		- src[12] * src[ 6] * src[11]
		+ src[12] * src[ 7] * src[10];

	dst[5] =
		+ src[ 0] * src[10] * src[15]
		- src[ 0] * src[11] * src[14]
		- src[ 8] * src[ 2] * src[15]
		+ src[ 8] * src[ 3] * src[14]
		+ src[12] * src[ 2] * src[11]
		- src[12] * src[ 3] * src[10];

	dst[6] =
		- src[ 0] * src[ 6] * src[15]
		+ src[ 0] * src[ 7] * src[14]
		+ src[ 4] * src[ 2] * src[15]
		- src[ 4] * src[ 3] * src[14]
		- src[12] * src[ 2] * src[ 7]
		+ src[12] * src[ 3] * src[ 6];

	dst[7] =
		+ src[ 0] * src[ 6] * src[11]
		- src[ 0] * src[ 7] * src[10]
		- src[ 4] * src[ 2] * src[11]
		+ src[ 4] * src[ 3] * src[10]
		+ src[ 8] * src[ 2] * src[ 7]
		- src[ 8] * src[ 3] * src[ 6];

	dst[8] =
		+ src[ 4] * src[ 9] * src[15]
		- src[ 4] * src[11] * src[13]
		- src[ 8] * src[ 5] * src[15]
		+ src[ 8] * src[ 7] * src[13]
		+ src[12] * src[ 5] * src[11]
		- src[12] * src[ 7] * src[ 9];

	dst[9] =
		- src[ 0] * src[ 9] * src[15]
		+ src[ 0] * src[11] * src[13]
		+ src[ 8] * src[ 1] * src[15]
		- src[ 8] * src[ 3] * src[13]
		- src[12] * src[ 1] * src[11]
		+ src[12] * src[ 3] * src[ 9];

	dst[10] =
		+ src[ 0] * src[ 5] * src[15]
		- src[ 0] * src[ 7] * src[13]
		- src[ 4] * src[ 1] * src[15]
		+ src[ 4] * src[ 3] * src[13]
		+ src[12] * src[ 1] * src[ 7]
		- src[12] * src[ 3] * src[ 5];

	dst[11] =
		- src[ 0] * src[ 5] * src[11]
		+ src[ 0] * src[ 7] * src[ 9]
		+ src[ 4] * src[ 1] * src[11]
		- src[ 4] * src[ 3] * src[ 9]
		- src[ 8] * src[ 1] * src[ 7]
		+ src[ 8] * src[ 3] * src[ 5];

	dst[12] =
		- src[ 4] * src[ 9] * src[14]
		+ src[ 4] * src[10] * src[13]
		+ src[ 8] * src[ 5] * src[14]
		- src[ 8] * src[ 6] * src[13]
		- src[12] * src[ 5] * src[10]
		+ src[12] * src[ 6] * src[ 9];

	dst[13] =
		+ src[ 0] * src[ 9] * src[14]
		- src[ 0] * src[10] * src[13]
		- src[ 8] * src[ 1] * src[14]
		+ src[ 8] * src[ 2] * src[13]
		+ src[12] * src[ 1] * src[10]
		- src[12] * src[ 2] * src[ 9];

	dst[14] =
		- src[ 0] * src[ 5] * src[14]
		+ src[ 0] * src[ 6] * src[13]
		+ src[ 4] * src[ 1] * src[14]
		- src[ 4] * src[ 2] * src[13]
		- src[12] * src[ 1] * src[ 6]
		+ src[12] * src[ 2] * src[ 5];

	dst[15] =
		+ src[ 0] * src[ 5] * src[10]
		- src[ 0] * src[ 6] * src[ 9]
		- src[ 4] * src[ 1] * src[10]
		+ src[ 4] * src[ 2] * src[ 9]
		+ src[ 8] * src[ 1] * src[ 6]
		- src[ 8] * src[ 2] * src[ 5];

	/* Compute determinant: */

	det = + src[0] * dst[0] + src[1] * dst[4] + src[2] * dst[8] + src[3] * dst[12];

	/* Multiply adjoint with reciprocal of determinant: */

	det = 1.0f / det;

	dst[ 0] *= det;
	dst[ 1] *= det;
	dst[ 2] *= det;
	dst[ 3] *= det;
	dst[ 4] *= det;
	dst[ 5] *= det;
	dst[ 6] *= det;
	dst[ 7] *= det;
	dst[ 8] *= det;
	dst[ 9] *= det;
	dst[10] *= det;
	dst[11] *= det;
	dst[12] *= det;
	dst[13] *= det;
	dst[14] *= det;
	dst[15] *= det;
	
	return result;
}
/*static inline mat4 M_Inverse4x4(mat4 M) {
	// https://github.com/HandmadeMath/HandmadeMath/blob/bdc7dd2a516b08715a56f8b8eecefe44c9d68f40/HandmadeMath.h#L1699C1-L1699C8
	M = M_Transpose4x4(M);

	vec3 C01 = M_Cross3(M.row[0].xyz, M.row[1].xyz);
	vec3 C23 = M_Cross3(M.row[2].xyz, M.row[3].xyz);
	vec3 B10 = M.row[0].xyz * M.row[1].w - M.row[1].xyz * M.row[0].w;
	vec3 B32 = M.row[2].xyz * M.row[3].w - M.row[3].xyz * M.row[2].w;

	float InvDeterminant = 1.0f / (M_Dot3(C01, B32) + M_Dot3(C23, B10));
	C01 = C01 * InvDeterminant;
	C23 = C23 * InvDeterminant;
	B10 = B10 * InvDeterminant;
	B32 = B32 * InvDeterminant;

	mat4 Result;
	Result.row[0] = vec4{ M_Cross3(M.row[1].xyz, B32) + C23 * M.row[1].w, -M_Dot3(M.row[1].xyz, C23) };
	Result.row[1] = vec4{ M_Cross3(B32, M.row[0].xyz) + C23 * M.row[0].w, +M_Dot3(M.row[0].xyz, C23) };
	Result.row[2] = vec4{ M_Cross3(M.row[3].xyz, B10) + C01 * M.row[3].w, -M_Dot3(M.row[3].xyz, C01) };
	Result.row[3] = vec4{ M_Cross3(B10, M.row[2].xyz) + C01 * M.row[2].w, +M_Dot3(M.row[2].xyz, C01) };

	//return M_Transpose4x4(Result);
	return Result; //return M_Transpose4x4(Result);
}*/

// ---------------------------------------------------------

// Assumes row-vectors and right-handed rotation
static mat4 M_MatRotateX(float angle) {
		float cos_a = cosf(angle);
		float sin_a = sinf(angle);
		mat4 result = {
			1.f,    0.f,    0.f,  0.f,
			0.f,  cos_a,  sin_a,  0.f,
			0.f, -sin_a,  cos_a,  0.f,
			0.f,    0.f,    0.f,  1.f,
		};
		return result;
}

// Assumes row-vectors and right-handed rotation
static mat4 M_MatRotateY(float angle) {
	float cos_a = cosf(angle);
	float sin_a = sinf(angle);
	mat4 result = {
		cos_a,  0.f, -sin_a,  0.f,
		  0.f,  1.f,    0.f,  0.f,
		sin_a,  0.f,  cos_a,  0.f,
		  0.f,  0.f,    0.f,  1.f,
	};
	return result;
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