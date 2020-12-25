#version 450

layout (location = 0) in vec4 a_position;
layout (location = 1) in vec4 a_normal;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec3 v_color;

void main() {
    gl_Position = a_position;
    v_color = a_color.rgb;
}