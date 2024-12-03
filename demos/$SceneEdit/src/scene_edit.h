
static Camera SceneEditUpdate(HT_API* ht, Scene__Scene* scene) {
	Camera camera = {};
	{
		// find or add EditorCamera to the scene extended data
		SceneEdit__EditorCamera* editor_camera = NULL;
		for (int i = 0; i < scene->extended_data.count; i++) {
			HT_Any any = ((HT_Any*)scene->extended_data.data)[i];
			if (any.type.handle == ht->types->SceneEdit__EditorCamera) {
				editor_camera = (SceneEdit__EditorCamera*)any.data;
				break;
			}
		}
		HT_ASSERT(editor_camera); // TODO: add an empty editor camera automatically to the scene if not found!!
		
		camera = {editor_camera->position, editor_camera->pitch, editor_camera->yaw};
		UpdateCamera(&camera, ht->input_frame);
		editor_camera->position = camera.pos;
		editor_camera->pitch = camera.pitch;
		editor_camera->yaw = camera.yaw;
	}
	return camera;
}
