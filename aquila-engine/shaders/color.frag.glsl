#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) out vec4 o_color;

layout (location = 0) in vec4 v_position;
layout (location = 1) in vec4 v_normal;
layout (location = 2) in vec2 v_tex_coord;

layout (constant_id = 0) const int MAX_NR_TEXTURES = 100;

layout(set=0, binding=0) uniform CameraBuffer {
	mat4 view_projection;
	vec4 position;
} camera;

struct MaterialProperties{
	vec3 albedo; 
	uint albedo_ti;

	float roughness;
	uint roughness_ti;

	float metalness;
	uint metalness_ti;

	vec3 ambient;
	uint ambient_occlusion_ti;

	uint normal_ti;
	float fdata0;
	int idata0;
	int idata1;
};

layout (std140, set=1, binding=0) readonly buffer MaterialPropertiesBuffer {
	MaterialProperties material_properties[];
} material_properties_buffer;

layout (set=1, binding=1) uniform sampler samp;
layout (set=1, binding=2) uniform texture2D tex[MAX_NR_TEXTURES];

struct LightProperties{
	vec4 color;
	vec3 position;
	int type; // 0:Point, 1:Sun, 2:Area, 3:Spot
	vec3 direction;
	uint shadow_map_ti;
	vec4 misc;
};

layout (std140, set=1, binding=3) readonly buffer LightPropertiesBuffer {
	LightProperties light_properties[];
} light_properties_buffer;

layout (push_constant) uniform VertConstants {
	layout(offset=64) uint material_index;
	layout(offset=68) uint nr_lights;
} push_constants;

#include "lighting.glsl"

vec3 lighting() {
	MaterialProperties mat_props = material_properties_buffer.material_properties[push_constants.material_index];

	vec3 albedo;
	if (mat_props.albedo_ti == 0) {
		albedo = mat_props.albedo;
	} else {
		albedo = texture(sampler2D(tex[mat_props.albedo_ti], samp), v_tex_coord).rgb;
	}
	float roughness;
	if (mat_props.roughness_ti == 0) {
		roughness = mat_props.roughness;
	} else {
		roughness = texture(sampler2D(tex[mat_props.roughness_ti], samp), v_tex_coord).r;
	}
	float metalness;
	if (mat_props.metalness_ti == 0) {
		metalness = mat_props.metalness;
	} else {
		metalness = texture(sampler2D(tex[mat_props.metalness_ti], samp), v_tex_coord).r;
	}
	vec3 ambient = albedo * mat_props.ambient;
	if (mat_props.ambient_occlusion_ti != 0) {
		ambient *= texture(sampler2D(tex[mat_props.ambient_occlusion_ti], samp), v_tex_coord).r;
	}

	vec3 normal = normalize(v_normal.xyz);
	vec3 view = normalize(camera.position.xyz - v_position.xyz);

	vec3 total_color = ambient;
	for (uint i=0; i<push_constants.nr_lights; ++i) {
		LightProperties light_props = light_properties_buffer.light_properties[i];

		switch (light_props.type) { // 0:Point, 1:Sun, 2:Area, 3:Spot
		case 0: { // Point
			float light_distance = distance(light_props.position, v_position.xyz);
			vec3 light_direction = normalize(light_props.position-v_position.xyz);
			vec3 light_color = light_props.color.xyz * light_props.color.w;
			light_color /= light_distance * light_distance; // falloff

			vec3 BRDF = BRDF_Cook_Torrance(normal, view, light_direction, roughness, metalness, albedo);
			total_color += BRDF * light_color * max(dot(normal, light_direction), 0.0f);
		} break;
		case 1: // Sun
			break;
		case 2: // Area
			break;
		case 3: // Spot
			break;
		default:
			break;
		}
	}

	return total_color;

}

void main() {
	o_color = vec4(lighting(), 1.0f);
}