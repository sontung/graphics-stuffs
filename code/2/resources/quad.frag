#version 330

in vec2 texCoords;
out vec4 fragColor;

uniform sampler2D screen_texture;
uniform sampler2D depth_texture;
uniform sampler2D normal_texture;

uniform float near;
uniform float far;
uniform float light_on;

uniform mat4 inverse_view;
uniform mat4 inverse_proj;
uniform vec3 light_pos;

uniform int depth_rendered;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
	z = (2.0 * near * far) / (far + near - z * (far - near));
	z = (z-near)/(far-near);
    return z;	
}


void main() {
    vec4 color = vec4(texCoords,0,1);
    
    float Ia    = 1.0;
    float Iin   = 1.0;
    float kamb  = 0.2;
    float kdiff = 0.8;
    
    float Iamb = kamb * Ia;
    
    float pos_z = texture(depth_texture, texCoords).x*2.0f - 1.0f;
	vec2 pos_xy = texCoords*vec2(2.0f) - vec2(1.0f);

	vec4 clip_pos = vec4(pos_xy, pos_z, 1.0f);
	vec4 view_pos = inverse_proj * clip_pos;
	view_pos /= view_pos.w;
	vec4 world_pos = inverse_view * view_pos;
	
	vec3 light = light_pos;
	vec3 normal = texture(normal_texture, texCoords).rgb * vec3(2.0) - vec3(1.0);
	float angle_n_l = dot(light, normal) / (length(light)*length(normal));

	float Idiff = Iin * kdiff * angle_n_l;
	float Iout = (Iamb + Idiff)*light_on;

	if (depth_rendered == 0) {
		if (Iout > 0.0f) fragColor = texture(screen_texture, texCoords) * vec4(Iout, Iout, Iout, 1.0f);
		else fragColor = texture(screen_texture, texCoords);
	} else {
		float d = LinearizeDepth(texture(depth_texture, texCoords).x);
		fragColor = vec4(d, d, d, 1.0f);
	}
}
