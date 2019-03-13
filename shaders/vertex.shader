#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

uniform mat4 MVP;

out vec2 UV;

void main(){
	gl_Position = MVP * vec4(pos, 1);
	UV = uv;
}