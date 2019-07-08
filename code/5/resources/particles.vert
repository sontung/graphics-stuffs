#version 440

layout(location = 0) in vec4  in_position;

uniform mat4 modelMX;
uniform mat4 viewMX;
uniform mat4 projMX;



void main() {
    vec4 vert = projMX * viewMX * modelMX * vec4(in_position.xyz, 1);

    gl_Position = vert;
	gl_PointSize = 3;
}
