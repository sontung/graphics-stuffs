#version 440

layout(location = 0) in vec3 in_position;

uniform mat4 viewMX;
uniform mat4 projMX;

out vec3 color;

void main() {
	vec4 pos = projMX * viewMX * vec4(in_position, 1.0);
    gl_Position = pos;
	color = in_position;
}
