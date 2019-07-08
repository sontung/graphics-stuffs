#version 440

layout(location = 0) in vec4 in_position;
uniform mat4 viewMx;
uniform mat4 projMx;
out vec2 vTexCoords; 
 
void main() {
    gl_Position = in_position;
}
