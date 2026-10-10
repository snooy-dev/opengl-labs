// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 1
// - Create and display an 800 × 600 window.
//   - Set the initial background color to white.
// - Display the window and change the background color based on keyboard input.
// Keyboard inputs :
// - c : Cyan(green + blue)
// - m : Magenta(red + blue)
// - y : Yellow(red + green)
// - a : Random color
// - g : Gray
// - k : Black
// - t : Start a timer that continuously changes the background to a random color at specified intervals.
// - s : Stop the timer.
// - esc : Exit the program.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using namespace std;
using namespace glm;

constexpr int kWindowWidth{800};
constexpr int kWindowHeight{600};

random_device rd;
default_random_engine dre{rd()};
uniform_real_distribution<float> urd{0.0f, 1.0f};

vec3 bg_color{1.0f, 1.0f, 1.0f};  // Inital background color
bool enable_timer{};

void ProcessInput(GLFWwindow* window);
void DrawScene();

int main() {
    if (!glfwInit()) {
        cerr << "fail to init glfw!" << endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 01", nullptr, nullptr)};
    if (!window) {
        cerr << "fail to create window" << endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "fail to init glew" << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, kWindowWidth, kWindowHeight);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        if (enable_timer) {
            if (glfwGetTime() > 1) {
                bg_color = vec3{urd(dre), urd(dre), urd(dre)};
                glfwSetTime(0.0f);
            }
        }

        DrawScene();

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void ProcessInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        bg_color = {0.0f, 1.0f, 1.0f};
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        bg_color = {1.0f, 0.0f, 1.0f};
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        bg_color = {1.0f, 1.0f, 0.0f};
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        bg_color = {urd(dre), urd(dre), urd(dre)};
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        bg_color = {0.5f, 0.5f, 0.5f};
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        bg_color = {0.0f, 0.0f, 0.0f};
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        enable_timer = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        enable_timer = false;
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void DrawScene() {
    glClearColor(bg_color.x, bg_color.y, bg_color.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}