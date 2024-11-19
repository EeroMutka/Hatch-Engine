
struct Camera {
	vec3 pos;
};

void UpdateCamera(Camera* camera, const HT_InputFrame* in) {
	if (InputIsDown(in, HT_InputKey_W)) {
		camera->pos.z += 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_S)) {
		camera->pos.z -= 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_D)) {
		camera->pos.x += 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_A)) {
		camera->pos.x -= 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_E)) {
		camera->pos.y -= 0.1f;
	}
	if (InputIsDown(in, HT_InputKey_Q)) {
		camera->pos.y += 0.1f;
	}
}

