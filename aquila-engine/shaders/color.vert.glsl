#version 450

layout (location = 0) in vec4 a_position;
layout (location = 1) in vec4 a_normal;
layout (location = 2) in vec2 a_tex_coord;

layout (location = 0) out vec4 v_normal;
layout (location = 1) out vec2 v_tex_coord;

layout(set=0, binding=0) uniform CameraBuffer {
	mat4 view_projection;
} camera_data;

layout (push_constant) uniform FragConstants {
	mat4 model;
} push_constants;

void main() {
    gl_Position = (camera_data.view_projection * push_constants.model) * a_position;
	v_normal = a_normal;
	v_tex_coord = a_tex_coord;
}