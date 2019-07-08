#version 330

layout(location = 0) in vec4 in_position;
layout(location = 1) in float in_color;
layout(location = 2) in vec2 in_angles;


out float color_tri; 
out vec2 angles;
 
void main() {
    gl_Position = in_position;
	color_tri = in_color;
	angles = in_angles;
}
