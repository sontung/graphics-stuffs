

#version 330

uniform sampler2D texture_sampler;
uniform vec3 pickIdCol;

in vec2 tex_coords;
in vec3 normal;

layout(location = 0) out vec4 fragColor0;
layout(location = 1) out vec4 fragColor1;
layout(location = 2) out vec4 fragColor2;


void main() {
	 fragColor0 = vec4(texture(texture_sampler, tex_coords).rgb, 1.0f);
	 fragColor1 = vec4(pickIdCol/vec3(255.0f), 1.0f);
	 fragColor2 = vec4(normal.xyz, 1.0f);
}
