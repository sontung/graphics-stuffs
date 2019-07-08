#version 440

layout(location = 0) in vec2  in_position;

out vec2 texCoords;

void main() {
    gl_Position = vec4(in_position*2-1, 0, 1);
    texCoords = in_position;
}
