#version 330

layout(location = 0) in vec2 PosH;

out vec2 uv;

void main(){
	uv = (PosH + vec2(1.0f,1.0f))/2.0f;
	gl_Position = vec4(PosH, 0.0f, 1.0f);
}
