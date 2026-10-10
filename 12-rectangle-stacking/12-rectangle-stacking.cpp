// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 12
//
// Match two moving rectangles inside the marked area and stack them on the right.
//
// Keyboard inputs:
// - Enter: Stop a matching pair and animate it into the tower.
// - r: Reset and start again.
// - q: Quit.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using namespace std;
using namespace glm;

constexpr int kWindowWidth{1000};
constexpr int kWindowHeight{1000};

struct MeshData {
    string id;
    GLenum primitive;
    vector<vec2> vertices;
    vector<uint32_t> indices;
};

struct Mesh {
    Mesh(const MeshData& mesh_data)
        : primitive{mesh_data.primitive}, index_count{static_cast<uint32_t>(mesh_data.indices.size())} {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * mesh_data.vertices.size(), mesh_data.vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), nullptr);
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

GLuint shader_program_id;
unordered_map<string, Mesh> meshes;
GLint position_location{}, scale_location{}, color_location{};

//=================
constexpr float kRectangleHalfWidth{0.085f};
constexpr float kRectangleHalfHeight{0.04f};
constexpr float kLaneBottom{-0.84f};
constexpr float kLaneTop{0.84f};
constexpr float kMatchHalfHeight{0.17f};
constexpr float kTowerX{0.66f};
constexpr float kTransferDuration{0.8f};
constexpr array<float, 2> kLaneX{-0.65f, -0.12f};
constexpr array<float, 2> kInitialY{0.58f, -0.58f};
constexpr array<float, 2> kInitialVelocity{0.62f, -0.79f};

struct Rectangle {
    vec2 position;
    vec2 scale{kRectangleHalfWidth, kRectangleHalfHeight};
    vec3 color{1.0f};
};

using RectanglePair = array<Rectangle, 2>;

enum class Phase {
    Moving,
    Transferring
};

struct StackingState {
    Phase phase{Phase::Moving};
    RectanglePair active{}, transfer_start{};
    vector<RectanglePair> tower;
    array<float, 2> velocity{};
    float transfer_elapsed{};
    float tower_scale{1.0f};
};

StackingState stacking;
default_random_engine dre{random_device{}()};

const vector<MeshData> kMeshData{
    {"quad", GL_TRIANGLES,
        {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}},
        {0, 1, 2, 0, 2, 3}},
    {"outline", GL_LINE_LOOP,
        {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}},
        {0, 1, 2, 3}}};
//=================

string FileToBuffer(const char* file);
GLuint MakeShaders(GLenum shader_type, const char* file);
GLuint MakeShaderProgram(GLuint vertex_shader, GLuint fragment_shader);

void ProcessInput(GLFWwindow* window);
void DrawScene();
void Initialize();

//=================
void SpawnPair();
void ResetStack();
bool CanMatchPair();
void MatchPair();
vec2 GetTowerPosition(size_t rectangle_index);
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

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 12", nullptr, nullptr)};
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

    PollKeyPressedOnce(GLFW_KEY_ENTER, []() { MatchPair(); });
    PollKeyPressedOnce(GLFW_KEY_KP_ENTER, []() { MatchPair(); });
    PollKeyPressedOnce(GLFW_KEY_R, []() { ResetStack(); });
    PollKeyPressedOnce(GLFW_KEY_Q, [window]() { glfwSetWindowShouldClose(window, GL_TRUE); });
}

void DrawRectangle(const Rectangle& rectangle, bool outline = false) {
    glUniform2f(position_location, rectangle.position.x, rectangle.position.y);
    glUniform2f(scale_location, rectangle.scale.x, rectangle.scale.y);
    glUniform3f(color_location, rectangle.color.r, rectangle.color.g, rectangle.color.b);

    const Mesh& mesh{meshes.at(outline ? "outline" : "quad")};
    glBindVertexArray(mesh.vao);
    glDrawElements(mesh.primitive, mesh.index_count, GL_UNSIGNED_INT, nullptr);
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program_id);

    const vec3 blue{0.22f, 0.43f, 0.68f};
    for (float x : kLaneX)
        DrawRectangle({{x, 0.0f}, {0.15f, kLaneTop}, blue}, true);

    for (float x{-0.9f}; x < 0.15f; x += 0.07f)
        for (float y : {-kMatchHalfHeight, kMatchHalfHeight})
            DrawRectangle({{x + 0.022f, y}, {0.022f, 0.002f}, blue});

    for (float y{-kMatchHalfHeight}; y < kMatchHalfHeight; y += 0.07f)
        for (float x : {-0.9f, 0.2f})
            DrawRectangle({{x, y + 0.02f}, {0.002f, 0.02f}, blue});

    DrawRectangle({{kTowerX, kLaneBottom - 0.015f}, {0.14f, 0.003f}, blue});

    const auto DrawPair = [](const RectanglePair& pair) {
        for (const auto& rectangle : pair) {
            DrawRectangle(rectangle);
            Rectangle outline{rectangle};
            outline.color = {0.18f, 0.3f, 0.42f};
            DrawRectangle(outline, true);
        }
    };

    for (const auto& pair : stacking.tower)
        DrawPair(pair);
    DrawPair(stacking.active);
}

void Initialize() {
    position_location = glGetUniformLocation(shader_program_id, "uPosition");
    scale_location = glGetUniformLocation(shader_program_id, "uScale");
    color_location = glGetUniformLocation(shader_program_id, "uColor");
    ResetStack();
}

void SpawnPair() {
    uniform_real_distribution<float> shade{0.25f, 0.85f};
    for (size_t i{}; i < stacking.active.size(); ++i)
        stacking.active[i] = {{kLaneX[i], kInitialY[i]},
            {kRectangleHalfWidth, kRectangleHalfHeight},
            i == 0 ? vec3{0.2f, shade(dre), 0.85f} : vec3{0.95f, shade(dre), 0.15f}};
    stacking.velocity = kInitialVelocity;
    stacking.transfer_elapsed = 0.0f;
    stacking.phase = Phase::Moving;
}

void ResetStack() {
    stacking = {};
    SpawnPair();
}

bool CanMatchPair() {
    return stacking.phase == Phase::Moving &&
           ranges::all_of(stacking.active, [](const Rectangle& rectangle) {
               return std::abs(rectangle.position.y) + rectangle.scale.y <= kMatchHalfHeight;
           });
}

vec2 GetTowerPosition(size_t rectangle_index) {
    return {kTowerX,
        kLaneBottom + (static_cast<float>(rectangle_index) * 2.0f + 1.0f) * kRectangleHalfHeight * stacking.tower_scale};
}

void MatchPair() {
    if (!CanMatchPair())
        return;

    stacking.transfer_start = stacking.active;
    stacking.transfer_elapsed = 0.0f;
    stacking.phase = Phase::Transferring;
    stacking.tower_scale = std::min(1.0f,
        (kLaneTop - kLaneBottom) / (static_cast<float>(stacking.tower.size() + 1) * 4.0f * kRectangleHalfHeight));
    for (size_t pair_index{}; pair_index < stacking.tower.size(); ++pair_index)
        for (size_t i{}; i < stacking.active.size(); ++i) {
            auto& rectangle{stacking.tower[pair_index][i]};
            rectangle.position = GetTowerPosition(pair_index * stacking.active.size() + i);
            rectangle.scale.y = kRectangleHalfHeight * stacking.tower_scale;
        }
}

void Tick(const float delta_time) {
    float remaining_time{delta_time};
    if (stacking.phase == Phase::Transferring) {
        stacking.transfer_elapsed += delta_time;
        const float t{std::clamp(stacking.transfer_elapsed / kTransferDuration, 0.0f, 1.0f)};
        for (size_t i{}; i < stacking.active.size(); ++i) {
            const vec2 destination{GetTowerPosition(stacking.tower.size() * stacking.active.size() + i)};
            const vec2 scale{kRectangleHalfWidth, kRectangleHalfHeight * stacking.tower_scale};
            stacking.active[i].position = glm::mix(stacking.transfer_start[i].position, destination, t);
            stacking.active[i].scale = glm::mix(stacking.transfer_start[i].scale, scale, t);
        }
        if (t < 1.0f)
            return;

        remaining_time = stacking.transfer_elapsed - kTransferDuration;
        stacking.tower.push_back(stacking.active);
        SpawnPair();
    }

    for (size_t i{}; i < stacking.active.size(); ++i) {
        float& y{stacking.active[i].position.y};
        const float bottom{kLaneBottom + stacking.active[i].scale.y};
        const float top{kLaneTop - stacking.active[i].scale.y};
        y += stacking.velocity[i] * remaining_time;
        while (y > top || y < bottom) {
            y = y > top ? 2.0f * top - y : 2.0f * bottom - y;
            stacking.velocity[i] = -stacking.velocity[i];
        }
    }
}
