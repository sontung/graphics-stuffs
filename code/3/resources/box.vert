#version 330

layout(location = 0) in vec4  in_position;

uniform mat4 viewMX;
uniform mat4 projMX;
uniform vec3 translate;

void main() {
    vec4 vert = in_position - vec4(translate,0);
    gl_Position = projMX * viewMX * vert;
}
