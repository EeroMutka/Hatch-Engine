
struct SceneEditState {
	TranslationGizmo translate_gizmo;

	// render parameters
	M_PerspectiveView view;
};

static SceneEdit__EditorCamera* FindSceneEditCamera(HT_API* ht, Scene__Scene* scene) {
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
	return editor_camera;
}

static void SceneEditUpdate(HT_API* ht, SceneEditState* s, HT_Rect viewport_rect, vec2 mouse_pos, Scene__Scene* scene) {
	SceneEdit__EditorCamera* editor_camera = FindSceneEditCamera(ht, scene);
	
	Camera camera = { editor_camera->position, editor_camera->pitch, editor_camera->yaw };
	UpdateCamera(&camera, ht->input_frame);
	editor_camera->position = camera.pos;
	editor_camera->pitch = camera.pitch;
	editor_camera->yaw = camera.yaw;

	vec2 rect_size = viewport_rect.max - viewport_rect.min;
	vec2 rect_middle = (viewport_rect.max + viewport_rect.min) * 0.5f;
	mat4 cs_to_ss = M_MatScale(vec3{0.5f*rect_size.x, 0.5f*rect_size.y, 1.f}) * M_MatTranslate(vec3{rect_middle.x, rect_middle.y, 0});

	mat4 ws_to_cs = camera.ws_to_vs * M_MakePerspectiveMatrix(M_DegToRad * 70.f, rect_size.x / rect_size.y, 0.1f, 1000.f);
	mat4 ws_to_ss = ws_to_cs * cs_to_ss;

	M_PerspectiveView view = {};
	view.has_camera_position = true;
	view.camera_position = camera.pos;
	view.ws_to_ss = ws_to_ss;
	view.ss_to_ws = M_Inverse4x4(ws_to_ss);
	s->view = view;
	
	HT_ItemHandle item_handle = ht->GetSelectedItemHandle();
	HT_ItemHandleDecoded item_handle_decoded = *(HT_ItemHandleDecoded*)&item_handle;

	for (HT_ItemGroupEach(&scene->entities, entity_i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);

		// find box component

		if (item_handle_decoded.index == entity_i) {
			TranslationGizmoUpdate(ht->input_frame, &view, &s->translate_gizmo, mouse_pos, &entity->position, 0.f);
		}
	}
}

// Include rendering functions using fire-UI
#ifdef FIRE_UI_INCLUDED
// gizmo drawing requires fire-UI
static void SceneEditDrawGizmos(HT_API* ht, SceneEditState* s, Scene__Scene* scene) {
	DrawGrid3D(&s->view, UI_DARKGRAY);
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
