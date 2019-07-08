#version 330

layout(location = 0) out vec4 fragColor;

in vec2 texCoords;

void main() {
    fragColor = vec4(0.3,0.3,0.3,0.8);
}
