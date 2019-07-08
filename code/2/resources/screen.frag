#version 330

in vec2 texCoords;

out vec4 fragColor;
uniform sampler2D screenTexture;

void main() {
    fragColor = texture(screenTexture, texCoords);
}
