// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 5
// Erasing the screen
// - Display a window and draw 20–40 small rectangles of the same size at random positions using various colors.
// - When the left mouse button is pressed:
//   - Draw a black rectangle(eraser rectangle) that is twice the size of the rectangles on the screen.
//   - While holding the mouse button and moving the mouse, move the eraser rectangle along with the cursor.
// - Any rectangle that collides with the eraser rectangle disappears. Whenever a rectangle disappears, increase the size of the eraser rectangle and change its color to the color of the collided rectangle.
// - When the left mouse button is released:
//   - Remove the eraser rectangle.
//   - Restore all erased rectangles to their original positions.
// - When the left mouse button is pressed again:
//   - Create a black eraser rectangle using its original size.
// - When the right mouse button is pressed:
//   - Create a new rectangle at the clicked position.
//   - Reduce the size of the eraser rectangle whenever a new rectangle is created.
//   - Up to 10 new rectangles can be created.
// - Keyboard input `r`: Delete the existing rectangles, reset the scene, and start over.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using namespace std;
using namespace glm;

struct Rect {
    Rect(const vec4& in_bounds, const vec3& in_color = {}) : bounds{in_bounds}, color{in_color} {}

    void Move(const float dx, const float dy) {
        bounds.x += dx;
        bounds.y += dy;
        bounds.z += dx;
        bounds.w += dy;
    }

    vec4 bounds;
    vec3 color;
    bool is_hidden{};
};

constexpr int kWindowWidth{800};
constexpr int kWindowHeight{600};

random_device rd{};
default_random_engine dre{rd()};
uniform_real_distribution<float> urd{0.0f, 1.0f};
uniform_real_distribution<float> rpos{-1.0f, 1.0f};

constexpr float kR{0.1f};
constexpr float kE{kR * 1.5f};
vector<Rect> rects;
Rect eraser{{}};
vector<size_t> stored_rects;

void ProcessInput(GLFWwindow* window);
void DrawScene();

vec3 GetRandomColor();
void CreateRect(const float pos_x, const float pos_y);
void ResetScreen();
void ResetEraser();
bool IsEraserClicked(const double xpos, const double ypos);
void ScaleBounds(vec4& bounds, const float scale_factor);

int main() {
    if (!glfwInit()) {
        cerr << "fail to init glfw!" << endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);  // Required for glColor3f() and glRectf()

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 03", nullptr, nullptr)};
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

    glViewport(0, 0, 800, 600);

    ResetScreen();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        DrawScene();

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void ProcessInput(GLFWwindow* window) {
    auto PollKeyPressedOnce = [=](const int key, auto func) {
        static bool is_pressed{};
        if (glfwGetKey(window, key) == GLFW_PRESS && !is_pressed) {
            is_pressed = true;
            func();
        } else if (glfwGetKey(window, key) == GLFW_RELEASE) {
            is_pressed = false;
        }
    };

    auto PollMousePressedOnce = [=](const int key, auto func) {
        static bool is_pressed{};
        if (glfwGetMouseButton(window, key) == GLFW_PRESS && !is_pressed) {
            is_pressed = true;
            func();
        } else if (glfwGetMouseButton(window, key) == GLFW_RELEASE) {
            is_pressed = false;
        }
    };

    PollKeyPressedOnce(GLFW_KEY_R, []() { ResetScreen(); });

    static double prev_x{}, prev_y{};
    static bool is_drag{};
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double x{}, y{};
        glfwGetCursorPos(window, &x, &y);
        x = x / kWindowWidth * 2 - 1.0f;
        y = 1.0f - y / kWindowHeight * 2;

        if (!is_drag) {
            is_drag = true;
            ResetEraser();
            eraser.is_hidden = false;
            eraser.bounds = {x - kE, y - kE, x + kE, y + kE};
            prev_x = x;
            prev_y = y;
        }

        if (is_drag) {
            eraser.Move(static_cast<float>(x - prev_x), static_cast<float>(y - prev_y));
            prev_x = x;
            prev_y = y;

            const float& min_x{eraser.bounds.x};
            const float& min_y{eraser.bounds.y};
            const float& max_x{eraser.bounds.z};
            const float& max_y{eraser.bounds.w};
            for (size_t i{}; i < rects.size(); ++i) {
                if (!rects[i].is_hidden && !(rects[i].bounds.x > max_x || rects[i].bounds.z < min_x || rects[i].bounds.y > max_y || rects[i].bounds.w < min_y)) {
                    if (stored_rects.size() >= 10) {
                        stored_rects.erase(stored_rects.begin());
                    }
                    stored_rects.push_back(i);
                    rects[i].is_hidden = true;
                    eraser.color = rects[i].color;
                    ScaleBounds(eraser.bounds, 1.1f);
                    break;
                }
            }
        }
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE && is_drag) {
        is_drag = false;
        eraser.is_hidden = true;
        for (auto& rect : rects) {
            rect.is_hidden = false;
        }
    }

    PollMousePressedOnce(GLFW_MOUSE_BUTTON_RIGHT, [=]() {
        if (stored_rects.size() && is_drag) {
            CreateRect(rpos(dre), rpos(dre));
            rects.erase(rects.begin() + stored_rects.back());
            stored_rects.pop_back();
            ScaleBounds(eraser.bounds, 0.9f);
        }
    });
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& rect : rects) {
        if (!rect.is_hidden) {
            glColor3f(rect.color.x, rect.color.y, rect.color.z);
            glRectf(rect.bounds.x, rect.bounds.y, rect.bounds.z, rect.bounds.w);
        }
    }

    if (!eraser.is_hidden) {
        glColor3f(eraser.color.x, eraser.color.y, eraser.color.z);
        glRectf(eraser.bounds.x, eraser.bounds.y, eraser.bounds.z, eraser.bounds.w);
    }
}

vec3 GetRandomColor() {
    return {urd(dre), urd(dre), urd(dre)};
}

void CreateRect(const float pos_x, const float pos_y) {
    rects.push_back(Rect{{pos_x - kR, pos_y - kR, pos_x + kR, pos_y + kR}, GetRandomColor()});
}

void ResetScreen() {
    ResetEraser();
    rects.clear();
    for (int i{}; i < 20; ++i) {
        CreateRect(rpos(dre), rpos(dre));
    }
}

void ResetEraser() {
    eraser.is_hidden = true;
    eraser.color = {};
    stored_rects.clear();
}

bool IsEraserClicked(const double xpos, const double ypos) {
    return eraser.bounds.x <= xpos && xpos <= eraser.bounds.z && eraser.bounds.y <= ypos && ypos <= eraser.bounds.w;
}

void ScaleBounds(vec4& bounds, const float scale_factor) {
    const float center_x{(bounds.x + bounds.z) * 0.5f};
    const float center_y{(bounds.y + bounds.w) * 0.5f};

    bounds.x -= center_x;
    bounds.y -= center_y;
    bounds.z -= center_x;
    bounds.w -= center_y;

    bounds *= scale_factor;

    bounds.x += center_x;
    bounds.y += center_y;
    bounds.z += center_x;
    bounds.w += center_y;
}