#version 440

layout(location = 0) in vec4  in_position;

uniform mat4 viewMX;
uniform mat4 projMX;

void main() {
    vec4 vert = in_position;
    gl_Position = projMX * viewMX * vert;
}
