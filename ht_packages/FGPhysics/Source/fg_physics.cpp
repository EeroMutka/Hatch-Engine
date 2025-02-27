#define HT_STATIC_PLUGIN_ID FGPhysics

#include <hatch_api.h>

#include "../FGPhysics.inc.ht"

#include <vector>

#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>

// -- math functions ----------------------------

// Euler angles defined in XYZ order, in degrees
static vec3 QuatToEulerAnglesXYZ(vec4 q) {
	// Source: https://github.com/ralphtandetzky/num-quaternion/blob/master/src/unit_quaternion.rs (MIT-license, Ralph Tandetzky)
	float sin_pitch = 2.f * (q.w * q.y - q.z * q.x);

	vec3 xyz;
	if (sin_pitch >= 0.99999f) { // Check for gimbal lock, which occurs when sin_pitch is close to 1 or -1
		xyz.x = 0.f;
		xyz.y = M_PI/2.f; // 90 degrees
		xyz.z = -2.f * atan2f(q.x, q.w);
	} else if (sin_pitch <= -0.99999f) {
		xyz.x = 0.f;
		xyz.y = -M_PI/2.f; // -90 degrees
		xyz.z = 2.f * atan2f(q.x, q.w);
	} else {
		xyz.y = asinf(sin_pitch);
		xyz.x = atan2f(2.f * (q.w * q.x + q.y * q.z), 1.f - 2.f * (q.x * q.x + q.y * q.y));
		xyz.z = atan2f(2.f * (q.w * q.z + q.x * q.y), 1.f - 2.f * (q.y * q.y + q.z * q.z));
	}
	xyz *= M_RadToDeg;
	return xyz;
}

// Euler angles defined in XYZ order, in degrees
static vec4 EulerAnglesXYZToQuat(vec3 xyz) {
	xyz *= M_DegToRad;
	// Source: https://github.com/ralphtandetzky/num-quaternion/blob/master/src/unit_quaternion.rs (MIT-license, Ralph Tandetzky)
	float half = 0.5f;
	float sr = sinf(xyz.x * 0.5f);
	float cr = cosf(xyz.x * half);
	float sp = sinf(xyz.y * 0.5f);
	float cp = cosf(xyz.y * 0.5f);
	float sy = sinf(xyz.z * 0.5f);
	float cy = cosf(xyz.z * 0.5f);
	return {
		sr * cp * cy - cr * sp * sy,
		cr * sp * cy + sr * cp * sy,
		cr * cp * sy - sr * sp * cy,
		cr * cp * cy + sr * sp * sy,
	};
}

// ----------------------------------------------

#define FIND_COMPONENT(HT, ENTITY, T) FindComponent<T>(ENTITY, HT->types->T)

template<typename T>
static T* FindComponent(Scene__SceneEntity* entity, HT_Asset struct_type) {
	T* result = NULL;
	for (int i = 0; i < entity->components.count; i++) {
		HT_Any component = ((HT_Any*)entity->components.data)[i];
		if (component.type.handle == struct_type) {
			result = (T*)component.data;
			break;
		}
	}
	return result;
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

struct PhysicsBody
{
	Scene__SceneEntity* entity;
	bool is_sphere;
};

static void StartSimulation(HT_API* ht, Scene__Scene* scene) {
}

static void EndSimulation(HT_API* ht, Scene__Scene* scene) {
}

static const vec4 CUBE_CORNERS[8] = {
	{-0.5f, -0.5f, -0.5f, 1.f},
	{+0.5f, -0.5f, -0.5f, 1.f},
	{-0.5f, +0.5f, -0.5f, 1.f},
	{+0.5f, +0.5f, -0.5f, 1.f},
	{-0.5f, -0.5f, +0.5f, 1.f},
	{+0.5f, -0.5f, +0.5f, 1.f},
	{-0.5f, +0.5f, +0.5f, 1.f},
	{+0.5f, +0.5f, +0.5f, 1.f},
};

static const float SPHERE_RADIUS = 0.5f;

static void ResolveCollisionSphereAndSphere(PhysicsBody& a, PhysicsBody& b)
{
	vec3 a_to_b = b.entity->position - a.entity->position;
	float fix_dist = M_Len3(a_to_b) - SPHERE_RADIUS*2.f;
	if (fix_dist < 0.f)
	{
		b.entity->position -= 0.5f * M_Norm3(a_to_b) * fix_dist;
		a.entity->position += 0.5f * M_Norm3(a_to_b) * fix_dist;
	}
}

static void ResolveCollisionBoxAndSphere(PhysicsBody& a, PhysicsBody& b)
{
	mat4 a_local_to_world =
		M_MatRotateX(a.entity->rotation.x * M_DegToRad) *
		M_MatRotateY(a.entity->rotation.y * M_DegToRad) *
		M_MatRotateZ(a.entity->rotation.z * M_DegToRad) *
		M_MatTranslate(a.entity->position);

	mat4 a_world_to_local =
		M_MatTranslate(a.entity->position * -1.f) *
		M_MatRotateZ(a.entity->rotation.z * -M_DegToRad) *
		M_MatRotateY(a.entity->rotation.y * -M_DegToRad) *
		M_MatRotateX(a.entity->rotation.x * -M_DegToRad);

	vec3 p = (vec4{b.entity->position, 1.f} * a_world_to_local).xyz;
	
	float dx[2] = { p.x - 0.5f, -p.x - 0.5f }; // 0 is +X plane, 1 is -X plane
	float dy[2] = { p.y - 0.5f, -p.y - 0.5f };
	float dz[2] = { p.z - 0.5f, -p.z - 0.5f };
	
	vec3 surf_to_p = {};
	bool is_corner = true;
	/**/ if (dx[0] > 0.f && dy[0] > 0.f && dz[0] > 0.f)  surf_to_p = p - vec3{+0.5f, +0.5f, +0.5f};
	else if (dx[1] > 0.f && dy[0] > 0.f && dz[0] > 0.f)  surf_to_p = p - vec3{-0.5f, +0.5f, +0.5f};
	else if (dx[0] > 0.f && dy[1] > 0.f && dz[0] > 0.f)  surf_to_p = p - vec3{+0.5f, -0.5f, +0.5f};
	else if (dx[1] > 0.f && dy[1] > 0.f && dz[0] > 0.f)  surf_to_p = p - vec3{-0.5f, -0.5f, +0.5f};
	else if (dx[0] > 0.f && dy[0] > 0.f && dz[1] > 0.f)  surf_to_p = p - vec3{+0.5f, +0.5f, -0.5f};
	else if (dx[1] > 0.f && dy[0] > 0.f && dz[1] > 0.f)  surf_to_p = p - vec3{-0.5f, +0.5f, -0.5f};
	else if (dx[0] > 0.f && dy[1] > 0.f && dz[1] > 0.f)  surf_to_p = p - vec3{+0.5f, -0.5f, -0.5f};
	else if (dx[1] > 0.f && dy[1] > 0.f && dz[1] > 0.f)  surf_to_p = p - vec3{-0.5f, -0.5f, -0.5f};
	else is_corner = false;

	bool is_inside = false;
	bool is_face = false;
	if (!is_corner)
	{
		if (dy[0] < 0.f && dy[1] < 0.f && dz[0] < 0.f && dz[1] < 0.f)
		{
			/**/ if (dx[0] > 0.f) surf_to_p = vec3{p.x - 0.5f, 0.f, 0.f};
			else if (dx[1] > 0.f) surf_to_p = vec3{p.x + 0.5f, 0.f, 0.f};
			else { surf_to_p = vec3{p.x - 0.5f, 0.f, 0.f}; is_inside = true; }
			is_face = true;
		}
		else if (dz[0] < 0.f && dz[1] < 0.f && dx[0] < 0.f && dx[1] < 0.f)
		{
			/**/ if (dy[0] > 0.f) surf_to_p = vec3{0.f, p.y - 0.5f, 0.f};
			else if (dy[1] > 0.f) surf_to_p = vec3{0.f, p.y + 0.5f, 0.f};
			else { surf_to_p = vec3{0.f, p.y - 0.5f, 0.f}; is_inside = true; }
			is_face = true;
		}
		else if (dx[0] < 0.f && dx[1] < 0.f && dy[0] < 0.f && dy[1] < 0.f)
		{
			/**/ if (dz[0] > 0.f) surf_to_p = vec3{0.f, 0.f, p.z - 0.5f};
			else if (dz[1] > 0.f) surf_to_p = vec3{0.f, 0.f, p.z + 0.5f};
			else { surf_to_p = vec3{0.f, 0.f, p.z - 0.5f}; is_inside = true; }
			is_face = true;
		}
	}

	if (!is_corner && !is_face)
	{
		// xy
		/**/ if (dx[0] > 0.f && dy[0] > 0.f)  surf_to_p = vec3{p.x - 0.5f, p.y - 0.5f, 0.f};
		else if (dx[1] > 0.f && dy[0] > 0.f)  surf_to_p = vec3{p.x + 0.5f, p.y - 0.5f, 0.f};
		else if (dx[0] > 0.f && dy[1] > 0.f)  surf_to_p = vec3{p.x - 0.5f, p.y + 0.5f, 0.f};
		else if (dx[1] > 0.f && dy[1] > 0.f)  surf_to_p = vec3{p.x + 0.5f, p.y + 0.5f, 0.f};
		// yz
		else if (dy[0] > 0.f && dz[0] > 0.f)  surf_to_p = vec3{0.f, p.y - 0.5f, p.z - 0.5f};
		else if (dy[1] > 0.f && dz[0] > 0.f)  surf_to_p = vec3{0.f, p.y + 0.5f, p.z - 0.5f};
		else if (dy[0] > 0.f && dz[1] > 0.f)  surf_to_p = vec3{0.f, p.y - 0.5f, p.z + 0.5f};
		else if (dy[1] > 0.f && dz[1] > 0.f)  surf_to_p = vec3{0.f, p.y + 0.5f, p.z + 0.5f};
		// zx
		else if (dz[0] > 0.f && dx[0] > 0.f)  surf_to_p = vec3{p.x - 0.5f, 0.f, p.z - 0.5f};
		else if (dz[1] > 0.f && dx[0] > 0.f)  surf_to_p = vec3{p.x - 0.5f, 0.f, p.z + 0.5f};
		else if (dz[0] > 0.f && dx[1] > 0.f)  surf_to_p = vec3{p.x + 0.5f, 0.f, p.z - 0.5f};
		else if (dz[1] > 0.f && dx[1] > 0.f)  surf_to_p = vec3{p.x + 0.5f, 0.f, p.z + 0.5f};
		else HT_ASSERT(0);
	}

	surf_to_p = (vec4{surf_to_p, 0.f} * a_local_to_world).xyz;
	vec3 towards_outside_dir = M_Norm3(surf_to_p);
	
	float signed_dist = M_Len3(surf_to_p);
	if (is_inside) {
		signed_dist *= -1.f;
		towards_outside_dir *= -1.f;
	}

	float fix_dist = signed_dist - SPHERE_RADIUS;
	
	if (fix_dist < 0.f) {
		b.entity->position -= 0.5f * towards_outside_dir * fix_dist;
		a.entity->position += 0.5f * towards_outside_dir * fix_dist;
	}
}

static void ResolveCollisionBoxAndBox(PhysicsBody& a, PhysicsBody& b)
{
	// Loop through each plane of mesh A to see if it's a separating plane, then vice versa
	// if there is a separating plane, there is NO collision.

	// A plane is stored as the a, c, b, d coefficients to the plane equation (ax + bx + cx + d = 0)
	vec4 a_planes[6];
	vec4 b_planes[6];

	mat4 a_local_to_world =
		M_MatRotateX(a.entity->rotation.x * M_DegToRad) *
		M_MatRotateY(a.entity->rotation.y * M_DegToRad) *
		M_MatRotateZ(a.entity->rotation.z * M_DegToRad) *
		M_MatTranslate(a.entity->position);
	mat4 b_local_to_world =
		M_MatRotateX(b.entity->rotation.x * M_DegToRad) *
		M_MatRotateY(b.entity->rotation.y * M_DegToRad) *
		M_MatRotateZ(b.entity->rotation.z * M_DegToRad) *
		M_MatTranslate(b.entity->position);
	
	vec3 a_corners[8];
	for (int i = 0; i < 8; i++)
		a_corners[i] = (CUBE_CORNERS[i] * a_local_to_world).xyz;

	vec3 b_corners[8];
	for (int i = 0; i < 8; i++)
		b_corners[i] = (CUBE_CORNERS[i] * b_local_to_world).xyz;

	a_planes[0].xyz = a_local_to_world.row[0].xyz; // +X plane
	a_planes[0].w = -M_Dot3(a_planes[0].xyz, a_corners[7]); // solve d from the positive X,Y,Z corner point (dot(plane_abc, corner_point) + d = 0)
	a_planes[1].xyz = a_local_to_world.row[1].xyz; // +Y plane
	a_planes[1].w = -M_Dot3(a_planes[1].xyz, a_corners[7]); // solve d
	a_planes[2].xyz = a_local_to_world.row[2].xyz; // +Z plane
	a_planes[2].w = -M_Dot3(a_planes[2].xyz, a_corners[7]); // solve d
	a_planes[3].xyz = a_local_to_world.row[0].xyz * -1.f; // -X plane
	a_planes[3].w = -M_Dot3(a_planes[3].xyz, a_corners[0]); // solve d
	a_planes[4].xyz = a_local_to_world.row[1].xyz * -1.f; // -Y plane
	a_planes[4].w = -M_Dot3(a_planes[4].xyz, a_corners[0]); // solve d
	a_planes[5].xyz = a_local_to_world.row[2].xyz * -1.f; // -Z plane
	a_planes[5].w = -M_Dot3(a_planes[5].xyz, a_corners[0]); // solve d

	b_planes[0].xyz = b_local_to_world.row[0].xyz; // +X plane
	b_planes[0].w = -M_Dot3(b_planes[0].xyz, b_corners[7]); // solve d from the positive X,Y,Z corner point (dot(plane_abc, corner_point) + d = 0)
	b_planes[1].xyz = b_local_to_world.row[1].xyz; // +Y plane
	b_planes[1].w = -M_Dot3(b_planes[1].xyz, b_corners[7]); // solve d
	b_planes[2].xyz = b_local_to_world.row[2].xyz; // +Z plane
	b_planes[2].w = -M_Dot3(b_planes[2].xyz, b_corners[7]); // solve d
	b_planes[3].xyz = b_local_to_world.row[0].xyz * -1.f; // -X plane
	b_planes[3].w = -M_Dot3(b_planes[3].xyz, b_corners[0]); // solve d
	b_planes[4].xyz = b_local_to_world.row[1].xyz * -1.f; // -Y plane
	b_planes[4].w = -M_Dot3(b_planes[4].xyz, b_corners[0]); // solve d
	b_planes[5].xyz = b_local_to_world.row[2].xyz * -1.f; // -Z plane
	b_planes[5].w = -M_Dot3(b_planes[5].xyz, b_corners[0]); // solve d

	bool a_has_separating_plane = false;
	bool b_has_separating_plane = false;

	float max_neg_d = -100000000.f;
	vec3 max_neg_d_dir = {};

	for (int i = 0; i < 6; i++)
	{
		vec4 a_plane = a_planes[i];

		bool all_points_are_outside = true;
		float min_d = 1000000.f;
		for (int j = 0; j < 8; j++)
		{
			vec3 b_point = b_corners[j];
			float d = M_Dot3(a_plane.xyz, b_point) + a_plane.w;
			if (d < 0)
			{
				if (d < min_d)
					min_d = d;
				all_points_are_outside = false;
			}
		}
		if (min_d > max_neg_d)
		{
			max_neg_d = min_d;
			max_neg_d_dir = a_plane.xyz;
		}

		if (all_points_are_outside)
		{
			a_has_separating_plane = true;
			break;
		}
	}

	for (int i = 0; i < 6; i++)
	{
		vec4 b_plane = b_planes[i];

		bool all_points_are_outside = true;
		float min_d = 1000000.f;
		for (int j = 0; j < 8; j++)
		{
			vec3 a_point = a_corners[j];
			float d = M_Dot3(b_plane.xyz, a_point) + b_plane.w;
			if (d < 0)
			{
				if (d < min_d)
					min_d = d;

				all_points_are_outside = false;
			}
		}
		if (min_d > max_neg_d)
		{
			max_neg_d = min_d;
			max_neg_d_dir = b_plane.xyz * -1.f;
		}

		if (all_points_are_outside)
		{
			b_has_separating_plane = true;
			break;
		}
	}

	if (!a_has_separating_plane && !b_has_separating_plane)
	{
		b.entity->position -= 0.5f * max_neg_d_dir * max_neg_d;
		a.entity->position += 0.5f * max_neg_d_dir * max_neg_d;
	}
}

static bool BoxRaycast(PhysicsBody& b, vec3 ray_pos, vec3 ray_dir, float* out_t, vec3* out_p)
{
	mat4 local_to_world =
		M_MatRotateX(b.entity->rotation.x * M_DegToRad) *
		M_MatRotateY(b.entity->rotation.y * M_DegToRad) *
		M_MatRotateZ(b.entity->rotation.z * M_DegToRad) *
		M_MatTranslate(b.entity->position);

	vec4 planes[6];
	vec3 corners[8];
	for (int i = 0; i < 8; i++)
		corners[i] = (CUBE_CORNERS[i] * local_to_world).xyz;

	planes[0].xyz = local_to_world.row[0].xyz; // +X plane
	planes[0].w = -M_Dot3(planes[0].xyz, corners[7]); // solve d from the positive X,Y,Z corner point (dot(plane_abc, corner_point) + d = 0)
	planes[1].xyz = local_to_world.row[1].xyz; // +Y plane
	planes[1].w = -M_Dot3(planes[1].xyz, corners[7]); // solve d
	planes[2].xyz = local_to_world.row[2].xyz; // +Z plane
	planes[2].w = -M_Dot3(planes[2].xyz, corners[7]); // solve d
	planes[3].xyz = local_to_world.row[0].xyz * -1.f; // -X plane
	planes[3].w = -M_Dot3(planes[3].xyz, corners[0]); // solve d
	planes[4].xyz = local_to_world.row[1].xyz * -1.f; // -Y plane
	planes[4].w = -M_Dot3(planes[4].xyz, corners[0]); // solve d
	planes[5].xyz = local_to_world.row[2].xyz * -1.f; // -Z plane
	planes[5].w = -M_Dot3(planes[5].xyz, corners[0]); // solve d

	float min_t = 1000000.f;
	int hit_count = 0;
	bool result = false;
	for (int i = 0; i < 6; i++)
	{
		float t; vec3 p;
		if (M_RayPlaneIntersect(ray_pos, ray_dir, planes[i], &t, &p))
		{
			// is point behind all other planes?
			bool is_in_box = true;
			for (int j = 0; j < 6; j++)
			{
				if (j != i && M_Dot3(planes[j].xyz, p) + planes[j].w > 0.f)
					is_in_box = false;
			}
			if (t < min_t && is_in_box)
			{
				*out_p = p;
				min_t = t;
				result = true;
			}
			hit_count++;
		}
	}
	*out_t = min_t;
	return result;
}

static bool SphereRaycast(PhysicsBody& s, vec3 ray_pos, vec3 ray_dir, float* out_t, vec3* out_p)
{
	float r = SPHERE_RADIUS;
	vec3 ro = ray_pos - s.entity->position; // make the ray relative to the sphere position so that we can say that the sphere is at the origin
	vec3 rd = ray_dir;

	// sphere equation
	// sqrt((px - sx)^2 + (py - sy)^2 + (pz - sz)^2) = r
	
	// ray equation
	// p = ro + rd*t
	
	// (ro.x + rd.x*t)^2 + (ro.y + rd.y*t)^2 + (ro.z + rd.z*t)^2 = r^2
	// ro.x^2 + 2*ro.x*rd.x*t + rd.x^2*t^2  +  ...  = r^2
	// (ro.x^2 + ro.y^2 + ro.z^2) + 2*(ro.x*rd.x + ro.y*rd.y + ro.z*rd.z)*t + (rd.x^2 + rd.y^2 + rd.z^2)*t^2 = r^2

	//float T = 0.13732338f;
	//float TEST = (ro.x*ro.x + ro.y*ro.y + ro.z*ro.z) + (ro.x*rd.x + ro.y*rd.y + ro.z*rd.z)*T + (rd.x*rd.x + rd.y*rd.y + rd.z*rd.z)*T*T - r*r;
	//vec3 test_p = ro + rd*T;

	// use the quadratic formula
	float a = rd.x*rd.x + rd.y*rd.y + rd.z*rd.z;
	float b = 2.f * (ro.x*rd.x + ro.y*rd.y + ro.z*rd.z);
	float c = ro.x*ro.x + ro.y*ro.y + ro.z*ro.z - r*r;
	float discriminant = b*b - 4*a*c;
	if (discriminant < 0)
		return false;

	float t = (-b - sqrtf(discriminant)) / (2.f*a);
	*out_t = t;
	*out_p = ray_pos + ray_dir*t;
	return t >= 0.f;
}

static void SimulateScene(HT_API* ht, Scene__Scene* scene)
{
	std::vector<PhysicsBody> bodies;

	Scene__SceneEntity* viz_entity = NULL; // for debugging

	for (HT_ItemGroupEach(&scene->entities, entity_i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);
		Scene__MeshComponent* mesh_component = FIND_COMPONENT(ht, entity, Scene__MeshComponent);
		FGPhysics__FGPhysicsComponent* body_component = FIND_COMPONENT(ht, entity, FGPhysics__FGPhysicsComponent);

		if (body_component)
		{
			FGPhysics__FGBoxCollisionComponent* box_collision_component = FIND_COMPONENT(ht, entity, FGPhysics__FGBoxCollisionComponent);
			FGPhysics__FGSphereCollisionComponent* sphere_collision_component = FIND_COMPONENT(ht, entity, FGPhysics__FGSphereCollisionComponent);
			if (sphere_collision_component || box_collision_component)
			{
				PhysicsBody body;
				body.entity = entity;
				body.is_sphere = sphere_collision_component != NULL;
				bodies.push_back(body);
			}
			else
				viz_entity = entity;
		}
	}
	
	vec3 camera_pos = {};
	vec3 camera_dir = {};
	SceneEdit__EditorCamera* camera = NULL;
	for (int i = 0; i < scene->extended_data.count; i++) {
		HT_Any any = ((HT_Any*)scene->extended_data.data)[i];
		if (any.type.handle == ht->types->SceneEdit__EditorCamera) {
			camera = (SceneEdit__EditorCamera*)any.data;
			camera_pos = camera->position;
			camera_dir = (vec4{1.f, 0.f, 0.f, 0.f} * M_MatRotateY(camera->pitch) * M_MatRotateZ(camera->yaw)).xyz;
		}
	}

	//if (ht->input_frame->key_is_down[HT_InputKey_0]) // cast ray!
	{
		float min_ray_t = 100000000.f;
		vec3 min_ray_p = {};

		// Iterate through all body pairs
		for (int i = 0; i < bodies.size(); i++)
		{
			PhysicsBody& a = bodies[i];
			if (a.is_sphere)
			{
				vec3 p; float t;
				if (SphereRaycast(a, camera_pos, camera_dir, &t, &p) && t < min_ray_t)
				{
					min_ray_t = t;
					min_ray_p = p;
				}
			}
			else
			{
				vec3 p; float t;
				if (BoxRaycast(a, camera_pos, camera_dir, &t, &p) && t < min_ray_t)
				{
					min_ray_t = t;
					min_ray_p = p;
				}
			}
		}
		
		if (viz_entity)
			viz_entity->position = min_ray_p;
	}

	// Iterate through all body pairs
	for (int i = 0; i < bodies.size(); i++)
	{
		PhysicsBody& a = bodies[i];
		
		for (int j = i + 1; j < bodies.size(); j++)
		{
			PhysicsBody& b = bodies[j];

			if (!a.is_sphere && !b.is_sphere)
				ResolveCollisionBoxAndBox(a, b);
			else if (a.is_sphere && !b.is_sphere)
				ResolveCollisionBoxAndSphere(b, a);
			else if (!a.is_sphere && b.is_sphere)
				ResolveCollisionBoxAndSphere(a, b);
			else
				ResolveCollisionSphereAndSphere(a, b);
		}
	}
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	static bool was_simulating = false;

	Scene__Scene* scene = NULL;
	int open_scenes_count = 0;
	HT_Asset* open_scenes = ht->GetAllOpenAssetsOfType(ht->types->Scene__Scene, &open_scenes_count);
	for (int i = 0; i < open_scenes_count; i++) {
		scene = HT_GetAssetData(Scene__Scene, ht, open_scenes[i]);
	}

	bool is_simulating = ht->IsSimulating();
	if (is_simulating && !was_simulating) {
		StartSimulation(ht, scene);
	}
	if (is_simulating) {
		SimulateScene(ht, scene);
	}
	if (!is_simulating && was_simulating) {
		EndSimulation(ht, scene);
	}
	was_simulating = is_simulating;
}
