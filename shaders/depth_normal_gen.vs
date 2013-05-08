#version 330

layout(location = 0) in vec3 PosL;
layout(location = 1) in vec3 NorL;

uniform mat4 P;
uniform mat4 V;
uniform mat4 W;

out vec3 PosV; // Координата точки в View space.
out vec3 NorV; // Нормаль точки в View space.

void main(){
	mat4 VW = V*W;
	PosV = (VW*vec4(PosL, 1.0f)).xyz;
	NorV = (VW*vec4(NorL, 0.0f)).xyz;
	mat4 PVW = P*VW;
	gl_Position = PVW*vec4(PosL, 1.0f);
}
