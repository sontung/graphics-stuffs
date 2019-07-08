#version 440

layout(location = 0) out vec4 frag_color;

layout( std430, binding = 1 ) buffer grids {
	readonly int grids_status[]; 
};

in float id;
uniform int time_step;
uniform int nb_time_steps;

void main() {
	int idx = int(id);
	int if_near = grids_status[nb_time_steps*idx+time_step];
    if (if_near == 1) frag_color = vec4(0.5, 0.3, 0.1, 1);
	else frag_color = vec4(0.0, 0.5, 0.0, 1);
}
