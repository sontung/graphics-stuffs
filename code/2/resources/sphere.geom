#version 330

uniform mat4 projMX;
uniform mat4 viewMX;
uniform mat4 modelMX;

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;

in vec2 vTexCoords[];

out vec2 tex_coords;
out vec3 normal;

void main() {
    vec4 v1 = gl_in[0].gl_Position;
    vec4 v2 = gl_in[1].gl_Position;
    vec4 v3 = gl_in[2].gl_Position;
    
    
    gl_Position = projMX * viewMX * modelMX * v1;
    tex_coords = vTexCoords[0];
	normal = v1.xyz / vec3(0.3f); // normalize by dividing to sphere radius;
	normal = (normal + vec3(1.0))/vec3(2.0);

    EmitVertex();
    
    gl_Position = projMX * viewMX * modelMX * v2;
    tex_coords = vTexCoords[1];
	normal = v2.xyz / vec3(0.3f); // normalize by dividing to sphere radius;
	normal = (normal + vec3(1.0))/vec3(2.0);

    EmitVertex();
    
    gl_Position = projMX * viewMX * modelMX * v3;
    tex_coords = vTexCoords[2];
	normal = v3.xyz / vec3(0.3f); // normalize by dividing to sphere radius;
	normal = (normal + vec3(1.0))/vec3(2.0);

    EmitVertex();
    
    EndPrimitive();
}
