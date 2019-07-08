#version 440

layout(location = 0) out vec4 frag_color;

layout( std430, binding = 1 ) buffer particles {
	readonly float particles_position[]; 
};


in vec2 texCoords;

uniform float radius;
uniform float pixel_width;
uniform float pixel_height;
uniform int res_x;
uniform int res_y;
uniform int nb_particles;

uniform vec3 u;
uniform vec3 v;
uniform vec3 L_pos;
uniform vec3 cam_pos;
uniform vec3 light_pos;

uniform samplerCube skybox;


bool test_sphere_intersection(vec3 c, float r, vec3 e, vec3 d, inout float res) {
	float A = dot(d, d);
	float B = dot(d, e - c) * 2.0f;
	float C = dot(e - c, e - c) - r * r;
	float delta = B * B - 4 * A * C;

	if (delta < 0.0f) {
		res = -1;
		return false;
	}
	else {
		float discriminant = sqrt(delta);
		res = (-B - discriminant) / (2.0f * A);
		return true;
	}
}

bool validate(vec3 p, int dim) {
	int thresh = 1;
	if (dim == 0) return abs(p.y) <= thresh && abs(p.z) <= thresh;
	else if (dim == 1) return abs(p.x) <= thresh && abs(p.z) <= thresh;
	else if (dim == 2) return abs(p.x) <= thresh && abs(p.y) <= thresh;
}

bool test_cube_intersection(vec3 e, vec3 d, inout vec3 res) {
	vec3 furthest_intersect = vec3(0.0);
	int thresh = 1;
	float furthest_distance = -1.0f;

	for (int i = 0; i<3; i++) {

		float t = max((-thresh - e[i]) / d[i], (thresh - e[i]) / d[i]);
		vec3 intersect_point = e + d*t;
		if (validate(intersect_point, i)) {
			float distance = dot(intersect_point-e, intersect_point-e);
			if (distance > furthest_distance) {
				furthest_distance = distance;
				furthest_intersect = intersect_point;
			}
		}

	
	}
	if (furthest_distance > -1.0f) {
		res = furthest_intersect;
		return true;
	} else {
		return false;
	}
}

bool test_sphere_intersection(vec3 c, float r, vec3 e, vec3 d) {
	float A = dot(d, d);
	float B = dot(d, e - c) * 2.0f;
	float C = dot(e - c, e - c) - r * r;
	float delta = B * B - 4 * A * C;
	
	return delta >= 0.0f;
}


void main() {

	float total = floor((texCoords.x)*res_x+0.00001f) + floor((texCoords.y)*res_y+0.00001f);

	float i = res_x - floor((texCoords.x)*res_x+0.00001f);
	float j = res_y - floor((texCoords.y)*res_y+0.00001f);
	vec3 s = L_pos + u * i * pixel_width + v * j * pixel_height;

	bool first_test = true;
	float t_min = -1000.0f;
	vec3 center;
	float t;
	vec3 closest_center = vec3(-10, -10, -10);
	int id_collided_particle = -1;

	// test intersect
	for (int idx=0; idx < nb_particles; idx+=3) {
		center = vec3(particles_position[idx], particles_position[idx+1], particles_position[idx+2]);
		center.z -= 2;
		if (test_sphere_intersection(center, radius, cam_pos, s-cam_pos, t)) {
			if (first_test) {
				t_min = t;
				closest_center = center;
				first_test = false;
				id_collided_particle = idx;
			} else {
				if (t < t_min) {
					t_min = t;
					closest_center = center;
					id_collided_particle = idx;
				}
			}
		}
	}

	// test shading
	vec3 collided_point = cam_pos + t_min*(s-cam_pos);
	vec3 color;
	vec3 light_vec = normalize(light_pos-collided_point);

	// hit particles
	if (t_min > -1000.0f) {
		color = vec3(0, 0.8, 1.0);

		for (int idx=0; idx < nb_particles; idx+=3) {
			center = vec3(particles_position[idx], particles_position[idx+1], particles_position[idx+2]);
			center.z -= 2;
			if (idx == id_collided_particle) continue;
			if (test_sphere_intersection(center, radius, collided_point, light_vec)) {
				color = vec3(0, 0.1, 0.9);
				break;
			}
		}
		
		vec3 normal = normalize(collided_point-closest_center);
		color = color*clamp(dot(normal, light_vec), 0, 1);
		
	// hit BG
	} else {
		color = texture(skybox, s-cam_pos).rgb;
	}

	frag_color = vec4(color, 1.0);
}
