#version 330

#define M_PI  3.14159265

in vec3 vray_dir;
in vec3 transformed_cam;
in vec3 texture_coord;


layout(location = 0) out vec4 frag_color;

uniform sampler3D volume;        //!< 3D texture handle 
uniform sampler1D transferTex;   // colormap for mode 3

uniform ivec3 volume_dims;
uniform float dt;
uniform int N;
uniform float c_global;

uniform mat4 viewMX;            //!< view matrix
uniform mat4 modelMX;           //!< model matrix

uniform int mode;


uniform float isovalue;    //!< value for iso surface

uniform float ambient;      //!< ambient color
uniform float diffuse;      //!< diffuse color

uniform float k_amb;       //!< ambient factor
uniform float k_diff;      //!< diffuse factor
uniform float k_spec;      //!< specular factor
uniform float k_exp;       //!< specular exponent



vec3 blinnPhong(vec3 n, vec3 l, vec3 v) {
	vec3 h = normalize(v+l);
	float c_amb = ambient * k_amb;
	float c_diff = diffuse * k_diff * clamp(dot(n, l), 0, 1);
	float c_spec = diffuse * k_spec * (k_exp+2)/(2*M_PI) * clamp(pow(dot(h, n), k_exp), 0, 1);
	vec3 color = vec3(c_amb+c_diff+c_spec);
    return color;
}

vec2 intersect_box(vec3 orig, vec3 dir) {
	const vec3 box_min = vec3(0);
	const vec3 box_max = vec3(1);
	vec3 inv_dir = 1.0 / dir;
	vec3 tmin_tmp = (box_min - orig) * inv_dir;
	vec3 tmax_tmp = (box_max - orig) * inv_dir;
	vec3 tmin = min(tmin_tmp, tmax_tmp);
	vec3 tmax = max(tmin_tmp, tmax_tmp);
	float t0 = max(tmin.x, max(tmin.y, tmin.z));
	float t1 = min(tmax.x, min(tmax.y, tmax.z));
	return vec2(t0, t1);
}

vec4 compute() {
	vec4 res = vec4(0.0f);
	vec3 ray_dir = normalize(vray_dir);
	vec2 t_hit = intersect_box(transformed_cam, ray_dir);
	if (t_hit.x > t_hit.y) {
		discard;
	}
	t_hit.x = max(t_hit.x, 0.0);
	float s0 = t_hit.x;
	vec3 p = transformed_cam + t_hit.x * ray_dir;
	vec3 Co;
	float alpha_o;
	vec3 Ca = vec3(0.0f);
	float alpha_a = 0.0f;

	for (int t = 0; t < N; t++) {
		float val = texture(volume, p).r;

		if (mode == 0) res += val * vec4(c_global, c_global, c_global, 1.0f);
		else if (mode == 1) {
			if (val > res.x) res = vec4(val, val, val, 1.0f);
		} else if (mode == 2) {
			vec3 p2 = p + ray_dir * dt;
			float val2 = texture(volume, p2).r;
			if ((val - isovalue)*(val2 - isovalue) < 0) {
				float dummy = (isovalue - val)/(val2 - val);
				vec3 p3 = p + dummy*(p2-p);
				vec3 normal = vec3(textureOffset(volume, p3, ivec3(1, 0, 0)).r - textureOffset(volume, p3, ivec3(-1, 0, 0)).r,
						           textureOffset(volume, p3, ivec3(0, 1, 0)).r - textureOffset(volume, p3, ivec3(0, -1, 0)).r,
				                   textureOffset(volume, p3, ivec3(0, 0, 1)).r - textureOffset(volume, p3, ivec3(0, 0, -1)).r);
				normal = normalize(normal);
				vec3 shading = blinnPhong(normal, normalize(vec3(1.0f, 0.5f, 1.0f)), normalize(p3-transformed_cam));
				res = vec4(shading, 1.0f);
				break;
			}
		} else if (mode == 3) {
			vec4 val_color;
			if (val == 1.0f) {
				val_color = vec4(0.0f);
			}
			else val_color = texture(transferTex, val);
			vec3 Cb = val_color.rgb;
			float alpha_b = val_color.a;
			Co = Ca*alpha_a + Cb*alpha_b*(1-alpha_a);
			alpha_o = alpha_a + alpha_b*(1-alpha_a); 
			Ca = Co;
			alpha_a = alpha_o;
			res = vec4(Co, alpha_o);
			if (alpha_a >= 1.0f) break;
		}
		s0 += dt;
		if (s0 >= t_hit.y) break;
		p += ray_dir * dt;
	}
	return res;
}

void main() {
    vec4 color = vec4(0.0);

    switch (mode) {
        case 0: {  // line-of-sight
			color = compute();
            break;
        }
        case 1: {  // maximum-intesity projection
			color = compute();
            break;
        }
        case 2: {  // isosurface
			color = compute();
            break;
        }
        case 3: {  // volume visualization with transfer function
			color = compute();
            break;
        }
    }

    frag_color = color;
}
