// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 1
// - Create and display an 800 × 600 window.
//   - Set the initial background color to white.
// - Display the window and change the background color based on keyboard input.
//   - Keyboard inputs :
//     - c : Cyan(green + blue)
//     - m : Magenta(red + blue)
//     - y : Yellow(red + green)
//     - a : Random color
//     - g : Gray
//     - k : Black
//     - t : Start a timer that continuously changes the background to a random color at specified intervals.
//     - s : Stop the timer.
//     - esc : Exit the program.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;

struct Vector3
{
    float x;
    float y;
    float z;
};

std::random_device rd{};
std::default_random_engine dre{ rd() };
std::uniform_real_distribution<float> urd{ 0.0f, 1.0f };

Vector3 bgColor{ 1.0f, 1.0f, 1.0f };	// Inital background color
bool enabledTimer{};

void InputProcess(GLFWwindow* window);
void DrawScene();

int main()
{
    if (!glfwInit())
    {
        std::cerr << "fail to init glfw!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);	// This exercise uses legacy OpenGL features.

    GLFWwindow* window{ glfwCreateWindow(800, 600, "OpenGL Labs 01", nullptr, nullptr) };
    if (!window)
    {
        std::cerr << "fail to create window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "fail to init glew" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, 800, 600);

    while (!glfwWindowShouldClose(window))
    {
        InputProcess(window);

        if (enabledTimer)
        {
            if (glfwGetTime() > 1)
            {
                bgColor = Vector3{ urd(dre), urd(dre), urd(dre) };
                glfwSetTime(0.0f);
            }
        }

        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void InputProcess(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        bgColor = Vector3{ 0.0f, 1.0f, 1.0f };
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        bgColor = Vector3{ 1.0f, 0.0f, 1.0f };
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
    {
        bgColor = Vector3{ 1.0f, 1.0f, 0.0f };
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        bgColor = Vector3{ urd(dre), urd(dre), urd(dre) };
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
    {
        bgColor = Vector3{ 0.5f, 0.5f, 0.5f };
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    {
        bgColor = Vector3{ 0.0f, 0.0f, 0.0f };
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
    {
        enabledTimer = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        enabledTimer = false;
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void DrawScene()
{
    glClearColor(bgColor.x, bgColor.y, bgColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}