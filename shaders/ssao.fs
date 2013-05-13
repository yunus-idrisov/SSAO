#version 330

layout(location = 0) out float occFactor;
in vec2 uv;

uniform sampler2D LinDepthMap;
uniform sampler2D SamplesMap;	  // Вектора для определения occlusion factor.
uniform sampler2D RandVectorsMap; // Вектора для поворота.

uniform mat4 P;
uniform vec4 winParams;// winRatio, near, far, winWidth
const int SamplesMapSize = 4;

void main(){
	// Получения положения точки в View Space.
	vec3 viewRay = vec3( -(uv.x*2.0f - 1.0f)*winParams[0], uv.y*2.0f - 1.0f, P[1][1] );
	viewRay = normalize(viewRay);
	float d = mix(winParams[1], winParams[2], texture(LinDepthMap, uv).r);
	float t = d/viewRay.z;
	vec3 orig = vec3( viewRay.x*t, viewRay.y*t, d );

	// Строим матрицу для поворота тестируемых векторов.
	vec3 z = texture(RandVectorsMap, uv*vec2(winParams[3]/4, winParams[3]/winParams[0]/4)).xyz*2.0f - 1.0f;
	z = normalize(z);
	vec3 x = normalize(vec3(1,-z.x/z.y - 2*z.z/z.y, 2));
	vec3 y = cross(x,z);
	mat3 RotMatrix = mat3(x,y,z);

	float inc = 1.0f/SamplesMapSize;
	int SuccedSamples = 0;
	for(float i = 1.0f/(SamplesMapSize*2); i < 1.0; i += inc){
		for(float j = 1.0f/(SamplesMapSize*2); j < 1.0; j += inc){
			vec3 sample = texture(SamplesMap, vec2(i,j)).xyz*2.0f - 1.0f;
			sample = RotMatrix*sample;
			sample = orig + sample;
			vec4 sampleProj = P*vec4(sample,1.0f);
			sampleProj.xy /= sampleProj.w;
			if( abs(sampleProj.x) > 1 || abs(sampleProj.y) > 1 )
				continue;
			sampleProj.xy = sampleProj.xy*0.5f + 0.5f;
			float sampleProjDepth = mix(winParams[1], winParams[2], texture(LinDepthMap, sampleProj.xy).r);
			if( sample.z >= sampleProjDepth )
				SuccedSamples++;
		}
	}
	occFactor = 1.0f - SuccedSamples/float(SamplesMapSize*SamplesMapSize);
}
