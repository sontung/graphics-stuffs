#version 330

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_values;

uniform mat4 projMX;
uniform int  channel;

void main() {
    float x = in_position.x*2.0f-1.0f;
    float y = in_values[channel]*2.0f-1.0f;
    
    vec4 vert = vec4(x,y,-1,1);
    gl_Position = vert;
}
