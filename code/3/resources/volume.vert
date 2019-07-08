#version 330

layout(location = 0) in vec4 in_position;

uniform mat4 projMX;
uniform mat4 viewMX;
uniform mat4 modelMX;

uniform vec3 cam_pos;
uniform vec3 volume_scale;

out vec3 vray_dir;
out vec3 transformed_cam;
out vec3 texture_coord;

void main() {
	vec3 pos = in_position.xyz;
	vec3 volume_translation = vec3(-0.5);
	gl_Position = projMX * viewMX * vec4(pos + volume_translation, 1);
	transformed_cam = cam_pos - volume_translation;
	vray_dir = pos - transformed_cam;
	texture_coord = in_position.xyz;
}
