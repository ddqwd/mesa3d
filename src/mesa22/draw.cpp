#include <GL/gl.h>
#include <GL/freeglut.h>


void HandleKeyEvent(unsigned char key, int x , int y)
{
	if (key == 27) {
		exit(0);
	}

}
void init() {
	glClearColor(1,1,1,1);
}

void DrawLine()
{

	LineWidth(100);
}	
void display() {

	glClear(GL_COLOR_BUFFER_BIT);
	glutSwapBuffers();
}

int main(int argc, char *argv[])
{
	glutInit(&argc,argv);
	glutInitWindowSize(500,500);
	glutCreateWindow(" shiji Opengl Test");
	init();
	glutDisplayFunc(display);
	glutKeyboardFunc(HandleKeyEvent);
	glutMainLoop();




	return 0;
}
