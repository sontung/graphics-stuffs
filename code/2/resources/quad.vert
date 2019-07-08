#version 330

layout(location = 1) in vec2  in_position;
layout(location = 0) in vec2  in_tex_coord;

out vec2 texCoords;

void main() {    
    gl_Position = vec4(in_position,0,1);       
    texCoords = in_tex_coord;
}
