#version 330

in float PosV_Z;
in vec3 NorV;

layout(location = 0) out float linDepth;
layout(location = 1) out vec3 normal;

void main(){
	float minDepth = 0.1f;
	float maxDepth = 100.0f;
	linDepth = (PosV_Z - minDepth)/(maxDepth - minDepth);// [minDepth, maxDepth] -> [0,1]
	/*linDepth = PosV_Z;*/
	normal = normalize(NorV);
}
