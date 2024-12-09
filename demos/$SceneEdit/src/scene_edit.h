
struct SceneEditState {
	TranslationGizmo translate_gizmo;

	// draw parameters
	M_PerspectiveView view;
};

static Camera SceneEditUpdate(HT_API* ht, SceneEditState* s, const M_PerspectiveView* view, vec2 mouse_pos, Scene__Scene* scene) {
	s->view = *view;

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
	
	HT_ItemHandle item_handle = ht->GetSelectedItemHandle();
	HT_ItemHandleDecoded item_handle_decoded = *(HT_ItemHandleDecoded*)&item_handle;

	for (HT_ItemGroupEach(&scene->entities, entity_i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);
		if (item_handle_decoded.index == entity_i) {
			TranslationGizmoUpdate(ht->input_frame, view, &s->translate_gizmo, mouse_pos, &entity->position, 0.f);

			//DrawArrow3D(view, entity->position, entity->position + vec3{1.f, 0.f, 0.f}, 0.03f, 0.012f, 12, 5.f, UI_RED);
			//DrawArrow3D(view, entity->position, entity->position + vec3{0.f, 1.f, 0.f}, 0.03f, 0.012f, 12, 5.f, UI_GREEN);
			//DrawArrow3D(view, entity->position, entity->position + vec3{0.f, 0.f, 1.f}, 0.03f, 0.012f, 12, 5.f, UI_BLUE);
		}
	}

	return camera;
}

// gizmo drawing requires fire-UI
static void SceneEditDrawGizmos(HT_API* ht, SceneEditState* s, Scene__Scene* scene) {
	TranslationGizmoDraw(&s->view, &s->translate_gizmo);
}
