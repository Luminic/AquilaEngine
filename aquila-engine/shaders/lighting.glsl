#define PI 3.1415926535

// Source: https://learnopengl.com/PBR/Lighting

float pos_dot(vec3 a, vec3 b) {
    return max(dot(a, b), 0.0);
}

// GGX / Trowbridge-Reitz
float distribution(vec3 n, vec3 h, float roughness) {
    float a2 = roughness * roughness * roughness * roughness;
    float n_dot_h = pos_dot(n, h);
    float denom = (n_dot_h * n_dot_h * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// Schlick GGX
float geometry_Schlick(float n_dot_v, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return n_dot_v / (n_dot_v * (1.0 - k) + k);
}

// Smith GGX
float geometry(vec3 n, vec3 v, vec3 l, float roughness) {
    return geometry_Schlick(pos_dot(n, l), roughness) * 
           geometry_Schlick(pos_dot(n, v), roughness);
}

// Schlick
vec3 fresnel(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

// Adapted from: https://learnopengl.com/PBR/Lighting
// and "Devsh's PBR discussion" https://docs.google.com/document/d/1ZLT1-fIek2JkErN9ZPByeac02nWipMbO89oCW2jxzXo/edit
vec3 BRDF_Cook_Torrance(vec3 n, vec3 v, vec3 l, float roughness, float metalness, vec3 albedo) {
    vec3 h = normalize(v + l);
    float n_dot_v = pos_dot(n, v);
    float n_dot_l = pos_dot(n, l);

    vec3 F0 = 0.04f.xxx;
    F0 = mix(F0, albedo, metalness);

    vec3 ks = fresnel(pos_dot(h, l), F0);
    float specular = geometry(n, v, l, roughness) * distribution(n, h, roughness)
                     / max(4 * n_dot_v * n_dot_l, 0.001);
    
    vec3 kd = 1.0f.xxx;//(1.0f.xxx - fresnel(n_dot_l, F0)) * (1.0f.xxx - fresnel(n_dot_v, F0));
    vec3 diffuse = albedo;

    return ks*specular;// + kd*diffuse;
}