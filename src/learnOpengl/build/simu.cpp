#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/ext/matrix_clip_space.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

int main(int argc, char *argv[])
{
	unsigned int ubo;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, 2*sizeof(glm::mat4) , NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	


	glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo, 0 , 2*sizeof(glm::mat4));

	
#define SCR_WIDTH  1
#define SCR_HEIGHT 1
	glm::mat4 projection = glm::perspective(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);

	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	///glBindVertexArray(cubeVAO);




	return 0;

}
