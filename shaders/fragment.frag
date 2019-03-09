#version 330 core

in vec2 uv;
uniform sampler2D texure_exampler; 

void main(void) {
    //gl_FragColor = texture2D(texure_exampler, uv);
    gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
