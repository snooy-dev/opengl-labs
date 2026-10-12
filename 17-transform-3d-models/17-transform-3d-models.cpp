// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 17
//

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;
import <glm/ext.hpp>;
import <glm/ext/matrix_transform.hpp>;

import gllab_obj;

using namespace std;
using namespace glm;

constexpr int kWindowWidth{1280};
constexpr int kWindowHeight{720};

class PrimitiveRenderer {
public:
    PrimitiveRenderer() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, kPrimitives.size() * sizeof(float), kPrimitives.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    GLuint GetVao() const { return vao; };

private:
    const vector<float> kPrimitives{
        // position  // color
        2, 0, 0, 1, 0, 0,  // x-axis
        -2, 0, 0, 1, 0, 0,

        0, 2, 0, 0, 1, 0,  // y-axis
        0, -2, 0, 0, 1, 0,

        0, 0, 2, 0, 0, 1,  // z-axis
        0, 0, -2, 0, 0, 1};

    GLuint vao;
    GLuint vbo;
};

struct Mesh {
    Mesh(const ObjMesh& mesh)
        : index_count{static_cast<uint32_t>(mesh.indices.size())} {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.vertices.size(), mesh.vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * mesh.indices.size(), mesh.indices.data(), GL_STATIC_DRAW);
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

    const uint32_t index_count;
    GLuint vao{};
    GLuint vbo{};
    GLuint ebo{};
};

struct Object {
    filesystem::path mesh_path;
    vec3 position;
    vec3 rotation;
    vec3 scale{1.0f};
};

GLuint shader_program_id;

vec3 camera_pos;
vec3 camera_target;
vec3 camera_up{0.0f, 1.0f, 0.0f};

unordered_map<filesystem::path, Mesh> meshes;
vector<Object> scene;

//=================
constexpr float move_speed{0.01f};
vec2 spin_velocity;
constexpr float spin_speed{50.0f};
//=================

string FileToBuffer(const char* file);
GLuint MakeShaders(GLenum shader_type, const char* file);
GLuint MakeShaderProgram(GLuint vertex_shader, GLuint fragment_shader);

void ProcessInput(GLFWwindow* window);
void DrawScene(PrimitiveRenderer primitive_renderer);

void LoadMesh(filesystem::path file_path);
void Initialize();

//=================
void Tick(const float delta_time);
//=================

int main() {
    if (!glfwInit()) {
        cerr << "fail to init glfw!" << '\n';
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 14", nullptr, nullptr)};
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

    PrimitiveRenderer primitive_renderer{};

    Initialize();

    double previous{glfwGetTime()};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        double current{glfwGetTime()};
        Tick(static_cast<float>(current - previous));
        previous = current;

        DrawScene(primitive_renderer);

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

    PollKeyPressedOnce(GLFW_KEY_Q, [window]() { glfwSetWindowShouldClose(window, GL_TRUE); });
}

void DrawScene(PrimitiveRenderer primitive_renderer) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program_id);

    const GLint view_location{glGetUniformLocation(shader_program_id, "uView")};
    const mat4 view{lookAt(camera_pos, camera_target, camera_up)};
    glUniformMatrix4fv(view_location, 1, GL_FALSE, value_ptr(view));

    const GLint projection_location{glGetUniformLocation(shader_program_id, "uProjection")};
    const mat4 proj{perspective(radians(60.0f), static_cast<float>(kWindowWidth) / kWindowHeight, 0.1f, 1000.0f)};
    glUniformMatrix4fv(projection_location, 1, GL_FALSE, value_ptr(proj));

    {
        const GLint model_location{glGetUniformLocation(shader_program_id, "uModel")};
        mat4 model{1.0f};
        glUniformMatrix4fv(model_location, 1, GL_FALSE, value_ptr(model));

        glBindVertexArray(primitive_renderer.GetVao());
        glDrawArrays(GL_LINES, 0, 6);
    }

    for (const auto& obj : scene) {
        auto it = meshes.find(obj.mesh_path);
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

        glBindVertexArray(mesh.vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
    }
}

void LoadMesh(filesystem::path file_path) {
    auto obj_mesh = LoadObj(file_path);
    meshes.try_emplace(file_path, *obj_mesh);
}

void Initialize() {
    glEnable(GL_DEPTH_TEST);

    LoadMesh("sphere.obj");
    LoadMesh("monkey.obj");

    scene.clear();
    scene.push_back({"sphere.obj", {2.0f, 0.0f, 0.0f}});
    scene.push_back({"monkey.obj", {-2.0f, 0.0f, 0.0f}});

    camera_pos = {2.0f, 2.0f, 3.0f};
    mat4 camera_rotation{1.0f};
    camera_rotation = rotate(camera_rotation, radians(0.0f), vec3{0.0f, 0.0f, 1.0f});
    camera_rotation = rotate(camera_rotation, radians(30.0f), vec3{0.0f, 1.0f, 0.0f});
    camera_rotation = rotate(camera_rotation, radians(-30.0f), vec3{1.0f, 0.0f, 0.0f});
    camera_target = normalize(vec3{camera_rotation * vec4{0.0f, 0.0f, -0.1f, 0.0f}}) + camera_pos;
}

void Tick(const float delta_time) { /*
     obj.rotation.x += spin_velocity.x * delta_time;
     obj.rotation.y += spin_velocity.y * delta_time;*/
}
