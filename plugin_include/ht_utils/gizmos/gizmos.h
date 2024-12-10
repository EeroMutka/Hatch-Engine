#pragma once

// TODO: Plugin utility libraries should be implemented in a .cpp file to allow for math operator overloading,
// but still allow using the library from C.

#ifndef MATH_EXTRAS_INCLUDED
#error "math_extras.h" must be included before this file!
#endif

#ifndef FIRE_UI_INCLUDED
#error "fire_ui.h" must be included before this file!
#endif

#define GIZMOS_API static

typedef enum TranslationAxis {
	TranslationAxis_None,
	TranslationAxis_X,
	TranslationAxis_Y,
	TranslationAxis_Z,
	TranslationAxis_YZ, // perpendicular to X
	TranslationAxis_ZX, // perpendicular to Y
	TranslationAxis_XY, // perpendicular to Z
} TranslationAxis;

typedef struct TranslationGizmo {
	TranslationAxis hovered_axis_;
	TranslationAxis moving_axis_;
	
	bool plane_gizmo_is_implicitly_hovered;

	float arrow_scale;
	vec3 origin;

	float plane_gizmo_visibility[4];
	vec2 plane_gizmo_quads[3][4];

	// linear movement
	//vec2 moving_axis_arrow_end_ss;
	vec2 moving_begin_mouse_pos;
	vec2 moving_begin_origin_ss;
	vec3 moving_begin_translation;
	vec3 moving_axis_arrow_end;
	
	// planar movement
	vec2 planar_moving_begin_mouse_pos;
	vec2 planar_moving_begin_origin_ss;
	vec3 planar_moving_begin_translation;
} TranslationGizmo;

#define ROTATION_GIZMO_POINTS_COUNT 28

/*typedef struct RotationGizmo {
	int hovered_axis; // 0 = none, 1 = x, 2 = y, 3 = z
	int dragging_axis; // 0 = none, 1 = x, 2 = y, 3 = z

	vec3 origin;

	vec2 points[3][ROTATION_GIZMO_POINTS_COUNT];
	bool points_loop[3];

	vec2 drag_start_mouse_pos;
	quat drag_start_rotation;
} RotationGizmo;*/

GIZMOS_API void DrawPoint3D(const M_PerspectiveView* view, vec3 p, float thickness, UI_Color color);
GIZMOS_API void DrawGrid3D(const M_PerspectiveView* view, UI_Color color);
GIZMOS_API void DrawGridEx3D(const M_PerspectiveView* view, vec3 origin, vec3 x_dir, vec3 y_dir, UI_Color color);
GIZMOS_API void DrawLine3D(const M_PerspectiveView* view, vec3 a, vec3 b, float thickness, UI_Color color);
GIZMOS_API void DrawCuboid3D(const M_PerspectiveView* view, vec3 min, vec3 max, float thickness, UI_Color color);
GIZMOS_API void DrawParallelepiped3D(const M_PerspectiveView* view, vec3 min_corner, vec3 extent_x, vec3 extent_y, vec3 extent_z, float thickness, UI_Color color);
GIZMOS_API void DrawArrow3D(const M_PerspectiveView* view, vec3 from, vec3 to, float head_length, float head_radius, int vertices, float thickness, UI_Color color);

// NOTE: These will not be clipped to the viewport properly if the shape is too big. The shape should always be either fully visible or fully out of the screen.
//       I should try to fix this.
GIZMOS_API void DrawQuad3D(const M_PerspectiveView* view, vec3 a, vec3 b, vec3 c, vec3 d, UI_Color color);
GIZMOS_API void DrawQuadFrame3D(const M_PerspectiveView* view, vec3 a, vec3 b, vec3 c, vec3 d, float thickness, UI_Color color);

//GIZMOS_API bool TranslationGizmoShouldApply(const TranslationGizmo* gizmo);
GIZMOS_API void TranslationGizmoUpdate(const HT_InputFrame* in, const M_PerspectiveView* view, TranslationGizmo* gizmo, vec2 mouse_pos, vec3* translation, float snap_size);
GIZMOS_API void TranslationGizmoDraw(const M_PerspectiveView* view, const TranslationGizmo* gizmo);

// GIZMOS_API bool RotationGizmoShouldApply(const RotationGizmo* gizmo);
// GIZMOS_API void RotationGizmoUpdate(const M_PerspectiveView* view, RotationGizmo* gizmo, vec3 origin, quat* rotation);
// GIZMOS_API void RotationGizmoDraw(const M_PerspectiveView* view, const RotationGizmo* gizmo);

typedef struct LineTranslation {
	vec3 line_dir;
	vec2 moving_begin_mouse_pos;
	vec2 moving_begin_origin_ss;
	vec3 moving_begin_translation;
	vec3 moving_axis_arrow_end;
} LineTranslation;

GIZMOS_API void LineTranslationBegin(LineTranslation* translation, const M_PerspectiveView* view, vec2 mouse_pos, vec3 line_p, vec3 line_dir);

GIZMOS_API void LineTranslationUpdate(const HT_InputFrame* in, LineTranslation* translation, const M_PerspectiveView* view, vec3* point, vec2 mouse_pos, float snap_size);

// ----------------------------------------------------------

//GIZMOS_API bool TranslationGizmoShouldApply(const TranslationGizmo* gizmo) {
//	return gizmo->moving_axis_ != 0 && !InputIsDown(in, HT_InputKey_MouseLeft);
//}

static void LineTranslationBegin(LineTranslation* translation, const M_PerspectiveView* view, vec2 mouse_pos, vec3 point, vec3 line_dir)
{
	vec4 origin_ss = vec4{point, 1.f} * view->ws_to_ss;
	origin_ss.xy /= origin_ss.w;

	translation->line_dir = line_dir;
	translation->moving_begin_mouse_pos = mouse_pos;
	translation->moving_begin_translation = point;
	translation->moving_begin_origin_ss = origin_ss.xy;
	//gizmo->moving_axis_arrow_end_ss = arrow_end_point_ss[min_distance_axis];
	translation->moving_axis_arrow_end = point + line_dir * 0.001f;
}

static void SnapPointToGrid(vec3* point, vec3 grid_origin, float snap_size, int axis) {
	float p_relative = point->_[axis] - grid_origin._[axis];
	p_relative = roundf(p_relative / snap_size) * snap_size;
	point->_[axis] = grid_origin._[axis] + p_relative;
}

static void SnapPointToGrid2(vec3* point, vec3 grid_origin, float snap_size, vec3 axis) {
	float p_relative = M_Dot3(*point - grid_origin, axis);
	float p_relative_delta = roundf(p_relative / snap_size) * snap_size - p_relative;
	*point += axis * p_relative_delta;
}

GIZMOS_API void LineTranslationUpdate(const HT_InputFrame* in, LineTranslation* translation, const M_PerspectiveView* view, vec3* point, vec2 mouse_pos, float snap_size)
{
	vec2 movement_delta = mouse_pos - translation->moving_begin_mouse_pos;

	vec4 moving_begin_origin_ss = vec4{translation->moving_begin_translation, 1.f} * view->ws_to_ss;
	moving_begin_origin_ss.xy /= moving_begin_origin_ss.w;
	
	vec4 moving_begin_arrow_end_ss = vec4{translation->moving_axis_arrow_end, 1.f} * view->ws_to_ss;
	moving_begin_arrow_end_ss.xy /= moving_begin_arrow_end_ss.w;

	vec2 arrow_direction = moving_begin_arrow_end_ss.xy - moving_begin_origin_ss.xy;
	float arrow_direction_len = M_Len2(arrow_direction);
	arrow_direction = arrow_direction_len == 0.f ?
		vec2{1, 0} :
		arrow_direction / arrow_direction_len;

	float t = M_Dot2(arrow_direction, movement_delta);
	t -= M_Dot2(arrow_direction, moving_begin_origin_ss.xy - translation->moving_begin_origin_ss); // account for camera movement

	vec2 movement_delta_projected = arrow_direction * t;
	vec2 moved_pos_ss = moving_begin_origin_ss.xy + movement_delta_projected;

	vec3 moved_pos_ray_dir = M_RayDirectionFromSSPoint(view, moved_pos_ss);

	vec3 plane_tangent = M_Cross3(moved_pos_ray_dir, translation->line_dir);
	vec3 plane_normal = M_Cross3(plane_tangent, translation->line_dir);

	// Make a plane perpendicular to the line axis
	vec4 plane;
	plane.xyz = plane_normal;
	plane.w = -M_Dot3(plane_normal, translation->moving_begin_translation);
	vec3 intersection_pos;

	HT_ASSERT(view->has_camera_position);
	M_RayPlaneIntersect(view->camera_position, moved_pos_ray_dir, plane, NULL, &intersection_pos);

	// The intersection position is roughly correct, but may not be entirely accurate. For accuracy, let's project the intersection pos onto the translation line.
	intersection_pos = M_ProjectPointOntoLine(intersection_pos, translation->moving_begin_translation, translation->line_dir);

	if (snap_size > 0) {
		vec3 snap_grid_origin = InputIsDown(in, HT_InputKey_Alt) ? vec3{} : translation->moving_begin_translation;
		SnapPointToGrid2(&intersection_pos, snap_grid_origin, snap_size, translation->line_dir);
	}

	*point = intersection_pos;
}

GIZMOS_API void TranslationGizmoUpdate(const HT_InputFrame* in, const M_PerspectiveView* view, TranslationGizmo* gizmo, vec2 mouse_pos, vec3* translation, float snap_size) {
	HT_ASSERT(view->has_camera_position);

	gizmo->hovered_axis_ = TranslationAxis_None;
	gizmo->plane_gizmo_is_implicitly_hovered = false;
	
	vec4 origin = vec4{*translation, 1.f};
	
	// Update planar movement
	if (gizmo->moving_axis_ >= TranslationAxis_YZ && gizmo->moving_axis_ <= TranslationAxis_XY) {
		// For planar movement, we want to calculate the movement delta in 2D, then add that to the screen-space origin to find the 3D ray direction,
		// then find the ray-plane intersection and that's the final position

		vec2 movement_delta = mouse_pos - gizmo->planar_moving_begin_mouse_pos;
		vec2 moved_pos_ss = gizmo->planar_moving_begin_origin_ss + movement_delta;

		vec3 moved_pos_ray_dir = M_RayDirectionFromSSPoint(view, moved_pos_ss);

		vec3 plane_normal = {0};
		plane_normal._[gizmo->moving_axis_ - TranslationAxis_YZ] = 1.f;

		// Make a plane perpendicular to the line axis
		vec4 plane;
		plane.xyz = plane_normal;
		plane.w = -M_Dot3(plane_normal, gizmo->planar_moving_begin_translation);
		vec3 intersection_pos;

		HT_ASSERT(view->has_camera_position);
		M_RayPlaneIntersect(view->camera_position, moved_pos_ray_dir, plane, NULL, &intersection_pos);
		
		if (snap_size > 0 && !InputIsDown(in, HT_InputKey_Control)) {
			vec3 snap_grid_origin = InputIsDown(in, HT_InputKey_Alt) ? vec3{} : gizmo->planar_moving_begin_translation;
			for (int i = 1; i <= 2; i++) {
				SnapPointToGrid(&intersection_pos, snap_grid_origin, snap_size, (gizmo->moving_axis_ - TranslationAxis_YZ + i) % 3);
			}
		}

		origin.xyz = intersection_pos;
	}

	// Update linear movement
	// TODO: use LineTranslation API
	if (gizmo->moving_axis_ >= TranslationAxis_X && gizmo->moving_axis_ <= TranslationAxis_Z) {
		vec2 movement_delta = mouse_pos - gizmo->moving_begin_mouse_pos;

		vec4 moving_begin_origin_ss = vec4{gizmo->moving_begin_translation, 1.f} * view->ws_to_ss;
		moving_begin_origin_ss.xy /= moving_begin_origin_ss.w;

		vec4 moving_begin_arrow_end_ss = vec4{gizmo->moving_axis_arrow_end, 1.f} * view->ws_to_ss;
		moving_begin_arrow_end_ss.xy /= moving_begin_arrow_end_ss.w;

		vec2 arrow_direction = moving_begin_arrow_end_ss.xy - moving_begin_origin_ss.xy;
		arrow_direction = M_Norm2(arrow_direction);

		float t = M_Dot2(arrow_direction, movement_delta);
		t -= M_Dot2(arrow_direction, moving_begin_origin_ss.xy - gizmo->moving_begin_origin_ss); // account for camera movement
		
		vec2 movement_delta_projected = arrow_direction * t;
		vec2 moved_pos_ss = moving_begin_origin_ss.xy + movement_delta_projected;

		vec3 axis = {0.f, 0.f, 0.f};
		axis._[gizmo->moving_axis_ - TranslationAxis_X] = 1.f;

		vec3 moved_pos_ray_dir = M_RayDirectionFromSSPoint(view, moved_pos_ss);

		vec3 plane_tangent = M_Cross3(moved_pos_ray_dir, axis);
		vec3 plane_normal = M_Cross3(plane_tangent, axis);

		// Make a plane perpendicular to the line axis
		vec4 plane;
		plane.xyz = plane_normal;
		plane.w = -M_Dot3(plane_normal, gizmo->moving_begin_translation);
		vec3 intersection_pos;
		M_RayPlaneIntersect(view->camera_position, moved_pos_ray_dir, plane, NULL, &intersection_pos);
		
		if (snap_size > 0.f && !InputIsDown(in, HT_InputKey_Control)) {
			vec3 snap_grid_origin = InputIsDown(in, HT_InputKey_Alt) ? vec3{} : gizmo->moving_begin_translation;
			SnapPointToGrid(&intersection_pos, snap_grid_origin, snap_size, gizmo->moving_axis_ - TranslationAxis_X);
		}
		
		origin.xyz = intersection_pos;
	}

	vec3 origin_dir_from_camera = M_Norm3(origin.xyz - view->camera_position);

	M_GetPointScreenSpaceScale(view, origin.xyz, &gizmo->arrow_scale);
	gizmo->arrow_scale *= 0.15f;
	
	vec4 origin_ss = origin * view->ws_to_ss;
	origin_ss.xy /= origin_ss.w;
	
	float min_distance = 10000000.f;
	int min_distance_axis = -1;
	vec2 arrow_end_point_ss[3];
	vec3 arrow_end_point[3];

	//mouse_pos = {920.f, 306.f};
	for (int i = 0; i < 3; i++) {
		vec3 arrow_end = origin.xyz;
		arrow_end._[i] += gizmo->arrow_scale;
		arrow_end_point[i] = arrow_end;

		vec4 b_ss = vec4{arrow_end, 1.f} * view->ws_to_ss;

		if (b_ss.z > 0.f) {
			b_ss.xy /= b_ss.w;
			arrow_end_point_ss[i] = b_ss.xy;
			float d = M_DistanceToLineSegment2D(mouse_pos, origin_ss.xy, arrow_end_point_ss[i]);
			if (d < min_distance) {
				min_distance = d;
				min_distance_axis = i;
			}
		}
	}
	
	float plane_gizmos_min_distance = 10000000.f;
	int plane_gizmos_min_dist_axis = -1;

	// plane gizmos
	{
		vec3 o = origin.xyz;
		float k = 0.7f*gizmo->arrow_scale; // inner offset
		float l = 0.8f*gizmo->arrow_scale; // outer offset

		vec4 quads[3][4] = {
			{{o.x, o.y+k, o.z+k, 1.f}, {o.x, o.y+k, o.z+l, 1.f}, {o.x, o.y+l, o.z+l, 1.f}, {o.x, o.y+l, o.z+k, 1.f}}, // YZ-plane
			{{o.x+k, o.y, o.z+k, 1.f}, {o.x+l, o.y, o.z+k, 1.f}, {o.x+l, o.y, o.z+l, 1.f}, {o.x+k, o.y, o.z+l, 1.f}}, // ZX-plane
			{{o.x+k, o.y+k, o.z, 1.f}, {o.x+k, o.y+l, o.z, 1.f}, {o.x+l, o.y+l, o.z, 1.f}, {o.x+l, o.y+k, o.z, 1.f}}, // XY-plane
		};

		for (int i = 0; i < 3; i++) {
			vec4 quad_ss[4] = {
				quads[i][0] * view->ws_to_ss,
				quads[i][1] * view->ws_to_ss,
				quads[i][2] * view->ws_to_ss,
				quads[i][3] * view->ws_to_ss,
			};
			
			vec3 plane_normal = {0};
			plane_normal._[i] = 1.f;
			float view_dot = M_Dot3(origin_dir_from_camera, plane_normal);
			
			float visibility = M_Clamp(fabsf(view_dot)*8.f - 1.f, 0.f, 1.f);
			
			bool in_view = quad_ss[0].z > 0.f && quad_ss[1].z > 0.f && quad_ss[2].z > 0.f && quad_ss[3].z > 0.f;
			if (!in_view) visibility = 0.f;
			
			gizmo->plane_gizmo_visibility[i] = visibility;
			if (in_view) {
				gizmo->plane_gizmo_quads[i][0] = quad_ss[0].xy / quad_ss[0].w;
				gizmo->plane_gizmo_quads[i][1] = quad_ss[1].xy / quad_ss[1].w;
				gizmo->plane_gizmo_quads[i][2] = quad_ss[2].xy / quad_ss[2].w;
				gizmo->plane_gizmo_quads[i][3] = quad_ss[3].xy / quad_ss[3].w;
				
				float dist = M_DistanceToPolygon2D(mouse_pos, gizmo->plane_gizmo_quads[i], 4);
				if (dist < plane_gizmos_min_distance) {
					plane_gizmos_min_distance = dist;
					plane_gizmos_min_dist_axis = i;
				}
			}
		}
	}

	if (min_distance < 10.f) {
		gizmo->hovered_axis_ = (TranslationAxis)(TranslationAxis_X + min_distance_axis);

		if (InputWentDown(in, HT_InputKey_MouseLeft)) { // Begin press?
			gizmo->moving_axis_ = gizmo->hovered_axis_;
			gizmo->moving_begin_mouse_pos = mouse_pos;
			gizmo->moving_begin_translation = origin.xyz;
			gizmo->moving_begin_origin_ss = origin_ss.xy;
			//gizmo->moving_axis_arrow_end_ss = arrow_end_point_ss[min_distance_axis];
			gizmo->moving_axis_arrow_end = arrow_end_point[min_distance_axis];
		}
	}
	else {
		if (plane_gizmos_min_distance < 10.f) {
			gizmo->hovered_axis_ = (TranslationAxis)(TranslationAxis_YZ + plane_gizmos_min_dist_axis);
		}
		else {
			/*vec3 forward = view->forward_dir;
			vec3 forward_abs = {M_ABS(forward.x), M_ABS(forward.y), M_ABS(forward.z)};

			gizmo->hovered_axis_ = TranslationAxis_YZ;
			float max_axis_value = forward_abs.x;
			if (forward_abs.y > max_axis_value) {
				max_axis_value = forward_abs.y;
				gizmo->hovered_axis_ = TranslationAxis_ZX;
			}
			if (forward_abs.z > max_axis_value) {
				max_axis_value = forward_abs.z;
				gizmo->hovered_axis_ = TranslationAxis_XY;
			}
			
			gizmo->plane_gizmo_is_implicitly_hovered = true;*/
		}
		
		if (InputWentDown(in, HT_InputKey_MouseLeft)) { // Begin press?
			gizmo->moving_axis_ = gizmo->hovered_axis_;
			gizmo->planar_moving_begin_mouse_pos = mouse_pos;
			gizmo->planar_moving_begin_translation = origin.xyz;
			gizmo->planar_moving_begin_origin_ss = origin_ss.xy;
		}
	}
	
	if (!InputIsDown(in, HT_InputKey_MouseLeft)) {
		gizmo->moving_axis_ = TranslationAxis_None;
	}
	
	gizmo->origin = origin.xyz;
	*translation = origin.xyz;
}

GIZMOS_API void TranslationGizmoDraw(const M_PerspectiveView* view, const TranslationGizmo* gizmo) {
	TranslationAxis active_axis = gizmo->moving_axis_ ? gizmo->moving_axis_ : gizmo->hovered_axis_;
	bool x_active = active_axis == TranslationAxis_X;
	bool y_active = active_axis == TranslationAxis_Y;
	bool z_active = active_axis == TranslationAxis_Z;
	
	UI_Color nonhovered_colors[] = {UI_COLOR{225, 50, 10, 255}, UI_COLOR{50, 225, 10, 255}, UI_COLOR{10, 120, 225, 255}};
	DrawArrow3D(view, gizmo->origin, gizmo->origin + vec3{gizmo->arrow_scale, 0.f, 0.f}, 0.03f, 0.012f, 12, x_active ? 3.f : 3.f, x_active ? UI_YELLOW : nonhovered_colors[0]);
	DrawArrow3D(view, gizmo->origin, gizmo->origin + vec3{0.f, gizmo->arrow_scale, 0.f}, 0.03f, 0.012f, 12, y_active ? 3.f : 3.f, y_active ? UI_YELLOW : nonhovered_colors[1]);
	DrawArrow3D(view, gizmo->origin, gizmo->origin + vec3{0.f, 0.f, gizmo->arrow_scale}, 0.03f, 0.012f, 12, z_active ? 3.f : 3.f, z_active ? UI_YELLOW : nonhovered_colors[2]);

	UI_Color inner_colors[] = {{255, 30, 30, 130+50}, {30, 255, 30, 130+50}, {60, 60, 255, 180+50}};

	for (int i = 0; i < 3; i++) {
		if (gizmo->plane_gizmo_visibility[i] > 0.f) {
			bool active = active_axis == TranslationAxis_YZ + i;
			bool hovered_implicitly = active && gizmo->plane_gizmo_is_implicitly_hovered;

			UI_Color frame_color = active ? UI_YELLOW : nonhovered_colors[i];
			UI_Color inner_color = frame_color;
			
			float frame_width = active ? 4.f : 2.f;
			if (hovered_implicitly) frame_width = 2.f;

			inner_color.a = (uint8_t)((float)inner_color.a * 0.5f * gizmo->plane_gizmo_visibility[i]);
			frame_color.a = (uint8_t)((float)frame_color.a * gizmo->plane_gizmo_visibility[i]);

			vec2 q[] = {
				UI_VEC2{gizmo->plane_gizmo_quads[i][0].x, gizmo->plane_gizmo_quads[i][0].y},
				UI_VEC2{gizmo->plane_gizmo_quads[i][1].x, gizmo->plane_gizmo_quads[i][1].y},
				UI_VEC2{gizmo->plane_gizmo_quads[i][2].x, gizmo->plane_gizmo_quads[i][2].y},
				UI_VEC2{gizmo->plane_gizmo_quads[i][3].x, gizmo->plane_gizmo_quads[i][3].y},
			};
			UI_DrawQuad(q[0], q[1], q[2], q[3], inner_color);

			UI_DrawLine(q[0], q[1], frame_width, frame_color);
			UI_DrawLine(q[1], q[2], frame_width, frame_color);
			UI_DrawLine(q[2], q[3], frame_width, frame_color);
			UI_DrawLine(q[3], q[0], frame_width, frame_color);
			
		}
	}
}

#if 0

GIZMOS_API bool RotationGizmoShouldApply(const RotationGizmo* gizmo) {
	return gizmo->dragging_axis != 0 && !InputIsDown(in, HT_InputKey_MouseLeft);
}

GIZMOS_API void RotationGizmoUpdate(const M_PerspectiveView* view, RotationGizmo* gizmo, vec3 origin, quat* rotation) {
	gizmo->hovered_axis = 0;
	//M_Mat4 rotation_mat4 = M_QToM4(*rotation);
	//vec3 X = rotation_mat4.Columns[0].xyz;
	//vec3 Y = rotation_mat4.Columns[1].xyz;
	//vec3 Z = rotation_mat4.Columns[2].xyz;

	float scale;
	if (M_GetPointScreenSpaceScale(&view->camera, origin, &scale)) {
		scale *= 0.15f;
		
		vec3 camera_to_origin = M_Norm3(M_SubV3(origin, view->position));
		vec4 sphere_half_plane;
		sphere_half_plane.xyz = camera_to_origin;
		sphere_half_plane.w = -M_Dot3(camera_to_origin, origin);

		float min_dist = 10000000.f;
		int min_dist_axis = 0;

		for (int z_axis_i = 0; z_axis_i < 3; z_axis_i++) {
			int x_axis_i = (z_axis_i + 1) % 3;
			int y_axis_i = (z_axis_i + 2) % 3;

			vec3 x_axis_dir = {0}, y_axis_dir = {0}, z_axis_dir = {0};
			x_axis_dir._[x_axis_i] = 1.f;
			y_axis_dir._[y_axis_i] = 1.f;
			z_axis_dir._[z_axis_i] = 1.f;

			// Find the intersecting line between the axis plane and the sphere_half_plane, then to get the start/end points
			// of the half-circle, just move forward one unit along that line direction.
			vec3 intersecting_line_dir = M_Norm3(M_Cross3(z_axis_dir, camera_to_origin));
			float p0_x = M_Dot3(intersecting_line_dir, x_axis_dir);
			float p0_y = M_Dot3(intersecting_line_dir, y_axis_dir);
			float p0_theta = atan2f(p0_y, p0_x);
			if (isnan(p0_theta)) p0_theta = 0.f;

			float dot = M_Dot3(camera_to_origin, z_axis_dir);
			float dot2 = dot*dot;
			float dot4 = dot2*dot2;
			float dot8 = dot4*dot4;

			float extra_half_circle_length = M_PI32 * M_MIN(dot8*dot8 + 0.05f, 1.f);
			float half_circle_length = M_PI32 + extra_half_circle_length;
			p0_theta -= 0.5f*extra_half_circle_length;

			vec2* points = gizmo->points[z_axis_i];
			
			for (int i = 0; i < ROTATION_GIZMO_POINTS_COUNT; i++) {
				float theta = p0_theta + half_circle_length * (float)i / (float)ROTATION_GIZMO_POINTS_COUNT;
				vec4 p_world = {0.f, 0.f, 0.f, 1.f};
				p_world._[x_axis_i] = scale * cosf(theta);
				p_world._[y_axis_i] = scale * sinf(theta);

				float side = M_Dot3(p_world.xyz, sphere_half_plane.xyz) + sphere_half_plane.w;
				vec4 p_clip = M_MulM4V4(view->cs_from_ws, p_world);
				vec2 p = M_CSToSS(p_clip, view->window_size);
				points[i] = UI_VEC2{p.x, p.y};
			}
			
			bool points_loop = extra_half_circle_length == M_PI32;

			assert(ROTATION_GIZMO_POINTS_COUNT % 2 == 0); // For performance, let's skip every second point
			for (int i = 0; i < ROTATION_GIZMO_POINTS_COUNT; i += 2) {
				vec2 p1 = *(vec2*)&points[i == 0 ? (points_loop ? ROTATION_GIZMO_POINTS_COUNT-1 : 0) : i-2];
				vec2 p2 = *(vec2*)&points[i];
				float dist = M_DistanceToLineSegment2D(*(vec2*)&UI_STATE.mouse_pos, p1, p2);
				if (dist < min_dist) {
					min_dist = dist;
					min_dist_axis = z_axis_i;
				}
			}

			gizmo->points_loop[z_axis_i] = points_loop;
		}

		if (min_dist < 10.f) {
			gizmo->hovered_axis = min_dist_axis+1;
			
			if (InputWentDown(HT_InputKey_MouseLeft)) {
				gizmo->dragging_axis = gizmo->hovered_axis;
				gizmo->drag_start_mouse_pos = UI_STATE.mouse_pos;
				gizmo->drag_start_rotation = *rotation;
				//gizmo->drag_mouse_angle
			}
		}
		
		if (!InputIsDown(in, HT_InputKey_MouseLeft)) {
			gizmo->dragging_axis = 0;
		}
		
		if (gizmo->dragging_axis > 0) {
			// so... we want to figure out the rotation about the point

			vec4 origin_clip = M_MulM4V4(view->cs_from_ws, M_V4V(origin, 1.f));
			vec2 origin_ss = M_CSToSS(origin_clip, view->window_size);
			
			// hmm... we can't just, or can we? Do we care about winding?
			float start_mouse_angle = atan2f(gizmo->drag_start_mouse_pos.y - origin_ss.y, gizmo->drag_start_mouse_pos.x - origin_ss.x);
			float new_mouse_angle = atan2f(UI_STATE.mouse_pos.y - origin_ss.y, UI_STATE.mouse_pos.x - origin_ss.x);
			// get the projected origin
			
			vec3 axis_dir = {0};
			axis_dir._[gizmo->dragging_axis-1] = camera_to_origin._[gizmo->dragging_axis-1] > 0.f ? 1.f : -1.f;

			float rotation_delta = new_mouse_angle - start_mouse_angle;

			quat rotation_delta_q = M_QFromAxisAngle_RH(axis_dir, rotation_delta);
			quat new_rotation = M_MulQ(rotation_delta_q, gizmo->drag_start_rotation);
			*rotation = new_rotation;
		}
	}
}

GIZMOS_API void RotationGizmoDraw(const M_PerspectiveView* view, const RotationGizmo* gizmo) {
	int active_axis = gizmo->dragging_axis > 0 ? gizmo->dragging_axis : gizmo->hovered_axis;

	UI_Color colors[] = {UI_RED, UI_GREEN, UI_BLUE};
	for (int i = 0; i < 3; i++) {
		UI_Color color = active_axis == i+1 ? UI_YELLOW : colors[i];
		if (gizmo->dragging_axis > 0 && i+1 != active_axis) continue; // don't draw non-active axes

		UI_Color point_colors[ROTATION_GIZMO_POINTS_COUNT];
		for (int j = 0; j < ROTATION_GIZMO_POINTS_COUNT; j++) point_colors[j] = color;

		if (gizmo->points_loop[i]) {
			UI_DrawPolylineLoop(gizmo->points[i], point_colors, ROTATION_GIZMO_POINTS_COUNT, 3.f);
		} else {
			UI_DrawPolyline(gizmo->points[i], point_colors, ROTATION_GIZMO_POINTS_COUNT, 3.f);
		}
	}
}

#endif

// ----------------------------------------------------------

// To do a blender-style translation modal, we want to calculate how much the mouse is moved along the 2D line. Then, move the object origin in screen space by that delta. Then, project that to the original 3D axis.

GIZMOS_API void DrawLine3D(const M_PerspectiveView* view, vec3 a, vec3 b, float thickness, UI_Color color) {
	vec4 a_ss = vec4{a, 1.f} * view->ws_to_ss;
	vec4 b_ss = vec4{b, 1.f} * view->ws_to_ss;

	// NOTE: we don't clip against the far plane, only the near plane

	bool in_front = true;
	if (a_ss.z > 0.f && b_ss.z < 0.f) { // a is in view, b is behind view plane
		// z = 0 at the view plane, so just solve t from lerp(a_ss.z, b_ss.z, t) = 0
		float t = -(a_ss.z) / (b_ss.z - a_ss.z);
		b_ss = M_Lerp4(a_ss, b_ss, t);
	}
	else if (a_ss.z < 0.f && b_ss.z > 0.f) { // b is in view, a is behind view plane
		float t = -(a_ss.z) / (b_ss.z - a_ss.z);
		a_ss = M_Lerp4(a_ss, b_ss, t);
	}
	else if (b_ss.z < 0.f && a_ss.z < 0.f) {
		in_front = false; // both points are behind the view plane
	}

	if (in_front) {
		UI_DrawLine(a_ss.xy / a_ss.w, b_ss.xy / b_ss.w, thickness, color);
	}
}

GIZMOS_API void DrawPoint3D(const M_PerspectiveView* view, vec3 p, float thickness, UI_Color color) {
	vec4 p_ss = vec4{p, 1.f} * view->ws_to_ss;
	if (p_ss.z > 0.f && p_ss.z < p_ss.w) {
		UI_DrawPoint(p_ss.xy / p_ss.w, thickness, color);
	}
}

GIZMOS_API void DrawGrid3D(const M_PerspectiveView* view, UI_Color color) {
	int grid_extent = 16;
	for (int x = -grid_extent; x <= grid_extent; x++) {
		DrawLine3D(view, vec3{(float)x, -(float)grid_extent, 0.f}, vec3{(float)x, (float)grid_extent, 0.f}, 2.f, color);
	}
	for (int y = -grid_extent; y <= grid_extent; y++) {
		DrawLine3D(view, vec3{-(float)grid_extent, (float)y, 0.f}, vec3{(float)grid_extent, (float)y, 0.f}, 2.f, color);
	}
}

GIZMOS_API void DrawGridEx3D(const M_PerspectiveView* view, vec3 origin, vec3 x_dir, vec3 y_dir, UI_Color color) {
	int grid_extent = 16;

	vec3 x_origin_a = origin - y_dir * (float)grid_extent;
	vec3 x_origin_b = origin + y_dir * (float)grid_extent;
	
	for (int x = -grid_extent; x <= grid_extent; x++) {
		vec3 x_offset = x_dir * (float)x;
		DrawLine3D(view, x_origin_a + x_offset, x_origin_b + x_offset, 2.f, color);
	}

	vec3 y_origin_a = origin - x_dir * (float)grid_extent;
	vec3 y_origin_b = origin + x_dir * (float)grid_extent;

	for (int y = -grid_extent; y <= grid_extent; y++) {
		vec3 y_offset = y_dir * (float)y;
		DrawLine3D(view, y_origin_a + y_offset, y_origin_b + y_offset, 2.f, color);
	}
}

GIZMOS_API void DrawParallelepiped3D(const M_PerspectiveView* view, vec3 min_corner, vec3 extent_x, vec3 extent_y, vec3 extent_z, float thickness, UI_Color color)
{
	vec3 OOO = min_corner;
	vec3 OOI = OOO + extent_z;
	vec3 OIO = OOO + extent_y;
	vec3 OII = OOI + extent_y;
	vec3 IOO = OOO + extent_x;
	vec3 IOI = OOI + extent_x;
	vec3 IIO = OIO + extent_x;
	vec3 III = OII + extent_x;

	// Lines going in the direction of X
	DrawLine3D(view, OOO, IOO, thickness, color);
	DrawLine3D(view, OIO, IIO, thickness, color);
	DrawLine3D(view, OOI, IOI, thickness, color);
	DrawLine3D(view, OII, III, thickness, color);

	// Lines going in the direction of Y
	DrawLine3D(view, OOO, OIO, thickness, color);
	DrawLine3D(view, IOO, IIO, thickness, color);
	DrawLine3D(view, OOI, OII, thickness, color);
	DrawLine3D(view, IOI, III, thickness, color);

	// Lines going in the direction of Z
	DrawLine3D(view, OOO, OOI, thickness, color);
	DrawLine3D(view, IOO, IOI, thickness, color);
	DrawLine3D(view, OIO, OII, thickness, color);
	DrawLine3D(view, IIO, III, thickness, color);
}

GIZMOS_API void DrawCuboid3D(const M_PerspectiveView* view, vec3 min, vec3 max, float thickness, UI_Color color) {
	// Lines going in the direction of X
	DrawLine3D(view, vec3{min.x, min.y, min.z}, vec3{max.x, min.y, min.z}, thickness, color);
	DrawLine3D(view, vec3{min.x, max.y, min.z}, vec3{max.x, max.y, min.z}, thickness, color);
	DrawLine3D(view, vec3{min.x, min.y, max.z}, vec3{max.x, min.y, max.z}, thickness, color);
	DrawLine3D(view, vec3{min.x, max.y, max.z}, vec3{max.x, max.y, max.z}, thickness, color);
	
	// Lines going in the direction of Y
	DrawLine3D(view, vec3{min.x, min.y, min.z}, vec3{min.x, max.y, min.z}, thickness, color);
	DrawLine3D(view, vec3{max.x, min.y, min.z}, vec3{max.x, max.y, min.z}, thickness, color);
	DrawLine3D(view, vec3{min.x, min.y, max.z}, vec3{min.x, max.y, max.z}, thickness, color);
	DrawLine3D(view, vec3{max.x, min.y, max.z}, vec3{max.x, max.y, max.z}, thickness, color);
	
	// Lines going in the direction of Z
	DrawLine3D(view, vec3{min.x, min.y, min.z}, vec3{min.x, min.y, max.z}, thickness, color);
	DrawLine3D(view, vec3{max.x, min.y, min.z}, vec3{max.x, min.y, max.z}, thickness, color);
	DrawLine3D(view, vec3{min.x, max.y, min.z}, vec3{min.x, max.y, max.z}, thickness, color);
	DrawLine3D(view, vec3{max.x, max.y, min.z}, vec3{max.x, max.y, max.z}, thickness, color);
}

GIZMOS_API void DrawQuad3D(const M_PerspectiveView* view, vec3 a, vec3 b, vec3 c, vec3 d, UI_Color color) {
	vec4 a_ss = vec4{a, 1.f} * view->ws_to_ss;
	vec4 b_ss = vec4{b, 1.f} * view->ws_to_ss;
	vec4 c_ss = vec4{c, 1.f} * view->ws_to_ss;
	vec4 d_ss = vec4{d, 1.f} * view->ws_to_ss;

	if (a_ss.z > 0.f && b_ss.z > 0.f && c_ss.z > 0.f && d_ss.z > 0.f) {
		uint32_t first_vertex;
		UI_DrawVertex* verts = UI_AddVerticesUnsafe(4, &first_vertex);
		verts[0] = UI_DRAW_VERTEX{a_ss.xy, {0, 0}, color};
		verts[1] = UI_DRAW_VERTEX{b_ss.xy, {0, 0}, color};
		verts[2] = UI_DRAW_VERTEX{c_ss.xy, {0, 0}, color};
		verts[3] = UI_DRAW_VERTEX{d_ss.xy, {0, 0}, color};
		UI_AddQuadIndices(first_vertex, first_vertex+1, first_vertex+2, first_vertex+3, NULL);
	}
}

GIZMOS_API void DrawQuadFrame3D(const M_PerspectiveView* view, vec3 a, vec3 b, vec3 c, vec3 d, float thickness, UI_Color color) {
	DrawLine3D(view, a, b, thickness, color);
	DrawLine3D(view, b, c, thickness, color);
	DrawLine3D(view, c, d, thickness, color);
	DrawLine3D(view, d, a, thickness, color);

	/*vec4 a_clip = M_MulM4V4(view->cs_from_ws, M_V4V(a, 1.f));
	vec4 b_clip = M_MulM4V4(view->cs_from_ws, M_V4V(b, 1.f));
	vec4 c_clip = M_MulM4V4(view->cs_from_ws, M_V4V(c, 1.f));
	vec4 d_clip = M_MulM4V4(view->cs_from_ws, M_V4V(d, 1.f));

	if (a_clip.z > 0.f && b_clip.z > 0.f && c_clip.z > 0.f && d_clip.z > 0.f) {
		vec3 midpoint = a;
		midpoint = M_AddV3(midpoint, b);
		midpoint = M_AddV3(midpoint, c);
		midpoint = M_AddV3(midpoint, d);
		midpoint = M_MulV3F(midpoint, 1.f/4.f);

		vec3 a_inner = M_LerpV3(a, inset, midpoint);
		vec3 b_inner = M_LerpV3(b, inset, midpoint);
		vec3 c_inner = M_LerpV3(c, inset, midpoint);
		vec3 d_inner = M_LerpV3(d, inset, midpoint);

		vec2 a_ss = ClipSpaceToScreen(view, a_clip);
		vec2 b_ss = ClipSpaceToScreen(view, b_clip);
		vec2 c_ss = ClipSpaceToScreen(view, c_clip);
		vec2 d_ss = ClipSpaceToScreen(view, d_clip);
		
		vec2 a_inner_ss = ClipSpaceToScreen(view, M_MulM4V4(view->cs_from_ws, M_V4V(a_inner, 1.f)));
		vec2 b_inner_ss = ClipSpaceToScreen(view, M_MulM4V4(view->cs_from_ws, M_V4V(b_inner, 1.f)));
		vec2 c_inner_ss = ClipSpaceToScreen(view, M_MulM4V4(view->cs_from_ws, M_V4V(c_inner, 1.f)));
		vec2 d_inner_ss = ClipSpaceToScreen(view, M_MulM4V4(view->cs_from_ws, M_V4V(d_inner, 1.f)));

		uint32_t first_vertex;
		UI_DrawVertex* verts = UI_AddVertices(8, &first_vertex);
		verts[0] = UI_DRAW_VERTEX{{a_ss.x, a_ss.y}, {0, 0}, color};
		verts[1] = UI_DRAW_VERTEX{{a_inner_ss.x, a_inner_ss.y}, {0, 0}, color};
		verts[2] = UI_DRAW_VERTEX{{b_ss.x, b_ss.y}, {0, 0}, color};
		verts[3] = UI_DRAW_VERTEX{{b_inner_ss.x, b_inner_ss.y}, {0, 0}, color};
		verts[4] = UI_DRAW_VERTEX{{c_ss.x, c_ss.y}, {0, 0}, color};
		verts[5] = UI_DRAW_VERTEX{{c_inner_ss.x, c_inner_ss.y}, {0, 0}, color};
		verts[6] = UI_DRAW_VERTEX{{d_ss.x, d_ss.y}, {0, 0}, color};
		verts[7] = UI_DRAW_VERTEX{{d_inner_ss.x, d_inner_ss.y}, {0, 0}, color};
		UI_AddQuadIndices(first_vertex+1, first_vertex+0, first_vertex+2, first_vertex+3, UI_TEXTURE_ID_NIL);
		UI_AddQuadIndices(first_vertex+3, first_vertex+2, first_vertex+4, first_vertex+5, UI_TEXTURE_ID_NIL);
		UI_AddQuadIndices(first_vertex+5, first_vertex+4, first_vertex+6, first_vertex+7, UI_TEXTURE_ID_NIL);
		UI_AddQuadIndices(first_vertex+7, first_vertex+6, first_vertex+0, first_vertex+1, UI_TEXTURE_ID_NIL);
	}*/
}

GIZMOS_API void DrawArrow3D(const M_PerspectiveView* view, vec3 from, vec3 to,
	float head_length, float head_radius, int vertices, float thickness, UI_Color color)
{
	vec4 to_ss = vec4{to, 1.f} * view->ws_to_ss;

	head_radius *= to_ss.w;
	head_length *= to_ss.w;

	vec3 dir = M_Norm3(to - from);
	vec3 head_base = to - dir * head_length;

	DrawLine3D(view, head_base, from, thickness, color);

	vec3 random_nondegenerate_vector = {dir.x == 0.f ? 1.f : 0.f, 1.f, 1.f};
	vec3 tangent = M_Norm3(M_Cross3(dir, random_nondegenerate_vector));
	vec3 bitangent = M_Cross3(tangent, dir);

	uint32_t first_vertex;
	UI_DrawVertex* vertices_data = UI_AddVerticesUnsafe(vertices+1, &first_vertex);
	
	vertices_data[0] = UI_DRAW_VERTEX{to_ss.xy / to_ss.w, {0, 0}, color};

	bool head_is_visible = to_ss.z > 0.f;
	if (head_is_visible) {

		for (int i = 0; i < vertices; i++) {
			float theta = M_TAU * (float)i / (float)vertices;

			vec3 vertex_dir = tangent*cosf(theta) + bitangent*sinf(theta);
			
			vec4 p_ss = vec4{head_base + vertex_dir*head_radius, 1.f} * view->ws_to_ss;
			vertices_data[1+i] = UI_DRAW_VERTEX{p_ss.xy / p_ss.w, {0, 0}, color}; 

			UI_AddTriangleIndices(first_vertex, first_vertex + 1 + i, first_vertex + 1 + ((i + 1) % vertices), NULL);
		}
	}
}
