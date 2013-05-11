#version 330

in vec2 uv;

uniform sampler2D LinDepthMap;
uniform sampler2D NormalMap;
uniform sampler2D SamplesMap;	  // Samples for occlusion test
uniform sampler2D RandVectorsMap; // Map of random vectors for samples rotation(4x4)

uniform mat4 P;
uniform float winRatio;// winWidth/winHeight

out vec3 FragColor;
/*layout(location = 1) out vec3 PosVS;// view space*/

void main(){
	int w = 8;
	vec3 viewRay = vec3( -(uv.x*2.0f - 1.0f)*winRatio, uv.y*2.0f - 1.0f, P[1][1] );
	viewRay = normalize(viewRay);
	float d = mix(0.1f, 100.0f, texture(LinDepthMap, uv).r);
	float t = d/viewRay.z;
	vec3 orig = vec3( viewRay.x*t, viewRay.y*t, d );

	vec3 z = texture(NormalMap, uv).xyz*2.0f - 1.0f;
	z = normalize(z);
	vec3 x = texture(RandVectorsMap, uv * vec2(800/4, 600/4)).xyz*2.0f - 1.0f;
	x = normalize(x - dot(x,z)*z);
	vec3 y = cross(z,x);
	mat3 RotMatrix = mat3(x,y,z);

	float i = 1.0f/(w*2);
	float j = i;
	float inc = 1.0f/w;
	int SuccedPoint = 0;
	for(float i = 1.0f/(w*2); i < 1.0; i += inc){
		for(float j = 1.0f/(w*2); j < 1.0; j += inc){
			vec3 rvec = texture(SamplesMap, vec2(i,j)).xyz*2.0f - 1.0f;
			rvec = RotMatrix*rvec;
			rvec = orig + rvec;
			vec4 projPoint = P*vec4(rvec,1.0f);
			projPoint.xy /= projPoint.w;
			if( projPoint.x > 1.0f || projPoint.x < -1.0f )
				continue;
			if( projPoint.y > 1.0f || projPoint.y < -1.0f )
				continue;
			projPoint.xy = projPoint.xy*0.5f + 0.5f;
			float projPointDepth = mix(0.1f, 100.0f, texture(LinDepthMap, projPoint.xy).r);
			if( rvec.z >= projPointDepth )
				SuccedPoint++;
		}
	}
	float c = 1.0f - SuccedPoint/float(w*w);
	FragColor = vec3(c,c,c);

	/*vec3 rvec = texture(SamplesMap, vec2(0.125 + 3*0.25,0.125 + 3*0.25)).xyz*2.0f - 1.0f;*/
	/*PosVS = rvec;*/
	/*FragColor = texture(RandVectorsMap, uv*vec2(800/4,600/4)).xyz;*/
	/*FragColor = texture(LinDepthMap, uv).xxx;*/
}
