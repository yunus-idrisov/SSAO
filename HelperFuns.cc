#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <vector>
#include <GL/glfw.h>
#include <stdlib.h>
#include <math.h>
#include "HelperFuns.h"
using namespace std;

const GLfloat SceneParameters::PI;
const GLfloat SceneParameters::PI_OVER_TWO;

SceneParameters gScene;

static GLfloat GetRand();// [-1,1]

static void InputHandler(GLfloat deltaTime);
static void MouseWheelHandler(GLint pos);

int InitGL(int winWidth, int winHeight, int glver_major, int glver_minor){
	if( !glfwInit() ){
		cerr << "Failed to initialize GLFW." << endl;
		return -1;
	}
	// Параметры OpenGL.
	// Antialiasing.
	//glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);// ??
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, glver_major);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, glver_minor);
	glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// w, h, rbits, gbits, bbits, abits, depth bits, stencil bits, fullscreen/windowed.
	// Если 0, то GLFW выберет значение по умолчанию или запретит.
	if( !glfwOpenWindow(winWidth, winHeight, 0,0,0,0, 32, 0, GLFW_WINDOW) ){
		cerr << "Failed to open GLFW window" << endl;
		glfwTerminate();
		return -1;
	}

	glViewport(0, 0, winWidth, winHeight);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthRange(0.0f, 1.0f);

	glClearColor(1,0,0,1);
	glClearDepth(1.0f);

	gScene.winWidth = winWidth;
	gScene.winHeight = winHeight;
	return 1;
}

int InitScene(){
	glfwSetWindowTitle("SSAO demo");
	glfwSetMouseWheelCallback( MouseWheelHandler );

	gScene.ratio = gScene.winWidth/(float)gScene.winHeight;
	gScene.texelSize.x = 1.0/gScene.winWidth;
	gScene.texelSize.y = 1.0/gScene.winHeight;
	gScene.cam = Camera();
	// Создаём VertexArray.
	glGenVertexArrays(1, &gScene.vertexArrayID);
	glBindVertexArray(gScene.vertexArrayID);

	// Создаём framebuffer.
	glGenFramebuffers(1, &gScene.DepthFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gScene.DepthFrameBuffer);

	// Создаём буфер глубины для framebuffer'a.
	glGenRenderbuffers(1, &gScene.DepthRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, gScene.DepthRenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, gScene.winWidth, gScene.winHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gScene.DepthRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// Создаём текстуру для рендеринга линейной глубины.
	glGenTextures(1, &gScene.LinearDepthTexture);
	glBindTexture(GL_TEXTURE_2D, gScene.LinearDepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, gScene.winWidth, gScene.winHeight, 0, GL_RED, GL_UNSIGNED_SHORT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Ambient occlusion текстура.
	glGenTextures(1, &gScene.AmbientOcclusionTexture);
	glBindTexture(GL_TEXTURE_2D, gScene.AmbientOcclusionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, gScene.winWidth, gScene.winHeight, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLenum drawBuffes[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffes);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Создаем четырехугольник для рендеринга текстуры.
	GLfloat quad_vers_pos[] = {
		-1, -1,
		 1, -1,
		 1,  1,
		-1,  1,
	};

	glGenBuffers(1, &gScene.VerBuffer[0]);
	glBindBuffer(GL_ARRAY_BUFFER, gScene.VerBuffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vers_pos), quad_vers_pos, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	gScene.VerCount[0] = 4;

	GLuint indices[6] = { 0,1,2,
						  0,2,3 };
	glGenBuffers(1, &gScene.IndBuffer[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gScene.IndBuffer[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	gScene.IndCount[0] = 6;

	// Создаём шейдеры.
	gScene.DepGenShader = CreateProgram("shaders/depth_gen.vs", "shaders/depth_gen.fs");
	if( gScene.DepGenShader == 0 ){
		cerr <<  "Failed to create shader." << endl;
		return -1;
	}
	gScene.DepGenShader_P_Ref = glGetUniformLocation(gScene.DepGenShader, "P");
	gScene.DepGenShader_V_Ref = glGetUniformLocation(gScene.DepGenShader, "V");
	gScene.DepGenShader_W_Ref = glGetUniformLocation(gScene.DepGenShader, "W");
	gScene.DepGenShader_near_Ref = glGetUniformLocation(gScene.DepGenShader, "near");
	gScene.DepGenShader_far_Ref = glGetUniformLocation(gScene.DepGenShader, "far");

	gScene.SSAO_Shader = CreateProgram("shaders/ssao.vs", "shaders/ssao.fs");
	if( gScene.SSAO_Shader == 0 ){
		cerr <<  "Failed to create shader." << endl;
		return -1;
	}
	gScene.SSAO_Shader_LinDepthMap_Ref = glGetUniformLocation(gScene.SSAO_Shader, "LinDepthMap");
	gScene.SSAO_Shader_SamplesMap_Ref = glGetUniformLocation(gScene.SSAO_Shader, "SamplesMap");
	gScene.SSAO_Shader_RandVectorsMap_Ref = glGetUniformLocation(gScene.SSAO_Shader, "RandVectorsMap");
	gScene.SSAO_Shader_P_Ref = glGetUniformLocation(gScene.SSAO_Shader, "P");
	gScene.SSAO_Shader_winParams_Ref = glGetUniformLocation(gScene.SSAO_Shader, "winParams");

	gScene.SSAO_Blur_Shader = CreateProgram("shaders/ssao_blur.vs", "shaders/ssao_blur.fs");
	if( gScene.SSAO_Blur_Shader == 0 ){
		cerr <<  "Failed to create shader." << endl;
		return -1;
	}
	gScene.SSAO_Blur_Shader_AmbOcclusionMap_Ref = glGetUniformLocation(gScene.SSAO_Blur_Shader, "AmbOcclusionMap");
	gScene.SSAO_Blur_Shader_texelSize_Ref = glGetUniformLocation(gScene.SSAO_Blur_Shader, "texelSize");

	// Вектора для определения occlusion factor'а.
	const GLuint SamplesImgSize = 4;
	GLubyte SamplesImg[SamplesImgSize][SamplesImgSize][3] = {
		{ {121, 127, 127}, {135, 135, 129}, {122, 127, 128}, {126, 128, 126} },
		{ {127, 126, 127}, {126, 144, 141}, {134, 129, 127}, {123, 144, 121} },
		{ {126, 138, 143}, {166, 136, 102}, {108, 144, 119}, {135, 135, 131} },
		{ {141, 112, 115}, { 74,  75,  97}, {129, 129, 124}, {149, 127, 135} } };

	glGenTextures(1, &gScene.SamplesTexture);
	glBindTexture(GL_TEXTURE_2D, gScene.SamplesTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SamplesImgSize, SamplesImgSize, 0, GL_RGB, GL_UNSIGNED_BYTE, SamplesImg);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Вектора используемые для поворота.
	GLubyte RandVectors[4][4][3] = {
		{ {  0, 124, 121}, {115, 218, 216}, {151, 215,  38}, {143, 186,  15} },
		{ { 82, 230,  66}, { 27, 134,  49}, {144,   1, 117}, {204, 190, 207} },
		{ {250, 161, 129}, { 68, 100, 237}, {  6, 160, 104}, {199, 232, 118} },
		{ {232, 152,  59}, {209,  55, 193}, { 33, 110, 212}, {183, 152,  15} } };

	glGenTextures(1, &gScene.RandVectorsTexture);
	glBindTexture(GL_TEXTURE_2D, gScene.RandVectorsTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, RandVectors);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Загружаем объект.
	LoadModel("models/house.obj");
	return 1;
}

void UpdateScene(){
	static double lastTime = glfwGetTime();
	GLfloat deltaTime = glfwGetTime() - lastTime;

	InputHandler(deltaTime);

	lastTime = glfwGetTime();

	// Вычисление FPS.
	static double startTime = glfwGetTime();
	static int FPS = 0;
	if( glfwGetTime() - startTime > 1.0f ){
		//cout << FPS << endl;
		startTime = glfwGetTime();
		FPS = 0;
	}
	FPS++;
}

void RenderScene(){
	// Рендерим текстуру глубины.
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, gScene.DepthFrameBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gScene.LinearDepthTexture, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram( gScene.DepGenShader );
	glBindBuffer(GL_ARRAY_BUFFER, gScene.VerBuffer[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gScene.IndBuffer[1]);
	Mat4x4 P, V, W;
	Mat4x4Pers(P, gScene.cam.GetFov(), gScene.cam.GetRatio(), gScene.cam.GetNear(), gScene.cam.GetFar());
	Mat4x4View(V, gScene.cam.GetEye(), gScene.cam.GetTarget(), gScene.cam.GetUp());
	Mat4x4Identity(W);
	glUniformMatrix4fv( gScene.DepGenShader_P_Ref, 1, GL_TRUE, P.m );
	glUniformMatrix4fv( gScene.DepGenShader_V_Ref, 1, GL_TRUE, V.m );
	glUniformMatrix4fv( gScene.DepGenShader_W_Ref, 1, GL_TRUE, W.m );
	glUniform1f( gScene.DepGenShader_near_Ref, gScene.cam.GetNear() );
	glUniform1f( gScene.DepGenShader_far_Ref, gScene.cam.GetFar() );
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Pos), (void*)0);
	glDrawElements(GL_TRIANGLES, gScene.IndCount[1], GL_UNSIGNED_INT, 0);

	// Рендерим ambient occlusion текстуру.
	glDisable(GL_DEPTH_TEST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gScene.AmbientOcclusionTexture, 0);
	glUseProgram(gScene.SSAO_Shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gScene.LinearDepthTexture);
	glUniform1i(gScene.SSAO_Shader_LinDepthMap_Ref, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gScene.SamplesTexture);
	glUniform1i(gScene.SSAO_Shader_SamplesMap_Ref, 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gScene.RandVectorsTexture);
	glUniform1i(gScene.SSAO_Shader_RandVectorsMap_Ref, 2);
	glUniformMatrix4fv( gScene.SSAO_Shader_P_Ref, 1, GL_TRUE, P.m );
	glUniform4f( gScene.SSAO_Shader_winParams_Ref, gScene.ratio, gScene.cam.GetNear(), gScene.cam.GetFar(), gScene.winWidth );
	glBindBuffer(GL_ARRAY_BUFFER, gScene.VerBuffer[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gScene.IndBuffer[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glDrawElements(GL_TRIANGLES, gScene.IndCount[0], GL_UNSIGNED_INT, 0);

	// Размывание ambient occlusion текстуры и вывод на экран.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(gScene.SSAO_Blur_Shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gScene.AmbientOcclusionTexture);
	glUniform1i(gScene.SSAO_Blur_Shader_AmbOcclusionMap_Ref, 0);
	glUniform2f(gScene.SSAO_Blur_Shader_texelSize_Ref, gScene.texelSize.x, gScene.texelSize.y);
	glBindBuffer(GL_ARRAY_BUFFER, gScene.VerBuffer[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gScene.IndBuffer[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glDrawElements(GL_TRIANGLES, gScene.IndCount[0], GL_UNSIGNED_INT, 0);
}

void LoadModel(const char* path){
	vector<Vertex_Pos> vers;
	vector<GLuint> inds;
	FILE* f = fopen(path, "r");
	if( f == NULL ){
		cerr << "Can not open file: \"" << path << '\"' << endl;
		return;
	}
	char buf[1024];
	Vertex_Pos ver = {0,0,0};
	GLuint index[3];
	while( fgets(buf, 1024,f) != NULL ){
		if( strncmp(buf, "v ", 2) == 0 ){
			sscanf(buf, "v %f %f %f", &ver.pos.x, &ver.pos.y, &ver.pos.z);
			vers.push_back(ver);
			continue;
		}
		if( strncmp(buf, "f ", 2) == 0 ){
			sscanf(buf, "f %u %u %u", &index[0], &index[1], &index[2]);
			inds.push_back(index[0]-1);
			inds.push_back(index[1]-1);
			inds.push_back(index[2]-1);
		}
	}
	fclose(f);

	glGenBuffers(1, &gScene.VerBuffer[1]);
	glBindBuffer(GL_ARRAY_BUFFER, gScene.VerBuffer[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_Pos)*vers.size(), &vers[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	gScene.VerCount[1] = vers.size();

	glGenBuffers(1, &gScene.IndBuffer[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gScene.IndBuffer[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*inds.size(), &inds[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	gScene.IndCount[1] = inds.size();
}

GLuint CreateProgram(const char *vertex_shader_path, const char *fragment_shader_path){
	// Сначала создаём вершинный шейдер.

	// Считываем код вер. шейдера.
	string verShaderCode("");
	ifstream verShaderStream(vertex_shader_path);
	if( verShaderStream.is_open() ){
		string line("");
		while( getline(verShaderStream, line) )
			verShaderCode += line + "\n";
		verShaderStream.close();
	} else{
		cerr << "Error: Cann't open \"" << vertex_shader_path << "\" file." << endl;
		return 0;
	}

	// Компилируем вер. шейдер.
	GLuint verShader = glCreateShader(GL_VERTEX_SHADER);
	const char *verShaderSource = verShaderCode.c_str();
	glShaderSource(verShader, 1, &verShaderSource, NULL);
	glCompileShader(verShader);

	// Проверём не возникли ли ошибки во время компиляции.
	GLint result;
	int infoLogLength;
	glGetShaderiv(verShader, GL_COMPILE_STATUS, &result);
	if( result == GL_FALSE ){
		glGetShaderiv(verShader, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar *errMessage = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(verShader, infoLogLength, NULL, errMessage);
		errMessage[infoLogLength] = '\0';
		cerr << "Error in \"" << vertex_shader_path << "\":" << endl;
		cerr << errMessage << endl;
		delete [] errMessage;
		return 0;
	}

	// Теперь создаём фрагментный шейдер.
	// Считываем код фраг. шейдера.
	string fragShaderCode("");
	ifstream fragShaderStream(fragment_shader_path);
	if( fragShaderStream.is_open() ){
		string line("");
		while( getline(fragShaderStream, line) )
			fragShaderCode += line + "\n";
		fragShaderStream.close();
	} else{
		cerr << "Error: Cann't open \"" << fragment_shader_path << "\" file." << endl;
		return 0;
	}

	// Компилируем фраг. шейдер.
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fragShaderSource = fragShaderCode.c_str();
	glShaderSource(fragShader, 1, &fragShaderSource, NULL);
	glCompileShader(fragShader);

	// Проверём не возникли ли ошибки во время компиляции.
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
	if( result == GL_FALSE ){
		glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar *errMessage = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(fragShader, infoLogLength, NULL, errMessage);
		errMessage[infoLogLength] = '\0';
		cerr << "Error in \"" << fragment_shader_path << "\":" << endl;
		cerr << errMessage << endl;
		delete [] errMessage;
		return 0;
	}

	// Создаём шейдерную программу.
	GLuint program = glCreateProgram();
	glAttachShader(program, verShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	// Проверём не возникли ли ошибки во время компоновки.
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if( result == GL_FALSE ){
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar *errMessage = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(fragShader, infoLogLength, NULL, errMessage);
		errMessage[infoLogLength] = '\0';
		cerr << "Error in \"" << fragment_shader_path << "\":" << endl;
		cerr << errMessage << endl;
		delete [] errMessage;
		return 0;
	}

	glDeleteShader(verShader);
	glDeleteShader(fragShader);
	return program;
}

void ReleaseSceneResources(){
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &gScene.DepthFrameBuffer);
	glDeleteRenderbuffers(1, &gScene.DepthRenderBuffer);
	glDeleteTextures(1, &gScene.LinearDepthTexture);
	glDeleteTextures(1, &gScene.AmbientOcclusionTexture);
	glDeleteProgram(gScene.DepGenShader);
	glDeleteProgram(gScene.SSAO_Shader);
	glDeleteProgram(gScene.SSAO_Blur_Shader);
	glDeleteVertexArrays(1, &gScene.vertexArrayID);
	glDeleteBuffers(2, gScene.VerBuffer);
	glDeleteBuffers(2, gScene.IndBuffer);

	glfwTerminate();
}

static void InputHandler(GLfloat deltaTime){
	if( glfwGetKey(GLFW_KEY_LEFT) == GLFW_PRESS )
		gScene.cam.SetAlpha( gScene.cam.GetAlpha() + 3*deltaTime );

	if( glfwGetKey(GLFW_KEY_RIGHT) == GLFW_PRESS )
		gScene.cam.SetAlpha( gScene.cam.GetAlpha() - 3*deltaTime );

	if( glfwGetKey(GLFW_KEY_UP) == GLFW_PRESS )
		if( (gScene.cam.GetBeta() + 3.0f*deltaTime) < gScene.PI_OVER_TWO )
			gScene.cam.SetBeta( gScene.cam.GetBeta() + 3*deltaTime );

	if( glfwGetKey(GLFW_KEY_DOWN) == GLFW_PRESS )
		if( (gScene.cam.GetBeta() - 3.0f*deltaTime) > -gScene.PI_OVER_TWO )
			gScene.cam.SetBeta( gScene.cam.GetBeta() - 3*deltaTime );
	gScene.cam.CalculatePV();
}

static void MouseWheelHandler(GLint pos){
	static GLint WheelPrevPos = 0;
	GLint WheelCurrentPos = glfwGetMouseWheel();
	if( WheelCurrentPos != WheelPrevPos ){
		gScene.cam.SetRadius( 
				gScene.cam.GetRadius() + (WheelPrevPos - WheelCurrentPos)
				);
		if( gScene.cam.GetRadius() < 1.0f )
			gScene.cam.SetRadius( 1.0f );
	}
	WheelPrevPos = WheelCurrentPos;
}

static GLfloat GetRand(){
	return rand()/GLfloat(RAND_MAX)*2.0f - 1.0f;
}
