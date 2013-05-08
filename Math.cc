#include <math.h>
#include <string.h>
#include <ctype.h>
#include <iostream>
#include "Math.h"
using namespace std;

GLfloat Vec3Length(const Vector3f& v){
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

void Vec3Normalize(Vector3f& v){
	GLfloat len = Vec3Length(v);
	if( len != 0.0f ){
		v.x = v.x/len;
		v.y = v.y/len;
		v.z = v.z/len;
	}
}

void Vec3Sub(Vector3f& res, const Vector3f& v1, const Vector3f& v2){
	res.x = v1.x - v2.x;
	res.y = v1.y - v2.y;
	res.z = v1.z - v2.z;
}

void Vec3Add(Vector3f& res, const Vector3f& v1, const Vector3f& v2){
	res.x = v1.x + v2.x;
	res.y = v1.y + v2.y;
	res.z = v1.z + v2.z;
}

GLfloat Vec3DotProduct(const Vector3f& v1, const Vector3f& v2){
	return (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z);
}

void Vec3CrossProduct(Vector3f& res, const Vector3f& v1, const Vector3f& v2){
	res.x = v1.y*v2.z - v1.z*v2.y;
	res.y = -(v1.x*v2.z - v1.z*v2.x);
	res.z = v1.x*v2.y - v1.y*v2.x;
}

// Matrices.

static void Mat4x4Null(Mat4x4& m);// Обнуление матрицы

void Mat4x4View(Mat4x4& m, const Vector3f& Eye, const Vector3f& Target, const Vector3f& Up){
	Vector3f w;
	Vec3Sub(w, Target, Eye);
	Vec3Normalize(w);

	Vector3f u;
	Vec3CrossProduct(u, Up, w);
	Vec3Normalize(u);

	Vector3f v;
	Vec3CrossProduct(v, w, u);
	m._11 = u.x; m._12 = u.y; m._13 = u.z; m._14 = -Vec3DotProduct(u,Eye);
	m._21 = v.x; m._22 = v.y; m._23 = v.z; m._24 = -Vec3DotProduct(v,Eye);
	m._31 = w.x; m._32 = w.y; m._33 = w.z; m._34 = -Vec3DotProduct(w,Eye);
	m._41 =  0 ; m._42 =  0 ; m._43 =  0 ; m._44 = 1.0f;
}

void Mat4x4Pers(Mat4x4& m, GLfloat fov, GLfloat aspect, GLfloat n, GLfloat f){
	Mat4x4Null(m);
	GLfloat r = aspect;
	GLfloat d = 1.0f/tanf(fov/2.0f);
	m._11 = -1.0f/r*d;// -1 нужна т.к.система координат(a именно, homogeneous clip space),
					  // в которой находится точка после выхода из vertex shader, 
					  // является левой.
	m._22 = d;
	m._33 = -(n + f)/(n - f);
	m._34 = 2.0f*n*f/(n - f);
	m._43 = 1;
}

void Mat4x4Ortho(Mat4x4& m, GLfloat width, GLfloat height, GLfloat n, GLfloat f){
	Mat4x4Null(m);
	m._11 = -2/width;
	m._22 =  2/height;
	m._33 = 2/(f - n);
	m._34 = -(f + n)/(f - n);
	m._44 = 1;
}

void Mat4x4Vec4Mult(Vector4f& res, const Mat4x4& m, const Vector4f& v){
	Vector4f t;
	t.x = m._11*v.x + m._12*v.y + m._13*v.z + m._14*v.w;
	t.y = m._21*v.x + m._22*v.y + m._23*v.z + m._24*v.w;
	t.z = m._31*v.x + m._32*v.y + m._33*v.z + m._34*v.w;
	t.w = m._41*v.x + m._42*v.y + m._43*v.z + m._44*v.w;
	res.x = t.x;
	res.y = t.y;
	res.z = t.z;
	res.w = t.w;
}

void Mat4x4Mult(Mat4x4& res, const Mat4x4& m1, const Mat4x4& m2){
	Mat4x4 r;
	r._11 = m1._11*m2._11 + m1._12*m2._21 + m1._13*m2._31 + m1._14*m2._41;
	r._12 = m1._11*m2._12 + m1._12*m2._22 + m1._13*m2._32 + m1._14*m2._42;
	r._13 = m1._11*m2._13 + m1._12*m2._23 + m1._13*m2._33 + m1._14*m2._43;
	r._14 = m1._11*m2._14 + m1._12*m2._24 + m1._13*m2._34 + m1._14*m2._44;

	r._21 = m1._21*m2._11 + m1._22*m2._21 + m1._23*m2._31 + m1._24*m2._41;
	r._22 = m1._21*m2._12 + m1._22*m2._22 + m1._23*m2._32 + m1._24*m2._42;
	r._23 = m1._21*m2._13 + m1._22*m2._23 + m1._23*m2._33 + m1._24*m2._43;
	r._24 = m1._21*m2._14 + m1._22*m2._24 + m1._23*m2._34 + m1._24*m2._44;

	r._31 = m1._31*m2._11 + m1._32*m2._21 + m1._33*m2._31 + m1._34*m2._41;
	r._32 = m1._31*m2._12 + m1._32*m2._22 + m1._33*m2._32 + m1._34*m2._42;
	r._33 = m1._31*m2._13 + m1._32*m2._23 + m1._33*m2._33 + m1._34*m2._43;
	r._34 = m1._31*m2._14 + m1._32*m2._24 + m1._33*m2._34 + m1._34*m2._44;

	r._41 = m1._41*m2._11 + m1._42*m2._21 + m1._43*m2._31 + m1._44*m2._41;
	r._42 = m1._41*m2._12 + m1._42*m2._22 + m1._43*m2._32 + m1._44*m2._42;
	r._43 = m1._41*m2._13 + m1._42*m2._23 + m1._43*m2._33 + m1._44*m2._43;
	r._44 = m1._41*m2._14 + m1._42*m2._24 + m1._43*m2._34 + m1._44*m2._44;

	res = r;
}

void Mat4x4Translate(Mat4x4& m, GLfloat x, GLfloat y, GLfloat z){
	Mat4x4Null(m);
	m._11 = m._22 = m._33 = m._44 = 1;
	m._14 = x;
	m._24 = y;
	m._34 = z;
}

void Mat4x4Scale(Mat4x4& m, GLfloat x, GLfloat y, GLfloat z){
	Mat4x4Null(m);
	m._11 = x;
	m._22 = y;
	m._33 = z;
	m._44 = 1.0f;
}

void Mat4x4Rotate(Mat4x4& m, char axis, GLfloat angle){
	if( !isalpha(axis) )
		return;
	else 
		axis = tolower(axis);
	Mat4x4Null(m);
	GLfloat s = sinf(angle);
	GLfloat c = cosf(angle);
	switch( axis ){
		case 'x' : m._11 = 1.0f; m._22 = c; m._23 = -s; m._32 = s; m._33 = c; m._44 = 1.0f; break;
		case 'y' : m._11 = c; m._13 = s; m._22 = 1.0f; m._31 = -s; m._33 = c; m._44 = 1.0f; break;
		case 'z' : m._11 = c; m._12 = -s; m._21 = s; m._22 = c; m._22 = 1.0f; m._44 = 1.0f; break;
	};
}

void Mat4x4Identity(Mat4x4& m){
	Mat4x4Null(m);
	m._11 = m._22 = m._33 = m._44 = 1.0f;
}

static void Mat4x4Null(Mat4x4& m){
	for(int i = 0; i < 16; i++)
		m.m[i] = 0.0f;
}
