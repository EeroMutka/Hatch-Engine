// This file is part of the Hatch codebase, written by Eero Mutka.
// For educational purposes only.

#ifndef MATH_INCLUDED
#error requires <math.h>
#endif

struct Camera {
	vec3 pos;
	quat ori;

	vec3 lazy_pos;
	quat lazy_ori;

	float aspect_ratio, z_near, z_far;

	// The following are cached in `UpdateCamera`, do not set by hand
	mat4 clip_from_world;
	mat4 clip_from_view;
	mat4 view_from_world;
	mat4 view_from_clip;
	mat4 world_from_view;
	mat4 world_from_clip;
};

static vec3 CameraGetRightVector(const Camera* camera) { return camera->world_from_view.Columns[0].xyz; }

// The world & camera coordinates are always right handed. It's either +y is down +z forward, or +y up +z back
#ifdef CAMERA_VIEW_SPACE_IS_POSITIVE_Y_DOWN
static vec3 CameraGetUpVector(const Camera* camera)        { return camera->world_from_view.Columns[1].xyz * -1.f; }
static vec3 CameraGetDownVector(const Camera* camera)      { return camera->world_from_view.Columns[1].xyz; }
static vec3 CameraGetForwardVector(const Camera* camera)   { return camera->world_from_view.Columns[2].xyz; }
#else
static vec3 CameraGetUpVector(const Camera* camera)        { return camera->world_from_view.Columns[1].xyz; }
static vec3 CameraGetDownVector(const Camera* camera)      { return camera->world_from_view.Columns[1].xyz * -1.f; }
static vec3 CameraGetForwardVector(const Camera* camera)   { return camera->world_from_view.Columns[2].xyz * -1.f; }
#endif

static vec3 RotateV3(quat q, vec3 v) {
	// from https://stackoverflow.com/questions/44705398/about-glm-quaternion-rotation
	vec3 a = v * q.w;
	vec3 b = M_Cross(q.xyz, v);
	vec3 c = b + a;
	vec3 d = M_Cross(q.xyz, c);
	return v + d * 2.f;
}

static void UpdateCamera(Camera* camera, const HT_InputFrame* in, float movement_speed, float mouse_speed,
	float FOV, float aspect_ratio_x_over_y, float z_near, float z_far)
{
	if (camera->ori.x == 0 && camera->ori.y == 0 && camera->ori.z == 0 && camera->ori.w == 0) { // reset ori?

#ifdef CAMERA_VIEW_SPACE_IS_POSITIVE_Y_DOWN
		camera->ori = M_QFromAxisAngle_RH(M_V3(1, 0, 0), -M_PI32 / 2.f); // Rotate the camera to face +y (zero rotation would be facing +z)
#else
		camera->ori = M_QFromAxisAngle_RH(M_V3(1, 0, 0), M_PI32 / 2.f); // the M_PI32 / 2.f is to rotate the camera to face +y (zero rotation would be facing -z)
#endif
		//camera->lazy_ori = camera->ori;
	}

	bool has_focus = InputIsDown(in, HT_InputKey_MouseRight) ||
		InputIsDown(in, HT_InputKey_LeftControl) ||
		InputIsDown(in, HT_InputKey_RightControl);

	if (InputIsDown(in, HT_InputKey_MouseRight)) {
		// we need to make rotators for the pitch delta and the yaw delta, and then just multiply the camera's orientation with it!

		float yaw_delta = -mouse_speed * (float)in->raw_mouse_input[0];
		float pitch_delta = -mouse_speed * (float)in->raw_mouse_input[1];

		// So for this, we need to figure out the "right" axis of the camera.
		vec3 cam_right_old = RotateV3(camera->ori, M_V3(1, 0, 0));
		quat pitch_rotator = M_QFromAxisAngle_RH(cam_right_old, pitch_delta);
		camera->ori = M_MulQ(pitch_rotator, camera->ori);

		quat yaw_rotator = M_QFromAxisAngle_RH(M_V3(0, 0, 1), yaw_delta);
		camera->ori = M_MulQ(yaw_rotator, camera->ori);
		camera->ori = M_NormQ(camera->ori);
	}

	if (has_focus) {
		// TODO: have a way to return `CameraGetForwardVector`, `camera_right` and `camera_up` straight from the `world_from_view` matrix
		if (InputIsDown(in, HT_InputKey_Shift)) {
			movement_speed *= 3.f;
		}
		if (InputIsDown(in, HT_InputKey_Control)) {
			movement_speed *= 0.1f;
		}
		if (InputIsDown(in, HT_InputKey_W)) {
			camera->pos = camera->pos + CameraGetForwardVector(camera) * movement_speed;
		}
		if (InputIsDown(in, HT_InputKey_S)) {
			camera->pos = camera->pos + CameraGetForwardVector(camera) * -movement_speed;
		}
		if (InputIsDown(in, HT_InputKey_D)) {
			camera->pos = camera->pos + CameraGetRightVector(camera) * movement_speed;
		}
		if (InputIsDown(in, HT_InputKey_A)) {
			camera->pos = camera->pos + CameraGetRightVector(camera) * -movement_speed;
		}
		if (InputIsDown(in, HT_InputKey_E)) {
			camera->pos = camera->pos + vec3{0, 0, movement_speed};
		}
		if (InputIsDown(in, HT_InputKey_Q)) {
			camera->pos = camera->pos + vec3{0, 0, -movement_speed};
		}
	}

	// Smoothly interpolate lazy position and ori
	camera->lazy_pos = M_LerpV3(camera->lazy_pos, 0.2f, camera->pos);
	camera->lazy_ori = M_SLerp(camera->lazy_ori, 0.2f, camera->ori);
	//camera->lazy_pos = M_LerpV3(camera->lazy_pos, 1.f, camera->pos);
	//camera->lazy_ori = M_SLerp(camera->lazy_ori, 1.f, camera->ori);

	camera->aspect_ratio = aspect_ratio_x_over_y;
	camera->z_near = z_near;
	camera->z_far = z_far;

#if 1
	camera->world_from_view = M_MulM4(M_Translate(camera->lazy_pos), M_QToM4(camera->lazy_ori));
#else
	camera->world_from_view = M_MulM4(M_Translate(camera->pos), M_QToM4(camera->ori));
#endif
	camera->view_from_world = M_InvGeneralM4(camera->world_from_view);//M_QToM4(M_InvQ(camera->ori)) * M_Translate(-1.f * camera->pos);
	
	// "_RH_ZO" means "right handed, zero-to-one NDC z range"
#ifdef CAMERA_VIEW_SPACE_IS_POSITIVE_Y_DOWN
	// Somewhat counter-intuitively, the M_Perspective_LH_ZO function is the true right-handed perspective function. RH_ZO requires a z flip, RH_NO doesn't, and LH_ZO doesn't either.
	camera->clip_from_view = M_Perspective_LH_ZO(M_AngleDeg(FOV), camera->aspect_ratio, camera->z_near, camera->z_far);
#else
	camera->clip_from_view = M_Perspective_RH_ZO(M_AngleDeg(FOV), camera->aspect_ratio, camera->z_near, camera->z_far);
#endif

	camera->view_from_clip = M_InvGeneralM4(camera->clip_from_view);
	
	camera->clip_from_world = M_MulM4(camera->clip_from_view, camera->view_from_world);
	camera->world_from_clip = M_InvGeneralM4(camera->clip_from_world);//camera->view_to_world * camera->clip_to_view;
}