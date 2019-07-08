#version 440

layout (location = 0) in vec3 aPos;

out vec3 Position;

uniform mat4 model;
uniform mat4 viewMX;
uniform mat4 projMX;

void main()
{
    Position =  aPos;
    gl_Position = projMX * viewMX * vec4(aPos, 1.0);
}