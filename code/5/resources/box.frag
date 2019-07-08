#version 440

layout(location = 0) out vec4 frag_color;

in vec3 TexCoords;
uniform samplerCube skybox;

void main() {
    frag_color = texture(skybox, TexCoords);
}
