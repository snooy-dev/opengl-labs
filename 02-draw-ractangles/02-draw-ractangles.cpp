// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 2
// Display a window and implement mouse interactions.
// - Draw the background using a color of your choice.
// - Divide the screen horizontally and vertically into two halves, creating four regions.
//   - Assign each region a different random color.
// - Use the following keyboard and mouse commands to draw shapes and modify their properties.
//   - 1/2/3/4: Draw a rectangle at the center of the corresponding quadrant.Choose the color and size as desired.
//     - Pressing the same key again redraws it with a different size and color.
//     - Up to five rectangles can be drawn in each region.
//   - Left mouse button : Select a drawn rectangle. Indicate the selected state using a method of your choice.
//   - +/-: Increase or decrease the size of the selected rectangle.
//   - c: Change the selected rectangle's color randomly.
//   - r: Remove all drawn rectangles, change the region colors, and reset the scene.
//   - q: Exit the program.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using glm::vec3;
using glm::vec4;

struct Rect
{
    Rect(const vec4& inBounds, const vec3& inColor = {}) : bounds{ inBounds }, color{ inColor } {}

    vec4 bounds;
    vec3 color;
};

struct Area
{
    Area(const vec4& inBounds) : bounds{ inBounds } {}

    const vec4 bounds;
    vec3 color{};
    std::vector<Rect> rects{};
};

const int windowWidth{ 800 };
const int windowHight{ 600 };

std::random_device rd{};
std::default_random_engine dre{ rd() };
std::uniform_real_distribution<float> urd{ 0.0f, 1.0f };

std::array<Area, 4> areas{
    Area{{-1.0f,  0.0f,  0.0f,  1.0f}},
    Area{{ 0.0f,  0.0f,  1.0f,  1.0f}},
    Area{{-1.0f, -1.0f,  0.0f,  0.0f}},
    Area{{ 0.0f, -1.0f,  1.0f,  0.0f}}
};

const int maxRects{ 5 };

int selectedRectId{ -1 };
int selectedAreaId{ -1 };

void processInput(GLFWwindow* window);
void drawScene();

vec3 getRandomColor();
void resetAreas();
void createRect(const int areaId);
std::tuple<int, int> getClickedRect(const double xpos, const double ypos);
void scaleBounds(vec4& bounds, const float scaleFactor, const vec4& border);

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

    GLFWwindow* window{ glfwCreateWindow(windowWidth, windowHight, "OpenGL Labs 02", nullptr, nullptr) };
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

    resetAreas();

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

    pollKeyPressedOnce(GLFW_KEY_1, []() { createRect(0); });
    pollKeyPressedOnce(GLFW_KEY_2, []() { createRect(1); });
    pollKeyPressedOnce(GLFW_KEY_3, []() { createRect(2); });
    pollKeyPressedOnce(GLFW_KEY_4, []() { createRect(3); });

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double x{}, y{};
        glfwGetCursorPos(window, &x, &y);

        std::tie(selectedRectId, selectedAreaId) = getClickedRect(x / windowWidth * 2 - 1.0f, 1.0f - y / windowHight * 2);
    }

    pollKeyPressedOnce(GLFW_KEY_EQUAL, []()
        {
            if (selectedRectId != -1)
            {
                scaleBounds(areas[selectedAreaId].rects[selectedRectId].bounds, 1.1f, areas[selectedAreaId].bounds);
            }
        });

    pollKeyPressedOnce(GLFW_KEY_MINUS, []()
        {
            if (selectedRectId != -1)
            {
                scaleBounds(areas[selectedAreaId].rects[selectedRectId].bounds, 0.9f, areas[selectedAreaId].bounds);
            }
        });

    pollKeyPressedOnce(GLFW_KEY_C, []()
        {
            if (selectedRectId)
            {
                areas[selectedAreaId].rects[selectedRectId].color = getRandomColor();
            }
        });

    pollKeyPressedOnce(GLFW_KEY_R, []() { resetAreas(); });

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void drawScene()
{
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& area : areas)
    {
        glColor3f(area.color.x, area.color.y, area.color.z);
        glRectf(area.bounds.x, area.bounds.y, area.bounds.z, area.bounds.w);
    }

    for (int i{}; i < areas.size(); ++i)
    {
        auto& area = areas[i];
        for (int j{}; j < area.rects.size(); ++j)
        {
            auto& rect = area.rects[j];
            if (i == selectedAreaId && j == selectedRectId && static_cast<int>(glfwGetTime() * 4) % 2)
            {
                continue;
            }
            glColor3f(rect.color.x, rect.color.y, rect.color.z);
            glRectf(rect.bounds.x, rect.bounds.y, rect.bounds.z, rect.bounds.w);
        }
    }
}

vec3 getRandomColor()
{
    return { urd(dre), urd(dre), urd(dre) };
}

void resetAreas()
{
    for (auto& area : areas)
    {
        area.color = getRandomColor();
        area.rects.clear();
    }
}

void createRect(const int areaId)
{
    auto& area = areas[areaId];
    if (area.rects.size() < maxRects)
    {
        std::uniform_real_distribution rx{ area.bounds.x, area.bounds.z };
        std::uniform_real_distribution ry{ area.bounds.y, area.bounds.w };

        area.rects.push_back(Rect{ {rx(dre), ry(dre), rx(dre), ry(dre)}, getRandomColor() });
    }
}

std::tuple<int, int> getClickedRect(const double xpos, const double ypos)
{
    for (int i{}; i < areas.size(); ++i)
    {
        auto& area = areas[i];
        for (int j = static_cast<int>(area.rects.size()) - 1; j >= 0; --j)
        {
            auto& rect = area.rects[j];
            const float minX{ std::min(rect.bounds.x, rect.bounds.z) };
            const float minY{ std::min(rect.bounds.y, rect.bounds.w) };
            const float maxX{ std::max(rect.bounds.x, rect.bounds.z) };
            const float maxY{ std::max(rect.bounds.y, rect.bounds.w) };
            if (minX <= xpos && xpos <= maxX && minY <= ypos && ypos <= maxY)
            {
                Rect r = std::move(rect);
                area.rects.erase(area.rects.begin() + j);
                area.rects.push_back(std::move(r));
                return { j, i };
            }
        }
    }
    return { -1, -1 };
}

void scaleBounds(vec4& bounds, const float scaleFactor, const vec4& border)
{
    const float centerX{ (bounds.x + bounds.z) * 0.5f };
    const float centerY{ (bounds.y + bounds.w) * 0.5f };

    vec4 newBounds{ bounds };
    newBounds.x -= centerX;
    newBounds.y -= centerY;
    newBounds.z -= centerX;
    newBounds.w -= centerY;

    newBounds *= scaleFactor;

    newBounds.x += centerX;
    newBounds.y += centerY;
    newBounds.z += centerX;
    newBounds.w += centerY;

    if (border.x <= newBounds.x && newBounds.x <= border.z
        && border.y <= newBounds.y && newBounds.y <= border.w
        && border.x <= newBounds.z && newBounds.z <= border.z
        && border.y <= newBounds.w && newBounds.w <= border.w)
    {
        bounds = newBounds;
    }
}