// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 14
//
// Draw 3d objects
//
// - Draw the coordinate axes at the center.
// - Draw a cube or a square pyramid based on keyboard input.
//
// Keyboard inputs:
// - c: Draw a cube.
// - p: Draw a pyramid.
// - h: Toggle depth testing.
// - w/W: Switch between wireframe and solid rendering.
// - x/X: Spin the object around the x-axis in the positive/negative direction.
// - y/Y: Spin the object around the y-axis in the positive/negative direction.
// - left/right/up/down: Move the object along the x-axis/y-axis.
// - s: Reset the object to its initial position and stop all spins.

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

struct Vertex {
    vec3 position;
    vec3 color{1.0f};
};

struct MeshData {
    string id;
    GLenum primitive;
    vector<Vertex> vertices;
    vector<uint32_t> indices;
};

struct Mesh {
    Mesh(const MeshData& mesh_data)
        : primitive{mesh_data.primitive}, index_count{static_cast<uint32_t>(mesh_data.indices.size())} {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh_data.vertices.size(), mesh_data.vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * mesh_data.indices.size(), mesh_data.indices.data(), GL_STATIC_DRAW);
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
};

GLuint shader_program_id;

vec3 camera_pos;
vec3 camera_target;
vec3 camera_up{0.0f, 1.0f, 0.0f};

unordered_map<string, Mesh> meshes;
vector<Object> scene;

//=================
const vector<MeshData> kMeshData{
    {"x-axis", GL_LINES,
        {{{2.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{-2.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}},
        {0, 1}},
    {"y-axis", GL_LINES,
        {{{0.0f, 2.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{0.0f, -2.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}},
        {0, 1}},
    {"z-axis", GL_LINES,
        {{{0.0f, 0.0f, 2.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 1.0f}}},
        {0, 1}},
    {"cube", GL_TRIANGLES,
        {{{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
            {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
            {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
            {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},
            {{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}},
        {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5, 0, 1, 5, 0, 5, 4, 3, 7, 6, 3, 6, 2}},
    {"pyramid", GL_TRIANGLES,
        {{{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}}},
        {0, 2, 1, 0, 3, 2, 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4}}};

constexpr size_t kControlledObjectIndex{3};
bool show_wireframe;
constexpr float move_speed{0.01f};
vec2 spin_velocity;
constexpr float spin_speed{50.0f};
//=================

string FileToBuffer(const char* file);
GLuint MakeShaders(GLenum shader_type, const char* file);
GLuint MakeShaderProgram(GLuint vertex_shader, GLuint fragment_shader);

void ProcessInput(GLFWwindow* window);
void DrawScene();

void Initialize();

//=================
void ToggleDepthTest();
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

    for (const auto& mesh : kMeshData) {
        meshes.try_emplace(mesh.id, mesh);
    }

    Initialize();

    double previous{glfwGetTime()};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        double current{glfwGetTime()};
        Tick(static_cast<float>(current - previous));
        previous = current;

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

    Object& obj{scene[kControlledObjectIndex]};

    PollKeyPressedOnce(GLFW_KEY_C, [&obj]() {
        obj.mesh_id = "cube";
    });

    PollKeyPressedOnce(GLFW_KEY_P, [&obj]() {
        obj.mesh_id = "pyramid";
    });

    PollKeyPressedOnce(GLFW_KEY_H, []() {
        ToggleDepthTest();
    });

    PollKeyPressedOnce(GLFW_KEY_LEFT_SHIFT, []() {});

    PollKeyPressedOnce(GLFW_KEY_W, []() { show_wireframe = !pressed[GLFW_KEY_LEFT_SHIFT]; });

    if (!obj.mesh_id.empty()) {
        PollKeyPressedOnce(GLFW_KEY_X, []() {
            if (pressed[GLFW_KEY_LEFT_SHIFT])
                spin_velocity.x = spin_velocity.x < 0 ? 0.0f : -spin_speed;
            else
                spin_velocity.x = spin_velocity.x > 0 ? 0.0f : spin_speed;
        });

        PollKeyPressedOnce(GLFW_KEY_Y, []() {
            if (pressed[GLFW_KEY_LEFT_SHIFT])
                spin_velocity.y = spin_velocity.y < 0 ? 0.0f : -spin_speed;
            else
                spin_velocity.y = spin_velocity.y > 0 ? 0.0f : spin_speed;
        });

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            obj.position.x += move_speed;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            obj.position.x -= move_speed;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            obj.position.y += move_speed;
        }

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            obj.position.y -= move_speed;
        }
    }

    PollKeyPressedOnce(GLFW_KEY_S, [&obj]() {
        obj.position = {};
        spin_velocity = {};
    });

    PollKeyPressedOnce(GLFW_KEY_Q, [window]() { glfwSetWindowShouldClose(window, GL_TRUE); });
}

void DrawScene() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program_id);

    const GLint view_location{glGetUniformLocation(shader_program_id, "uView")};
    const mat4 view{lookAt(camera_pos, camera_target, camera_up)};
    glUniformMatrix4fv(view_location, 1, GL_FALSE, value_ptr(view));

    const GLint projection_location{glGetUniformLocation(shader_program_id, "uProjection")};
    const mat4 proj{perspective(radians(60.0f), static_cast<float>(kWindowWidth) / kWindowHeight, 0.1f, 1000.0f)};
    glUniformMatrix4fv(projection_location, 1, GL_FALSE, value_ptr(proj));

    for (const auto& obj : scene) {
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

        glBindVertexArray(mesh.vao);
        glPolygonMode(GL_FRONT_AND_BACK, show_wireframe ? GL_LINE : GL_FILL);
        glDrawElements(mesh.primitive, mesh.index_count, GL_UNSIGNED_INT, nullptr);
    }
}

void Initialize() {
    ToggleDepthTest();

    scene = vector<Object>{
        {"x-axis"},  // axis X
        {"y-axis"},  // axis Y
        {"z-axis"},  // axis Z
        {}           // empty object
    };

    camera_pos = {3.0f, 2.0f, 2.0f};
    mat4 camera_rotation{1.0f};
    camera_rotation = rotate(camera_rotation, radians(0.0f), vec3{0.0f, 0.0f, 1.0f});
    camera_rotation = rotate(camera_rotation, radians(56.3f), vec3{0.0f, 1.0f, 0.0f});
    camera_rotation = rotate(camera_rotation, radians(-29.0f), vec3{1.0f, 0.0f, 0.0f});
    camera_target = normalize(vec3{camera_rotation * vec4{0.0f, 0.0f, -0.1f, 0.0f}}) + camera_pos;
}

void ToggleDepthTest() {
    static bool enabled{};
    enabled = !enabled;
    if (enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void Tick(const float delta_time) {
    Object& obj{scene[kControlledObjectIndex]};
    obj.rotation.x += spin_velocity.x * delta_time;
    obj.rotation.y += spin_velocity.y * delta_time;
}
