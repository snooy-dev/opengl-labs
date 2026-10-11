// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 15, 16
//

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
    vec3 local_rotation;
    vec3 pivot;
};

GLuint shader_program_id;

vec3 camera_pos;
vec3 camera_target;
vec3 camera_up{0.0f, 1.0f, 0.0f};

unordered_map<string, Mesh> meshes;
vector<Object> scene;

//=================
enum class MeshType {
    kCube,
    kPyramid
};

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

MeshType current_mesh_type{MeshType::kCube};

bool enable_depth_test{};
bool enable_spin{};
float global_rotation_y{};
float global_spin_velocity{};
constexpr float global_spin_speed{50.0f};

function<void(const float)> anim_func{};
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

    PollKeyPressedOnce(GLFW_KEY_H, []() {
        ToggleDepthTest();
    });

    PollKeyPressedOnce(GLFW_KEY_P, []() {
        for (size_t i{3}; i < 9; ++i)
            scene[i].is_hidden = current_mesh_type == MeshType::kCube;
        for (size_t i{9}; i < 14; ++i)
            scene[i].is_hidden = current_mesh_type == MeshType::kPyramid;
        current_mesh_type = current_mesh_type == MeshType::kCube ? MeshType::kPyramid : MeshType::kCube;
    });

    PollKeyPressedOnce(GLFW_KEY_Y, []() {
        enable_spin = !enable_spin;
        global_spin_velocity = enable_spin ? global_spin_speed : 0.0f;
    });

    PollKeyPressedOnce(GLFW_KEY_R, []() { Initialize(); });

    static int current_anim{};
    if (current_mesh_type == MeshType::kCube && current_anim != 6) {
        PollKeyPressedOnce(GLFW_KEY_T, []() {
            auto& top = scene[5];
            if (current_anim == 1) {
                current_anim = 0;
                anim_func = {};
            } else {
                current_anim = 1;
                top.pivot = {};
                top.local_rotation = {};
                anim_func = [&top](const float delta_time) {
                    top.local_rotation.y += 50.0f * delta_time;
                };
            }
        });
        PollKeyPressedOnce(GLFW_KEY_F, []() {
            auto& front = scene[7];
            if (current_anim == 2) {
                current_anim = 0;
                anim_func = {};
            } else {
                current_anim = 2;
                front.pivot = {};
                front.local_rotation = {};
                anim_func = [&front](const float delta_time) {
                    static float elapsed_time{};
                    front.pivot.y = 1.0f;
                    front.local_rotation.x += 90.0f * -cos(fmod(elapsed_time, numbers::pi_v<float>)) * delta_time;
                    elapsed_time += delta_time;
                };
            }
        });
        PollKeyPressedOnce(GLFW_KEY_S, []() {
            auto& side = scene[3];
            if (current_anim == 3) {
                current_anim = 0;
                anim_func = {};
            } else {
                current_anim = 3;
                side.pivot = {};
                side.local_rotation = {};
                anim_func = [&side](const float delta_time) {
                    side.local_rotation.z += 50.0f * delta_time;
                };
            }
        });
        PollKeyPressedOnce(GLFW_KEY_B, []() {
            auto& back = scene[8];
            if (current_anim == 4) {
                current_anim = 0;
                anim_func = {};
            } else {
                current_anim = 4;
                anim_func = [&back](const float delta_time) {
                    static float elapsed_time{};
                    back.scale += -cos(fmod(elapsed_time, numbers::pi_v<float>)) * delta_time;
                    elapsed_time += delta_time;
                };
            }
        });

        PollKeyPressedOnce(GLFW_KEY_1, []() {
            const array<Object*, 4> sides{&scene[3], &scene[7], &scene[4], &scene[8]};
            uniform_int_distribution rand_side{0, 3};
            if (current_anim == 5) {
                current_anim = 0;
                anim_func = {};
            } else {
                current_anim = 5;
                const int side_idx{rand_side(dre)};
                auto& side1 = *sides[side_idx];
                auto& side2 = *sides[(side_idx + 1) % 4];
                side1.pivot = {};
                side1.local_rotation.y = {};
                side2.pivot = {};
                side2.local_rotation.y = {};
                anim_func = [&side1, &side2](const float delta_time) {
                    side1.pivot = {-1.0f, 0.0f, 0.0f};
                    side1.local_rotation.y += 50.0f * delta_time;
                    side2.pivot = {1.0f, 0.0f, 0.0f};
                    side2.local_rotation.y += 50.0f * delta_time;
                };
            }
        });
        PollKeyPressedOnce(GLFW_KEY_2, []() {
            const array<Object*, 4> sides{&scene[3], &scene[7], &scene[4], &scene[8]};
            uniform_int_distribution rand_side{0, 3};
            current_anim = 6;
            const int side_idx{rand_side(dre)};
            auto& side1 = *sides[side_idx];
            auto& side2 = *sides[(side_idx + 1) % 4];
            side1.pivot = {};
            side1.local_rotation.y = {};
            side2.pivot = {};
            side2.local_rotation.y = {};
            const vec3 side1_pos{side1.position};
            const vec3 side1_rot{side1.rotation};
            const vec3 side2_pos{side2.position};
            const vec3 side2_rot{side2.rotation};
            static float alpha{};
            alpha = 0.0f;
            anim_func = [=, &side1, &side2](const float delta_time) {
                side1.position = mix(side1_pos, side2_pos, alpha);
                side1.rotation = mix(side1_rot, side2_rot, alpha);
                side2.position = mix(side2_pos, side1_pos, alpha);
                side2.rotation = mix(side2_rot, side1_rot, alpha);
                if (alpha >= 1.0f) {
                    current_anim = 0;
                    swap(side1, side2);
                    anim_func = {};
                    return;
                }
                alpha = std::clamp(alpha + 0.5f * delta_time, 0.0f, 1.0f);
            };
        });

        PollKeyPressedOnce(GLFW_KEY_3, []() {

        });
        PollKeyPressedOnce(GLFW_KEY_4, []() {

        });
    } else if (current_mesh_type == MeshType::kPyramid) {
        PollKeyPressedOnce(GLFW_KEY_5, []() {

        });
        PollKeyPressedOnce(GLFW_KEY_6, []() {

        });
        PollKeyPressedOnce(GLFW_KEY_7, []() {

        });
        PollKeyPressedOnce(GLFW_KEY_8, []() {

        });
    }

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
        if (obj.is_hidden)
            continue;

        auto it = meshes.find(obj.mesh_id);
        if (it == meshes.end())
            continue;

        const Mesh& mesh{it->second};

        const GLint model_location{glGetUniformLocation(shader_program_id, "uModel")};
        mat4 model{1.0f};

        if (obj.mesh_id != "line")
            model = rotate(model, radians(global_rotation_y), vec3{0.0f, 1.0f, 0.0f});

        model = translate(model, obj.position);
        model = rotate(model, radians(obj.rotation.z), vec3{0.0f, 0.0f, 1.0f});
        model = rotate(model, radians(obj.rotation.y), vec3{0.0f, 1.0f, 0.0f});
        model = rotate(model, radians(obj.rotation.x), vec3{1.0f, 0.0f, 0.0f});
        model = translate(model, obj.pivot);
        model = rotate(model, radians(obj.local_rotation.z), vec3{0.0f, 0.0f, 1.0f});
        model = rotate(model, radians(obj.local_rotation.y), vec3{0.0f, 1.0f, 0.0f});
        model = rotate(model, radians(obj.local_rotation.x), vec3{1.0f, 0.0f, 0.0f});
        model = translate(model, -obj.pivot);
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
        {"quad", {1.0f, 0.0f, 0.0f}, {0.0f, 90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {"quad", {-1.0f, 0.0f, 0.0f}, {0.0f, -90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
        {"quad", {0.0f, 1.0f, 0.0f}, {-90.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {"quad", {0.0f, -1.0f, 0.0f}, {90.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
        {"quad", {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {"quad", {0.0f, 0.0f, -1.0f}, {0.0f, 180.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},

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

    enable_depth_test = {};
    current_mesh_type = MeshType::kCube;
    enable_spin = {};
    global_rotation_y = {};
    global_spin_velocity = {};
}

void ToggleDepthTest() {
    enable_depth_test = !enable_depth_test;
    if (enable_depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void Tick(const float delta_time) {
    global_rotation_y += global_spin_velocity * delta_time;

    if (anim_func)
        anim_func(delta_time);
}
