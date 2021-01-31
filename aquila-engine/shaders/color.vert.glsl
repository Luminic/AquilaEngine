#version 450

layout (location = 0) in vec4 a_position;
layout (location = 1) in vec4 a_normal;
layout (location = 2) in vec2 a_tex_coord;

layout (location = 0) out vec4 v_position;
layout (location = 1) out vec4 v_normal;
layout (location = 2) out vec2 v_tex_coord;

layout(set=0, binding=0) uniform CameraBuffer {
	mat4 view_projection;
	vec4 position;
} camera;

layout (push_constant) uniform FragConstants {
	mat4 model;
} push_constants;

void main() {
	v_position = push_constants.model * a_position;
	v_normal = vec4(transpose(inverse(mat3(push_constants.model))) * a_normal.xyz, 1.0f);
	v_tex_coord = a_tex_coord;
    gl_Position = camera.view_projection * v_position;
}