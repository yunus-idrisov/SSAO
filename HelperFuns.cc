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

SceneParameters gSceneParams;

static Vector3f NormalFrom3Points(const Vector3f& v1,const Vector3f& v2,const Vector3f& v3);
static void AddGridToScene(GLfloat gridCellSize, GLint CellCount);

static void InputHandler(GLfloat deltaTime);
static void MouseWheelHandler(GLint pos);

static void AmbientOcclusion();

static GLuint quadVerBuffer;
static GLuint quadShader;
static GLuint renderedTextureID;

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

	//glClearColor(0.1, 0.0, 0.35, 1.0f);
	glClearColor(1,0,0,1);
	glClearDepth(1.0f);

	gSceneParams.winWidth = winWidth;
	gSceneParams.winHeight = winHeight;
	return 1;
}

int InitScene(){
	glfwSetWindowTitle("Ambient occlusion demo");
	glfwSetMouseWheelCallback( MouseWheelHandler );

	gSceneParams.VerBuffer[0];
	gSceneParams.ratio = gSceneParams.winWidth/(float)gSceneParams.winHeight;
	gSceneParams.cam = Camera();
	// Создаём VertexArray.
	glGenVertexArrays(1, &gSceneParams.vertexArrayID);
	glBindVertexArray(gSceneParams.vertexArrayID);

	// Создаём framebuffer.
	glGenFramebuffers(1, &gSceneParams.DepthNormalFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gSceneParams.DepthNormalFrameBuffer);

	// Создаём буфер глубины для framebuffer'a.
	glGenRenderbuffers(1, &gSceneParams.DepthRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, gSceneParams.DepthRenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, gSceneParams.winWidth, gSceneParams.winHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gSceneParams.DepthRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// Создаём текстуры линейной глубины и нормалей.
	glGenTextures(1, &gSceneParams.LinearDepthTexture);
	glBindTexture(GL_TEXTURE_2D, gSceneParams.LinearDepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, gSceneParams.winWidth, gSceneParams.winHeight, 0, GL_RED, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gSceneParams.LinearDepthTexture, 0);

	GLenum drawBuffes[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffes);

	// Проверка на ошибки.
	GLenum er = glGetError();
	if( er != GL_NO_ERROR ){
		switch( er ){
			case GL_INVALID_ENUM : cout << "Invalid enum" << endl; break;
			case GL_INVALID_VALUE : cout << "2" << endl; break;
			default: cout << "some error\n" << endl;
		}
	}
	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
		cout << "framebuffer error.\n";

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Объект для теста буфера глубины и нормалей
	GLfloat quad_vers_pos[] = {
		-1, -1, 0,
		 1, -1, 0,
		-1,  1, 0,
		-1,  1, 0,
		 1, -1, 0,
		 1,  1, 0
	};

	glGenBuffers(1, &quadVerBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadVerBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vers_pos), quad_vers_pos, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	quadShader = CreateProgram("shaders/quadVerShader.vs", "shaders/quadFragShader.fs");
	if( quadShader == 0 ){
		cerr <<  "Failed to create shader." << endl;
		return -1;
	}
	renderedTextureID = glGetUniformLocation(quadShader, "renderedTexture");

	// Создаём шейдеры.
	gSceneParams.gridShader = CreateProgram("shaders/grid.vs", "shaders/grid.fs");
	if( gSceneParams.gridShader == 0 ){
		cerr <<  "Failed to create shader." << endl;
		return -1;
	}
	gSceneParams.gridShaderPVW_Ref = glGetUniformLocation(gSceneParams.gridShader, "mPVW");

	gSceneParams.DepNorShader = CreateProgram("shaders/depth_normal_gen.vs", "shaders/depth_normal_gen.fs");
	if( gSceneParams.DepNorShader == 0 ){
		cerr <<  "Failed to create shader." << endl;
		return -1;
	}
	gSceneParams.DepNorShader_P_Ref = glGetUniformLocation(gSceneParams.DepNorShader, "P");
	gSceneParams.DepNorShader_V_Ref = glGetUniformLocation(gSceneParams.DepNorShader, "V");
	gSceneParams.DepNorShader_W_Ref = glGetUniformLocation(gSceneParams.DepNorShader, "W");

	AddGridToScene(1.0f, 10);

	AmbientOcclusion();
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
		cout << FPS << endl;
		startTime = glfwGetTime();
		FPS = 0;
	}
	FPS++;
}

void RenderScene(){
	
	/*
	// Сетка.
	glUseProgram( gSceneParams.gridShader );
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, gSceneParams.VerBuffer[0]);
	glUniformMatrix4fv( gSceneParams.gridShaderPVW_Ref, 1, GL_TRUE, gSceneParams.cam.GetPV().m );
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Pos_Col), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Pos_Col), (void*)(sizeof(GLfloat)*3));
	glDrawArrays(GL_LINES, 0, gSceneParams.BufferVerCount[0]);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glUseProgram( 0 );
	*/

	// Рендерим текструы глубины и нормалей.
	glClearColor(1,0,0,1);
	glBindFramebuffer(GL_FRAMEBUFFER, gSceneParams.DepthNormalFrameBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram( gSceneParams.DepNorShader );
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, gSceneParams.testBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gSceneParams.testIndBuffer);
	Mat4x4 P, V, W;
	Mat4x4Pers(P, gSceneParams.cam.GetFov(), gSceneParams.cam.GetRatio(), gSceneParams.cam.GetNear(), gSceneParams.cam.GetFar());
	Mat4x4View(V, gSceneParams.cam.GetEye(), gSceneParams.cam.GetTarget(), gSceneParams.cam.GetUp());
	Mat4x4Identity(W);
	glUniformMatrix4fv( gSceneParams.DepNorShader_P_Ref, 1, GL_TRUE, P.m );
	glUniformMatrix4fv( gSceneParams.DepNorShader_V_Ref, 1, GL_TRUE, V.m );
	glUniformMatrix4fv( gSceneParams.DepNorShader_W_Ref, 1, GL_TRUE, W.m );
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Pos_Col), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Pos_Col), (void*)(sizeof(GLfloat)*3));
	glDrawElements(GL_TRIANGLES, gSceneParams.testIndCount, GL_UNSIGNED_SHORT, 0);

	glClearColor(0.1, 0.0, 0.35, 1.0f);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(quadShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gSceneParams.LinearDepthTexture);
	glUniform1i(renderedTextureID, 0);
	glBindBuffer(GL_ARRAY_BUFFER, quadVerBuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0);

	/*
	// Тестовый объект.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram( gSceneParams.gridShader );
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, gSceneParams.testBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gSceneParams.testIndBuffer);
	glUniformMatrix4fv( gSceneParams.gridShaderPVW_Ref, 1, GL_TRUE, gSceneParams.cam.GetPV().m );
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Pos_Col), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Pos_Col), (void*)(sizeof(GLfloat)*3));
	//glDrawArrays(GL_TRIANGLES, 0, gSceneParams.tBufVerCount);
	//glDrawArrays(GL_POINTS, 0, gSceneParams.tBufVerCount);
	glDrawElements(GL_TRIANGLES, gSceneParams.testIndCount, GL_UNSIGNED_SHORT, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glUseProgram( 0 );
	*/
}

static void AmbientOcclusion(){
	// Загружаем объект.
	vector<Vertex_Pos_Col> vers;
	vector<GLushort> inds;
	FILE* f = fopen("many.obj", "r");
	if( f == NULL ){
		cerr << "Could not open file." << endl;
		return;
	}
	char buf[1024];
	Vertex_Pos_Col ver = {0,0,0,0,0,0};
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
	//cout << inds.size() << endl;
	fclose(f);

	GLuint triCount = inds.size()/3;
	Vector3f norms[ vers.size() ];
	for(int i = 0; i < vers.size(); i++){
		Vector3f n = {0,0,0};
		norms[i] = n;
	}

	// Вычисление нормалей.
	for(int i = 0; i < triCount; i++){
		GLushort i1 = inds[3*i + 0];
		GLushort i2 = inds[3*i + 1];
		GLushort i3 = inds[3*i + 2];
		Vector3f n = NormalFrom3Points( vers[i1].pos, vers[i2].pos, vers[i3].pos );
		Vec3Add(norms[i1], norms[i1], n);
		Vec3Add(norms[i2], norms[i2], n);
		Vec3Add(norms[i3], norms[i3], n);
	}

	for(int i = 0; i < vers.size(); i++){
		Vec3Normalize( norms[i] );
		vers[i].col = norms[i];
	}

	glGenBuffers(1, &gSceneParams.testBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, gSceneParams.testBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_Pos_Col)*vers.size(), &vers[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	gSceneParams.tBufVerCount = vers.size();

	glGenBuffers(1, &gSceneParams.testIndBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gSceneParams.testIndBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*inds.size(), &inds[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	gSceneParams.testIndCount = inds.size();
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
	glDeleteVertexArrays(1, &gSceneParams.vertexArrayID);

	glDeleteBuffers(1, &gSceneParams.VerBuffer[0]);
	glDeleteProgram(gSceneParams.gridShader);
}

static void AddGridToScene(GLfloat gridCellSize, GLint CellCount){
	if( CellCount < 2 )
		CellCount = 2;
	if( (CellCount % 2) != 0 )
		CellCount++;
	GLint cc = CellCount;
	GLuint verCount = 2*2*(cc+1);

	Vertex_Pos_Col gridVers[verCount];
	for(int i = 0; i < verCount; i++){
		gridVers[i].pos.y = 0.0f;

		gridVers[i].col.x = 0.3f;
		gridVers[i].col.y = 0.3f;
		gridVers[i].col.z = 0.3f;
	}

	GLfloat t = -(cc/2)*gridCellSize;
	GLuint v = 0;
	for(int i = 0; i <= cc; i++){
		gridVers[v].pos.x = t + i*gridCellSize;
		gridVers[v].pos.z = t;
		v++;
		gridVers[v].pos.x = t + i*gridCellSize;
		gridVers[v].pos.z = -t;
		v++;
	}

	for(int i = 0; i <= cc; i++){
		gridVers[v].pos.x = t; 
		gridVers[v].pos.z = t + i*gridCellSize;
		v++;             
		gridVers[v].pos.x = -t;
		gridVers[v].pos.z =  t + i*gridCellSize;
		v++;
	}

	gridVers[cc].col.x = 0.0f;
	gridVers[cc].col.y = 0.0f;
	gridVers[cc].col.z = 1.0f;
	gridVers[cc+1].col.x = 0.0f;
	gridVers[cc+1].col.y = 0.0f;
	gridVers[cc+1].col.z = 1.0f;

	gridVers[cc + verCount/2].col.x = 1.0f;
	gridVers[cc + verCount/2].col.y = 0.0f;
	gridVers[cc + verCount/2].col.z = 0.0f;
	gridVers[cc + verCount/2+1].col.x = 1.0f;
	gridVers[cc + verCount/2+1].col.y = 0.0f;
	gridVers[cc + verCount/2+1].col.z = 0.0f;

	glGenBuffers(1, &gSceneParams.VerBuffer[0]);
	glBindBuffer(GL_ARRAY_BUFFER, gSceneParams.VerBuffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_Pos_Col)*verCount, gridVers, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	gSceneParams.BufferVerCount[0] = verCount;
}

static void InputHandler(GLfloat deltaTime){
	if( glfwGetKey(GLFW_KEY_LEFT) == GLFW_PRESS )
		gSceneParams.cam.SetAlpha( gSceneParams.cam.GetAlpha() + 3*deltaTime );

	if( glfwGetKey(GLFW_KEY_RIGHT) == GLFW_PRESS )
		gSceneParams.cam.SetAlpha( gSceneParams.cam.GetAlpha() - 3*deltaTime );

	if( glfwGetKey(GLFW_KEY_UP) == GLFW_PRESS )
		if( (gSceneParams.cam.GetBeta() + 3.0f*deltaTime) < gSceneParams.PI_OVER_TWO )
			gSceneParams.cam.SetBeta( gSceneParams.cam.GetBeta() + 3*deltaTime );

	if( glfwGetKey(GLFW_KEY_DOWN) == GLFW_PRESS )
		if( (gSceneParams.cam.GetBeta() - 3.0f*deltaTime) > -gSceneParams.PI_OVER_TWO )
			gSceneParams.cam.SetBeta( gSceneParams.cam.GetBeta() - 3*deltaTime );
	gSceneParams.cam.CalculatePV();
}

static void MouseWheelHandler(GLint pos){
	static GLint WheelPrevPos = 0;
	GLint WheelCurrentPos = glfwGetMouseWheel();
	if( WheelCurrentPos != WheelPrevPos ){
		gSceneParams.cam.SetRadius( 
				gSceneParams.cam.GetRadius() + (WheelPrevPos - WheelCurrentPos)
				);
		if( gSceneParams.cam.GetRadius() < 1.0f )
			gSceneParams.cam.SetRadius( 1.0f );
	}
	WheelPrevPos = WheelCurrentPos;
}

static Vector3f NormalFrom3Points(const Vector3f& v1,const Vector3f& v2,const Vector3f& v3){
	Vector3f e1, e2, n;
	Vec3Sub(e1, v2, v1);
	Vec3Sub(e2, v3, v1);
	Vec3CrossProduct(n, e1, e2);
	Vec3Normalize(n);
	return n;
}
