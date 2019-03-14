#version 310 es
#undef lowp
#undef mediump
#undef highp

precision mediump float;

in vec2 UV;
out vec3 color;

uniform vec3 diffuse_color;
uniform sampler2D sampler;

void main()
{
	color = diffuse_color * texture(sampler, UV).rgb;
}
