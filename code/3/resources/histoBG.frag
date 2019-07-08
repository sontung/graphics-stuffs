#version 330
#define PI 3.14159265

layout(location = 0) out vec4 fragColor;

uniform float aspect;
uniform int   showWhat;
uniform sampler2D tex;

in vec2 texCoords;

void main() {
    vec4 color = vec4(0);
    float freq = 30.0;
    
    if (showWhat==0) {
        // show the background for the transfer function
        float val = sign( sin(texCoords.x*PI*freq) * sin(texCoords.y*PI*freq/3) );
        color = vec4(vec3(1.0-0.1*val),1.0);
    } else {
        // show the background for the histogram
        float val = sign( sin(texCoords.x*PI*freq) * sin(texCoords.y*PI*2) );
        color = vec4(vec3(0.1+0.1*val),1.0);
        vec4 rgba = texture( tex, vec2(texCoords.x,0.5) );
        color = mix(color,rgba,rgba.a);
    }
    fragColor = color;
}
