#version 440

layout(location = 0) in vec4 in_position;

uniform mat4 viewMX;
uniform mat4 projMX;

out float id;
void main() {
    gl_Position = projMX * viewMX * vec4(in_position.xyz, 1.0f);
	gl_PointSize = 5;
	id = in_position.w;
}
