#version 440

layout(location = 0) out vec4 frag_color;

layout( std430, binding = 2 ) buffer isosurface {
	readonly float iso_position[]; 
};


in vec2 texCoords;

uniform float radius;
uniform float pixel_width;
uniform float pixel_height;
uniform float visual_eff;

uniform int res_x;
uniform int res_y;
uniform int nb_triangles;

uniform vec3 u;
uniform vec3 v;
uniform vec3 L_pos;
uniform vec3 cam_pos;
uniform vec3 light_pos;

uniform samplerCube skybox;

bool inside_triangle(vec3 bary) {
	return bary.x >= 0 && bary.y >= 0 && bary.z >= 0;
}

vec3 barycentric(vec3 p, vec3 a, vec3 b, vec3 c) {
    vec3 v0 = b - a;
	vec3 v1 = c - a;
	vec3 v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
	return vec3(u, v, w);
}

vec3 test_plane_intersect(vec3 e, vec3 d, vec3 a, vec3 b, vec3 c, inout float res) {
	vec3 n = cross(b-a, c-a);
	res = (dot(n, a)-dot(n, e)) / dot(n, d);
	return e+d*res;
}

void main() {

	float total = floor((texCoords.x)*res_x+0.00001f) + floor((texCoords.y)*res_y+0.00001f);

	float i = res_x - floor((texCoords.x)*res_x+0.00001f);
	float j = res_y - floor((texCoords.y)*res_y+0.00001f);
	vec3 s = L_pos + u * i * pixel_width + v * j * pixel_height; // ray eq: cam + (s-cam)*t

	bool first_test = true;
	bool collided = false;
	float t;
	float t_min;
	vec3 tri_a;
	vec3 tri_b;
	vec3 tri_c;
	vec3 intersect;
	vec3 bary_coord;
	vec3 closest_tri_a;
	vec3 closest_tri_b;
	vec3 closest_tri_c;
	int id_collided_particle = -1;

	// test intersect
	for (int idx=0; idx < nb_triangles; idx+=9) {
		tri_a = vec3(iso_position[idx], iso_position[idx+1], iso_position[idx+2]-2);
		tri_b = vec3(iso_position[idx+3], iso_position[idx+4], iso_position[idx+5]-2);
		tri_c = vec3(iso_position[idx+6], iso_position[idx+7], iso_position[idx+8]-2);
		intersect = test_plane_intersect(cam_pos, s-cam_pos, tri_a, tri_b, tri_c, t);
		bary_coord = barycentric(intersect, tri_a, tri_b, tri_c);
		if (inside_triangle(bary_coord)) {
			if (first_test) {
				first_test = false;
				collided = true;
				t_min = t;
				closest_tri_a = tri_a;
				closest_tri_b = tri_b;
				closest_tri_c = tri_c;
			} else {
				if (t < t_min) {
					t_min = t;
					closest_tri_a = tri_a;
					closest_tri_b = tri_b;
					closest_tri_c = tri_c;
				}
			}
		}
	}
	if (collided) {
		vec3 normal = normalize(cross(closest_tri_b-closest_tri_a, closest_tri_c-closest_tri_a));
		vec3 reflect_vec = reflect(normalize(s-cam_pos), normal);
		vec3 refract_vec = refract(normalize(s-cam_pos), normal, 1.0f/1.33f);
		intersect = texture(skybox, reflect_vec).rgb*visual_eff + texture(skybox, refract_vec).rgb*(1-visual_eff);
	} else {
		intersect = texture(skybox, s-cam_pos).rgb;
	}
	frag_color = vec4(intersect, 1.0);
}
