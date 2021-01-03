#version 450

layout (location = 0) out vec4 o_color;

layout (location = 0) in vec2 v_tex_coord;

layout (set=1, binding=0) uniform sampler2D tex;

struct MaterialProperties{
	vec4 albedo; 
	vec4 roughness;
	vec4 metalness;
	vec4 ambient_occlusion;
};

layout (std140, set=1, binding=1) uniform MaterialPropertiesBuffer {
	MaterialProperties material_properties;
} material_properties_buffer;

void main() {
	// o_color = texture(sampler2D(tex1, samp), v_tex_coord);
	// o_color = texture(tex, v_tex_coord);
	o_color = material_properties_buffer.material_properties.albedo;
}