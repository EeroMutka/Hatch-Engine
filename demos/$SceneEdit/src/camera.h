
struct Camera {
	// In default orientation:
	// +X: forward
	// +Y: left
	// +Z: up
	vec3 pos;
	
	float pitch;
	float yaw;
	
	// computed in UpdateCamera
	mat4 ws_to_vs;
};

static void UpdateCamera(Camera* camera, const HT_InputFrame* in) {
	mat4 rotation_matrix = M_MatRotateY(camera->pitch) * M_MatRotateZ(camera->yaw);
	
	float mov_speed = InputIsDown(in, HT_InputKey_Shift) ? 3.f : 1.f;
	float rot_speed = InputIsDown(in, HT_InputKey_Shift) ? 1.5f : 1.f;
	
	if (InputIsDown(in, HT_InputKey_W)) {
		camera->pos += rotation_matrix.row[0].xyz * 0.1f * mov_speed;
	}
	if (InputIsDown(in, HT_InputKey_S)) {
		camera->pos -= rotation_matrix.row[0].xyz * 0.1f * mov_speed;
	}
	if (InputIsDown(in, HT_InputKey_D)) {
		camera->pos -= rotation_matrix.row[1].xyz * 0.1f * mov_speed;
	}
	if (InputIsDown(in, HT_InputKey_A)) {
		camera->pos += rotation_matrix.row[1].xyz * 0.1f * mov_speed;
	}
	if (InputIsDown(in, HT_InputKey_E)) {
		camera->pos.z += 0.1f * rot_speed;
	}
	if (InputIsDown(in, HT_InputKey_Q)) {
		camera->pos.z -= 0.1f * rot_speed;
	}
	
	if (InputIsDown(in, HT_InputKey_Right)) {
		camera->yaw -= 0.02f * rot_speed;
	}
	if (InputIsDown(in, HT_InputKey_Left)) {
		camera->yaw += 0.02f * rot_speed;
	}
	if (InputIsDown(in, HT_InputKey_Up)) {
		camera->pitch -= 0.02f * rot_speed;
	}
	if (InputIsDown(in, HT_InputKey_Down)) {
		camera->pitch += 0.02f * rot_speed;
	}
	
	// NOTE: view-space has +Y down, and is right-handed (+X right, +Z forward)!!
		// +x -> +z
		// +y -> -x
		// +z -> -y
	camera->ws_to_vs =
		// M_MatRotateZ(0.5f*sinf(time)) *
		M_MatTranslate(-1.f*camera->pos) *
		M_MatRotateZ(-camera->yaw) *
		M_MatRotateY(-camera->pitch) *
		mat4{
			0.f,  0.f,  1.f, 0.f,
			-1.f,  0.f,  0.f, 0.f,
			0.f, -1.f,  0.f, 0.f,
			0.f,  0.f,  0.f, 1.f,
		};
}

