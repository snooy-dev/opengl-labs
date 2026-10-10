// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 7

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

struct Object {
    Object(Primitive* in_prim, const vec3& in_pos, const vec3& in_color)
        : prim{in_prim}, pos{in_pos}, color{in_color} {
        switch (prim->v_pos.size()) {
            case 1:
                bounds = {-0.01f + pos.x, -0.01f + pos.y, 0.01f + pos.x, 0.01f + pos.y};
                break;
            case 2:
                bounds = {prim->v_pos[0].x + pos.x, -0.01f + pos.y, prim->v_pos[1].x + pos.x, 0.01f + pos.y};
                break;
            case 3:
                bounds = {prim->v_pos[1].x + pos.x, prim->v_pos[1].y + pos.y, pos.x, prim->v_pos[0].y + pos.y};
                break;
            case 4:
                bounds = {prim->v_pos[2].x + pos.x, prim->v_pos[2].y + pos.y, prim->v_pos[0].x + pos.x, prim->v_pos[0].y + pos.y};
                break;
            default:
                break;
        }
    }

    void Move(const float dx, const float dy) {
        pos.x += dx;
        pos.y += dy;
    }

    Primitive* prim;
    vec3 pos;
    vec3 color;
    vec4 bounds;
};

constexpr int kWindowWidth{1000};
constexpr int kWindowHeight{1000};

random_device rd{};
default_random_engine dre{rd()};
uniform_real_distribution<float> r_color{0.0f, 1.0f};

GLuint shader_program_id;
GLuint vertex_shader;
GLuint fragment_shader;

Primitive point{{{0.0f, 0.0f, 0.0f}}, {0}};
Primitive line{{{-0.1f, 0.0f, 0.0f}, {0.1f, 0.0f, 0.0f}}, {0, 1}};
Primitive triangle{{{0.0f, 0.1f, 0.0f}, {-0.1f, -0.1f, 0.0f}, {0.1f, -0.1f, 0.0f}}, {0, 1, 2}};
Primitive rectangle{{{0.1f, 0.1f, 0.0f}, {-0.1f, 0.1f, 0.0f}, {-0.1f, -0.1f, 0.0f}, {0.1f, -0.1f, 0.0f}}, {0, 1, 2, 0, 2, 3}};

vector<Object> scene{};
constexpr size_t kMaxObject{50};
Object* picked_object{};

string FileToBuffer(const char* file);
void MakeVertexShaders();
void MakeFragmentShaders();
GLuint MakeShaderProgram();

void ProcessInput(GLFWwindow* window);
void DrawScene();
void InitObject(Object& obj);

vec3 GetRandomColor();

void CreateObject(Primitive* prim, GLFWwindow* window);
void MoveAllObject(const float dx, const float dy);
void PickObject(const float x, const float y);
void MovePicked(const float dx, const float dy);

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

    PollKeyPressedOnce(GLFW_KEY_P, [=]() { CreateObject(&point, window); });

    PollKeyPressedOnce(GLFW_KEY_E, [=]() { CreateObject(&line, window); });
    PollKeyPressedOnce(GLFW_KEY_T, [=]() { CreateObject(&triangle, window); });
    PollKeyPressedOnce(GLFW_KEY_R, [=]() { CreateObject(&rectangle, window); });

    PollMousePressedOnce(GLFW_MOUSE_BUTTON_LEFT, [=]() {
        double x{}, y{};
        glfwGetCursorPos(window, &x, &y);
        x = x / kWindowWidth * 2 - 1.0f;
        y = 1.0f - y / kWindowHeight * 2;
        PickObject(x, y);
    });

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        MovePicked(0.0f, 0.01f);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        MovePicked(-0.01f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        MovePicked(0.0f, -0.01f);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        MovePicked(0.01f, 0.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        MoveAllObject(0.0f, 0.01f);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        MoveAllObject(-0.01f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        MoveAllObject(0.0f, -0.01f);
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        MoveAllObject(0.01f, 0.0f);
    }

    PollKeyPressedOnce(GLFW_KEY_C, [&]() { scene.clear(); });

    PollKeyPressedOnce(GLFW_KEY_Q, [=]() { glfwSetWindowShouldClose(window, GL_TRUE); });
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program_id);

    for (const auto& obj : scene) {
        GLint v_model_location{glGetUniformLocation(shader_program_id, "vModel")};
        GLint v_color_location{glGetUniformLocation(shader_program_id, "vColor")};

        glUniform3f(v_model_location, obj.pos.x, obj.pos.y, obj.pos.z);
        glUniform3f(v_color_location, obj.color.r, obj.color.g, obj.color.b);

        glBindVertexArray(obj.prim->vao);

        glPointSize(5.0f);
        glDrawElements(GL_POINTS, obj.prim->idx.size(), GL_UNSIGNED_INT, nullptr);
        glDrawElements(GL_LINE_LOOP, obj.prim->idx.size(), GL_UNSIGNED_INT, nullptr);
        glDrawElements(GL_TRIANGLES, obj.prim->idx.size(), GL_UNSIGNED_INT, nullptr);
    }
}

void InitObject(Object& obj) {
    glGenVertexArrays(1, &obj.prim->vao);
    glBindVertexArray(obj.prim->vao);

    glGenBuffers(1, &obj.prim->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, obj.prim->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * obj.prim->v_pos.size(), obj.prim->v_pos.data(), GL_STATIC_DRAW);
    GLint p_attribute{glGetAttribLocation(shader_program_id, "vPos")};
    glVertexAttribPointer(p_attribute, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
    glEnableVertexAttribArray(p_attribute);

    glGenBuffers(1, &obj.prim->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.prim->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * obj.prim->idx.size(), obj.prim->idx.data(), GL_STATIC_DRAW);
}

void CreateObject(Primitive* prim, GLFWwindow* window) {
    double x{}, y{};
    glfwGetCursorPos(window, &x, &y);
    x = x / kWindowWidth * 2 - 1.0f;
    y = 1.0f - y / kWindowHeight * 2;

    if (scene.size() < kMaxObject) {
        Object obj{prim, {x, y, 0.0f}, GetRandomColor()};
        InitObject(obj);
        scene.push_back(std::move(obj));
    }
}

vec3 GetRandomColor() {
    return {r_color(dre), r_color(dre), r_color(dre)};
}

void MoveAllObject(const float dx, const float dy) {
    for (auto& obj : scene) {
        obj.Move(dx, dy);
    }
}

void PickObject(const float x, const float y) {
    for (auto& obj : scene) {
        if (obj.bounds.x <= x && obj.bounds.z >= x && obj.bounds.y <= y && obj.bounds.w >= y) {
            picked_object = &obj;
            return;
        }
    }
}

void MovePicked(const float dx, const float dy) {
    if (picked_object) {
        picked_object->Move(dx, dy);
    }
}
