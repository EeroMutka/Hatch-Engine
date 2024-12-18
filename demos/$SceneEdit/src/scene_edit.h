
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

		// find box component

		if (item_handle_decoded.index == entity_i) {
			TranslationGizmoUpdate(ht->input_frame, view, &s->translate_gizmo, mouse_pos, &entity->position, 0.f);
		}
	}

	return camera;
}

// Include rendering functions using fire-UI
#ifdef FIRE_UI_INCLUDED
// gizmo drawing requires fire-UI
static void SceneEditDrawGizmos(HT_API* ht, SceneEditState* s, Scene__Scene* scene) {
	TranslationGizmoDraw(&s->view, &s->translate_gizmo);

	/*for (HT_ItemGroupEach(&scene->entities, entity_i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);

		for (int comp_i = 0; comp_i < entity->components.count; comp_i++) {
			HT_Any* comp_any = &((HT_Any*)entity->components.data)[comp_i];
			if (comp_any->type.handle == ht->types->Scene__BoxComponent) {
				M_PerspectiveView cuboid_view = s->view;
				cuboid_view.ws_to_ss =
					M_MatScale(entity->scale) *
					M_MatRotateX(entity->rotation.x*M_DegToRad) *
					M_MatRotateY(entity->rotation.y*M_DegToRad) *
					M_MatRotateZ(entity->rotation.z*M_DegToRad) *
					M_MatTranslate(entity->position) *
					cuboid_view.ws_to_ss;
				
				DrawCuboid3D(&cuboid_view, vec3{-0.5f, -0.5f, -0.5f}, vec3{0.5f, 0.5f, 0.5f}, 2.f, UI_SKYBLUE);
			}
		}
	}*/
}
#endif
