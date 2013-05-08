#version 330 core

layout(location = 0) in vec3 verPosH;

out vec2 uv;
uniform float time;

void main(){
	uv = (verPosH.xy + vec2(1,1))/2.0f;
	gl_Position = vec4(verPosH, 1.0);
}
