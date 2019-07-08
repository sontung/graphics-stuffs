#version 440

layout(location = 0) out vec4 frag_color;

in vec3 color;

void main() {
    frag_color = vec4(0.5, 0.6, 0.7, 1);
}
