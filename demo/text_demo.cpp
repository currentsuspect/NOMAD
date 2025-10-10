#include <iostream>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../src/text/TextRenderer.h"

// Window dimensions
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;

// Global variables
GLFWwindow* window = nullptr;
TextRenderer* textRenderer = nullptr;
glm::mat4 projectionMatrix;

// Callbacks
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    projectionMatrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

bool initializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MSDF Text Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    return true;
}

bool initializeTextRenderer() {
    textRenderer = new TextRenderer();
    
    // Try to find a system font
    std::string fontPath;
    
#ifdef _WIN32
    fontPath = "C:\\Windows\\Fonts\\arial.ttf";
#elif __APPLE__
    fontPath = "/System/Library/Fonts/Arial.ttf";
#else
    fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif

    if (!textRenderer->init(fontPath, 48, 2048)) {
        std::cerr << "Failed to initialize text renderer with font: " << fontPath << std::endl;
        return false;
    }

    // Set MSDF parameters for crisp rendering
    textRenderer->setSDFParams(4.0f, 0.5f);

    return true;
}

void render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!textRenderer) return;

    // Demo text with different scales and effects
    glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 red(1.0f, 0.3f, 0.3f, 1.0f);
    glm::vec4 blue(0.3f, 0.6f, 1.0f, 1.0f);
    glm::vec4 green(0.3f, 1.0f, 0.3f, 1.0f);

    // Main title
    textRenderer->drawText(20.0f, 50.0f, "Nomad UI — MSDF Text", white, 1.0f);

    // Different scales
    textRenderer->drawText(20.0f, 120.0f, "Scale 1.5x", red, 1.5f);
    textRenderer->drawText(20.0f, 180.0f, "Scale 0.75x", blue, 0.75f);

    // Outline effect (render black text behind white text)
    textRenderer->drawText(22.0f, 252.0f, "Outline Effect", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f);
    textRenderer->drawText(20.0f, 250.0f, "Outline Effect", white, 1.0f);

    // Glow effect (render multiple times with different colors and offsets)
    textRenderer->drawText(22.0f, 322.0f, "Glow Effect", glm::vec4(0.0f, 0.0f, 0.0f, 0.5f), 1.0f);
    textRenderer->drawText(21.0f, 321.0f, "Glow Effect", glm::vec4(1.0f, 0.0f, 1.0f, 0.7f), 1.0f);
    textRenderer->drawText(20.0f, 320.0f, "Glow Effect", white, 1.0f);

    // Multi-line text
    textRenderer->drawText(20.0f, 400.0f, "Multi-line text\nwith line breaks\nand different colors", green, 1.0f);

    // Character set demo
    textRenderer->drawText(20.0f, 550.0f, "ASCII 32-126: !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", white, 0.8f);

    // Performance test
    std::string perfText = "Performance Test: ";
    for (int i = 0; i < 50; ++i) {
        perfText += "A";
    }
    textRenderer->drawText(20.0f, 650.0f, perfText, glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.6f);
}

void cleanup() {
    if (textRenderer) {
        textRenderer->cleanup();
        delete textRenderer;
        textRenderer = nullptr;
    }

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}

int main() {
    std::cout << "MSDF Text Demo" << std::endl;
    std::cout << "==============" << std::endl;

    // Initialize GLFW and OpenGL
    if (!initializeGLFW()) {
        std::cerr << "Failed to initialize GLFW/OpenGL" << std::endl;
        return -1;
    }

    // Initialize text renderer
    if (!initializeTextRenderer()) {
        std::cerr << "Failed to initialize text renderer" << std::endl;
        cleanup();
        return -1;
    }

    // Set initial projection matrix
    projectionMatrix = glm::ortho(0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);

    std::cout << "Demo initialized successfully!" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        render();

        glfwSwapBuffers(window);
    }

    cleanup();
    std::cout << "Demo completed successfully!" << std::endl;
    return 0;
}