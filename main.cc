#include <iostream>
#include <GL/glfw.h>
#include "HelperFuns.h"
using namespace std;

int main(int argc, char* argv[]){
	if( InitGL(800, 600, 3, 3) == -1 ){
		cerr << "Failed to initialize OpenGL" << endl;
		return -1;
	}

	if( InitScene() == -1 ){
		cerr << "Failed to initialize scene" << endl;
		return -1;
	}

	do{
		UpdateScene();
		RenderScene();
		glfwSwapBuffers();
	}
	while( glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS &&
			glfwGetWindowParam(GLFW_OPENED) );

	ReleaseSceneResources();
	return 0;
}
