#version 450

layout (location = 0) in vec4 a_position;
layout (location = 1) in vec4 a_normal;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec3 v_color;

layout(set=0, binding=0) uniform CameraBuffer {
	mat4 view_projection;
} camera_data;

layout (push_constant) uniform constants {
	vec4 data;
	mat4 model;
} push_constants;

void main() {
    gl_Position = (camera_data.view_projection * push_constants.model) * a_position;
    v_color = a_color.rgb;
}