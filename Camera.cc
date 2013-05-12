#include <math.h>
#include "HelperFuns.h"
#include "Camera.h"

Camera::Camera(){
	fov = gSceneParams.PI/4;
	ratio = gSceneParams.ratio;
	near = 0.1f;
	far = 100.0f;

	eye.x = eye.y = eye.z = 10.0f;
	target.x = target.y = target.z = 0.0f;
	up.x = up.z = 0.0f; up.y = 1.0f;

	alpha = gSceneParams.PI/4;
	beta = gSceneParams.PI/4;
	sph_Radius = 15.0f;
	UpdateEye();

	CalculatePV();
}

void Camera::CalculatePV(){
	Mat4x4Pers(PV, fov, ratio, near, far);
	Mat4x4 V;
	Mat4x4View(V, eye, target, up);
	Mat4x4Mult(PV, PV, V);
}

void Camera::UpdateEye(){
	eye.x = sph_Radius*cosf(beta)*cosf(alpha);
	eye.y = sph_Radius*sinf(beta);
	eye.z = sph_Radius*cosf(beta)*sinf(alpha);
}
