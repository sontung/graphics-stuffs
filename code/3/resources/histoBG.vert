#version 330

layout(location = 0) in vec2 in_position;

uniform mat4 projMX;

out vec2 texCoords;

void main() {
    gl_Position = vec4(in_position*vec2(2)-vec2(1),0,1);
    texCoords = in_position*vec2(2)-vec2(1);
}
