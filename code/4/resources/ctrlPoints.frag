#version 330

layout(location = 0) out vec4 fragColor0;
layout(location = 1) out vec4 fragColor1;

in vec3 pnt_color;
in vec3 display_color;


void main() {
	fragColor0 = vec4(display_color, 1.0);
    fragColor1 = vec4(pnt_color,1.0);
}
