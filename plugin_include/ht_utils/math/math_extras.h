#ifndef MATH_CORE_INCLUDED
#error "math_core.h" must be included!
#endif

#pragma once
#define MATH_EXTRAS_INCLUDED

typedef struct M_PerspectiveCamera {
	vec3 position;
	mat4 ws_to_cs;
	mat4 cs_to_ws;
} M_PerspectiveCamera;

// Implicit plane; stores the coefficients A, B, C and D to the plane equation A*x + B*y + C*z + D = 0
typedef vec4 M_Plane;

// if from = -to, then fallback_axis will be used as the axis of rotation.
// static quat M_ShortestRotationBetweenUnitVectors(vec3 from, vec3 to, vec3 fallback_axis);

// static vec3 M_RotateV3(vec3 v, quat q);


// Make a perspective projection matrix which does no flips to the coordinate space in any way.
// Same as https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml,
// except that the Z is not flipped.
// static mat4 M_MakePerspectiveMatrix(float fov_y, float aspect_w_to_y, float near, float far);

static mat4 M_MakePerspectiveMatrixSimple(float d);

static M_Plane M_PlaneFromPointAndNormal(vec3 plane_p, vec3 plane_n);
static M_Plane M_FlipPlane(M_Plane plane);

static vec3 M_ProjectPointOntoPlane(vec3 p, vec3 plane_p, vec3 plane_n);

static vec3 M_ProjectPointOntoLine(vec3 p, vec3 line_p, vec3 line_dir);

static bool M_RayTriangleIntersect(vec3 ro, vec3 rd, vec3 a, vec3 b, vec3 c, float* out_t, float* out_hit_dir);
static bool M_RayPlaneIntersect(vec3 ro, vec3 rd, M_Plane plane, float* out_t, vec3* out_p);

// static M_PerspectiveCamera M_MakePerspectiveCamera(vec3 position, quat rotation,
//	float FOV_radians, float aspect_ratio_x_over_y, float z_near, float z_far);

// Terms:
// WS - world space
// VS - view space
// CS: Clip space
// SS: Screen space
// NDC: Normalized device coordinate space
// TODO: get rid of these!!!
static inline vec2 M_CSToSS(vec4 p_cs, vec2 screen_size);
static inline vec4 M_SSToCS(vec2 p_ss, vec2 screen_size_inv);
static inline vec2 M_NDCToSS(vec2 p_ndc, vec2 screen_size);
static inline vec2 M_SSToNDC(vec2 p_ss, vec2 screen_size_inv);

static vec3 M_RayDirectionFromSSPoint(const M_PerspectiveCamera* camera, vec2 p_ss, vec2 screen_size_inv);
static vec3 M_RayDirectionFromCSPoint(const M_PerspectiveCamera* camera, vec4 p_cs);

static bool M_GetPointScreenSpaceScale(const M_PerspectiveCamera* camera, vec3 p_ws, float* out_scale);

// Signed Distance Functions
static float M_DistanceToLineSegment2D(vec2 p, vec2 a, vec2 b);
static float M_DistanceToPolygon2D(vec2 p, const vec2* v, int v_count);

static float M_SignedDistanceToPlane(vec3 p, M_Plane plane);

// ---------------------------------------------

static mat4 M_MakePerspectiveMatrixSimple(float d) {
	mat4 result = {
		1.f,   0.f,   0.f,     0.f,
		0.f,   1.f,   0.f,     0.f,
		0.f,   0.f,   1.f, 1.f / d,
		0.f,   0.f,   0.f,     0.f,
	};
	return result;
}

/*static mat4 M_MakePerspectiveMatrix(float fov_y, float aspect_w_to_y, float near, float far) {
	ASSERT_COVERED(M_Perspective_RH_ZO);
	
	mat4 result = {0};

	float f = 1.0f / M_TanF(fov_y * 0.5f);
	result._[0][0] = f / aspect_w_to_y;
	result._[1][1] = f;
	result._[2][3] = 1.0f;
	
	result._[2][2] = (far) / (far - near);
	result._[3][2] = (near * far) / (far - near);

	return result;
}*/

static bool M_GetPointScreenSpaceScale(const M_PerspectiveCamera* camera, vec3 p, float* out_scale) {
	vec4 p_w1 = {p, 1.f};
	vec4 p_clip = p_w1 * camera->ws_to_cs;
	*out_scale = p_clip.w;
	return p_clip.z > 0;
}

static vec3 M_RayDirectionFromSSPoint(const M_PerspectiveCamera* camera, vec2 p_ss, vec2 screen_size_inv) {
	vec4 p_cs = M_SSToCS(p_ss, screen_size_inv);
	vec3 dir = M_RayDirectionFromCSPoint(camera, p_cs);
	return dir;
}

static vec3 M_RayDirectionFromCSPoint(const M_PerspectiveCamera* camera, vec4 p_cs) {
	vec4 point_in_front = p_cs * camera->cs_to_ws;
	point_in_front = point_in_front / point_in_front.w;

	// I guess we can derive position from the camera matrix...?
	HT_ASSERT(0); // TODO
	// vec3 dir = M_NormV3(M_SubV3(point_in_front.xyz, camera->position));
	// return dir;
	return {0, 0, 0};
}

static float M_DistanceToLineSegment2D(vec2 p, vec2 a, vec2 b) {
	// See sdSegment from https://iquilezles.org/articles/distfunctions2d/
	vec2 ap = p - a;
	vec2 ab = b - a;
	float h = M_Clamp(M_Dot2(ap, ab) / M_LenSquared2(ab), 0.f, 1.f);
	return M_Len2(ap - ab*h);
}

static float M_DistanceToPolygon2D(vec2 p, const vec2* v, int v_count) {
	// See sdPolygon from https://iquilezles.org/articles/distfunctions2d/
	HT_ASSERT(v_count >= 3);

	float d = M_Dot2(p - v[0], p - v[0]);
	float s = 1.f;
	for (int i=0, j=v_count-1; i<v_count; j=i, i++) {
		vec2 e = v[j] - v[i];
		vec2 w = p - v[i];
		vec2 b = w - e * M_Clamp(M_Dot2(w,e) / M_LenSquared2(e), 0.f, 1.f);
		
		d = M_Min(d, M_LenSquared2(b));
		
		bool cx = p.y >= v[i].y;
		bool cy = p.y < v[j].y;
		bool cz = e.x*w.y > e.y*w.x;
		if ((cx && cy && cz) || (!cx && !cy && !cz)) s *= -1.f;
	}
	return s * sqrtf(d);
}

static inline vec4 M_SSToCS(vec2 p_ss, vec2 screen_size_inv) {
	vec4 p_cs = {0.f, 0.f, 0.f, 1.f};
	p_cs.x = 2.f * p_ss.x * screen_size_inv.x - 1.f;
#ifdef M_CLIP_SPACE_INVERT_Y
	p_cs.y = 2.f - 2.f * p_ss.y * screen_size_inv.y - 1.f;
#else
	p_cs.y = 2.f * p_ss.y * screen_size_inv.y - 1.f;
#endif
	return p_cs;
}

static inline vec2 M_SSToNDC(vec2 p_ss, vec2 screen_size_inv) {
	vec2 p_ndc;
	p_ndc.x = 2.f * p_ss.x * screen_size_inv.x - 1.f;
#ifdef M_CLIP_SPACE_INVERT_Y
	p_ndc.y = 2.f - 2.f * p_ss.y * screen_size_inv.y - 1.f;
#else
	p_ndc.y = 2.f * p_ss.y * screen_size_inv.y - 1.f;
#endif
	return p_ndc;
}

static inline vec2 M_NDCToSS(vec2 p_ndc, vec2 screen_size) {
	vec2 p_ss;
	p_ss.x = (0.5f * p_ndc.x + 0.5f) * screen_size.x;
#ifdef M_CLIP_SPACE_INVERT_Y
	p_ss.y = (0.5f - 0.5f * p_ndc.y) * screen_size.y;
#else
	p_ss.y = (0.5f * p_ndc.y + 0.5f) * screen_size.y;
#endif
	return p_ss;
}

static inline vec2 M_CSToSS(vec4 p_cs, vec2 screen_size) {
	vec2 result;
	result.x = (0.5f * p_cs.x / p_cs.w + 0.5f) * screen_size.x;
#ifdef GIZMOS_CLIP_SPACE_INVERT_Y
	result.y = (0.5f - 0.5f * p_cs.y / p_cs.w) * screen_size.y;
#else
	result.y = (0.5f * p_cs.y / p_cs.w + 0.5f) * screen_size.y;
#endif
	return result;
}

static M_Plane M_FlipPlane(M_Plane plane) {
	M_Plane flipped = {-plane.x, -plane.y, -plane.z, -plane.w};
	return flipped;
}

static M_Plane M_PlaneFromPointAndNormal(vec3 plane_p, vec3 plane_n) {
	M_Plane p = {plane_n, -M_Dot3(plane_p, plane_n)};
	return p;
}

static vec3 M_ProjectPointOntoLine(vec3 p, vec3 line_p, vec3 line_dir) {
	float t = M_Dot3(p - line_p, line_dir);
	vec3 projected = line_p + line_dir * t;
	return projected;
}

static vec3 M_ProjectPointOntoPlane(vec3 p, vec3 plane_p, vec3 plane_n) {
	vec3 p_rel = p - plane_p;
	float d = M_Dot3(p_rel, plane_n);
	p_rel = p_rel - plane_n * d;
	return plane_p + p_rel;
}

/*static vec3 M_RotateV3(vec3 v, quat q) {
	// from https://stackoverflow.com/questions/44705398/about-glm-quaternion-rotation
	vec3 a = M_MulV3F(v, q.w);
	vec3 b = M_Cross(q.xyz, v);
	vec3 c = M_AddV3(b, a);
	vec3 d = M_Cross(q.xyz, c);
	return M_AddV3(v, M_MulV3F(d, 2.f));
}*/

static float M_SignedDistanceToPlane(vec3 p, M_Plane plane) {
	return M_Dot3(p, plane.xyz) + plane.w;
}

/*static quat M_ShortestRotationBetweenUnitVectors(vec3 from, vec3 to, vec3 fallback_axis) {
	float k_cos_theta = M_Dot3(from, to);
	float k = sqrtf(M_Dot3(from, from) * M_Dot3(to, to));
	quat q;
	if (k_cos_theta == -k) {
		q.xyz = fallback_axis;
		q.w = 0.f;
	}
	else {
		q.xyz = M_Cross(from, to);
		q.w = k_cos_theta + k;
		q = M_NormQ(q);
	}
	return q;
}*/

static bool M_RayTriangleIntersect(vec3 ro, vec3 rd, vec3 a, vec3 b, vec3 c, float* out_t, float* out_hit_dir) {
	// see https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
	vec3 edge1 = b - a;
	vec3 edge2 = c - a;
	vec3 ray_cross_e2 = M_Cross3(rd, edge2);
	float det = M_Dot3(edge1, ray_cross_e2);
	if (det == 0.f) return false;

	float inv_det = 1.f / det;
	vec3 s = ro - a;
	float u = inv_det * M_Dot3(s, ray_cross_e2);
	if (u < 0.f || u > 1.f) return false;

	vec3 s_cross_e1 = M_Cross3(s, edge1);
	float v = inv_det * M_Dot3(rd, s_cross_e1);

	if (v < 0.f || u + v > 1.f) return false;

	float t = inv_det * M_Dot3(edge2, s_cross_e1);
	*out_t = t;
	*out_hit_dir = det;
	return t > 0.f;
}

static bool M_RayPlaneIntersect(vec3 ro, vec3 rd, vec4 plane, float* out_t, vec3* out_p) {
	// We know that  ro + rd*t = p  AND  dot(p, plane.xyz) + plane.w = 0
	//
	//                 dot(ro + rd*t, plane.xyz) + plane.w = 0
	// dot(ro, plane.xyz) + dot(rd*t, plane.xyz) + plane.w = 0
	// dot(ro, plane.xyz) + dot(rd, plane.xyz)*t + plane.w = 0
	// t = (-plane.w - dot(ro, plane.xyz)) / dot(rd, plane.xyz)

	float t_num = -M_Dot3(ro, plane.xyz) - plane.w;
	float t_denom = M_Dot3(rd, plane.xyz);
	if (t_denom == 0.f) return false;

	float t = t_num / t_denom;
	if (out_t) *out_t = t;
	if (out_p) *out_p = ro + rd * t;
	return t > 0.f;
}
