// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 3
// Moving rectangles
// - When the a key is pressed:
//   - Create a rectangle with a random position and color.
//   - Up to 10 rectangles can be created.
//   - Rectangles may overlap.The most recently created rectangle is always drawn on top.
// - When the left mouse button is clicked and dragged over a rectangle:
//   - Move the rectangle according to the mouse movement.
//   - When the mouse button is released, stop moving the rectangle.
// - When a rectangle is dragged with the left mouse button so that it overlaps another rectangle:
//   - Merge the two rectangles into one larger rectangle using the minimum x, y coordinates and maximum x, y coordinates of the overlapping rectangles.
//   - Assign the merged rectangle a random color.
// - When the right mouse button is clicked over a rectangle:
//   - Split the rectangle into two rectangles.
//   - Assign random sizes and colors to the resulting rectangles.
// - The total number of rectangles is limited to 20.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using glm::vec3;
using glm::vec4;

struct Rect
{
    Rect(const vec4& inBounds, const vec3& inColor = {}) : bounds{ inBounds }, color{ inColor } {}

    void move(const float dx, const float dy)
    {
        bounds.x += dx;
        bounds.y += dy;
        bounds.z += dx;
        bounds.w += dy;
    }

    vec4 bounds;
    vec3 color;
};

const int windowWidth{ 800 };
const int windowHight{ 600 };

std::random_device rd{};
std::default_random_engine dre{ rd() };
std::uniform_real_distribution<float> urd{ 0.0f, 1.0f };
std::uniform_real_distribution<float> rpos{ -1.0f, 1.0f };

std::vector<Rect> rects;
const int maxRects{ 20 };

void processInput(GLFWwindow* window);
void drawScene();

vec3 getRandomColor();
void createRect();
std::vector<Rect>::iterator getClickedRect(const double xpos, const double ypos);

int main()
{
    if (!glfwInit())
    {
        std::cerr << "fail to init glfw!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);	// Required for glColor3f() and glRectf()

    GLFWwindow* window{ glfwCreateWindow(windowWidth, windowHight, "OpenGL Labs 03", nullptr, nullptr) };
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
        glfwPollEvents();

        processInput(window);

        drawScene();

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    auto pollKeyPressedOnce = [=](const int key, auto func)
        {
            static bool isPressed{};
            if (glfwGetKey(window, key) == GLFW_PRESS && !isPressed)
            {
                isPressed = true;
                func();
            }
            else if (glfwGetKey(window, key) == GLFW_RELEASE)
            {
                isPressed = false;
            }
        };

    auto pollMousePressedOnce = [=](const int key, auto func)
        {
            static bool isPressed{};
            if (glfwGetMouseButton(window, key) == GLFW_PRESS && !isPressed)
            {
                isPressed = true;
                func();
            }
            else if (glfwGetMouseButton(window, key) == GLFW_RELEASE)
            {
                isPressed = false;
            }
        };

    pollKeyPressedOnce(GLFW_KEY_A, []() { createRect(); });

    static double prevX{}, prevY{};
    static bool isDrag{};
    static std::vector<Rect>::iterator selectedRect{};
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double x{}, y{};
        glfwGetCursorPos(window, &x, &y);
        x = x / windowWidth * 2 - 1.0f;
        y = 1.0f - y / windowHight * 2;


        if (!isDrag)
        {
            selectedRect = getClickedRect(x, y);
            prevX = x;
            prevY = y;
            isDrag = true;
        }

        if (selectedRect != rects.end())
        {
            selectedRect->move(static_cast<float>(x - prevX), static_cast<float>(y - prevY));
            prevX = x;
            prevY = y;
        }
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE && isDrag)
    {
        isDrag = false;
        const float& minX{ selectedRect->bounds.x };
        const float& minY{ selectedRect->bounds.y };
        const float& maxX{ selectedRect->bounds.z };
        const float& maxY{ selectedRect->bounds.w };
        for (auto it = rects.rbegin(); it != rects.rend(); ++it)
        {
            if (std::prev(it.base()) != selectedRect)
            {
                if (!(it->bounds.x > maxX || it->bounds.z < minX || it->bounds.y > maxY || it->bounds.w < minY))
                {
                    selectedRect->bounds.x = std::min(minX, it->bounds.x);
                    selectedRect->bounds.y = std::min(minY, it->bounds.y);
                    selectedRect->bounds.z = std::max(maxX, it->bounds.z);
                    selectedRect->bounds.w = std::max(maxY, it->bounds.w);
                    rects.erase(std::prev(it.base()));
                    break;
                }
            }
        }
    }

    pollMousePressedOnce(GLFW_MOUSE_BUTTON_RIGHT, [=]()
        {
            {
                if (rects.size() < maxRects)
                {
                    double x{}, y{};
                    glfwGetCursorPos(window, &x, &y);
                    auto it = getClickedRect(x / windowWidth * 2 - 1.0f, 1.0f - y / windowHight * 2);
                    if (it != rects.end())
                    {
                        rects.erase(it);
                        createRect();
                        createRect();
                    }
                }
            }
        });
}

void drawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& rect : rects)
    {
        glColor3f(rect.color.x, rect.color.y, rect.color.z);
        glRectf(rect.bounds.x, rect.bounds.y, rect.bounds.z, rect.bounds.w);
    }
}

vec3 getRandomColor()
{
    return { urd(dre), urd(dre), urd(dre) };
}

void createRect()
{
    if (rects.size() < maxRects)
    {
        Rect newRect{ {rpos(dre), rpos(dre), rpos(dre), rpos(dre)}, getRandomColor() };
        if (newRect.bounds.x > newRect.bounds.z)
        {
            std::swap(newRect.bounds.x, newRect.bounds.z);
        }
        if (newRect.bounds.y > newRect.bounds.w)
        {
            std::swap(newRect.bounds.y, newRect.bounds.w);
        }
        rects.push_back(std::move(newRect));
    }
}

std::vector<Rect>::iterator getClickedRect(const double xpos, const double ypos)
{
    for (auto it = rects.rbegin(); it != rects.rend(); ++it)
    {
        if (it->bounds.x <= xpos && xpos <= it->bounds.z && it->bounds.y <= ypos && ypos <= it->bounds.w)
        {
            return std::prev(it.base());
        }
    }
    return rects.end();
}