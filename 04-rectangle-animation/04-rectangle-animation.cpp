// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 4
// Apply animations to rectangles
// - Set the background color to dark gray and draw the rectangles using random colors.
// - Draw a rectangle centered at the mouse click position. Up to five rectangles can be drawn.
// - Keyboard inputs:
//   - 1: Movement 1, Each rectangle moves diagonally and bounces in another direction when it hits a wall. Press again to stop.
//   - 2: Movement 2, The rectangles move horizontally in a zigzag pattern. Press again to stop.
//   - 3: Movement 3, All rectangles move clockwise in sequence along the edges of the window. Press again to stop.
//   - 4: Size animation, The rectangles repeatedly grow and shrink. Press again to stop.
//   - 5: Color animation, The rectangles randomly change colors. Press again to stop changing colors.
//   - s: Stop all animations.
//   - m: Move the rectangles back to their original positions.
//   - r: Delete all rectangles and allow new mouse input.
//   - q: Exit the program.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using namespace std;
using namespace glm;

random_device rd{};
default_random_engine dre{rd()};
uniform_real_distribution<float> urd{0.0f, 1.0f};

bitset<5> anim_flags{};

vec3 GetRandomColor();

struct Rect {
    Rect(const vec4& in_bounds, const vec2& in_pos, const vec3& in_color)
        : bounds{in_bounds}, c_pos{in_pos}, color{in_color} {
        position = c_pos;
    }

    void Move(const float dx, const float dy) {
        position.x += dx;
        position.y += dy;
    }

    void ResetPos() {
        position = c_pos;
    }

    void Tick(const float delta_time) {
        if (anim_flags[0]) {
            const float new_pos_x{position.x + velocity.x * delta_time};
            const float new_pos_y{position.y + velocity.y * delta_time};
            vec4 coll{bounds};
            coll.x = (coll.x * scale.x) + new_pos_x;
            coll.y = (coll.y * scale.y) + new_pos_y;
            coll.z = (coll.z * scale.x) + new_pos_x;
            coll.w = (coll.w * scale.y) + new_pos_y;
            if (coll.x < -1.0f || coll.z > 1.0f) {
                velocity.x *= -1;
            }
            if (coll.y < -1.0f || coll.w > 1.0f) {
                velocity.y *= -1;
            }
            position.x = new_pos_x;
            position.y = new_pos_y;
        } else if (anim_flags[1]) {
            const float new_pos_x{position.x + 0.4f * delta_time * velocity.x};

            vec4 coll{bounds};
            coll.x = (coll.x * scale.x) + new_pos_x;
            coll.z = (coll.z * scale.x) + new_pos_x;
            if (coll.x < -1.0f || coll.z > 1.0f) {
                velocity.x *= -1;
            }

            position.x = new_pos_x;
            position.y = c_pos.y + std::cos(position.x * 20.0f) * 0.2f;
        } else if (anim_flags[2]) {
            bool flag{};
            if (position.x >= 1.0f) {
                position.x = 1.0;
                position.y -= velocity.y * delta_time;
                flag = true;
            }
            if (position.y <= -1.0f) {
                position.x -= velocity.y * delta_time;
                position.y = -1.0f;
                flag = true;
            }
            if (position.x <= -1.0f) {
                position.x = -1.0;
                position.y += velocity.y * delta_time;
                flag = true;
            }
            if (position.y >= 1.0f) {
                position.x += velocity.y * delta_time;
                position.y = 1.0f;
                flag = true;
            }
            if (!flag) {
                position.y = 1.0f;
            }
        }

        if (anim_flags[3]) {
            static double time{};
            time += delta_time;
            const double wave{std::cos(time) + 1.5f};
            scale.x = static_cast<float>(wave);
            scale.y = static_cast<float>(wave);
        }
        if (anim_flags[4]) {
            color = GetRandomColor();
        }
    }

    vec4 bounds;
    vec3 color;

    vec2 position{};
    vec2 scale{1.0f, 1.0f};

    const vec2 c_pos;
    vec2 velocity{1.1f, 1.1f};
};

constexpr int kWindowWidth{800};
constexpr int kWindowHeight{600};

vector<Rect> rects;
constexpr int kMaxRects{5};

void ProcessInput(GLFWwindow* window);
void Tick(const float delta_time);
void DrawScene();

void CreateRect(const float x, const float y);

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

    double prev_time{glfwGetTime()};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        const double curr_time{glfwGetTime()};
        Tick(static_cast<float>(curr_time - prev_time));
        prev_time = curr_time;

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
        if (rects.size() < kMaxRects) {
            double x{}, y{};
            glfwGetCursorPos(window, &x, &y);
            CreateRect(static_cast<float>(x / kWindowWidth * 2 - 1.0f), static_cast<float>(1.0f - y / kWindowHeight * 2));
        }
    });

    PollKeyPressedOnce(GLFW_KEY_1, []() { anim_flags[0].flip(); });
    PollKeyPressedOnce(GLFW_KEY_2, []() { anim_flags[1].flip(); });
    PollKeyPressedOnce(GLFW_KEY_3, []() { anim_flags[2].flip(); });
    PollKeyPressedOnce(GLFW_KEY_4, []() { anim_flags[3].flip(); });
    PollKeyPressedOnce(GLFW_KEY_5, []() { anim_flags[4].flip(); });
    PollKeyPressedOnce(GLFW_KEY_S, []() { anim_flags.reset(); });
    PollKeyPressedOnce(GLFW_KEY_M, []() {
        for (auto& rect : rects) {
            rect.ResetPos();
        }
    });
    PollKeyPressedOnce(GLFW_KEY_R, []() { rects.clear(); });

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void Tick(const float delta_time) {
    for (auto& rect : rects) {
        rect.Tick(delta_time);
    }
}

void DrawScene() {
    glClearColor(0.22f, 0.22f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& rect : rects) {
        glColor3f(rect.color.x, rect.color.y, rect.color.z);
        vec4 prim{rect.bounds};
        prim.x = prim.x * rect.scale.x + rect.position.x;
        prim.y = prim.y * rect.scale.y + rect.position.y;
        prim.z = prim.z * rect.scale.x + rect.position.x;
        prim.w = prim.w * rect.scale.y + rect.position.y;
        glRectf(prim.x, prim.y, prim.z, prim.w);
    }
}

vec3 GetRandomColor() {
    return {urd(dre), urd(dre), urd(dre)};
}

void CreateRect(const float x, const float y) {
    const float r = 0.1f;
    Rect new_rect{{-r, -r, r, r}, {x, y}, GetRandomColor()};
    rects.push_back(std::move(new_rect));
}