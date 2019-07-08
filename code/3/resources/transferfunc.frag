#version 330

layout(location = 0) out vec4 fragColor;

uniform int channel;

void main() {
    vec4 color = vec4(0,0,0,1);
    if (channel==0) {
        color = vec4(1,0,0,1);
    } else if (channel==1) {
        color = vec4(0,1,0,1);
    } else if (channel==2) {
        color = vec4(0,0,1,1);
    } else if (channel==3) {
        color = vec4(0,0,0,1);
    }
	color[3] = 1.0;
    fragColor = color;
}
