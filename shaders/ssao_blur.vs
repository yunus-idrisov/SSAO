#version 330 core

layout(location = 0) in vec2 verPosH;

out vec2 uv;

void main(){
	uv = (verPosH + vec2(1,1))/2.0f;
	gl_Position = vec4(verPosH, 0.0, 1.0);
}
