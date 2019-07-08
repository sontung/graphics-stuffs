#version 440

layout(location = 0) out vec4 fragColor0;


flat in ivec2 patch_id3;
in vec3 normal_vec;
in vec3 tex_coord;

uniform int n_seg_n;
uniform int n_seg_m;
uniform int show_normals;
uniform int how_many_checkers;

void main() {

	float patch_id = patch_id3.x;
	float color;
	int x = int(patch_id) % n_seg_m;
	int y = int(patch_id-x) / n_seg_m;

	if (mod(y, 2) == 0) color = x;
	else color = x + 1;

	float total = floor((tex_coord.x+1)/2*how_many_checkers + 0.00001f) +
	floor((tex_coord.y+1)/2*how_many_checkers + 0.00001f) + 
	floor((tex_coord.z+1)/2*how_many_checkers + 0.00001f);

	if (mod(total, 2) == 0) fragColor0 = vec4(0.5, 0.5, 0.5, 1.0);
	else fragColor0 = vec4(0.1, 0.1, 0.1, 1.0);
	
	if (show_normals == 1) fragColor0 = vec4(normal_vec, 1.0);

}
