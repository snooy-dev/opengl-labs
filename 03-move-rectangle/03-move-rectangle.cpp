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
};

constexpr int kWindowWidth{800};
constexpr int kWindowHeight{600};

random_device rd{};
default_random_engine dre{rd()};
uniform_real_distribution<float> urd{0.0f, 1.0f};
uniform_real_distribution<float> rpos{-1.0f, 1.0f};

vector<Rect> rects;
constexpr int kMaxRects{20};

void ProcessInput(GLFWwindow* window);
void DrawScene();

vec3 GetRandomColor();
void CreateRect();
vector<Rect>::iterator GetClickedRect(const double xpos, const double ypos);

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

    PollKeyPressedOnce(GLFW_KEY_A, []() { CreateRect(); });

    static double prev_x{}, prev_y{};
    static bool is_drag{};
    static vector<Rect>::iterator selected_rect{};
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double x{}, y{};
        glfwGetCursorPos(window, &x, &y);
        x = x / kWindowWidth * 2 - 1.0f;
        y = 1.0f - y / kWindowHeight * 2;

        if (!is_drag) {
            selected_rect = GetClickedRect(x, y);
            prev_x = x;
            prev_y = y;
            is_drag = true;
        }

        if (selected_rect != rects.end()) {
            selected_rect->Move(static_cast<float>(x - prev_x), static_cast<float>(y - prev_y));
            prev_x = x;
            prev_y = y;
        }
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE && is_drag) {
        is_drag = false;
        const float& min_x{selected_rect->bounds.x};
        const float& min_y{selected_rect->bounds.y};
        const float& max_x{selected_rect->bounds.z};
        const float& max_y{selected_rect->bounds.w};
        for (auto it = rects.rbegin(); it != rects.rend(); ++it) {
            if (std::prev(it.base()) != selected_rect) {
                if (!(it->bounds.x > max_x || it->bounds.z < min_x || it->bounds.y > max_y || it->bounds.w < min_y)) {
                    selected_rect->bounds.x = std::min(min_x, it->bounds.x);
                    selected_rect->bounds.y = std::min(min_y, it->bounds.y);
                    selected_rect->bounds.z = std::max(max_x, it->bounds.z);
                    selected_rect->bounds.w = std::max(max_y, it->bounds.w);
                    rects.erase(std::prev(it.base()));
                    break;
                }
            }
        }
    }

    PollMousePressedOnce(GLFW_MOUSE_BUTTON_RIGHT, [=]() {
        if (rects.size() < kMaxRects) {
            double x{}, y{};
            glfwGetCursorPos(window, &x, &y);
            auto it = GetClickedRect(x / kWindowWidth * 2 - 1.0f, 1.0f - y / kWindowHeight * 2);
            if (it != rects.end()) {
                rects.erase(it);
                CreateRect();
                CreateRect();
            }
        }
    });
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& rect : rects) {
        glColor3f(rect.color.x, rect.color.y, rect.color.z);
        glRectf(rect.bounds.x, rect.bounds.y, rect.bounds.z, rect.bounds.w);
    }
}

vec3 GetRandomColor() {
    return {urd(dre), urd(dre), urd(dre)};
}

void CreateRect() {
    if (rects.size() < kMaxRects) {
        Rect new_rect{{rpos(dre), rpos(dre), rpos(dre), rpos(dre)}, GetRandomColor()};
        if (new_rect.bounds.x > new_rect.bounds.z) {
            std::swap(new_rect.bounds.x, new_rect.bounds.z);
        }
        if (new_rect.bounds.y > new_rect.bounds.w) {
            std::swap(new_rect.bounds.y, new_rect.bounds.w);
        }
        rects.push_back(std::move(new_rect));
    }
}

vector<Rect>::iterator GetClickedRect(const double xpos, const double ypos) {
    for (auto it = rects.rbegin(); it != rects.rend(); ++it) {
        if (it->bounds.x <= xpos && xpos <= it->bounds.z && it->bounds.y <= ypos && ypos <= it->bounds.w) {
            return std::prev(it.base());
        }
    }
    return rects.end();
}