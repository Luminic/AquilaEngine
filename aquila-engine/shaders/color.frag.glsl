#version 450

layout (location = 0) out vec4 o_color;

layout (location = 0) in vec2 v_tex_coord;

layout (set=1, binding=0) uniform sampler2D tex1;

void main() {
	o_color = texture(tex1, v_tex_coord);
}