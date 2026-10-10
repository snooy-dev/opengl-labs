// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 11
//
// Move one cell at a time through a 20x20 board in a horizontal zigzag.
// Swap shapes on collision and stop at the last cell of the zigzag path.
//
// Keyboard inputs:
// - left/right: Change horizontal direction.
// - a/d: Decrease/increase movement speed.
// - r: Reset the board.
// - q: Quit.

import std;

import <GL/glew.h>;
import <GLFW/glfw3.h>;
import <glm/glm.hpp>;

using namespace std;
using namespace glm;

int window_width{1000};
int window_height{1000};

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

struct Object {
    string mesh_id{"quad"};
    int row{};
    int column{};
    float scale{0.7f};
    vec3 color{0.15f, 0.25f, 0.85f};
};

GLuint shader_program_id;
unordered_map<string, Mesh> meshes;
GLint position_location{}, scale_location{}, color_location{};

//=================
constexpr int kBoardSize{20};
constexpr int kObstacleCount{120};
constexpr int kFinalColumn{kBoardSize % 2 == 0 ? 0 : kBoardSize - 1};
constexpr float kCellSize{2.0f / kBoardSize};
constexpr double kInitialMoveSpeed{5.0};
constexpr double kCollisionDuration{0.3};
constexpr array<string_view, 3> kObstacleMeshes{"triangle", "quad", "inverted-triangle"};

enum class Direction {
    Left,
    Right
};

struct BoardState {
    array<optional<Object>, kBoardSize * kBoardSize> obstacles{};
    Object player{};
    Direction direction{Direction::Right};
    bool finished{};
    double move_speed{kInitialMoveSpeed};
    double move_progress{};
    double elapsed_time{};
    double collision_until{};
};

BoardState board;
default_random_engine dre{random_device{}()};

const vector<MeshData> kMeshData = [] {
    vector<MeshData> data{
        {"triangle", GL_TRIANGLE_FAN, {{0.0f, 0.5f}, {-0.5f, -0.5f}, {0.5f, -0.5f}}, {0, 1, 2}},
        {"quad", GL_TRIANGLE_FAN, {{-0.5f, 0.5f}, {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}}, {0, 1, 2, 3}},
        {"inverted-triangle", GL_TRIANGLE_FAN, {{-0.5f, 0.5f}, {0.0f, -0.5f}, {0.5f, 0.5f}}, {0, 1, 2}}};
    MeshData grid{"grid", GL_LINES};
    for (int i{}; i <= kBoardSize; ++i) {
        const float coordinate{-1.0f + i * kCellSize};
        const auto index{static_cast<uint32_t>(grid.vertices.size())};
        grid.vertices.insert(grid.vertices.end(), {{-1.0f, coordinate}, {1.0f, coordinate},
                                                      {coordinate, -1.0f}, {coordinate, 1.0f}});
        grid.indices.insert(grid.indices.end(), {index, index + 1, index + 2, index + 3});
    }
    data.push_back(std::move(grid));
    return data;
}();
//=================

string FileToBuffer(const char* file);
GLuint MakeShaders(GLenum shader_type, const char* file);
GLuint MakeShaderProgram(GLuint vertex_shader, GLuint fragment_shader);
void ProcessInput(GLFWwindow* window);
void DrawScene();
void Initialize();

//=================
void UpdateWindowTitle(GLFWwindow* window);
void ResetBoard();
void MovePlayer();
void Tick(const double delta_time);
//=================

int main() {
    cout << "Window Size (width height, e.g. 1200 800) : ";
    if (!(cin >> window_width >> window_height) || window_width <= 0 || window_height <= 0)
        return 1;

    if (!glfwInit()) {
        cerr << "fail to init glfw!" << '\n';
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window{glfwCreateWindow(window_width, window_height, "OpenGL Labs 11", nullptr, nullptr)};
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

    const auto ResizeViewport = [](GLFWwindow*, int width, int height) {
        const int side{std::min(width, height)};
        glViewport((width - side) / 2, (height - side) / 2, side, side);
    };
    glfwSetFramebufferSizeCallback(window, ResizeViewport);
    int width{}, height{};
    glfwGetFramebufferSize(window, &width, &height);
    ResizeViewport(window, width, height);
    glfwSwapInterval(1);

    GLuint vertex_shader{MakeShaders(GL_VERTEX_SHADER, "vertex.glsl")};
    GLuint fragment_shader{MakeShaders(GL_FRAGMENT_SHADER, "fragment.glsl")};
    shader_program_id = MakeShaderProgram(vertex_shader, fragment_shader);
    if (!shader_program_id) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    for (const auto& mesh : kMeshData) {
        meshes.try_emplace(mesh.id, mesh);
    }

    Initialize();
    UpdateWindowTitle(window);

    double previous{glfwGetTime()};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        double current{glfwGetTime()};
        const bool was_finished{board.finished};
        Tick(current - previous);
        if (board.finished != was_finished) {
            UpdateWindowTitle(window);
            cout << "Reached the final cell of the zigzag path.\n";
        }
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
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint MakeShaderProgram(GLuint vertex_shader, GLuint fragment_shader) {
    if (!vertex_shader || !fragment_shader) {
        if (vertex_shader) glDeleteShader(vertex_shader);
        if (fragment_shader) glDeleteShader(fragment_shader);
        return 0;
    }

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

void UpdateWindowTitle(GLFWwindow* window) {
    const string title{std::format(
        "OpenGL Labs 11 | {} | Speed: {:.0f} cells/s | Left/Right: Direction | A/D: Speed | R: Restart | Q: Quit",
        board.finished ? "Finished!" : "Moving", board.move_speed)};
    glfwSetWindowTitle(window, title.c_str());
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
    PollKeyPressedOnce(GLFW_KEY_R, [window]() {
        ResetBoard();
        UpdateWindowTitle(window);
    });
    PollKeyPressedOnce(GLFW_KEY_LEFT, []() { if (!board.finished) board.direction = Direction::Left; });
    PollKeyPressedOnce(GLFW_KEY_RIGHT, []() { if (!board.finished) board.direction = Direction::Right; });
    PollKeyPressedOnce(GLFW_KEY_A, [window]() {
        if (!board.finished) {
            board.move_speed = std::max(1.0, board.move_speed - 1.0);
            UpdateWindowTitle(window);
        }
    });
    PollKeyPressedOnce(GLFW_KEY_D, [window]() {
        if (!board.finished) {
            board.move_speed = std::min(20.0, board.move_speed + 1.0);
            UpdateWindowTitle(window);
        }
    });
}

void DrawObject(const Object& object, bool outline = false, vec3 outline_color = {}) {
    const float scale{kCellSize * object.scale};
    glUniform2f(scale_location, scale, scale);
    glUniform2f(position_location,
        -1.0f + (object.column + 0.5f) * kCellSize,
        1.0f - (object.row + 0.5f) * kCellSize);
    const vec3 color{outline ? outline_color : object.color};
    glUniform3f(color_location, color.r, color.g, color.b);
    const Mesh& mesh{meshes.at(object.mesh_id)};
    glBindVertexArray(mesh.vao);
    glDrawElements(outline ? GL_LINE_LOOP : mesh.primitive, mesh.index_count, GL_UNSIGNED_INT, nullptr);
}

void DrawScene() {
    glClearColor(0.97f, 0.97f, 0.97f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program_id);
    glUniform2f(scale_location, 1.0f, 1.0f);
    glUniform2f(position_location, 0.0f, 0.0f);
    glUniform3f(color_location, 0.65f, 0.65f, 0.65f);
    const Mesh& grid{meshes.at("grid")};
    glBindVertexArray(grid.vao);
    glDrawElements(grid.primitive, grid.index_count, GL_UNSIGNED_INT, nullptr);

    for (const auto& obstacle : board.obstacles)
        if (obstacle) DrawObject(*obstacle);

    Object visible_player{board.player};
    vec3 border{0.05f, 0.05f, 0.05f};
    if (board.elapsed_time < board.collision_until) {
        const bool blink{static_cast<int>(board.elapsed_time * 20.0) % 2 != 0};
        border = blink ? vec3{1.0f, 0.35f, 0.0f} : vec3{1.0f};
        visible_player.scale = blink ? 1.0f : board.player.scale;
    }
    DrawObject(visible_player);
    DrawObject(visible_player, true, border);
}

void Initialize() {
    position_location = glGetUniformLocation(shader_program_id, "uPosition");
    scale_location = glGetUniformLocation(shader_program_id, "uScale");
    color_location = glGetUniformLocation(shader_program_id, "uColor");
    ResetBoard();
}

void ResetBoard() {
    board = {};
    array<int, kBoardSize * kBoardSize - 1> cells{};
    std::iota(cells.begin(), cells.end(), 1);
    std::shuffle(cells.begin(), cells.end(), dre);
    uniform_real_distribution<float> random_color{0.2f, 0.85f};
    uniform_real_distribution<float> random_scale{0.4f, 0.8f};
    for (int i{}; i < kObstacleCount; ++i) {
        const int cell{cells[i]};
        board.obstacles[cell] = Object{
            string{kObstacleMeshes[i % kObstacleMeshes.size()]}, cell / kBoardSize, cell % kBoardSize,
            random_scale(dre), {random_color(dre), random_color(dre), random_color(dre)}};
    }
}

void MovePlayer() {
    if (board.finished)
        return;
    const int step{board.direction == Direction::Right ? 1 : -1};
    const int next_column{board.player.column + step};
    if (next_column >= 0 && next_column < kBoardSize)
        board.player.column = next_column;
    else {
        if (board.player.row < kBoardSize - 1)
            ++board.player.row;
        else
            board.player.column -= step;
        board.direction = board.direction == Direction::Right ? Direction::Left : Direction::Right;
    }

    auto& obstacle{board.obstacles[board.player.row * kBoardSize + board.player.column]};
    if (obstacle) {
        std::swap(board.player.mesh_id, obstacle->mesh_id);
        // A buffered move happened before the frame's remaining elapsed time.
        board.collision_until = board.elapsed_time - board.move_progress / board.move_speed + kCollisionDuration;
    }
    if (board.player.row == kBoardSize - 1 && board.player.column == kFinalColumn) {
        board.finished = true;
        board.move_progress = 0.0;
    }
}

void Tick(const double delta_time) {
    board.elapsed_time += delta_time;
    if (!board.finished)
        board.move_progress += delta_time * board.move_speed;
    while (!board.finished && board.move_progress >= 1.0) {
        board.move_progress -= 1.0;
        MovePlayer();
    }
}
