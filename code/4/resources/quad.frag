#version 330

layout(location = 0) out vec4 frag_color;

uniform sampler2D tex;

in vec2 texCoords;

void main() {
	 frag_color = texture(tex, texCoords);
}
