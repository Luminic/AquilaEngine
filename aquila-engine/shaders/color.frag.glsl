#version 450

layout (location = 0) out vec4 o_color;

layout (location = 0) in vec4 v_normal;
layout (location = 1) in vec2 v_tex_coord;

layout (constant_id = 0) const int MAX_NR_TEXTURES = 100;

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

void main() {
	MaterialProperties mat_props = material_properties_buffer.material_properties[push_constants.material_index];

	vec3 diffuse_color;

	if (mat_props.albedo_ti == 0) {
		diffuse_color = mat_props.albedo;
	} else {
		diffuse_color = texture(sampler2D(tex[mat_props.albedo_ti], samp), v_tex_coord).rgb;
	}

	vec3 light_direction = normalize(vec3(-0.3f, 1.0f, 0.3f));
	vec3 normal = normalize(v_normal.xyz) * vec3(1.0f, -1.0f, 1.0f); // Flip the y normal because y is down in Vulkan
	float diffuse = max(dot(light_direction, normal), 0.0f);

	o_color = vec4(diffuse_color * diffuse, 1.0f);
}