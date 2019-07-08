#version 330

layout(location = 0) in vec4  in_position;
layout(location = 1) in vec3  in_color;
layout(location = 2) in int   in_selected;
layout(location = 3) in int   in_index;

uniform mat4 modelMX;
uniform mat4 viewMX;
uniform mat4 projMX;

uniform int point_size;
uniform int depth_test;

out vec3 pnt_color;
out vec3 display_color;

void main() {
    vec4 vert = projMX * viewMX * modelMX * vec4(in_position.xyz, 1);
	if (depth_test == 0) vert.z = 0.0;

    gl_Position = vert;
	gl_PointSize = point_size;

	if (in_selected == 0) display_color = vec3(0.3,0.3,1.0);
	else display_color = vec3(1.0,0.3,1.0);
	pnt_color = in_color;
}
