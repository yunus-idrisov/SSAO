#version 330 core

out vec3 color;

in vec2 uv;

uniform sampler2D AmbOcclusionMap;
uniform vec2 texelSize;

void main(){
	float aveValue = 0.0;
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			aveValue += texture(AmbOcclusionMap, uv + vec2(i,j)*texelSize).r;
		}
	}
	aveValue /= 16.0;
	color = vec3(aveValue, aveValue, aveValue);
}
