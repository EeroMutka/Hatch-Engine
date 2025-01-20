
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
	
	HT_ItemHandle selected_h = ht->GetSelectedItemHandle();
	if (selected_h) {
		Scene__SceneEntity* selected = HT_GetItem(Scene__SceneEntity, &scene->entities, ((HT_ItemHandleDecoded*)&selected_h)->index);
		TranslationGizmoUpdate(ht->input_frame, &view, &s->translate_gizmo, mouse_pos, &selected->position, 0.f);
	}

	//for (HT_ItemGroupEach(&scene->entities, entity_i)) {
	//	Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);
	//
	//	if (item_handle_decoded.index == entity_i) {
	//		TranslationGizmoUpdate(ht->input_frame, &view, &s->translate_gizmo, mouse_pos, &entity->position, 0.f);
	//	}
	//}
}

// Include rendering functions using fire-UI
#ifdef FIRE_UI_INCLUDED
// gizmo drawing requires fire-UI
static void SceneEditDrawGizmos(HT_API* ht, SceneEditState* s, Scene__Scene* scene) {
	//DrawGrid3D(&s->view, UI_DARKGRAY);

	HT_ItemHandle selected_h = ht->GetSelectedItemHandle();
	if (selected_h) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, ((HT_ItemHandleDecoded*)&selected_h)->index);

		TranslationGizmoDraw(&s->view, &s->translate_gizmo);

		for (int comp_i = 0; comp_i < entity->components.count; comp_i++) {
			HT_Any* comp = &((HT_Any*)entity->components.data)[comp_i];
			if (comp->type.handle == ht->types->Scene__DirectionalLightComponent) {

				mat4 local_to_world =
					M_MatRotateX(entity->rotation.x * M_DegToRad) *
					M_MatRotateY(entity->rotation.y * M_DegToRad) *
					M_MatRotateZ(entity->rotation.z * M_DegToRad);
				vec3 light_neg_dir = local_to_world.row[2].xyz;

				DrawArrow3D(&s->view, entity->position, entity->position - light_neg_dir, 0.1f, 0.03f, 8, 5.f, UI_WHITE);
			}
			else if (comp->type.handle == ht->types->Scene__PointLightComponent) {
				DrawEllipse3D(&s->view, entity->position, vec3{1, 0, 0}, vec3{0, 1, 0}, 5.f, UI_WHITE);
				DrawEllipse3D(&s->view, entity->position, vec3{0, 1, 0}, vec3{0, 0, 1}, 5.f, UI_WHITE);
				DrawEllipse3D(&s->view, entity->position, vec3{0, 0, 1}, vec3{1, 0, 0}, 5.f, UI_WHITE);
			}
			else if (comp->type.handle == ht->types->Scene__SpotLightComponent) {
				mat4 local_to_world =
					M_MatRotateX(entity->rotation.x * M_DegToRad) *
					M_MatRotateY(entity->rotation.y * M_DegToRad) *
					M_MatRotateZ(entity->rotation.z * M_DegToRad);
				vec3 x_extent = local_to_world.row[0].xyz * 1.f;
				vec3 y_extent = local_to_world.row[1].xyz * 1.f;
				vec3 light_neg_dir = local_to_world.row[2].xyz;

				for (float f = 0.f; f < 2.f; f += 0.51f) {
					DrawEllipse3D(&s->view, entity->position - f*light_neg_dir, f*x_extent, f*y_extent, 5.f, UI_WHITE);
				}
			}
		}
	}

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
