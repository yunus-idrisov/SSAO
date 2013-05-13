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
	Vector2f	texelSize;

	GLuint		vertexArrayID;

	// Буферы, используемые в сцене.
	// 1-й буфер - четырехугольник, в который рендерится текстура.
	// 2-й буфер - объекты сцены.
	GLuint		VerBuffer[2];
	GLuint		VerCount[2];
	GLuint		IndBuffer[2];
	GLuint		IndCount[2];

	// Framebuffer для генерации текстуры линейной глубины.
	GLuint		DepthFrameBuffer;
	// Renderbuffer глубины, используемый framebuffer'ом.
	GLuint		DepthRenderBuffer;
	// Текстура линейной глубины.
	GLuint		LinearDepthTexture;
	// Текстура ambient occlusion.
	GLuint		AmbientOcclusionTexture;

	// Шейдер для извлечения глубины.
	GLuint		DepGenShader;
	GLuint		DepGenShader_P_Ref;
	GLuint		DepGenShader_V_Ref;
	GLuint		DepGenShader_W_Ref;
	GLuint		DepGenShader_near_Ref;
	GLuint		DepGenShader_far_Ref;

	// Шейдер для получения ambient occlusion текстуры.
	GLuint		SSAO_Shader;
	GLuint		SSAO_Shader_LinDepthMap_Ref;
	GLuint		SSAO_Shader_SamplesMap_Ref;
	GLuint		SSAO_Shader_RandVectorsMap_Ref;
	GLuint		SSAO_Shader_P_Ref;
	GLuint		SSAO_Shader_winParams_Ref;
	GLuint		SamplesTexture;
	GLuint		RandVectorsTexture;

	// Шейдер для размывания ambient occlusion текстуры.
	GLuint		SSAO_Blur_Shader;
	GLuint		SSAO_Blur_Shader_AmbOcclusionMap_Ref;
	GLuint		SSAO_Blur_Shader_texelSize_Ref;

	Camera		cam;
};

extern SceneParameters gScene;

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

// Загрузка объекта.
void LoadModel(const char* path);

// Очистка.
void ReleaseSceneResources();

struct Vertex_Pos{
	Vector3f pos;
};

#endif // HELPERFUNS_H
