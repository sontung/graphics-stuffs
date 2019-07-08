#version 330

uniform mat4  projMX;
uniform float binStepHalf;

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;
in vec2 vTexCoords[];
out vec2 texCoords;

void main() {
    vec4 v1 = gl_in[0].gl_Position;
    vec4 v2 = gl_in[1].gl_Position;
    vec4 v3 = gl_in[2].gl_Position;


	gl_Position = v1;
	texCoords = vTexCoords[0];
	EmitVertex();

	gl_Position = v2;
	texCoords = vTexCoords[1];
	EmitVertex();

	gl_Position = v3;
	texCoords = vTexCoords[2];
	EmitVertex();

	EndPrimitive();
}
