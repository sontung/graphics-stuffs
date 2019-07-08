#version 330

uniform vec3 pickIdCol;

in vec3 normal;
in float color;


layout(location = 0) out vec4 fragColor0;
layout(location = 1) out vec4 fragColor1;
layout(location = 2) out vec4 fragColor2;

void main() {
     if (color == 0.0f) fragColor0 = vec4(1.0, 0.5, 0.5, 1.0);
	 else fragColor0 = vec4(0.6, 0.6, 0.8, 1.0);
	 fragColor1 = vec4(pickIdCol/vec3(255.0f), 1.0f);
	 fragColor2 = vec4(normal.xyz, 1.0f);
}