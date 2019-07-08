#version 440

layout(location = 0) in vec4  in_position;


uniform mat4 modelMX;
uniform mat4 viewMX;
uniform mat4 projMX;
uniform int  pickedID;

uniform int point_size;

out ivec2 patch_id;

void main() {
    vec4 vert = in_position;
	gl_PointSize = point_size;
	patch_id = ivec2(gl_InstanceID);
	gl_Position = vert;
}
