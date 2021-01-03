#version 450

layout (location = 0) out vec4 o_color;

layout (location = 0) in vec2 v_tex_coord;

struct MaterialProperties{
	vec4 albedo; 
	vec2 roughness;
	vec2 metalness;
	vec4 ambient_occlusion;
};

layout (std140, set=1, binding=0) readonly buffer MaterialPropertiesBuffer {
	MaterialProperties material_properties[];
} material_properties_buffer;

layout (set=1, binding=1) uniform sampler samp;
layout (set=1, binding=2) uniform texture2D tex[100];

layout (push_constant) uniform VertConstants {
	layout(offset=64) uint material_index;
} push_constants;

void main() {
	// o_color = texture(sampler2D(tex[42], samp), v_tex_coord);
	o_color = material_properties_buffer.material_properties[push_constants.material_index].albedo;
}