#version 330 core

out vec3 color;

in vec2 uv;

uniform sampler2D AmbOcclusionMap;

void main(){
	vec2 texelSize = vec2(1.0/800.0, 1.0/600);
	float aveValue = 0.0;
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			aveValue += texture(AmbOcclusionMap, uv + vec2(i,j)*texelSize).r;
		}
	}
	aveValue /= 16.0;
	color = vec3(aveValue, aveValue, aveValue);
}
