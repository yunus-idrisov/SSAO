#ifndef CAMERA_H
#define CAMERA_H
#include <GL/gl.h>
#include "Math.h"

/*
	Класс камеры перспективной проекции.
*/

class Camera{
	GLfloat fov;
	GLfloat ratio;
	GLfloat near;
	GLfloat far;

	Vector3f	eye;
	Vector3f	target;
	Vector3f	up;

	Mat4x4		PV;

	// Сферические координаты. Используются для
	// поворота вокруг начала координат.
	GLfloat alpha; 		// горизонтальное отклонение
	GLfloat beta;  		// вертикальное отклонение
	GLfloat sph_Radius; // радиус

	// После изменения каких-либо из сфер-х координат
	// происходит автоматическое перевычисление
	// вектора eye(матрицы PV не обновляется. Для обновления
	// должна быть вызвана функция CalculatePV()).
	void UpdateEye();
public:
	Camera();
	// Функции для изменения параметров камеры.
	// После изменения этих параметров матрица PV
	// автоматически не обновляется. Для обновления должна
	// быть вызвана функция CalculatePV().
	void SetFov(GLfloat f){ fov = f; }
	void SetRatio(GLfloat r){ ratio = r; }
	void SetNear(GLfloat n){ near = n; }
	void SetFar(GLfloat f){ far = f; }
	void SetEye(const Vector3f& e){ eye = e; }// Сферические координаты не обновляются.
	void SetTarget(const Vector3f& t){ target = t; }
	void SetUp(const Vector3f& u){ up = u; }

	void SetAlpha(GLfloat a){ alpha = a; UpdateEye(); }
	void SetBeta(GLfloat b){ beta = b; UpdateEye(); }
	void SetRadius(GLfloat r){ sph_Radius = r; UpdateEye(); }

	// Функции для извлечения параметров камеры.
	GLfloat  GetFov()     const  { return fov; }
	GLfloat  GetRatio()   const  { return ratio; }
	GLfloat  GetNear()    const  { return near; }
	GLfloat  GetFar()     const  { return far; }
	Vector3f GetEye()     const  { return eye; }
	Vector3f GetTarget()  const  { return target; }
	Vector3f GetUp()      const  { return up; }

	GLfloat  GetAlpha()   const  { return alpha; }
	GLfloat  GetBeta()    const  { return beta; }
	GLfloat  GetRadius()  const  { return sph_Radius; }

	Mat4x4   GetPV()      const  { return PV; }

	// Вычисление матрицы PV.
	void CalculatePV();
};

#endif // CAMERA_H
