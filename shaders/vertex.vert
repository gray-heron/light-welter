#version 330 core

layout(location=0) in vec3 in_Position;
layout(location=1) in vec4 in_Color;

uniform vec2 resolution;
uniform vec2 translation;
uniform vec2 scale;

out vec2 uv;

void main(void) {
    vec2 pos = vec2(in_Position.x, in_Position.y) * scale;
    pos.x += translation.x - (resolution.x / 2.0f);
    pos.y += translation.y - (resolution.y / 2.0f);

    gl_Position = vec4(pos.x / resolution.x * 2.0, pos.y / resolution.y * 2.0,
        0.0, 1.0);
    gl_Position = vec4(in_Position.x, in_Position.y, 0.0, 1.0);
 
    // Pass the color on to the fragment shader
    uv = vec2(in_Position.x, in_Position.y);
}
