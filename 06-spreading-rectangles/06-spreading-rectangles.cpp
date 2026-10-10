// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 6
// Spreading Rectangle Animation
// - Draw 5–10 rectangles at random positions with random colors and sizes.
// - When the user clicks inside a rectangle:
//   - Divide the rectangle into four smaller rectangles.
//   - Animate their positions, colors, and sizes.
//     - Position:
//       - Randomly select and apply one of the movement animations listed below.
//     - Size:
//       - Gradually shrink each rectangle.
//       - When a rectangle reaches a certain size, make it disappear completely.
//     - Color:
//       - Gradually make the original color brighter or darker.
//         - Example (darker): (1.0, 0.0, 0.0) -> (0.7, 0.0, 0.0) -> ... -> (0.0, 0.0, 0.0)
//         - Example (brighter): (1.0, 0.0, 0.0) -> (1.0, 0.3, 0.3) -> ... -> (1.0, 1.0, 1.0)
// - Movement animations:
//   1. Move left, right, up, and down.
//   2. Move diagonally.
//   3. Move all four divided rectangles together in the same direction.
//   4. Move horizontally, vertically, and diagonally in eight directions. In this mode, eight smaller rectangles appear.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using namespace std;
using namespace glm;

struct Rect {
    Rect(const vec4& in_bounds, const vec2& in_pos, const vec3& in_color = {})
        : bounds{in_bounds}, position{in_pos}, color{in_color} {
    }

    void Move(const float dx, const float dy) {
        bounds.x += dx;
        bounds.y += dy;
        bounds.z += dx;
        bounds.w += dy;
    }

    vec4 bounds;
    vec3 color;
    vec2 position;
    float lifetime{};
    vec2 velocity{};
};

constexpr int kWindowWidth{800};
constexpr int kWindowHeight{600};

random_device rd{};
default_random_engine dre{rd()};
uniform_real_distribution<float> urd{0.0f, 1.0f};
uniform_real_distribution<float> rpos{-0.9f, 0.9f};
uniform_int_distribution r_anim{0, 3};
uniform_int_distribution r_dir{-1, 1};

constexpr float kR{0.1f};
constexpr float kE{kR * 1.5f};
vector<Rect> rects;

void ProcessInput(GLFWwindow* window);
void DrawScene();
void Tick(const float delta_time);

vec3 GetRandomColor();
void CreateRect(const float pos_x, const float pos_y);
void ResetScreen();
void ScaleBounds(vec4& bounds, const float scale_factor);

int main() {
    if (!glfwInit()) {
        cerr << "fail to init glfw!" << endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);  // Required for glColor3f() and glRectf()

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 06", nullptr, nullptr)};
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

    double prev_time{glfwGetTime()};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        double cur_time{glfwGetTime()};
        Tick(static_cast<float>(cur_time - prev_time));
        prev_time = cur_time;

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

    PollMousePressedOnce(GLFW_MOUSE_BUTTON_LEFT, [=]() {
        double x{}, y{};
        glfwGetCursorPos(window, &x, &y);
        x = x / kWindowWidth * 2 - 1.0f;
        y = 1.0f - y / kWindowHeight * 2;

        for (auto rit = rects.rbegin(); rit != rects.rend(); ++rit) {
            const float min_x{rit->bounds.x + rit->position.x};
            const float min_y{rit->bounds.y + rit->position.y};
            const float max_x{rit->bounds.z + rit->position.x};
            const float max_y{rit->bounds.w + rit->position.y};
            if (x >= min_x && x <= max_x && y >= min_y && y <= max_y) {
                const float kR = rit->bounds.w * 0.5f;
                const vec3 c = rit->color;
                const vec2 p{rit->position};
                rects.erase(std::prev(rit.base()));
                const float lt{1.0f};
                const array<vec2, 4> a0{vec2{0.0f, 1.0f}, vec2{1.0f, 0.0f}, vec2{0.0f, -1.0f}, vec2{-1.0f, 0.0f}};
                const array<vec2, 4> a1{vec2{1.0f, 1.0f}, vec2{1.0f, -1.0f}, vec2{-1.0f, -1.0f}, vec2{-1.0f, 1.0f}};
                const int anim{r_anim(dre)};

                if (anim == 2) {
                    const vec2 vel{r_dir(dre), r_dir(dre)};
                    Rect rect1{Rect{{-kR, -kR, kR, kR}, {max_x, max_y}, c}};
                    rect1.lifetime = lt;
                    rect1.velocity = vel;
                    Rect rect2{Rect{{-kR, -kR, kR, kR}, {max_x, min_y}, c}};
                    rect2.lifetime = lt;
                    rect2.velocity = vel;
                    Rect rect3{Rect{{-kR, -kR, kR, kR}, {min_x, min_y}, c}};
                    rect3.lifetime = lt;
                    rect3.velocity = vel;
                    Rect rect4{Rect{{-kR, -kR, kR, kR}, {min_x, max_y}, c}};
                    rect4.lifetime = lt;
                    rect4.velocity = vel;
                    rects.push_back(rect1);
                    rects.push_back(rect2);
                    rects.push_back(rect3);
                    rects.push_back(rect4);
                } else {
                    if (anim == 0 || anim == 3) {
                        Rect rect1{Rect{{-kR, -kR, kR, kR}, {p.x, max_y}, c}};
                        rect1.lifetime = lt;
                        rect1.velocity = {0.0f, 1.0f};
                        Rect rect2{Rect{{-kR, -kR, kR, kR}, {max_x, p.y}, c}};
                        rect2.lifetime = lt;
                        rect2.velocity = {1.0f, 0.0f};
                        Rect rect3{Rect{{-kR, -kR, kR, kR}, {p.x, min_y}, c}};
                        rect3.lifetime = lt;
                        rect3.velocity = {0.0f, -1.0f};
                        Rect rect4{Rect{{-kR, -kR, kR, kR}, {min_x, p.y}, c}};
                        rect4.lifetime = lt;
                        rect4.velocity = {-1.0f, 0.0f};
                        rects.push_back(rect1);
                        rects.push_back(rect2);
                        rects.push_back(rect3);
                        rects.push_back(rect4);
                    }
                    if (anim == 1 || anim == 3) {
                        Rect rect1{Rect{{-kR, -kR, kR, kR}, {max_x, max_y}, c}};
                        rect1.lifetime = lt;
                        rect1.velocity = {1.0f, 1.0f};
                        Rect rect2{Rect{{-kR, -kR, kR, kR}, {max_x, min_y}, c}};
                        rect2.lifetime = lt;
                        rect2.velocity = {1.0f, -1.0f};
                        Rect rect3{Rect{{-kR, -kR, kR, kR}, {min_x, min_y}, c}};
                        rect3.lifetime = lt;
                        rect3.velocity = {-1.0f, -1.0f};
                        Rect rect4{Rect{{-kR, -kR, kR, kR}, {min_x, max_y}, c}};
                        rect4.lifetime = lt;
                        rect4.velocity = {-1.0f, 1.0f};
                        rects.push_back(rect1);
                        rects.push_back(rect2);
                        rects.push_back(rect3);
                        rects.push_back(rect4);
                    }
                }
                break;
            }
        }
    });

    PollKeyPressedOnce(GLFW_KEY_Q, [=]() { glfwSetWindowShouldClose(window, GL_TRUE); });
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& rect : rects) {
        glColor3f(rect.color.kR, rect.color.g, rect.color.b);
        vec4 prim{rect.bounds};
        prim.x += rect.position.x;
        prim.y += rect.position.y;
        prim.z += rect.position.x;
        prim.w += rect.position.y;
        glRectf(prim.x, prim.y, prim.z, prim.w);
    }
}

void Tick(const float delta_time) {
    float fps{1.0f / delta_time};
    for (auto it = rects.begin(); it != rects.end();) {
        if (it->lifetime != 0.0f) {
            if (it->lifetime < 0.1f) {
                it = rects.erase(it);
            } else {
                it->position += it->velocity * 0.2f / fps;
                it->color += 1.0f / fps;
                it->lifetime -= delta_time;
                ++it;
            }
        } else {
            ++it;
        }
    }
}

vec3 GetRandomColor() {
    return {urd(dre), urd(dre), urd(dre)};
}

void CreateRect(const float pos_x, const float pos_y) {
    uniform_real_distribution<float> rsize{0.1f, 0.2f};
    const float kR{rsize(dre)};
    rects.push_back(Rect{{-kR, -kR, kR, kR}, {pos_x, pos_y}, GetRandomColor()});
}

void ResetScreen() {
    rects.clear();
    for (int i{}; i < 10; ++i) {
        CreateRect(rpos(dre), rpos(dre));
    }
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