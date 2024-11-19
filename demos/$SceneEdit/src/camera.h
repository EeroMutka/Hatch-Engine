
struct Camera {
	// In default orientation:
	// +X: forward
	// +Y: left
	// +Z: up
	vec3 pos;
};

void UpdateCamera(Camera* camera, const HT_InputFrame* in) {
	if (InputIsDown(in, HT_InputKey_W)) {
		camera->pos.x += 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_S)) {
		camera->pos.x -= 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_D)) {
		camera->pos.y -= 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_A)) {
		camera->pos.y += 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_E)) {
		camera->pos.z += 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_Q)) {
		camera->pos.z -= 0.1f;
	}
}

