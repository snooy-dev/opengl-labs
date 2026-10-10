// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 13
//
// Draw the faces of a 3D cube and a square pyramid.
//
// - Draw the coordinate axes at the center.
// - Enable depth testing.
//
// Keyboard inputs:
// - 1/2/3/4/5/6: Draw the corresponding face of the cube.
// - 7/8/9/0: Draw the corresponding triangular face of the square pyramid.
// - c: Draw two random faces of the cube.
// - t: Draw the base and one random side face of the square pyramid.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;
import <glm/ext.hpp>;
import <glm/ext/matrix_transform.hpp>;

using namespace std;
using namespace glm;

constexpr int kWindowWidth{1280};
constexpr int kWindowHeight{720};

struct MeshData {
    string id;
    GLenum primitive;
    vector<vec3> vertices;
    vector<uint32_t> indices;
};

struct Mesh {
    Mesh(const MeshData& mesh_data)
        : primitive{mesh_data.primitive}, index_count{static_cast<uint32_t>(mesh_data.indices.size())} {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * mesh_data.vertices.size(), mesh_data.vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh_data.indices.size(), mesh_data.indices.data(), GL_STATIC_DRAW);
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    ~Mesh() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }

    const GLenum primitive;
    const uint32_t index_count;
    GLuint vao{};
    GLuint vbo{};
    GLuint ebo{};
};

struct Object {
    string mesh_id;
    vec3 position;
    vec3 rotation;
    vec3 scale{1.0f};

    vec3 color{1.0f};
    bool is_hidden;
};

GLuint shader_program_id;

vec3 camera_pos;
vec3 camera_target;
vec3 camera_up{0.0f, 1.0f, 0.0f};

unordered_map<string, Mesh> meshes;
vector<Object> scene;

//=================
random_device rd;
default_random_engine dre{rd()};

const vector<MeshData> kMeshData{
    {"line", GL_LINES,
        {{2.0f, 0.0f, 0.0f},
            {-2.0f, 0.0f, 0.0f}},
        {0, 1}},
    {"tri", GL_TRIANGLES,
        {{0.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            {1.0f, -1.0f, 0.0f}},
        {0, 1, 2}},
    {"quad", GL_TRIANGLES,
        {{1.0f, 1.0f, 0.0f},
            {-1.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            {1.0f, -1.0f, 0.0f}},
        {0, 1, 2, 0, 2, 3}}};
//=================

string FileToBuffer(const char* file);
GLuint MakeShaders(GLenum shader_type, const char* file);
GLuint MakeShaderProgram(GLuint vertex_shader, GLuint fragment_shader);

void ProcessInput(GLFWwindow* window);
void DrawScene();

void Initialize();

int main() {
    if (!glfwInit()) {
        cerr << "fail to init glfw!" << '\n';
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 13", nullptr, nullptr)};
    if (!window) {
        cerr << "fail to create window" << '\n';
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "fail to init glew" << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, kWindowWidth, kWindowHeight);

    GLuint vertex_shader{MakeShaders(GL_VERTEX_SHADER, "vertex.glsl")};
    GLuint fragment_shader{MakeShaders(GL_FRAGMENT_SHADER, "fragment.glsl")};
    shader_program_id = MakeShaderProgram(vertex_shader, fragment_shader);
    if (!shader_program_id)
        return 1;

    for (const auto& mesh : kMeshData) {
        meshes.try_emplace(mesh.id, mesh);
    }

    Initialize();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        DrawScene();

        glfwSwapBuffers(window);
    }

    meshes.clear();

    glDeleteProgram(shader_program_id);

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
    return source;
}

GLuint MakeShaders(GLenum shader_type, const char* file) {
    string shader_source{FileToBuffer(file)};
    const char* source{shader_source.c_str()};

    GLuint shader{glCreateShader(shader_type)};

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint result{};
    array<GLchar, 512> errorLog{};

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (!result) {
        glGetShaderInfoLog(shader, 512, nullptr, errorLog.data());
        cerr << "ERROR: fail to compile shader " << file << '\n'
             << errorLog.data() << '\n';
        return 0;
    }

    return shader;
}

GLuint MakeShaderProgram(GLuint vertex_shader, GLuint fragment_shader) {
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
        glDeleteProgram(shader_id);
        return 0;
    }

    return shader_id;
}

void ProcessInput(GLFWwindow* window) {
    static bitset<GLFW_KEY_LAST + 1> pressed{};

    const auto PollKeyPressedOnce = [window](const int key, auto&& action) {
        const int glew_key{glfwGetKey(window, key)};
        if (glew_key == GLFW_PRESS && !pressed[key]) {
            pressed[key] = true;
            action();
        } else if (glew_key == GLFW_RELEASE) {
            pressed[key] = false;
        }
    };

    PollKeyPressedOnce(GLFW_KEY_1, []() { scene[3].is_hidden = !scene[3].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_2, []() { scene[4].is_hidden = !scene[4].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_3, []() { scene[5].is_hidden = !scene[5].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_4, []() { scene[6].is_hidden = !scene[6].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_5, []() { scene[7].is_hidden = !scene[7].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_6, []() { scene[8].is_hidden = !scene[8].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_7, []() { scene[9].is_hidden = !scene[9].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_8, []() { scene[10].is_hidden = !scene[10].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_9, []() { scene[11].is_hidden = !scene[11].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_0, []() { scene[12].is_hidden = !scene[12].is_hidden; });
    PollKeyPressedOnce(GLFW_KEY_C, []() {
        uniform_int_distribution rand_face{3, 8};
        static bool toggle{};
        static int first{};
        static int second{};
        if (toggle) {
            scene[first].is_hidden = true;
            scene[second].is_hidden = true;
        } else {
            first = rand_face(dre);
            second = rand_face(dre);
            while (second == first) {
                second = rand_face(dre);
            }
            scene[first].is_hidden = false;
            scene[second].is_hidden = false;
        }
        toggle = !toggle;
    });
    PollKeyPressedOnce(GLFW_KEY_T, []() {
        static int side_face{10};
        uniform_int_distribution rand_face{10, 12};
        if (scene[13].is_hidden) {
            scene[13].is_hidden = false;
            side_face = rand_face(dre);
            scene[side_face].is_hidden = false;
        } else {
            scene[13].is_hidden = true;
            scene[side_face].is_hidden = true;
        }
    });

    PollKeyPressedOnce(GLFW_KEY_Q, [window]() { glfwSetWindowShouldClose(window, GL_TRUE); });
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program_id);

    const GLint view_location{glGetUniformLocation(shader_program_id, "uView")};
    const mat4 view{lookAt(camera_pos, camera_target, camera_up)};
    glUniformMatrix4fv(view_location, 1, GL_FALSE, value_ptr(view));

    const GLint projection_location{glGetUniformLocation(shader_program_id, "uProjection")};
    const mat4 proj{perspective(radians(60.0f), static_cast<float>(kWindowWidth) / kWindowHeight, 0.1f, 1000.0f)};
    glUniformMatrix4fv(projection_location, 1, GL_FALSE, value_ptr(proj));

    for (const auto& obj : scene) {
        if (obj.is_hidden)
            continue;

        auto it = meshes.find(obj.mesh_id);
        if (it == meshes.end())
            continue;

        const Mesh& mesh{it->second};

        const GLint model_location{glGetUniformLocation(shader_program_id, "uModel")};
        mat4 model{1.0f};
        model = translate(model, obj.position);
        model = rotate(model, radians(obj.rotation.z), vec3{0.0f, 0.0f, 1.0f});
        model = rotate(model, radians(obj.rotation.y), vec3{0.0f, 1.0f, 0.0f});
        model = rotate(model, radians(obj.rotation.x), vec3{1.0f, 0.0f, 0.0f});
        model = scale(model, obj.scale);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, value_ptr(model));

        const GLint color_location{glGetUniformLocation(shader_program_id, "uColor")};
        glUniform3f(color_location, obj.color.r, obj.color.g, obj.color.b);

        glBindVertexArray(mesh.vao);
        glDrawElements(mesh.primitive, mesh.index_count, GL_UNSIGNED_INT, nullptr);
    }
}

void Initialize() {
    glEnable(GL_DEPTH_TEST);

    scene = vector<Object>{
        {"line", {}, {}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.0f, 0.0f}},                   // axis X
        {"line", {}, {0.0f, 0.0f, 90.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.5f, 0.0f}},  // axis Y
        {"line", {}, {0.0f, -90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0, 0.5f}},  // axis Z

        // cube
        {"quad", {1.0f, 0.0f, 0.0f}, {0.0f, 90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, true},
        {"quad", {-1.0f, 0.0f, 0.0f}, {0.0f, -90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, true},
        {"quad", {0.0f, 1.0f, 0.0f}, {-90.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, true},
        {"quad", {0.0f, -1.0f, 0.0f}, {90.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, true},
        {"quad", {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, true},
        {"quad", {0.0f, 0.0f, -1.0f}, {0.0f, 180.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, true},

        // pyramid
        {"tri", {0.5f, 0.0f, 0.0f}, {-30.0f, 90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, true},
        {"tri", {-0.5f, 0.0f, 0.0f}, {-30.0f, -90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, true},
        {"tri", {0.0f, 0.0f, 0.5f}, {-30.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, true},
        {"tri", {0.0f, 0.0f, -0.5f}, {-30.0f, 180.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, true},
        {"quad", {0.0f, -0.86f, 0.0f}, {90.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, true}};

    camera_pos = {3.0f, 2.0f, 2.0f};
    mat4 camera_rotation{1.0f};
    camera_rotation = rotate(camera_rotation, radians(0.0f), vec3{0.0f, 0.0f, 1.0f});
    camera_rotation = rotate(camera_rotation, radians(56.3f), vec3{0.0f, 1.0f, 0.0f});
    camera_rotation = rotate(camera_rotation, radians(-29.0f), vec3{1.0f, 0.0f, 0.0f});
    camera_target = normalize(vec3{camera_rotation * vec4{0.0f, 0.0f, -0.1f, 0.0f}}) + camera_pos;
}
