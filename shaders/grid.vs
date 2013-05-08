#version 330

layout(location = 0) in vec3 verPosL;
layout(location = 1) in vec3 verColor;

out vec3 Color;
uniform mat4 mPVW;

void main(){
	gl_Position = mPVW*vec4(verPosL, 1.0f);
	Color = verColor;
}
