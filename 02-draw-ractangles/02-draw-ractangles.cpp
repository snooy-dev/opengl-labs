// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 2
// Display a window and implement mouse interactions.
// - Draw the background using a color of your choice.
// - Divide the screen horizontally and vertically into two halves, creating four regions.
//   - Assign each region a different random color.
// - Use the following keyboard and mouse commands to draw shapes and modify their properties.
//   - 1/2/3/4: Draw a rectangle at the corresponding quadrant. Choose the color and size as desired.
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

using namespace std;
using namespace glm;

struct Rect {
    Rect(const vec4& in_bounds, const vec3& in_color = {}) : bounds{in_bounds}, color{in_color} {}

    vec4 bounds;
    vec3 color;
};

struct Area {
    Area(const vec4& in_bounds) : bounds{in_bounds} {}

    const vec4 bounds;
    vec3 color{};
    vector<Rect> rects{};
};

constexpr int kWindowWidth{800};
constexpr int kWindowHeight{600};

random_device rd;
default_random_engine dre{rd()};
uniform_real_distribution<float> urd{0.0f, 1.0f};

array<Area, 4> areas{
    Area{{-1.0f, 0.0f, 0.0f, 1.0f}},
    Area{{0.0f, 0.0f, 1.0f, 1.0f}},
    Area{{-1.0f, -1.0f, 0.0f, 0.0f}},
    Area{{0.0f, -1.0f, 1.0f, 0.0f}}};

constexpr int kMaxRects{5};

int selected_rect_id{-1};
int selected_area_id{-1};

void ProcessInput(GLFWwindow* window);
void DrawScene();

vec3 GetRandomColor();
void ResetAreas();
void CreateRect(const int area_id);
tuple<int, int> ClickedRect(const double xpos, const double ypos);
void ScaleBounds(vec4& bounds, const float scale_factor, const vec4& border);

int main() {
    if (!glfwInit()) {
        cerr << "fail to init glfw!" << endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);  // Required for glColor3f() and glRectf()

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 02", nullptr, nullptr)};
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

    ResetAreas();

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

    PollKeyPressedOnce(GLFW_KEY_1, []() { CreateRect(0); });
    PollKeyPressedOnce(GLFW_KEY_2, []() { CreateRect(1); });
    PollKeyPressedOnce(GLFW_KEY_3, []() { CreateRect(2); });
    PollKeyPressedOnce(GLFW_KEY_4, []() { CreateRect(3); });

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double x{}, y{};
        glfwGetCursorPos(window, &x, &y);

        tie(selected_rect_id, selected_area_id) = ClickedRect(x / kWindowWidth * 2 - 1.0f, 1.0f - y / kWindowHeight * 2);
    }

    PollKeyPressedOnce(GLFW_KEY_EQUAL, []() {
        if (selected_rect_id != -1) {
            ScaleBounds(areas[selected_area_id].rects[selected_rect_id].bounds, 1.1f, areas[selected_area_id].bounds);
        }
    });

    PollKeyPressedOnce(GLFW_KEY_MINUS, []() {
        if (selected_rect_id != -1) {
            ScaleBounds(areas[selected_area_id].rects[selected_rect_id].bounds, 0.9f, areas[selected_area_id].bounds);
        }
    });

    PollKeyPressedOnce(GLFW_KEY_C, []() {
        if (selected_rect_id) {
            areas[selected_area_id].rects[selected_rect_id].color = GetRandomColor();
        }
    });

    PollKeyPressedOnce(GLFW_KEY_R, []() { ResetAreas(); });

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void DrawScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& area : areas) {
        glColor3f(area.color.x, area.color.y, area.color.z);
        glRectf(area.bounds.x, area.bounds.y, area.bounds.z, area.bounds.w);
    }

    for (int i{}; i < areas.size(); ++i) {
        auto& area = areas[i];
        for (int j{}; j < area.rects.size(); ++j) {
            auto& rect = area.rects[j];
            if (i == selected_area_id && j == selected_rect_id && static_cast<int>(glfwGetTime() * 4) % 2) {
                continue;
            }
            glColor3f(rect.color.x, rect.color.y, rect.color.z);
            glRectf(rect.bounds.x, rect.bounds.y, rect.bounds.z, rect.bounds.w);
        }
    }
}

vec3 GetRandomColor() {
    return {urd(dre), urd(dre), urd(dre)};
}

void ResetAreas() {
    for (auto& area : areas) {
        area.color = GetRandomColor();
        area.rects.clear();
    }
}

void CreateRect(const int area_id) {
    auto& area = areas[area_id];
    if (area.rects.size() < kMaxRects) {
        uniform_real_distribution rx{area.bounds.x, area.bounds.z};
        uniform_real_distribution ry{area.bounds.y, area.bounds.w};

        Rect new_rect{{rx(dre), ry(dre), rx(dre), ry(dre)}, GetRandomColor()};
        if (new_rect.bounds.x > new_rect.bounds.z) {
            swap(new_rect.bounds.x, new_rect.bounds.z);
        }
        if (new_rect.bounds.y > new_rect.bounds.w) {
            swap(new_rect.bounds.y, new_rect.bounds.w);
        }
        area.rects.push_back(move(new_rect));
    }
}

tuple<int, int> ClickedRect(const double xpos, const double ypos) {
    for (int i{}; i < areas.size(); ++i) {
        auto& area = areas[i];
        for (int j = static_cast<int>(area.rects.size()) - 1; j >= 0; --j) {
            auto& rect = area.rects[j];
            const float& min_x{rect.bounds.x};
            const float& min_y{rect.bounds.y};
            const float& max_x{rect.bounds.z};
            const float& max_y{rect.bounds.w};
            if (min_x <= xpos && xpos <= max_x && min_y <= ypos && ypos <= max_y) {
                Rect r = move(rect);
                area.rects.erase(area.rects.begin() + j);
                area.rects.push_back(move(r));
                return {j, i};
            }
        }
    }
    return {-1, -1};
}

void ScaleBounds(vec4& bounds, const float scale_factor, const vec4& border) {
    const float center_x{(bounds.x + bounds.z) * 0.5f};
    const float center_y{(bounds.y + bounds.w) * 0.5f};

    vec4 new_bounds{bounds};
    new_bounds.x -= center_x;
    new_bounds.y -= center_y;
    new_bounds.z -= center_x;
    new_bounds.w -= center_y;

    new_bounds *= scale_factor;

    new_bounds.x += center_x;
    new_bounds.y += center_y;
    new_bounds.z += center_x;
    new_bounds.w += center_y;

    if (border.x <= new_bounds.x && border.y <= new_bounds.y && new_bounds.z <= border.z && new_bounds.w <= border.w) {
        bounds = new_bounds;
    }
}