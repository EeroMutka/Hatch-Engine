#pragma once

#ifndef HANDMADE_MATH_INCLUDED
#error "HandmadeMath.h" must be included before this file!
#endif

#ifndef FIRE_UI_INCLUDED
#error "fire_ui.h" must be included before this file!
#endif


#define GIZMOS_API static

//static HMM_Mat4 AAAAA;

typedef struct GizmosViewport {
	HMM_PerspectiveCamera camera;
	vec2 window_size, window_size_inv;
} GizmosViewport;

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
	//vec2 moving_axis_arrow_end_screen;
	vec2 moving_begin_mouse_pos;
	vec2 moving_begin_origin_screen;
	vec3 moving_begin_translation;
	vec3 moving_axis_arrow_end;
	
	// planar movement
	vec2 planar_moving_begin_mouse_pos;
	vec2 planar_moving_begin_origin_screen;
	vec3 planar_moving_begin_translation;
} TranslationGizmo;

#define ROTATION_GIZMO_POINTS_COUNT 28

typedef struct RotationGizmo {
	int hovered_axis; // 0 = none, 1 = x, 2 = y, 3 = z
	int dragging_axis; // 0 = none, 1 = x, 2 = y, 3 = z

	vec3 origin;

	vec2 points[3][ROTATION_GIZMO_POINTS_COUNT];
	bool points_loop[3];

	vec2 drag_start_mouse_pos;
	quat drag_start_rotation;
} RotationGizmo;

GIZMOS_API void DrawPoint3D(const GizmosViewport* vp, vec3 p, float thickness, UI_Color color);
GIZMOS_API void DrawGrid3D(const GizmosViewport* vp, UI_Color color);
GIZMOS_API void DrawGridEx3D(const GizmosViewport* vp, vec3 origin, vec3 x_dir, vec3 y_dir, UI_Color color);
GIZMOS_API void DrawLine3D(const GizmosViewport* vp, vec3 a, vec3 b, float thickness, UI_Color color);
GIZMOS_API void DrawCuboid3D(const GizmosViewport* vp, vec3 min, vec3 max, float thickness, UI_Color color);
GIZMOS_API void DrawParallelepiped3D(const GizmosViewport* vp, vec3 min_corner, vec3 extent_x, vec3 extent_y, vec3 extent_z, float thickness, UI_Color color);
GIZMOS_API void DrawArrow3D(const GizmosViewport* vp, vec3 from, vec3 to, float head_length, float head_radius, int vertices, float thickness, UI_Color color);

// NOTE: These will not be clipped to the viewport properly if the shape is too big. The shape should always be either fully visible or fully out of the screen.
//       I should try to fix this.
GIZMOS_API void DrawQuad3D(const GizmosViewport* vp, vec3 a, vec3 b, vec3 c, vec3 d, UI_Color color);
GIZMOS_API void DrawQuadFrame3D(const GizmosViewport* vp, vec3 a, vec3 b, vec3 c, vec3 d, float thickness, UI_Color color);

//GIZMOS_API bool TranslationGizmoShouldApply(const TranslationGizmo* gizmo);
GIZMOS_API void TranslationGizmoUpdate(const GizmosViewport* vp, TranslationGizmo* gizmo, vec3* translation, float snap_size);
GIZMOS_API void TranslationGizmoDraw(const GizmosViewport* vp, const TranslationGizmo* gizmo);

GIZMOS_API bool RotationGizmoShouldApply(const RotationGizmo* gizmo);
GIZMOS_API void RotationGizmoUpdate(const GizmosViewport* vp, RotationGizmo* gizmo, vec3 origin, quat* rotation);
GIZMOS_API void RotationGizmoDraw(const GizmosViewport* vp, const RotationGizmo* gizmo);


typedef struct LineTranslation {
	vec3 line_dir;
	vec2 moving_begin_mouse_pos;
	vec2 moving_begin_origin_screen;
	vec3 moving_begin_translation;
	vec3 moving_axis_arrow_end;
} LineTranslation;


GIZMOS_API void LineTranslationBegin(LineTranslation* translation, const GizmosViewport* vp, vec2 mouse_pos, vec3 line_p, vec3 line_dir);

GIZMOS_API void LineTranslationUpdate(LineTranslation* translation, const GizmosViewport* vp, vec3* point, vec2 mouse_pos, float snap_size);


// ----------------------------------------------------------

//GIZMOS_API bool TranslationGizmoShouldApply(const TranslationGizmo* gizmo) {
//	return gizmo->moving_axis_ != 0 && !UI_InputIsDown(UI_Input_MouseLeft);
//}

static void LineTranslationBegin(LineTranslation* translation, const GizmosViewport* vp, vec2 mouse_pos, vec3 point, vec3 line_dir)
{
	HMM_Vec4 origin_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(point, 1.f));
	vec2 origin_screen = HMM_CSToSS(origin_clip, vp->window_size);

	translation->line_dir = line_dir;
	translation->moving_begin_mouse_pos = mouse_pos;
	translation->moving_begin_translation = point;
	translation->moving_begin_origin_screen = origin_screen;
	//gizmo->moving_axis_arrow_end_screen = arrow_end_point_screen[min_distance_axis];
	translation->moving_axis_arrow_end = HMM_AddV3(point, HMM_MulV3F(line_dir, 0.001f));
}

static void SnapPointToGrid(vec3* point, vec3 grid_origin, float snap_size, int axis) {
	float p_relative = point->Elements[axis] - grid_origin.Elements[axis];
	p_relative = roundf(p_relative / snap_size) * snap_size;
	point->Elements[axis] = grid_origin.Elements[axis] + p_relative;
}

static void SnapPointToGrid2(vec3* point, vec3 grid_origin, float snap_size, vec3 axis) {
	float p_relative = HMM_DotV3(HMM_SubV3(*point, grid_origin), axis);
	float p_relative_delta = roundf(p_relative / snap_size) * snap_size - p_relative;
	*point = HMM_AddV3(*point, HMM_MulV3F(axis, p_relative_delta));
}

GIZMOS_API void LineTranslationUpdate(LineTranslation* translation, const GizmosViewport* vp, vec3* point, vec2 mouse_pos, float snap_size)
{
	vec2 movement_delta = HMM_SubV2(mouse_pos, translation->moving_begin_mouse_pos);

	HMM_Vec4 moving_begin_origin_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(translation->moving_begin_translation, 1.f));
	vec2 moving_begin_origin_screen = HMM_CSToSS(moving_begin_origin_clip, vp->window_size);

	HMM_Vec4 moving_begin_arrow_end_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(translation->moving_axis_arrow_end, 1.f));
	vec2 moving_begin_arrow_end_screen = HMM_CSToSS(moving_begin_arrow_end_clip, vp->window_size);

	vec2 arrow_direction = HMM_SubV2(moving_begin_arrow_end_screen, moving_begin_origin_screen);
	float arrow_direction_len = HMM_LenV2(arrow_direction);
	arrow_direction = arrow_direction_len == 0.f ?
		HMM_V2(1, 0) :
		HMM_MulV2F(arrow_direction, 1.f / arrow_direction_len);

	float t = HMM_DotV2(arrow_direction, movement_delta);
	t -= HMM_DotV2(arrow_direction, HMM_SubV2(moving_begin_origin_screen, translation->moving_begin_origin_screen)); // account for camera movement

	vec2 movement_delta_projected = HMM_MulV2F(arrow_direction, t);
	vec2 moved_pos_screen = HMM_AddV2(moving_begin_origin_screen, movement_delta_projected);

	vec3 moved_pos_ray_dir = HMM_RayDirectionFromSSPoint(&vp->camera, moved_pos_screen, vp->window_size_inv);

	vec3 plane_tangent = HMM_Cross(moved_pos_ray_dir, translation->line_dir);
	vec3 plane_normal = HMM_Cross(plane_tangent, translation->line_dir);

	// Make a plane perpendicular to the line axis
	HMM_Vec4 plane = {plane_normal, -HMM_DotV3(plane_normal, translation->moving_begin_translation)};
	vec3 intersection_pos;
	HMM_RayPlaneIntersect(vp->camera.position, moved_pos_ray_dir, plane, NULL, &intersection_pos);

	// The intersection position is roughly correct, but may not be entirely accurate. For accuracy, let's project the intersection pos onto the translation line.
	intersection_pos = HMM_ProjectPointOntoLine(intersection_pos, translation->moving_begin_translation, translation->line_dir);

	if (snap_size > 0) {
		vec3 snap_grid_origin = UI_InputIsDown(UI_Input_Alt) ? HMM_V3(0, 0, 0) : translation->moving_begin_translation;
		SnapPointToGrid2(&intersection_pos, snap_grid_origin, snap_size, translation->line_dir);
	}

	*point = intersection_pos;
}

GIZMOS_API void TranslationGizmoUpdate(const GizmosViewport* vp, TranslationGizmo* gizmo, vec3* translation, float snap_size) {
	gizmo->hovered_axis_ = TranslationAxis_None;
	gizmo->plane_gizmo_is_implicitly_hovered = false;

	vec2 mouse_pos = {UI_STATE.mouse_pos.x, UI_STATE.mouse_pos.y};
	
	HMM_Vec4 origin = HMM_V4V(*translation, 1.f);
	
	// Update planar movement
	if (gizmo->moving_axis_ >= TranslationAxis_YZ && gizmo->moving_axis_ <= TranslationAxis_XY) {
		// For planar movement, we want to calculate the movement delta in 2D, then add that to the screen-space origin to find the 3D ray direction,
		// then find the ray-plane intersection and that's the final position

		vec2 movement_delta = HMM_SubV2(mouse_pos, gizmo->planar_moving_begin_mouse_pos);
		vec2 moved_pos_screen = HMM_AddV2(gizmo->planar_moving_begin_origin_screen, movement_delta);

		vec3 moved_pos_ray_dir = HMM_RayDirectionFromSSPoint(&vp->camera, moved_pos_screen, vp->window_size_inv);

		vec3 plane_normal = {0};
		plane_normal.Elements[gizmo->moving_axis_ - TranslationAxis_YZ] = 1.f;

		// Make a plane perpendicular to the line axis
		HMM_Vec4 plane = {plane_normal, -HMM_DotV3(plane_normal, gizmo->planar_moving_begin_translation)};
		vec3 intersection_pos;
		HMM_RayPlaneIntersect(vp->camera.position, moved_pos_ray_dir, plane, NULL, &intersection_pos);

		if (!UI_InputIsDown(UI_Input_Control)) {
			vec3 snap_grid_origin = UI_InputIsDown(UI_Input_Alt) ? HMM_V3(0, 0, 0) : gizmo->planar_moving_begin_translation;
			for (int i = 1; i <= 2; i++) {
				SnapPointToGrid(&intersection_pos, snap_grid_origin, snap_size, (gizmo->moving_axis_ - TranslationAxis_YZ + i) % 3);
			}
		}

		origin.XYZ = intersection_pos;
	}

	// Update linear movement
	// TODO: use LineTranslation API
	if (gizmo->moving_axis_ >= TranslationAxis_X && gizmo->moving_axis_ <= TranslationAxis_Z) {
		vec2 movement_delta = HMM_SubV2(mouse_pos, gizmo->moving_begin_mouse_pos);

		HMM_Vec4 moving_begin_origin_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(gizmo->moving_begin_translation, 1.f));
		vec2 moving_begin_origin_screen = HMM_CSToSS(moving_begin_origin_clip, vp->window_size);

		HMM_Vec4 moving_begin_arrow_end_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(gizmo->moving_axis_arrow_end, 1.f));
		vec2 moving_begin_arrow_end_screen = HMM_CSToSS(moving_begin_arrow_end_clip, vp->window_size);

		vec2 arrow_direction = HMM_SubV2(moving_begin_arrow_end_screen, moving_begin_origin_screen);
		arrow_direction = HMM_NormV2(arrow_direction);

		float t = HMM_DotV2(arrow_direction, movement_delta);
		t -= HMM_DotV2(arrow_direction, HMM_SubV2(moving_begin_origin_screen, gizmo->moving_begin_origin_screen)); // account for camera movement
		
		vec2 movement_delta_projected = HMM_MulV2F(arrow_direction, t);
		vec2 moved_pos_screen = HMM_AddV2(moving_begin_origin_screen, movement_delta_projected);

		vec3 axis = {0.f, 0.f, 0.f};
		axis.Elements[gizmo->moving_axis_ - TranslationAxis_X] = 1.f;

		vec3 moved_pos_ray_dir = HMM_RayDirectionFromSSPoint(&vp->camera, moved_pos_screen, vp->window_size_inv);

		vec3 plane_tangent = HMM_Cross(moved_pos_ray_dir, axis);
		vec3 plane_normal = HMM_Cross(plane_tangent, axis);

		// Make a plane perpendicular to the line axis
		HMM_Vec4 plane = {plane_normal, -HMM_DotV3(plane_normal, gizmo->moving_begin_translation)};
		vec3 intersection_pos;
		HMM_RayPlaneIntersect(vp->camera.position, moved_pos_ray_dir, plane, NULL, &intersection_pos);
		
		if (!UI_InputIsDown(UI_Input_Control)) {
			vec3 snap_grid_origin = UI_InputIsDown(UI_Input_Alt) ? HMM_V3(0, 0, 0) : gizmo->moving_begin_translation;
			SnapPointToGrid(&intersection_pos, snap_grid_origin, snap_size, gizmo->moving_axis_ - TranslationAxis_X);
		}

		origin.XYZ = intersection_pos;
	}

	vec3 origin_dir_from_camera = HMM_NormV3(HMM_SubV3(origin.XYZ, vp->camera.position));

	HMM_GetPointScreenSpaceScale(&vp->camera, origin.XYZ, &gizmo->arrow_scale);
	gizmo->arrow_scale *= 0.15f;
	
	HMM_Vec4 origin_clip = HMM_MulM4V4(vp->camera.clip_from_world, origin);
	vec2 origin_screen = HMM_CSToSS(origin_clip, vp->window_size);
	
	float min_distance = 10000000.f;
	int min_distance_axis = -1;
	vec2 arrow_end_point_screen[3];
	vec3 arrow_end_point[3];

	for (int i = 0; i < 3; i++) {
		vec3 arrow_end = origin.XYZ;
		arrow_end.Elements[i] += gizmo->arrow_scale;
		arrow_end_point[i] = arrow_end;

		HMM_Vec4 b_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(arrow_end, 1.f));
		if (b_clip.Z > 0.f) {
			arrow_end_point_screen[i] = HMM_CSToSS(b_clip, vp->window_size);
			float d = HMM_DistanceToLineSegment2D(mouse_pos, origin_screen, arrow_end_point_screen[i]);
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
		vec3 o = origin.XYZ;
		float k = 0.7f*gizmo->arrow_scale; // inner offset
		float l = 0.8f*gizmo->arrow_scale; // outer offset

		HMM_Vec4 quads[3][4] = {
			{{o.X, o.Y+k, o.Z+k, 1.f}, {o.X, o.Y+k, o.Z+l, 1.f}, {o.X, o.Y+l, o.Z+l, 1.f}, {o.X, o.Y+l, o.Z+k, 1.f}}, // YZ-plane
			{{o.X+k, o.Y, o.Z+k, 1.f}, {o.X+l, o.Y, o.Z+k, 1.f}, {o.X+l, o.Y, o.Z+l, 1.f}, {o.X+k, o.Y, o.Z+l, 1.f}}, // ZX-plane
			{{o.X+k, o.Y+k, o.Z, 1.f}, {o.X+k, o.Y+l, o.Z, 1.f}, {o.X+l, o.Y+l, o.Z, 1.f}, {o.X+l, o.Y+k, o.Z, 1.f}}, // XY-plane
		};

		for (int i = 0; i < 3; i++) {
			HMM_Vec4 quad_clip[4] = {
				HMM_MulM4V4(vp->camera.clip_from_world, quads[i][0]),
				HMM_MulM4V4(vp->camera.clip_from_world, quads[i][1]),
				HMM_MulM4V4(vp->camera.clip_from_world, quads[i][2]),
				HMM_MulM4V4(vp->camera.clip_from_world, quads[i][3]),
			};
			
			vec3 plane_normal = {0};
			plane_normal.Elements[i] = 1.f;
			float view_dot = HMM_DotV3(origin_dir_from_camera, plane_normal);
			
			float visibility = HMM_Clamp(HMM_ABS(view_dot)*8.f - 1.f, 0.f, 1.f);
			
			bool in_view = quad_clip[0].Z > 0.f && quad_clip[1].Z > 0.f && quad_clip[2].Z > 0.f && quad_clip[3].Z > 0.f;
			if (!in_view) visibility = 0.f;
			
			gizmo->plane_gizmo_visibility[i] = visibility;
			if (in_view) {
				gizmo->plane_gizmo_quads[i][0] = HMM_CSToSS(quad_clip[0], vp->window_size);
				gizmo->plane_gizmo_quads[i][1] = HMM_CSToSS(quad_clip[1], vp->window_size);
				gizmo->plane_gizmo_quads[i][2] = HMM_CSToSS(quad_clip[2], vp->window_size);
				gizmo->plane_gizmo_quads[i][3] = HMM_CSToSS(quad_clip[3], vp->window_size);
				
				float dist = HMM_DistanceToPolygon2D(mouse_pos, gizmo->plane_gizmo_quads[i], 4);
				if (dist < plane_gizmos_min_distance) {
					plane_gizmos_min_distance = dist;
					plane_gizmos_min_dist_axis = i;
				}
			}
		}
	}

	if (min_distance < 10.f) {
		gizmo->hovered_axis_ = (TranslationAxis)(TranslationAxis_X + min_distance_axis);

		if (UI_InputWasPressed(UI_Input_MouseLeft)) { // Begin press?
			gizmo->moving_axis_ = gizmo->hovered_axis_;
			gizmo->moving_begin_mouse_pos = mouse_pos;
			gizmo->moving_begin_translation = origin.XYZ;
			gizmo->moving_begin_origin_screen = origin_screen;
			//gizmo->moving_axis_arrow_end_screen = arrow_end_point_screen[min_distance_axis];
			gizmo->moving_axis_arrow_end = arrow_end_point[min_distance_axis];
		}
	}
	else {
		
		if (plane_gizmos_min_distance < 10.f) {
			gizmo->hovered_axis_ = (TranslationAxis)(TranslationAxis_YZ + plane_gizmos_min_dist_axis);
		}
		else {
			/*vec3 forward = vp->camera.forward_dir;
			vec3 forward_abs = {HMM_ABS(forward.X), HMM_ABS(forward.Y), HMM_ABS(forward.Z)};

			gizmo->hovered_axis_ = TranslationAxis_YZ;
			float max_axis_value = forward_abs.X;
			if (forward_abs.Y > max_axis_value) {
				max_axis_value = forward_abs.Y;
				gizmo->hovered_axis_ = TranslationAxis_ZX;
			}
			if (forward_abs.Z > max_axis_value) {
				max_axis_value = forward_abs.Z;
				gizmo->hovered_axis_ = TranslationAxis_XY;
			}
			
			gizmo->plane_gizmo_is_implicitly_hovered = true;*/
		}
		
		if (UI_InputWasPressed(UI_Input_MouseLeft)) { // Begin press?
			gizmo->moving_axis_ = gizmo->hovered_axis_;
			gizmo->planar_moving_begin_mouse_pos = mouse_pos;
			gizmo->planar_moving_begin_translation = origin.XYZ;
			gizmo->planar_moving_begin_origin_screen = origin_screen;
		}
	}
	
	if (!UI_InputIsDown(UI_Input_MouseLeft)) {
		gizmo->moving_axis_ = TranslationAxis_None;
	}
	
	gizmo->origin = origin.XYZ;
	*translation = origin.XYZ;
}

GIZMOS_API void TranslationGizmoDraw(const GizmosViewport* vp, const TranslationGizmo* gizmo) {
	TranslationAxis active_axis = gizmo->moving_axis_ ? gizmo->moving_axis_ : gizmo->hovered_axis_;
	bool x_active = active_axis == TranslationAxis_X;
	bool y_active = active_axis == TranslationAxis_Y;
	bool z_active = active_axis == TranslationAxis_Z;
	
	UI_Color nonhovered_colors[] = {UI_COLOR{225, 50, 10, 255}, UI_COLOR{50, 225, 10, 255}, UI_COLOR{10, 120, 225, 255}};
	DrawArrow3D(vp, gizmo->origin, HMM_AddV3(gizmo->origin, HMM_V3(gizmo->arrow_scale, 0.f, 0.f)), 0.03f, 0.012f, 12, x_active ? 3.f : 3.f, x_active ? UI_YELLOW : nonhovered_colors[0]);
	DrawArrow3D(vp, gizmo->origin, HMM_AddV3(gizmo->origin, HMM_V3(0.f, gizmo->arrow_scale, 0.f)), 0.03f, 0.012f, 12, y_active ? 3.f : 3.f, y_active ? UI_YELLOW : nonhovered_colors[1]);
	DrawArrow3D(vp, gizmo->origin, HMM_AddV3(gizmo->origin, HMM_V3(0.f, 0.f, gizmo->arrow_scale)), 0.03f, 0.012f, 12, z_active ? 3.f : 3.f, z_active ? UI_YELLOW : nonhovered_colors[2]);

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
				UI_VEC2{gizmo->plane_gizmo_quads[i][0].X, gizmo->plane_gizmo_quads[i][0].Y},
				UI_VEC2{gizmo->plane_gizmo_quads[i][1].X, gizmo->plane_gizmo_quads[i][1].Y},
				UI_VEC2{gizmo->plane_gizmo_quads[i][2].X, gizmo->plane_gizmo_quads[i][2].Y},
				UI_VEC2{gizmo->plane_gizmo_quads[i][3].X, gizmo->plane_gizmo_quads[i][3].Y},
			};
			UI_DrawQuad(q[0], q[1], q[2], q[3], inner_color);

			UI_DrawLine(q[0], q[1], frame_width, frame_color);
			UI_DrawLine(q[1], q[2], frame_width, frame_color);
			UI_DrawLine(q[2], q[3], frame_width, frame_color);
			UI_DrawLine(q[3], q[0], frame_width, frame_color);
			
		}
	}
}

GIZMOS_API bool RotationGizmoShouldApply(const RotationGizmo* gizmo) {
	return gizmo->dragging_axis != 0 && !UI_InputIsDown(UI_Input_MouseLeft);
}

GIZMOS_API void RotationGizmoUpdate(const GizmosViewport* vp, RotationGizmo* gizmo, vec3 origin, quat* rotation) {
	gizmo->hovered_axis = 0;
	//HMM_Mat4 rotation_mat4 = HMM_QToM4(*rotation);
	//vec3 X = rotation_mat4.Columns[0].XYZ;
	//vec3 Y = rotation_mat4.Columns[1].XYZ;
	//vec3 Z = rotation_mat4.Columns[2].XYZ;

	float scale;
	if (HMM_GetPointScreenSpaceScale(&vp->camera, origin, &scale)) {
		scale *= 0.15f;
		
		vec3 camera_to_origin = HMM_NormV3(HMM_SubV3(origin, vp->camera.position));
		HMM_Vec4 sphere_half_plane = {camera_to_origin, -HMM_DotV3(camera_to_origin, origin)};

		float min_dist = 10000000.f;
		int min_dist_axis = 0;

		for (int z_axis_i = 0; z_axis_i < 3; z_axis_i++) {
			int x_axis_i = (z_axis_i + 1) % 3;
			int y_axis_i = (z_axis_i + 2) % 3;

			vec3 x_axis_dir = {0}, y_axis_dir = {0}, z_axis_dir = {0};
			x_axis_dir.Elements[x_axis_i] = 1.f;
			y_axis_dir.Elements[y_axis_i] = 1.f;
			z_axis_dir.Elements[z_axis_i] = 1.f;

			// Find the intersecting line between the axis plane and the sphere_half_plane, then to get the start/end points
			// of the half-circle, just move forward one unit along that line direction.
			vec3 intersecting_line_dir = HMM_NormV3(HMM_Cross(z_axis_dir, camera_to_origin));
			float p0_x = HMM_DotV3(intersecting_line_dir, x_axis_dir);
			float p0_y = HMM_DotV3(intersecting_line_dir, y_axis_dir);
			float p0_theta = atan2f(p0_y, p0_x);
			if (isnan(p0_theta)) p0_theta = 0.f;

			float dot = HMM_DotV3(camera_to_origin, z_axis_dir);
			float dot2 = dot*dot;
			float dot4 = dot2*dot2;
			float dot8 = dot4*dot4;

			float extra_half_circle_length = HMM_PI32 * HMM_MIN(dot8*dot8 + 0.05f, 1.f);
			float half_circle_length = HMM_PI32 + extra_half_circle_length;
			p0_theta -= 0.5f*extra_half_circle_length;

			vec2* points = gizmo->points[z_axis_i];
			
			for (int i = 0; i < ROTATION_GIZMO_POINTS_COUNT; i++) {
				float theta = p0_theta + half_circle_length * (float)i / (float)ROTATION_GIZMO_POINTS_COUNT;
				HMM_Vec4 p_world = {0.f, 0.f, 0.f, 1.f};
				p_world.Elements[x_axis_i] = scale * cosf(theta);
				p_world.Elements[y_axis_i] = scale * sinf(theta);

				float side = HMM_DotV3(p_world.XYZ, sphere_half_plane.XYZ) + sphere_half_plane.W;
				HMM_Vec4 p_clip = HMM_MulM4V4(vp->camera.clip_from_world, p_world);
				vec2 p = HMM_CSToSS(p_clip, vp->window_size);
				points[i] = UI_VEC2{p.X, p.Y};
			}
			
			bool points_loop = extra_half_circle_length == HMM_PI32;

			assert(ROTATION_GIZMO_POINTS_COUNT % 2 == 0); // For performance, let's skip every second point
			for (int i = 0; i < ROTATION_GIZMO_POINTS_COUNT; i += 2) {
				vec2 p1 = *(vec2*)&points[i == 0 ? (points_loop ? ROTATION_GIZMO_POINTS_COUNT-1 : 0) : i-2];
				vec2 p2 = *(vec2*)&points[i];
				float dist = HMM_DistanceToLineSegment2D(*(vec2*)&UI_STATE.mouse_pos, p1, p2);
				if (dist < min_dist) {
					min_dist = dist;
					min_dist_axis = z_axis_i;
				}
			}

			gizmo->points_loop[z_axis_i] = points_loop;
		}

		if (min_dist < 10.f) {
			gizmo->hovered_axis = min_dist_axis+1;
			
			if (UI_InputWasPressed(UI_Input_MouseLeft)) {
				gizmo->dragging_axis = gizmo->hovered_axis;
				gizmo->drag_start_mouse_pos = UI_STATE.mouse_pos;
				gizmo->drag_start_rotation = *rotation;
				//gizmo->drag_mouse_angle
			}
		}
		
		if (!UI_InputIsDown(UI_Input_MouseLeft)) {
			gizmo->dragging_axis = 0;
		}
		
		if (gizmo->dragging_axis > 0) {
			// so... we want to figure out the rotation about the point

			HMM_Vec4 origin_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(origin, 1.f));
			vec2 origin_screen = HMM_CSToSS(origin_clip, vp->window_size);
			
			// hmm... we can't just, or can we? Do we care about winding?
			float start_mouse_angle = atan2f(gizmo->drag_start_mouse_pos.y - origin_screen.Y, gizmo->drag_start_mouse_pos.x - origin_screen.X);
			float new_mouse_angle = atan2f(UI_STATE.mouse_pos.y - origin_screen.Y, UI_STATE.mouse_pos.x - origin_screen.X);
			// get the projected origin
			
			vec3 axis_dir = {0};
			axis_dir.Elements[gizmo->dragging_axis-1] = camera_to_origin.Elements[gizmo->dragging_axis-1] > 0.f ? 1.f : -1.f;

			float rotation_delta = new_mouse_angle - start_mouse_angle;

			quat rotation_delta_q = HMM_QFromAxisAngle_RH(axis_dir, rotation_delta);
			quat new_rotation = HMM_MulQ(rotation_delta_q, gizmo->drag_start_rotation);
			*rotation = new_rotation;
		}
	}
}

GIZMOS_API void RotationGizmoDraw(const GizmosViewport* vp, const RotationGizmo* gizmo) {
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

// ----------------------------------------------------------

// To do a blender-style translation modal, we want to calculate how much the mouse is moved along the 2D line. Then, move the object origin in screen space by that delta. Then, project that to the original 3D axis.

GIZMOS_API void DrawLine3D(const GizmosViewport* vp, vec3 a, vec3 b, float thickness, UI_Color color) {
	HMM_Vec4 a_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(a, 1.f));
	HMM_Vec4 b_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(b, 1.f));

	bool in_front = true;
	if (a_clip.Z > 0.f && b_clip.Z < 0.f) { // a is in view, b is behind view plane
		// z = 0 at the view plane, so just solve t from lerp(a_clip.Z, b_clip.Z, t) = 0
		float t = -a_clip.Z / (b_clip.Z - a_clip.Z);
		b_clip = HMM_LerpV4(a_clip, t, b_clip);
	}
	else if (a_clip.Z < 0.f && b_clip.Z > 0.f) { // b is in view, a is behind view plane
		float t = -a_clip.Z / (b_clip.Z - a_clip.Z);
		a_clip = HMM_LerpV4(a_clip, t, b_clip);
	}
	else if (b_clip.Z < 0.f && a_clip.Z < 0.f) {
		in_front = false; // both points are behind the view plane
	}

	if (in_front) {
		vec2 a_screen = HMM_CSToSS(a_clip, vp->window_size);
		vec2 b_screen = HMM_CSToSS(b_clip, vp->window_size);
		UI_DrawLine(UI_VEC2{a_screen.X, a_screen.Y}, UI_VEC2{b_screen.X, b_screen.Y}, thickness, color);
	}
}

GIZMOS_API void DrawPoint3D(const GizmosViewport* vp, vec3 p, float thickness, UI_Color color) {
	HMM_Vec4 p_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(p, 1.f));
	if (p_clip.Z > 0.f) {
		vec2 p_screen = HMM_CSToSS(p_clip, vp->window_size);
		UI_DrawPoint(UI_VEC2{p_screen.X, p_screen.Y}, thickness, color);
	}
}

GIZMOS_API void DrawGrid3D(const GizmosViewport* vp, UI_Color color) {
	int grid_extent = 16;
	for (int x = -grid_extent; x <= grid_extent; x++) {
		DrawLine3D(vp, HMM_V3((float)x, -(float)grid_extent, 0.f), HMM_V3((float)x, (float)grid_extent, 0.f), 2.f, color);
	}
	for (int y = -grid_extent; y <= grid_extent; y++) {
		DrawLine3D(vp, HMM_V3(-(float)grid_extent, (float)y, 0.f), HMM_V3((float)grid_extent, (float)y, 0.f), 2.f, color);
	}
}

GIZMOS_API void DrawGridEx3D(const GizmosViewport* vp, vec3 origin, vec3 x_dir, vec3 y_dir, UI_Color color) {
	int grid_extent = 16;

	vec3 x_origin_a = HMM_AddV3(origin, HMM_MulV3F(y_dir, -(float)grid_extent));
	vec3 x_origin_b = HMM_AddV3(origin, HMM_MulV3F(y_dir, (float)grid_extent));
	
	for (int x = -grid_extent; x <= grid_extent; x++) {
		vec3 x_offset = HMM_MulV3F(x_dir, (float)x);
		DrawLine3D(vp, HMM_AddV3(x_origin_a, x_offset), HMM_AddV3(x_origin_b, x_offset), 2.f, color);
	}

	vec3 y_origin_a = HMM_AddV3(origin, HMM_MulV3F(x_dir, -(float)grid_extent));
	vec3 y_origin_b = HMM_AddV3(origin, HMM_MulV3F(x_dir, (float)grid_extent));

	for (int y = -grid_extent; y <= grid_extent; y++) {
		vec3 y_offset = HMM_MulV3F(y_dir, (float)y);
		DrawLine3D(vp, HMM_AddV3(y_origin_a, y_offset), HMM_AddV3(y_origin_b, y_offset), 2.f, color);
	}
}

GIZMOS_API void DrawParallelepiped3D(const GizmosViewport* vp, vec3 min_corner, vec3 extent_x, vec3 extent_y, vec3 extent_z, float thickness, UI_Color color)
{
	vec3 OOO = min_corner;
	vec3 OOI = HMM_AddV3(OOO, extent_z);
	vec3 OIO = HMM_AddV3(OOO, extent_y);
	vec3 OII = HMM_AddV3(OOI, extent_y);
	vec3 IOO = HMM_AddV3(OOO, extent_x);
	vec3 IOI = HMM_AddV3(OOI, extent_x);
	vec3 IIO = HMM_AddV3(OIO, extent_x);
	vec3 III = HMM_AddV3(OII, extent_x);

	// Lines going in the direction of X
	DrawLine3D(vp, OOO, IOO, thickness, color);
	DrawLine3D(vp, OIO, IIO, thickness, color);
	DrawLine3D(vp, OOI, IOI, thickness, color);
	DrawLine3D(vp, OII, III, thickness, color);

	// Lines going in the direction of Y
	DrawLine3D(vp, OOO, OIO, thickness, color);
	DrawLine3D(vp, IOO, IIO, thickness, color);
	DrawLine3D(vp, OOI, OII, thickness, color);
	DrawLine3D(vp, IOI, III, thickness, color);

	// Lines going in the direction of Z
	DrawLine3D(vp, OOO, OOI, thickness, color);
	DrawLine3D(vp, IOO, IOI, thickness, color);
	DrawLine3D(vp, OIO, OII, thickness, color);
	DrawLine3D(vp, IIO, III, thickness, color);
}

GIZMOS_API void DrawCuboid3D(const GizmosViewport* vp, vec3 min, vec3 max, float thickness, UI_Color color) {
	// Lines going in the direction of X
	DrawLine3D(vp, HMM_V3(min.X, min.Y, min.Z), HMM_V3(max.X, min.Y, min.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(min.X, max.Y, min.Z), HMM_V3(max.X, max.Y, min.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(min.X, min.Y, max.Z), HMM_V3(max.X, min.Y, max.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(min.X, max.Y, max.Z), HMM_V3(max.X, max.Y, max.Z), thickness, color);
	
	// Lines going in the direction of Y
	DrawLine3D(vp, HMM_V3(min.X, min.Y, min.Z), HMM_V3(min.X, max.Y, min.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(max.X, min.Y, min.Z), HMM_V3(max.X, max.Y, min.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(min.X, min.Y, max.Z), HMM_V3(min.X, max.Y, max.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(max.X, min.Y, max.Z), HMM_V3(max.X, max.Y, max.Z), thickness, color);
	
	// Lines going in the direction of Z
	DrawLine3D(vp, HMM_V3(min.X, min.Y, min.Z), HMM_V3(min.X, min.Y, max.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(max.X, min.Y, min.Z), HMM_V3(max.X, min.Y, max.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(min.X, max.Y, min.Z), HMM_V3(min.X, max.Y, max.Z), thickness, color);
	DrawLine3D(vp, HMM_V3(max.X, max.Y, min.Z), HMM_V3(max.X, max.Y, max.Z), thickness, color);
}

GIZMOS_API void DrawQuad3D(const GizmosViewport* vp, vec3 a, vec3 b, vec3 c, vec3 d, UI_Color color) {
	HMM_Vec4 a_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(a, 1.f));
	HMM_Vec4 b_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(b, 1.f));
	HMM_Vec4 c_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(c, 1.f));
	HMM_Vec4 d_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(d, 1.f));
	
	if (a_clip.Z > 0.f && b_clip.Z > 0.f && c_clip.Z > 0.f && d_clip.Z > 0.f) {
		vec2 a_screen = HMM_CSToSS(a_clip, vp->window_size);
		vec2 b_screen = HMM_CSToSS(b_clip, vp->window_size);
		vec2 c_screen = HMM_CSToSS(c_clip, vp->window_size);
		vec2 d_screen = HMM_CSToSS(d_clip, vp->window_size);
		
		uint32_t first_vertex;
		UI_DrawVertex* verts = UI_AddVerticesUnsafe(4, &first_vertex);
		verts[0] = UI_DRAW_VERTEX{{a_screen.X, a_screen.Y}, {0, 0}, color};
		verts[1] = UI_DRAW_VERTEX{{b_screen.X, b_screen.Y}, {0, 0}, color};
		verts[2] = UI_DRAW_VERTEX{{c_screen.X, c_screen.Y}, {0, 0}, color};
		verts[3] = UI_DRAW_VERTEX{{d_screen.X, d_screen.Y}, {0, 0}, color};
		UI_AddQuadIndices(first_vertex, first_vertex+1, first_vertex+2, first_vertex+3, NULL);
	}
}

GIZMOS_API void DrawQuadFrame3D(const GizmosViewport* vp, vec3 a, vec3 b, vec3 c, vec3 d, float thickness, UI_Color color) {
	DrawLine3D(vp, a, b, thickness, color);
	DrawLine3D(vp, b, c, thickness, color);
	DrawLine3D(vp, c, d, thickness, color);
	DrawLine3D(vp, d, a, thickness, color);

	/*HMM_Vec4 a_clip = HMM_MulM4V4(vp->clip_from_world, HMM_V4V(a, 1.f));
	HMM_Vec4 b_clip = HMM_MulM4V4(vp->clip_from_world, HMM_V4V(b, 1.f));
	HMM_Vec4 c_clip = HMM_MulM4V4(vp->clip_from_world, HMM_V4V(c, 1.f));
	HMM_Vec4 d_clip = HMM_MulM4V4(vp->clip_from_world, HMM_V4V(d, 1.f));

	if (a_clip.Z > 0.f && b_clip.Z > 0.f && c_clip.Z > 0.f && d_clip.Z > 0.f) {
		vec3 midpoint = a;
		midpoint = HMM_AddV3(midpoint, b);
		midpoint = HMM_AddV3(midpoint, c);
		midpoint = HMM_AddV3(midpoint, d);
		midpoint = HMM_MulV3F(midpoint, 1.f/4.f);

		vec3 a_inner = HMM_LerpV3(a, inset, midpoint);
		vec3 b_inner = HMM_LerpV3(b, inset, midpoint);
		vec3 c_inner = HMM_LerpV3(c, inset, midpoint);
		vec3 d_inner = HMM_LerpV3(d, inset, midpoint);

		vec2 a_screen = ClipSpaceToScreen(vp, a_clip);
		vec2 b_screen = ClipSpaceToScreen(vp, b_clip);
		vec2 c_screen = ClipSpaceToScreen(vp, c_clip);
		vec2 d_screen = ClipSpaceToScreen(vp, d_clip);
		
		vec2 a_inner_screen = ClipSpaceToScreen(vp, HMM_MulM4V4(vp->clip_from_world, HMM_V4V(a_inner, 1.f)));
		vec2 b_inner_screen = ClipSpaceToScreen(vp, HMM_MulM4V4(vp->clip_from_world, HMM_V4V(b_inner, 1.f)));
		vec2 c_inner_screen = ClipSpaceToScreen(vp, HMM_MulM4V4(vp->clip_from_world, HMM_V4V(c_inner, 1.f)));
		vec2 d_inner_screen = ClipSpaceToScreen(vp, HMM_MulM4V4(vp->clip_from_world, HMM_V4V(d_inner, 1.f)));

		uint32_t first_vertex;
		UI_DrawVertex* verts = UI_AddVertices(8, &first_vertex);
		verts[0] = UI_DRAW_VERTEX{{a_screen.X, a_screen.Y}, {0, 0}, color};
		verts[1] = UI_DRAW_VERTEX{{a_inner_screen.X, a_inner_screen.Y}, {0, 0}, color};
		verts[2] = UI_DRAW_VERTEX{{b_screen.X, b_screen.Y}, {0, 0}, color};
		verts[3] = UI_DRAW_VERTEX{{b_inner_screen.X, b_inner_screen.Y}, {0, 0}, color};
		verts[4] = UI_DRAW_VERTEX{{c_screen.X, c_screen.Y}, {0, 0}, color};
		verts[5] = UI_DRAW_VERTEX{{c_inner_screen.X, c_inner_screen.Y}, {0, 0}, color};
		verts[6] = UI_DRAW_VERTEX{{d_screen.X, d_screen.Y}, {0, 0}, color};
		verts[7] = UI_DRAW_VERTEX{{d_inner_screen.X, d_inner_screen.Y}, {0, 0}, color};
		UI_AddQuadIndices(first_vertex+1, first_vertex+0, first_vertex+2, first_vertex+3, UI_TEXTURE_ID_NIL);
		UI_AddQuadIndices(first_vertex+3, first_vertex+2, first_vertex+4, first_vertex+5, UI_TEXTURE_ID_NIL);
		UI_AddQuadIndices(first_vertex+5, first_vertex+4, first_vertex+6, first_vertex+7, UI_TEXTURE_ID_NIL);
		UI_AddQuadIndices(first_vertex+7, first_vertex+6, first_vertex+0, first_vertex+1, UI_TEXTURE_ID_NIL);
	}*/
}

GIZMOS_API void DrawArrow3D(const GizmosViewport* vp, vec3 from, vec3 to,
	float head_length, float head_radius, int vertices, float thickness, UI_Color color)
{
	HMM_Vec4 to_clip = HMM_MulM4V4(vp->camera.clip_from_world, HMM_V4V(to, 1.f));
	head_radius *= to_clip.W;
	head_length *= to_clip.W;

	vec3 dir = HMM_NormV3(HMM_SubV3(to, from));
	vec3 head_base = HMM_SubV3(to, HMM_MulV3F(dir, head_length));

	DrawLine3D(vp, head_base, from, thickness, color);

	vec3 random_nondegenerate_vector = HMM_V3(dir.X == 0.f ? 1.f : 0.f, 1.f, 1.f);
	vec3 tangent = HMM_NormV3(HMM_Cross(dir, random_nondegenerate_vector));
	vec3 bitangent = HMM_Cross(tangent, dir);

	uint32_t first_vertex;
	UI_DrawVertex* vertices_data = UI_AddVerticesUnsafe(vertices+1, &first_vertex);
	
	vec2 to_screen = HMM_CSToSS(to_clip, vp->window_size);
	vertices_data[0] = UI_DRAW_VERTEX{{to_screen.X, to_screen.Y}, {0, 0}, color};

	bool head_is_visible = to_clip.Z > 0.f;
	if (head_is_visible) {

		for (int i = 0; i < vertices; i++) {
			float theta = HMM_PI32 * 2.f * (float)i / (float)vertices;

			vec3 vertex_dir = HMM_AddV3(HMM_MulV3F(tangent, cosf(theta)), HMM_MulV3F(bitangent, sinf(theta)));
			HMM_Vec4 p_world = { HMM_AddV3(head_base, HMM_MulV3F(vertex_dir, head_radius)), 1.f };
			HMM_Vec4 p_clip = HMM_MulM4V4(vp->camera.clip_from_world, p_world);

			vec2 p_screen = HMM_CSToSS(p_clip, vp->window_size);
			vertices_data[1+i] = UI_DRAW_VERTEX{{p_screen.X, p_screen.Y}, {0, 0}, color}; 

			UI_AddTriangleIndices(first_vertex, first_vertex + 1 + i, first_vertex + 1 + ((i + 1) % vertices), NULL);
		}
	}
}
