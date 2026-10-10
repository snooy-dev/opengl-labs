// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 8

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using namespace std;
using namespace glm;

struct Primitive {
    Primitive(const vector<vec3>& in_pos, const vector<unsigned int>& in_idx) : v_pos{in_pos}, idx{in_idx} {
    }

    vector<vec3> v_pos;
    vector<unsigned int> idx;
    GLuint vao{};
    GLuint vbo{};
    GLuint ebo{};
};

struct Object;

constexpr int kWindowWidth{1000};
constexpr int kWindowHeight{1000};

Primitive triangle{{{0.0f, 0.1f, 0.0f}, {-0.1f, -0.1f, 0.0f}, {0.1f, -0.1f, 0.0f}}, {0, 1, 2}};
Primitive line_h{{{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, {0, 1}};
Primitive line_v{{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}, {0, 1}};

random_device rd{};
default_random_engine dre{rd()};
uniform_real_distribution<float> r_color{0.0f, 1.0f};
uniform_real_distribution<float> r_sca{0.1f, 2.0f};

GLuint shader_program_id;
GLuint vertex_shader;
GLuint fragment_shader;

array<unique_ptr<Object>, 4> tris;
int mode;

string FileToBuffer(const char* file);
void MakeVertexShaders();
void MakeFragmentShaders();
GLuint MakeShaderProgram();

void ProcessInput(GLFWwindow* window);
void DrawScene();

vec3 GetRandomColor();

void CreateObject(GLFWwindow* window);

struct Object {
    Object(Primitive* in_prim, const vec3& in_pos = {}, const vec3& in_color = {})
        : prim{in_prim}, pos{in_pos}, color{in_color} {
    }

    void InitObject() {
        glGenVertexArrays(1, &prim->vao);
        glBindVertexArray(prim->vao);

        glGenBuffers(1, &prim->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, prim->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * prim->v_pos.size(), prim->v_pos.data(), GL_STATIC_DRAW);
        GLint p_attribute{glGetAttribLocation(shader_program_id, "vPos")};
        glVertexAttribPointer(p_attribute, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
        glEnableVertexAttribArray(p_attribute);

        glGenBuffers(1, &prim->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * prim->idx.size(), prim->idx.data(), GL_STATIC_DRAW);
    }

    void Move(const float dx, const float dy) {
        pos.x += dx;
        pos.y += dy;
    }

    Primitive* prim;
    vec3 pos;
    vec3 color;
    vec3 scale{1.0f};
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

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 07", nullptr, nullptr)};
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
        line.InitObject();
    }

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

    PollKeyPressedOnce(GLFW_KEY_A, []() { mode = 0; });
    PollKeyPressedOnce(GLFW_KEY_B, []() { mode = 1; });

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

        float sca{r_sca(dre)};
        tris[area]->scale = {sca, sca, 0.0f};
    });

    PollKeyPressedOnce(GLFW_KEY_C, [&]() {
        for (size_t i{}; i < 4; ++i) {
            uniform_real_distribution<float> r_pos_x{i == 0 || i == 3 ? 0.0f : -1.0f, i == 0 || i == 3 ? 1.0f : 0.0f};
            uniform_real_distribution<float> r_pos_y{i == 0 || i == 1 ? 0.0f : -1.0f, i == 0 || i == 1 ? 1.0f : 0.0f};
            tris[i].reset();
            auto obj = make_unique<Object>(&triangle, vec3{r_pos_x(dre), r_pos_y(dre), 0.0f}, GetRandomColor());
            obj->InitObject();
            float sca{r_sca(dre)};
            obj->scale = {sca, sca, 0.0f};
            tris[i] = std::move(obj);
        }
    });

    PollKeyPressedOnce(GLFW_KEY_Q, [=]() { glfwSetWindowShouldClose(window, GL_TRUE); });
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program_id);

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

    for (const auto& tri : tris) {
        if (tri) {
            Object& obj = *tri;
            GLint v_sca_location{glGetUniformLocation(shader_program_id, "vSca")};
            GLint v_loc_location{glGetUniformLocation(shader_program_id, "vLoc")};
            GLint v_color_location{glGetUniformLocation(shader_program_id, "vColor")};

            glUniform3f(v_sca_location, obj.scale.x, obj.scale.y, obj.scale.z);
            glUniform3f(v_loc_location, obj.pos.x, obj.pos.y, obj.pos.z);
            glUniform3f(v_color_location, obj.color.r, obj.color.g, obj.color.b);

            glBindVertexArray(obj.prim->vao);

            glDrawElements(mode ? GL_LINE_LOOP : GL_TRIANGLES, obj.prim->idx.size(), GL_UNSIGNED_INT, nullptr);
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
    obj->InitObject();
    float sca{r_sca(dre)};
    obj->scale = {sca, sca, 0.0f};
    tris[area] = std::move(obj);
}

vec3 GetRandomColor() {
    return {r_color(dre), r_color(dre), r_color(dre)};
}