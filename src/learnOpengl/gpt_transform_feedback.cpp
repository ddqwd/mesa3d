#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// 顶点着色器代码
const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 inPosition;
    out vec2 outTexCoord;
    void main()
    {
        gl_Position = vec4(inPosition, 0.0, 1.0);
        outTexCoord = inPosition; // 将顶点坐标传递给片段着色器
    }
)";

// 片段着色器代码
const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 outTexCoord;
    out vec4 fragColor;
    void main()
    {
        fragColor = vec4(outTexCoord, 0.0, 1.0);
    }
)";

int main() {
    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "Transform Feedback Example", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 将窗口设置为当前上下文
    glfwMakeContextCurrent(window);

    // 初始化GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // 创建顶点数组和缓冲区对象
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // 定义一个三角形的顶点数据
    float vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.0f,  0.5f
    };

    // 将顶点数据传递到缓冲区
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // 创建并编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // 创建着色器程序并链接顶点和片段着色器
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 设置变换反馈
    const char* feedbackVaryings[] = { "outTexCoord" };
    glTransformFeedbackVaryings(shaderProgram, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

    // 使用着色器程序
    glUseProgram(shaderProgram);

    // 配置顶点属性指针
    GLint posAttrib = glGetAttribLocation(shaderProgram, "inPosition");
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(posAttrib);

    // 创建并绑定变换反馈缓冲区
    GLuint feedbackBuffer;
    glGenBuffers(1, &feedbackBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, feedbackBuffer);

    // 设置变换反馈模式
    glEnable(GL_RASTERIZER_DISCARD);

    // 开始渲染
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // 关闭变换反馈模式
    glDisable(GL_RASTERIZER_DISCARD);

    // 解绑变换反馈缓冲区
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 读取变换反馈数据
    float feedbackData[6];
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(feedbackData), feedbackData);

    // 打印变换反馈数据
    std::cout << "Transform Feedback Data:" << std::endl;
    for (int i = 0; i < 6; i++) {
        std::cout << feedbackData[i] << " ";
    }
    std::cout << std::endl;

    // 清理
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    // 关闭GLFW并释放资源
    glfwTerminate();

    return 0;
}

