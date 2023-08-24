#include <iostream>
#include <GL/freeglut.h>


#include <iostream>

void display()
{


}
void init()
{
	GLint maxColorAttachments;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
	printf(" maxColorAttachments = %d\n", maxColorAttachments);
}
int main(int argc, char *argv[])
{
	
	glutInit(&argc, argv);
	
	glutCreateWindow("test chatGPT code");


	init();
	glutDisplayFunc(display);
	glutMainLoop();


	return 0;
}
