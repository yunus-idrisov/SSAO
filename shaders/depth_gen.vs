#version 330

layout(location = 0) in vec3 PosL;

uniform mat4 P;
uniform mat4 V;
uniform mat4 W;

out float PosV_Z; // Координата z точки в View space.

void main(){
	mat4 VW = V*W;
	PosV_Z = (VW*vec4(PosL, 1.0f)).z;
	mat4 PVW = P*VW;
	gl_Position = PVW*vec4(PosL, 1.0f);
}
