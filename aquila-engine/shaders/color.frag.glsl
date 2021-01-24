#version 450
#extension GL_GOOGLE_include_directive : require

#include "lighting.glsl"

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

layout (push_constant) uniform VertConstants {
	layout(offset=64) uint material_index;
} push_constants;

vec3 BRDF_Cook_Torrance2(vec3 n, vec3 v, vec3 l, float roughness, float metalness, vec3 albedo) {
    vec3 h = normalize(v + l);
    vec3 F0 = 0.04f.xxx;

	vec3 kd = (1.0f.xxx - fresnel(pos_dot(h,l), F0)) * (1.0f.xxx - fresnel(pos_dot(h,v), F0));
	kd *= 1.0f - metalness;
    vec3 diffuse = albedo;

    F0 = mix(F0, albedo, metalness);

    vec3 ks = fresnel(pos_dot(h, l), F0);
    float specular = geometry(n, v, l, roughness) * distribution(n, h, roughness)
                     / max(4 * pos_dot(n, v) * pos_dot(n, l), 0.001);

    return ks*specular + kd*diffuse;
	// return ks*specular;
	// return kd*diffuse;
}

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

	vec3 normal = normalize(v_normal.xyz) * vec3(-1.0f, 1.0f, -1.0f); // Flip the y normal because y is down in Vulkan
	vec3 light_direction = -normalize(vec3(-0.3f, 1.0f, 0.3f));
	vec3 view = normalize(camera.position.xyz - v_position.xyz);

	vec3 BRDF = BRDF_Cook_Torrance2(normal, view, light_direction, roughness, metalness, albedo);
	return BRDF * vec3(1.0f,1.0f,1.0f) * max(dot(normal, light_direction), 0.0f);


	// return max(dot(normal, light_direction),0.0f).xxx;
	// vec3 refl_dir = reflect(light_direction, normal);
	// return pow(max(dot(refl_dir, view), 0.0f), 32).xxx + albedo * max(dot(normal, light_direction),0.0f).xxx;
}

void main() {
	// MaterialProperties mat_props = material_properties_buffer.material_properties[push_constants.material_index];

	// vec3 diffuse_color;

	// if (mat_props.albedo_ti == 0) {
	// 	diffuse_color = mat_props.albedo;
	// } else {
	// 	diffuse_color = texture(sampler2D(tex[mat_props.albedo_ti], samp), v_tex_coord).rgb;
	// }

	// vec3 light_direction = normalize(vec3(-0.3f, 1.0f, 0.3f));
	// vec3 normal = normalize(v_normal.xyz) * vec3(1.0f, -1.0f, 1.0f); // Flip the y normal because y is down in Vulkan
	// float diffuse = max(dot(light_direction, normal), 0.0f);

	o_color = vec4(lighting(), 1.0f);
}