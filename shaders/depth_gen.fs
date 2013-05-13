#version 330

in float PosV_Z;
uniform float near;
uniform float far;

layout(location = 0) out float linDepth;

void main(){
	linDepth = (PosV_Z - near)/(far - near);// [near, far] -> [0,1]
}
