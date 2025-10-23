#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 ourColor;\n"
"void main()\n"
"{\n"
"   FragColor = ourColor;\n"
"}\n\0";

// 检查是否支持program二进制
bool checkProgramBinarySupport() {
    GLint formats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);

    // 还需要检查函数指针是否可用
    if (formats < 1) {
        std::cout << "Driver reports no program binary formats supported" << std::endl;
        return false;
    }
    if (glGetProgramBinary == nullptr) {
        std::cout << "glGetProgramBinary function not available" << std::endl;
        return true;
    }

    std::cout << "Program binary supported. Number of formats: " << formats << std::endl;
    return true;
}

// program二进制保存函数
bool saveProgramBinary(GLuint program, const std::string& filename) {
    if (program == 0) {
        std::cout << "Invalid program object" << std::endl;
        return false;
    }

    // 检查program链接状态
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        std::cout << "Program is not successfully linked" << std::endl;
        return false;
    }

    // 检查二进制支持
    if (!checkProgramBinarySupport()) {
        std::cout << "Program binary not supported, cannot save" << std::endl;
        return false;
    }

    // 获取二进制数据大小
    GLint length = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &length);
    if (length <= 0) {
        std::cout << "Program binary length is 0 or invalid: " << length << std::endl;
        return false;
    }

    std::cout << "Program binary length: " << length << " bytes" << std::endl;

    // 分配内存并获取二进制数据
    std::vector<GLubyte> binaryData(length);
    GLenum binaryFormat = 0;
    GLsizei actualLength = 0;

    // 使用更安全的方式调用
    glGetProgramBinary(program, length, &actualLength, &binaryFormat, binaryData.data());

    // 检查OpenGL错误
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error after glGetProgramBinary: " << error << std::endl;
        return false;
    }

    if (actualLength != length) {
        std::cout << "Warning: Actual binary length (" << actualLength
            << ") differs from reported length (" << length << ")" << std::endl;
    }

    if (actualLength == 0) {
        std::cout << "No binary data retrieved" << std::endl;
        return false;
    }

    // 保存到文件
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    // 先写入格式，再写入实际长度，最后写入数据
    file.write(reinterpret_cast<const char*>(&binaryFormat), sizeof(binaryFormat));
    file.write(reinterpret_cast<const char*>(&actualLength), sizeof(actualLength));
    file.write(reinterpret_cast<const char*>(binaryData.data()), actualLength);

    file.close();

    if (!file.good()) {
        std::cout << "Failed to write program binary to file" << std::endl;
        return false;
    }

    std::cout << "Program binary saved successfully: " << filename
        << " (format: " << binaryFormat << ", size: " << actualLength << " bytes)" << std::endl;
    return true;
}

// 安全的program二进制加载函数
GLuint loadProgramBinary(const std::string& filename) {
    // 检查二进制支持
    if (!checkProgramBinarySupport()) {
        std::cout << "Program binary not supported, cannot load" << std::endl;
        return 0;
    }

    // 打开文件
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Failed to open file for reading: " << filename << std::endl;
        return 0;
    }

    // 读取格式和长度
    GLenum binaryFormat;
    GLsizei length;

    file.read(reinterpret_cast<char*>(&binaryFormat), sizeof(binaryFormat));
    file.read(reinterpret_cast<char*>(&length), sizeof(length));

    if (length <= 0) {
        std::cout << "Invalid binary length in file: " << length << std::endl;
        file.close();
        return 0;
    }

    // 读取二进制数据
    std::vector<GLubyte> binaryData(length);
    file.read(reinterpret_cast<char*>(binaryData.data()), length);
    file.close();

    if (file.gcount() != length) {
        std::cout << "File read incomplete. Expected: " << length
            << ", Actual: " << file.gcount() << std::endl;
        return 0;
    }

    // 创建新的program对象
    GLuint program = glCreateProgram();

    // 加载二进制数据
    glProgramBinary(program, binaryFormat, binaryData.data(), length);

    // 检查OpenGL错误
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error after glProgramBinary: " << error << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    // 检查链接状态
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE) {
        std::cout << "Failed to load program binary" << std::endl;

        // 获取错误信息
        GLint infoLogLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 0) {
            std::vector<GLchar> infoLog(infoLogLength);
            glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());
            std::cout << "Program binary load error: " << infoLog.data() << std::endl;
        }

        glDeleteProgram(program);
        return 0;
    }

    std::cout << "Program binary loaded successfully: " << filename << std::endl;
    return program;
}

GLuint createShaderProgramFromSource() {
    std::cout << "Compiling shaders from source..." << std::endl;

    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return 0;
    }

    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(shaderProgram);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "Shader program compiled and linked successfully from source" << std::endl;
    return shaderProgram;
}

GLuint createOrLoadShaderProgram() {
    const std::string binaryFilename = "shader_program.bin";
    static bool binarySupported = checkProgramBinarySupport();

    if (binarySupported) {
        // First try to load cached binary
        GLuint program = loadProgramBinary(binaryFilename);
        if (program != 0) {
            std::cout << "Successfully loaded program from binary cache" << std::endl;
            return program;
        }

        std::cout << "Binary cache not found or invalid, compiling from source..." << std::endl;
    }
    else {
        std::cout << "Program binary not supported, compiling from source..." << std::endl;
    }

    // Compile from source
    GLuint program = createShaderProgramFromSource();
    if (program == 0) {
        std::cout << "Failed to compile shader program from source" << std::endl;
        return 0;
    }

    // Try to save binary for next time (if supported)
    if (binarySupported) {
        if (!saveProgramBinary(program, binaryFilename)) {
            std::cout << "Warning: Failed to save program binary, but program compiled successfully" << std::endl;
        }
        else {
            std::cout << "Program binary cached for future use" << std::endl;
        }
    }

    return program;
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL - Program Binary Cache", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 输出OpenGL版本信息
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION)                  << std::endl;
    std::cout << "GLSL Version: "   << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: "         << glGetString(GL_VENDOR)                   << std::endl;
    std::cout << "Renderer: "       << glGetString(GL_RENDERER)                 << std::endl;

    // Create or load shader program
    unsigned int shaderProgram = createOrLoadShaderProgram();
    if (shaderProgram == 0) {
        std::cout << "Failed to create shader program" << std::endl;
        glfwTerminate();
        return -1;
    }

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
         0.0f,  0.5f, 0.0f   // top 
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        processInput(window);

        // render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // be sure to activate the shader before any calls to glUniform
        glUseProgram(shaderProgram);

        // update shader uniform
        double timeValue = glfwGetTime();
        float greenValue = static_cast<float>(sin(timeValue) / 2.0 + 0.5);
        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
        glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

        // render the triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // glfw: swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // glfw: terminate
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}