#version 330

in vec3 PosV;
in vec3 NorV;

layout(location = 0) out float linDepth;

void main(){
	float minDepth = 0.1f;
	float maxDepth = 100.0f;
	linDepth = (PosV.z - minDepth)/(maxDepth - minDepth);
}
