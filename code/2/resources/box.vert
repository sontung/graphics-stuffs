#version 330

uniform vec3 scale;
uniform mat4 projMX;
uniform mat4 viewMX;
uniform mat4 modelMX;

layout(location = 0) in vec4  in_position;

void main() {
    mat4 scaleMX = mat4(scale.x,0,0,0, 0,scale.y,0,0, 0,0,scale.z,0, 0,0,0,1);
    gl_Position = projMX * viewMX * modelMX * scaleMX * in_position;
}
