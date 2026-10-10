// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 9
// Move triangles with different directions and speeds.
// 1: Bounce, 2: Horizontal zigzag, 3: Vertical zigzag, 4: Spiral.
// Press the same number again to stop. a/b: Fill/outline, c: Clear, q: Quit.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using namespace std;
using namespace glm;

struct Primitive;
struct Object;

constexpr int kWindowWidth{1000};
constexpr int kWindowHeight{1000};

random_device rd{};
default_random_engine dre{rd()};
uniform_real_distribution<float> r_color{0.0f, 1.0f};
uniform_real_distribution<float> r_sca{0.5f, 2.0f};
uniform_real_distribution<float> r_speed{0.25f, 0.6f};
uniform_int_distribution<int> r_dir{0, 1};

GLuint shader_program_id;
GLuint vertex_shader;
GLuint fragment_shader;

array<unique_ptr<Object>, 4> tris;
int mode;
int last_mode{};
bool wireframe{};
vector<vec2> points{};

string FileToBuffer(const char* file);
void MakeVertexShaders();
void MakeFragmentShaders();
GLuint MakeShaderProgram();

void ProcessInput(GLFWwindow* window);
void DrawScene();

vec3 GetRandomColor();

void CreateObject(GLFWwindow* window);

struct Primitive {
    Primitive(const vector<vec3>& in_pos, const vector<unsigned int>& in_idx) : v_pos{in_pos}, idx{in_idx} {
    }

    void InitObject() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * v_pos.size(), v_pos.data(), GL_STATIC_DRAW);
        GLint p_attribute{glGetAttribLocation(shader_program_id, "vPos")};
        glVertexAttribPointer(p_attribute, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
        glEnableVertexAttribArray(p_attribute);

        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * idx.size(), idx.data(), GL_STATIC_DRAW);
    }

    vector<vec3> v_pos;
    vector<unsigned int> idx;
    GLuint vao{};
    GLuint vbo{};
    GLuint ebo{};
};

Primitive point{{{0.0f, 0.0f, 0.0f}}, {0}};
Primitive triangle{{{0.0f, 0.1f, 0.0f}, {-0.1f, -0.1f, 0.0f}, {0.1f, -0.1f, 0.0f}}, {0, 1, 2}};
Primitive line_h{{{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, {0, 1}};
Primitive line_v{{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}, {0, 1}};

struct Object {
    Object(Primitive* in_prim, const vec3& in_pos = {}, const vec3& in_color = {})
        : prim{in_prim}, pos{in_pos}, color{in_color} {
        prev_pos = pos;
    }

    void Move(const float dx, const float dy) {
        pos.x += dx;
        pos.y += dy;
    }

    vec4 GetBounds() const {
        vec4 bounds{100.0f, 100.0f, -100.0f, -100.0f};
        float c{std::cos(rotation)};
        float s{std::sin(rotation)};
        for (const auto& vertex : prim->v_pos) {
            vec3 p{vertex * scale};
            float x{p.x * c - p.y * s + pos.x};
            float y{p.x * s + p.y * c + pos.y};
            bounds.x = std::min(bounds.x, x);
            bounds.y = std::min(bounds.y, y);
            bounds.z = std::max(bounds.z, x);
            bounds.w = std::max(bounds.w, y);
        }
        return bounds;
    }

    void KeepInside() {
        vec4 bounds{GetBounds()};
        if (bounds.x < -1.0f) {
            pos.x += -1.0f - bounds.x;
        } else if (bounds.z > 1.0f) {
            pos.x -= bounds.z - 1.0f;
        }
        if (bounds.y < -1.0f) {
            pos.y += -1.0f - bounds.y;
        } else if (bounds.w > 1.0f) {
            pos.y -= bounds.w - 1.0f;
        }
    }

    void Tick(const float delta_time) {
        if (!mode) {
            return;
        }
        if (last_mode != mode) {
            step_left = 0.0f;
            angle = 0.0f;
            radius = 0.02f;
            radius_dir = 1.0f;
            prev_pos = {pos.x, pos.y};
        }

        if (mode != 2) {
            rotation = 0.0f;
        }

        vec2 delta{};
        switch (mode) {
            case 1:
                delta = velocity * delta_time;
                break;
            case 2:
                scale.x = std::abs(scale.x);
                scale.y = std::abs(scale.y);
                if (step_left > 0.0f) {
                    float step{std::min(step_left, std::abs(velocity.y) * delta_time)};
                    delta.y = velocity.y > 0.0f ? step : -step;
                    step_left -= step;
                } else {
                    delta.x = velocity.x * delta_time;
                }
                break;
            case 3:
                delta = vec2{velocity.x * 0.35f, velocity.y * 1.5f} * delta_time;
                if ((scale.y > 0 && velocity.y < 0) || (scale.y < 0 && velocity.y > 0)) {
                    scale *= -1;
                }
                break;
            case 4: {
                angle += speed * 5.0f * delta_time;
                radius += radius_dir * speed * 0.01f * delta_time;
                if (radius >= 0.5f || radius <= 0.02f) {
                    radius = std::clamp(radius, 0.02f, 0.5f);
                    radius_dir *= -1.0f;
                }
                pos = {prev_pos.x + radius * std::cos(angle), prev_pos.y + radius * std::sin(angle), 0.0f};
            } break;
            default:
                break;
        }

        if (mode == 4) {
            points.push_back({pos.x, pos.y});
        } else {
            if (mode == 2 && (delta.x != 0.0f || delta.y != 0.0f)) {
                rotation = std::atan2(delta.y, delta.x) - numbers::pi_v<float> * 0.5f;
            }

            Move(delta.x, delta.y);
        }

        vec4 bounds{GetBounds()};
        if ((bounds.x <= -1.0f && delta.x < 0.0f) || (bounds.z >= 1.0f && delta.x > 0.0f)) {
            velocity.x *= -1.0f;
            if (mode == 2) {
                step_left = 0.15f;
            }
        }
        if ((bounds.y <= -1.0f && delta.y < 0.0f) || (bounds.w >= 1.0f && delta.y > 0.0f)) {
            velocity.y *= -1.0f;
        }
        KeepInside();
    }

    Primitive* prim;
    vec3 pos;
    vec3 color;
    vec3 scale{1.0f};
    float speed{r_speed(dre)};
    vec2 velocity{(r_dir(dre) ? 1.0f : -1.0f) * speed,
        (r_dir(dre) ? 1.0f : -1.0f) * r_speed(dre)};
    float step_left{};
    float angle{};
    float rotation{};
    float radius{0.02f};
    float radius_dir{1.0f};
    vec2 prev_pos{};
};

array<Object, 2> lines = {Object{&line_h}, {&line_v}};

int main() {
    if (!glfwInit()) {
        cerr << "fail to init glfw!" << endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 09", nullptr, nullptr)};
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

    MakeVertexShaders();
    MakeFragmentShaders();
    shader_program_id = MakeShaderProgram();

    for (auto& line : lines) {
        line.prim->InitObject();
    }

    point.InitObject();

    double prev_time{glfwGetTime()};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        const double curr_time{glfwGetTime()};
        float delta_time{std::min(static_cast<float>(curr_time - prev_time), 0.1f)};
        prev_time = curr_time;
        while (delta_time > 0.0f) {
            float step{std::min(delta_time, 0.01f)};
            for (auto& tri : tris) {
                if (tri) {
                    tri->Tick(step);
                }
            }
            delta_time -= step;

            if (last_mode != mode && last_mode == 4) {
                points.clear();
            }
            last_mode = mode;
        }

        DrawScene();

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

string FileToBuffer(const char* file) {
    ifstream shader_file{file};
    if (!shader_file.is_open()) {
        return {};
    }

    string source{istreambuf_iterator<char>(shader_file), istreambuf_iterator<char>()};
    shader_file.close();
    return source;
}

void MakeVertexShaders() {
    string vertex_source{FileToBuffer("vertex.glsl")};
    const char* source{vertex_source.c_str()};

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex_shader, 1, &source, nullptr);
    glCompileShader(vertex_shader);

    GLint result{};
    array<GLchar, 512> error_log{};

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
    if (!result) {
        glGetShaderInfoLog(vertex_shader, 512, nullptr, error_log.data());
        cerr << "ERROR: fail to compile the vertex shader\n"
             << error_log.data() << '\n';
        return;
    }
}

void MakeFragmentShaders() {
    string fragment_source{FileToBuffer("fragment.glsl")};
    const char* source{fragment_source.c_str()};

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragment_shader, 1, &source, nullptr);
    glCompileShader(fragment_shader);

    GLint result{};
    array<GLchar, 512> error_log{};

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
    if (!result) {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, error_log.data());
        cerr << "ERROR: fail to compile the fragment shader\n"
             << error_log.data() << '\n';
        return;
    }
}

GLuint MakeShaderProgram() {
    GLint result{};
    array<GLchar, 512> error_log{};

    GLuint shader_id{glCreateProgram()};

    glAttachShader(shader_id, vertex_shader);
    glAttachShader(shader_id, fragment_shader);

    glLinkProgram(shader_id);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glGetProgramiv(shader_id, GL_LINK_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(shader_id, 512, nullptr, error_log.data());
        cerr << "ERROR: fail to link the shader program\n"
             << error_log.data() << '\n';
        return 0;
    }

    return shader_id;
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

    PollKeyPressedOnce(GLFW_KEY_A, []() { wireframe = false; });
    PollKeyPressedOnce(GLFW_KEY_B, []() { wireframe = true; });
    PollKeyPressedOnce(GLFW_KEY_1, []() { mode = mode == 1 ? 0 : 1; });
    PollKeyPressedOnce(GLFW_KEY_2, []() { mode = mode == 2 ? 0 : 2; });
    PollKeyPressedOnce(GLFW_KEY_3, []() { mode = mode == 3 ? 0 : 3; });
    PollKeyPressedOnce(GLFW_KEY_4, []() { mode = mode == 4 ? 0 : 4; });

    PollMousePressedOnce(GLFW_MOUSE_BUTTON_LEFT, [=]() {
        CreateObject(window);
    });

    PollMousePressedOnce(GLFW_MOUSE_BUTTON_RIGHT, [=]() {
        double x{}, y{};
        glfwGetCursorPos(window, &x, &y);
        x = x / kWindowWidth * 2 - 1.0f;
        y = 1.0f - y / kWindowHeight * 2;

        int area{};
        if (x > 0 && y > 0) {
            area = 0;
        } else if (x < 0 && y > 0) {
            area = 1;
        } else if (x < 0 && y < 0) {
            area = 2;
        } else if (x > 0 && y < 0) {
            area = 3;
        } else {
            return;
        }

        if (tris[area]) {
            float sca{r_sca(dre)};
            tris[area]->scale = {sca, sca, 0.0f};
            tris[area]->KeepInside();
        }
    });

    PollKeyPressedOnce(GLFW_KEY_C, [&]() { for (auto& tri : tris) tri.reset(); });

    PollKeyPressedOnce(GLFW_KEY_Q, [=]() { glfwSetWindowShouldClose(window, GL_TRUE); });
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program_id);
    GLint v_rot_location{glGetUniformLocation(shader_program_id, "vRot")};
    glUniform1f(v_rot_location, 0.0f);

    for (const auto& obj : lines) {
        GLint v_sca_location{glGetUniformLocation(shader_program_id, "vSca")};
        GLint v_loc_location{glGetUniformLocation(shader_program_id, "vLoc")};
        GLint v_color_location{glGetUniformLocation(shader_program_id, "vColor")};

        glUniform3f(v_sca_location, obj.scale.x, obj.scale.y, obj.scale.z);
        glUniform3f(v_loc_location, obj.pos.x, obj.pos.y, obj.pos.z);
        glUniform3f(v_color_location, obj.color.r, obj.color.g, obj.color.b);

        glBindVertexArray(obj.prim->vao);

        glDrawElements(GL_LINE_LOOP, obj.prim->idx.size(), GL_UNSIGNED_INT, nullptr);
    }

    for (const auto& obj : points) {
        GLint v_sca_location{glGetUniformLocation(shader_program_id, "vSca")};
        GLint v_loc_location{glGetUniformLocation(shader_program_id, "vLoc")};
        GLint v_color_location{glGetUniformLocation(shader_program_id, "vColor")};

        glUniform3f(v_sca_location, 1.0f, 1.0f, 1.0f);
        glUniform3f(v_loc_location, obj.x, obj.y, 0.0f);
        glUniform3f(v_color_location, 0.0f, 0.0f, 0.0f);

        glBindVertexArray(point.vao);

        glPointSize(1.0f);
        glDrawElements(GL_POINTS, point.idx.size(), GL_UNSIGNED_INT, nullptr);
    }

    for (const auto& tri : tris) {
        if (tri) {
            Object& obj = *tri;
            glUniform1f(v_rot_location, obj.rotation);
            GLint v_sca_location{glGetUniformLocation(shader_program_id, "vSca")};
            GLint v_loc_location{glGetUniformLocation(shader_program_id, "vLoc")};
            GLint v_color_location{glGetUniformLocation(shader_program_id, "vColor")};

            glUniform3f(v_sca_location, obj.scale.x, obj.scale.y, obj.scale.z);
            glUniform3f(v_loc_location, obj.pos.x, obj.pos.y, obj.pos.z);
            glUniform3f(v_color_location, obj.color.r, obj.color.g, obj.color.b);

            glBindVertexArray(obj.prim->vao);

            glDrawElements(wireframe ? GL_LINE_LOOP : GL_TRIANGLES, obj.prim->idx.size(), GL_UNSIGNED_INT, nullptr);
        }
    }
}

void CreateObject(GLFWwindow* window) {
    double x{}, y{};
    glfwGetCursorPos(window, &x, &y);
    x = x / kWindowWidth * 2 - 1.0f;
    y = 1.0f - y / kWindowHeight * 2;

    int area{};
    if (x > 0 && y > 0) {
        area = 0;
    } else if (x < 0 && y > 0) {
        area = 1;
    } else if (x < 0 && y < 0) {
        area = 2;
    } else if (x > 0 && y < 0) {
        area = 3;
    } else {
        return;
    }

    auto obj = make_unique<Object>(&triangle, vec3{static_cast<float>(x), static_cast<float>(y), 0.0f}, GetRandomColor());
    obj->prim->InitObject();
    float sca{r_sca(dre)};
    obj->scale = {sca, sca, 0.0f};
    obj->KeepInside();
    tris[area] = std::move(obj);
}

vec3 GetRandomColor() {
    return {r_color(dre), r_color(dre), r_color(dre)};
}