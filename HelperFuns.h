#ifndef HELPERFUNS_H
#define HELPERFUNS_H
#include <GL/gl.h>
#include "Camera.h"
#include "Math.h"

struct SceneParameters{
	static const GLfloat PI = 3.14159f;
	static const GLfloat PI_OVER_TWO = 1.57080f;
	// Window
	GLuint		winWidth;
	GLuint		winHeight;
	GLfloat		ratio;		  // winWidth/winHeight

	GLuint		vertexArrayID;
	GLuint		gridShader;
	GLuint		gridShaderPVW_Ref;

	GLuint		VerBuffer[2];
	GLuint		BufferVerCount[2];

	GLuint		testBuffer;
	GLuint		tBufVerCount;
	GLuint		testIndBuffer;
	GLuint		testIndCount;

	// Framebuffer для генерации текстур линейной
	// глубины и нормалей сцены.
	GLuint		DepthNormalFrameBuffer;
	// Renderbuffer глубины, используемый framebuffer'ом.
	GLuint		DepthRenderBuffer;
	// Текстуры линейной глубины и нормалей сцены.
	GLuint		LinearDepthTexture;
	GLuint		NormalTexture;
	GLuint		DepNorShader;
	GLuint		DepNorShader_P_Ref;
	GLuint		DepNorShader_V_Ref;
	GLuint		DepNorShader_W_Ref;

	Camera		cam;
};

extern SceneParameters gSceneParams;

// Функция для инициализации OpenGL и создания окна.
// При ошибке возвращается -1.
int InitGL(int winWidth, int winHeight, int glver_major, int glver_minor);

// Функция для начальной установки параметров приложения.
// При ошибке возвращается -1.
int InitScene();

// Функция CreateProgram создаёт шейдерную программу из 
// вершинного и фрагментного шейдеров.
// При ошибке возвращается 0.
GLuint CreateProgram(const char *vertex_shader_path, const char *fragment_shader_path);

// Функция для обновления сцены.
void UpdateScene();

// Функция для рендеринга сцены.
void RenderScene();

// Очистка.
void ReleaseSceneResources();

struct Vertex_Pos_Col{
	Vector3f pos;
	Vector3f col;
};

#endif // HELPERFUNS_H
