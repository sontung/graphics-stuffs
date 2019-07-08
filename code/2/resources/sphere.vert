#version 330

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec2 in_texCoords;

out vec2 vTexCoords; 
 
void main() {
    gl_Position = in_position;
	vTexCoords = in_texCoords;
}
