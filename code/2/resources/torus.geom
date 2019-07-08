#version 330

uniform mat4 projMX;
uniform mat4 viewMX;
uniform mat4 modelMX;

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;

in vec2 angles[];
in float color_tri[];

out vec3 normal;
out float color;

void main() {
    vec4 v1 = gl_in[0].gl_Position;
    vec4 v2 = gl_in[1].gl_Position;
    vec4 v3 = gl_in[2].gl_Position;

	vec2 a1 = angles[0];
	vec2 a2 = angles[1];
	vec2 a3 = angles[2];
    
	float tx, ty, tz, sx, sy, sz;
	float nx, ny, nz;
	float length;
	float iangle;
	float jangle;
    
    gl_Position = projMX * viewMX * modelMX * v1;
	color = color_tri[0];

	iangle = a1.x;
	jangle = a1.y;
	tx = -sin(jangle);
	ty = cos(jangle);
	tz = 0;
	sx = cos(jangle)*(-sin(iangle));
	sy = sin(jangle)*(-sin(iangle));
	sz = cos(iangle);
	/* normal is cross-product of tangents */
	nx = ty*sz - tz*sy;
	ny = tz*sx - tx*sz;
	nz = tx*sy - ty*sx;
	/* normalize normal */
	length = sqrt(nx*nx + ny*ny + nz*nz);
	nx /= length;
	ny /= length;
	nz /= length;
	normal = vec3(nx, ny, nz);
	normal = (normal + vec3(1.0))/vec3(2.0);


    EmitVertex();
    
    gl_Position = projMX * viewMX * modelMX * v2;
	color = color_tri[1];


	iangle = a2.x;
	jangle = a2.y;
	tx = -sin(jangle);
	ty = cos(jangle);
	tz = 0;
	sx = cos(jangle)*(-sin(iangle));
	sy = sin(jangle)*(-sin(iangle));
	sz = cos(iangle);
	/* normal is cross-product of tangents */
	nx = ty*sz - tz*sy;
	ny = tz*sx - tx*sz;
	nz = tx*sy - ty*sx;
	/* normalize normal */
	length = sqrt(nx*nx + ny*ny + nz*nz);
	nx /= length;
	ny /= length;
	nz /= length;
	normal = vec3(nx, ny, nz);
	normal = (normal + vec3(1.0))/vec3(2.0);


    EmitVertex();
    
    gl_Position = projMX * viewMX * modelMX * v3;
	color = color_tri[2];


	iangle = a3.x;
	jangle = a3.y;
	tx = -sin(jangle);
	ty = cos(jangle);
	tz = 0;
	sx = cos(jangle)*(-sin(iangle));
	sy = sin(jangle)*(-sin(iangle));
	sz = cos(iangle);
	/* normal is cross-product of tangents */
	nx = ty*sz - tz*sy;
	ny = tz*sx - tx*sz;
	nz = tx*sy - ty*sx;
	/* normalize normal */
	length = sqrt(nx*nx + ny*ny + nz*nz);
	nx /= length;
	ny /= length;
	nz /= length;
	normal = vec3(nx, ny, nz);
	normal = (normal + vec3(1.0))/vec3(2.0);


    EmitVertex();
    
    EndPrimitive();
}
