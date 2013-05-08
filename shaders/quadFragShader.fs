#version 330 core

out vec3 color;

in vec2 uv;

uniform sampler2D renderedTexture;
uniform float time;

void main(){
	float d = texture(renderedTexture, uv).r;
	color = vec3(d,d,d);
}
