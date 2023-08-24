#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>


const int windowWidth= 800;
const int windowHeight= 600;

void renderScene() {

	glClearColor(0.2f, 0.3f, 0.4f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_TRIANGLES);


	glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
	glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(0.5f, -0.5f);
	glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(-0.5f, 0.5f);

	
	glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(-0.5f, 0.5f);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(0.5f, -0.5f);
    glColor3f(0.0f, 1.0f, 1.0f); glVertex2f(0.5f, 0.5f);

	glEnd();
}

int main(int argc, char *argv[])
{
	
	if(!glfwInit()) {
		perror("glfwInit failed");
		return -1;
	}

	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight ," glCopyTexImage", NULL, NULL);
	glfwMakeContextCurrent(window);
	
	if(glewInit() != GLEW_OK) {
		perror("glewInit error");
		return -1;
	}
	
	GLuint textureID;
	glGenTextures(1, &textureID);


	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glViewport(0,0, windowWidth, windowHeight);


	while(!glfwWindowShouldClose(window)) {
		renderScene();


		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, windowWidth, windowHeight, 0);


		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}	

	glDeleteTextures(1, &textureID);
	glfwTerminate();

	return 0;
}






