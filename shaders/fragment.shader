#version 330 core

in vec2 UV;
out vec3 color;

uniform vec3 diffuse_color;
uniform sampler2D sampler;

void main()
{
	color = diffuse_color * texture(sampler, UV).rgb;
}
