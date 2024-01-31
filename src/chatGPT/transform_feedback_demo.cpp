#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

const int numParticles = 10;

GLuint VAO;
GLuint particlesPositionBuffer;
GLuint particlesVelocityBuffer;
GLuint transformFeedbackBuffer;

// Particle data
std::vector<GLfloat> initialParticlePositions(numParticles * 3); // x, y, z
std::vector<GLfloat> initialParticleVelocities(numParticles * 3); // vx, vy, vz

void initializeParticles() {
    for (int i = 0; i < numParticles; ++i) {
        // Initialize positions randomly within a cube
        initialParticlePositions[i * 3 + 0] = (rand() % 2000 - 1000) / 1000.0f;
        initialParticlePositions[i * 3 + 1] = (rand() % 2000 - 1000) / 1000.0f;
        initialParticlePositions[i * 3 + 2] = (rand() % 2000 - 1000) / 1000.0f;

        // Initialize velocities
        initialParticleVelocities[i * 3 + 0] = (rand() % 2000 - 1000) / 1000.0f;
        initialParticleVelocities[i * 3 + 1] = (rand() % 2000 - 1000) / 1000.0f;
        initialParticleVelocities[i * 3 + 2] = (rand() % 2000 - 1000) / 1000.0f;
    }
}

int main() {
    // Initialize GLFW and create a window
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Transform Feedback Example", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Initialize particles
    initializeParticles();

    // Create vertex array object (VAO)
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create buffers for particle positions, velocities, and transform feedback
    glGenBuffers(1, &particlesPositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, particlesPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, initialParticlePositions.size() * sizeof(GLfloat), initialParticlePositions.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &particlesVelocityBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, particlesVelocityBuffer);
    glBufferData(GL_ARRAY_BUFFER, initialParticleVelocities.size() * sizeof(GLfloat), initialParticleVelocities.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &transformFeedbackBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, transformFeedbackBuffer);
    glBufferData(GL_ARRAY_BUFFER, initialParticlePositions.size() * sizeof(GLfloat), nullptr, GL_STATIC_READ);

    // Set up shader programs
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 inPosition;
        layout(location = 1) in vec3 inVelocity;
        out vec3 outVelocity;

        void main() {
            gl_Position = vec4(inPosition, 1.0);
            outVelocity = inVelocity;
        }
    )";

    const char* updateShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 inVelocity;
        layout(location = 1) out vec3 outVelocity;

        void main() {
            // Update particle position using velocity
            gl_Position = vec4(gl_in[0].gl_Position.xyz + inVelocity, 1.0);
            outVelocity = inVelocity;
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint updateShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(updateShader, 1, &updateShaderSource, nullptr);
    glCompileShader(updateShader);

    // Create and link shader program for updating particles
    GLuint updateProgram = glCreateProgram();
    glAttachShader(updateProgram, vertexShader);
    glAttachShader(updateProgram, updateShader);
    const GLchar* feedbackVaryings[] = { "outVelocity" };
    glTransformFeedbackVaryings(updateProgram, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
    glLinkProgram(updateProgram);

    // Create and link shader program for rendering particles
    const char* renderShaderSource = R"(
        #version 330 core
        in vec3 outVelocity;
        out vec4 FragColor;

        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red particles
        }
    )";

    GLuint renderShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(renderShader, 1, &renderShaderSource, nullptr);
    glCompileShader(renderShader);

    GLuint renderProgram = glCreateProgram();
    glAttachShader(renderProgram, vertexShader);
    glAttachShader(renderProgram, renderShader);
    glLinkProgram(renderProgram);

    // Create a query object to count rendered particles
    GLuint query;
    glGenQueries(1, &query);

    while (!glfwWindowShouldClose(window)) {
        // Update particles using transform feedback
        glBindVertexArray(VAO);
        glEnable(GL_RASTERIZER_DISCARD);

        glUseProgram(updateProgram);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, transformFeedbackBuffer);

        glBeginTransformFeedback(GL_POINTS);
        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);

        glBindBuffer(GL_ARRAY_BUFFER, particlesPositionBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, particlesVelocityBuffer);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glDrawArrays(GL_POINTS, 0, numParticles);

        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        glEndTransformFeedback();

        glDisable(GL_RASTERIZER_DISCARD);

        // Render particles
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(renderProgram);

        glBindBuffer(GL_ARRAY_BUFFER, transformFeedbackBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // Draw the particles
        glDrawArrays(GL_POINTS, 0, numParticles);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &particlesPositionBuffer);
    glDeleteBuffers(1, &particlesVelocityBuffer);
    glDeleteBuffers(1, &transformFeedbackBuffer);
    glDeleteQueries(1, &query);
    glDeleteVertexArrays(1, &VAO);

    glfwTerminate();

    return 0;
}



