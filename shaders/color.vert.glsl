#version 450

const vec3 positions[3] = vec3[3](
    vec3( 1.0f, 1.0f, 0.0f),
    vec3(-1.0f, 1.0f, 0.0f),
    vec3( 0.0f,-1.0f, 0.0f)
);

const vec3 colors[3] = vec3[3](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f) 
);

layout (location = 0) out vec3 v_color;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
    v_color = colors[gl_VertexIndex];
}