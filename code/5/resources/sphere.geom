#version 440

uniform mat4 projMX;
uniform mat4 viewMX;
uniform mat4 modelMX;

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;


out vec3 pos;

void main() {
    vec4 v1 = gl_in[0].gl_Position;
    vec4 v2 = gl_in[1].gl_Position;
    vec4 v3 = gl_in[2].gl_Position;
    
    
    gl_Position = projMX * viewMX * modelMX * v1;
	pos = v1.xyz;
    EmitVertex();
    
    gl_Position = projMX * viewMX * modelMX * v2;
	pos = v2.xyz;
    EmitVertex();
    
    gl_Position = projMX * viewMX * modelMX * v3;
	pos = v2.xyz;
    EmitVertex();
    
    EndPrimitive();
}
