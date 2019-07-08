#version 330

layout(location = 0) in vec2  in_position;

uniform mat4 projMX;
uniform int logPlot;
uniform float most;
uniform float least;

out vec2 vTexCoords;

void main() {
	float x = in_position.x*2-1;
	float y;
    if (logPlot==0) {
		y = exp(in_position.y);
        y = (y-exp(least))/(exp(most)-exp(least));
    } else {
		y = (in_position.y-least)/(most-least);
	}
	y = y*2-1;
    vec4 vert = vec4(x,y,-0.5,1);
    gl_Position = vert;
	vTexCoords = in_position;
}
