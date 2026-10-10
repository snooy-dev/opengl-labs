// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// Exercise 10
//
// Drag randomly placed pieces onto three supplied boards and two custom boards.
// Completed boards lock their pieces; incomplete boards allow rearrangement.
//
// Keyboard inputs:
// - r: Reset the puzzle.
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

struct Object {
    string mesh_id;
    vec2 position{};
    vec2 scale{1.0f};
    float rotation{};
    vec3 color{0.25f, 0.4f, 0.6f};
    optional<size_t> slot_index;
};

GLuint shader_program_id;
unordered_map<string, Mesh> meshes;
GLint position_location{}, scale_location{}, rotation_location{}, color_location{};

//=================
constexpr float kPi{numbers::pi_v<float>};
constexpr float kSnapDistance{0.04f};
constexpr float kHalfSide{0.1f};
constexpr float kTriangleTip{2.0f * kHalfSide / numbers::sqrt3_v<float>};
constexpr float kTipOffset{kTriangleTip * 0.7f};
constexpr int kMaxExtraPieces{6};
constexpr vec2 kSourceMin{-0.95f, -0.92f};
constexpr vec2 kSourceMax{-0.05f, 0.92f};

struct Board {
    string_view name;
    vec2 center;
    vector<Object> shapes;
};

struct Slot {
    Object shape;
    size_t board_index;
    bool occupied{};
};

struct DragState {
    size_t piece_index;
    vec2 start_position;
    vec2 offset;
    optional<size_t> start_slot_index;
};

struct PuzzleState {
    vector<Slot> slots;
    vector<Object> pieces;
    optional<DragState> drag;
};

PuzzleState puzzle;
mt19937 random_engine{random_device{}()};

const vector<MeshData> kMeshData{
    {"triangle", GL_TRIANGLE_FAN,
        {{0.0f, kTriangleTip}, {-kHalfSide, -kTriangleTip / 2.0f}, {kHalfSide, -kTriangleTip / 2.0f}}, {0, 1, 2}},
    {"right-triangle", GL_TRIANGLE_FAN,
        {{kHalfSide, kHalfSide}, {-kHalfSide, kHalfSide}, {kHalfSide, -kHalfSide}}, {0, 1, 2}},
    {"quad", GL_TRIANGLE_FAN,
        {{kHalfSide, kHalfSide}, {-kHalfSide, kHalfSide}, {-kHalfSide, -kHalfSide}, {kHalfSide, -kHalfSide}}, {0, 1, 2, 3}}};

const array<Board, 5> kBoards{{{"Squares", {0.29f, 0.60f},
                                   {{"quad", {-0.06f, -0.06f}, vec2{0.6f}}, {"quad", {-0.06f, 0.06f}, vec2{0.6f}},
                                       {"quad", {0.06f, -0.06f}, vec2{0.6f}}, {"quad", {0.06f, 0.06f}, vec2{0.6f}}}},
    {"Pinwheel", {0.29f, 0.0f},
        {{"triangle", {0.0f, kTipOffset}, vec2{0.7f}, kPi},
            {"triangle", {0.0f, -kTipOffset}, vec2{0.7f}},
            {"triangle", {-kTipOffset, 0.0f}, vec2{0.7f}, -kPi / 2.0f},
            {"triangle", {kTipOffset, 0.0f}, vec2{0.7f}, kPi / 2.0f}}},
    {"Rectangle", {0.29f, -0.60f},
        {{"right-triangle", {}, {0.8f, 1.6f}}, {"right-triangle", {}, {0.8f, 1.6f}, kPi}}},
    {"House", {0.73f, 0.60f},
        {{"quad", {0.0f, -0.07f}, vec2{0.8f}},
            {"triangle", {0.0f, 0.01f + kTriangleTip / 2.0f * 0.8f}, vec2{0.8f}}}},
    {"Hourglass", {0.73f, 0.0f},
        {{"triangle", {0.0f, kTipOffset}, vec2{0.7f}, kPi},
            {"triangle", {0.0f, -kTipOffset}, vec2{0.7f}}}}}};
//=================

string FileToBuffer(const char* file);
GLuint MakeShaders(GLenum shader_type, const char* file);
GLuint MakeShaderProgram(GLuint vertex_shader, GLuint fragment_shader);
void ProcessInput(GLFWwindow* window);
void DrawScene();
void Initialize();

//=================
void ResetPuzzle();
vec2 LocalVertex(const Object& object, const vec2& vertex);
pair<vec2, vec2> LocalBounds(const Object& object);
bool Contains(const Object& object, const vec2& point);
bool Matches(const Object& piece, const Object& target);
bool BoardComplete(size_t board_index);
vec2 CursorPosition(GLFWwindow* window);
void BeginDrag(const vec2& point);
optional<size_t> EndDrag(const vec2& point);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int);
//=================

int main() {
    if (!glfwInit()) {
        cerr << "fail to init glfw!" << '\n';
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window{glfwCreateWindow(kWindowWidth, kWindowHeight, "OpenGL Labs 10", nullptr, nullptr)};
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
    if (!shader_program_id) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    for (const auto& mesh : kMeshData) {
        meshes.try_emplace(mesh.id, mesh);
    }

    Initialize();
    glfwSetMouseButtonCallback(window, MouseButtonCallback);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ProcessInput(window);

        int width{}, height{};
        glfwGetFramebufferSize(window, &width, &height);
        const int side{std::min(width, height)};
        glViewport((width - side) / 2, (height - side) / 2, side, side);

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

vec2 LocalVertex(const Object& object, const vec2& vertex) {
    const vec2 position{vertex * object.scale};
    const float c{std::cos(object.rotation)}, s{std::sin(object.rotation)};
    return {c * position.x - s * position.y, s * position.x + c * position.y};
}

pair<vec2, vec2> LocalBounds(const Object& object) {
    vec2 low{numeric_limits<float>::max()}, high{numeric_limits<float>::lowest()};
    const auto& data{*ranges::find(kMeshData, object.mesh_id, &MeshData::id)};
    for (const auto& vertex : data.vertices) {
        const vec2 position{LocalVertex(object, vertex)};
        low = glm::min(low, position);
        high = glm::max(high, position);
    }
    return {low, high};
}

bool Contains(const Object& object, const vec2& point) {
    const auto& data{*ranges::find(kMeshData, object.mesh_id, &MeshData::id)};
    bool positive{}, negative{};
    for (size_t i{}; i < data.vertices.size(); ++i) {
        const vec2 a{LocalVertex(object, data.vertices[i]) + object.position};
        const vec2 b{LocalVertex(object, data.vertices[(i + 1) % data.vertices.size()]) + object.position};
        const float cross{(b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x)};
        positive |= cross > 0.000001f;
        negative |= cross < -0.000001f;
    }
    return !(positive && negative);
}

bool Matches(const Object& piece, const Object& target) {
    if (piece.mesh_id != target.mesh_id)
        return false;
    const auto& data{*ranges::find(kMeshData, piece.mesh_id, &MeshData::id)};
    for (const auto& vertex : data.vertices) {
        const vec2 position{LocalVertex(piece, vertex)};
        if (!ranges::any_of(data.vertices, [&](const vec2& other) {
                return glm::length(position - LocalVertex(target, other)) < 0.001f;
            }))
            return false;
    }
    return true;
}

bool BoardComplete(size_t board_index) {
    return ranges::all_of(puzzle.slots, [board_index](const Slot& slot) {
        return slot.board_index != board_index || slot.occupied;
    });
}

void ResetPuzzle() {
    puzzle = {};
    for (size_t board_index{}; board_index < kBoards.size(); ++board_index)
        for (const auto& shape : kBoards[board_index].shapes) {
            Object target{shape};
            target.position += kBoards[board_index].center;
            puzzle.slots.push_back({target, board_index});
            puzzle.pieces.push_back(target);
        }

    uniform_int_distribution<int> extra_count{0, kMaxExtraPieces};
    uniform_int_distribution<size_t> choice{0, puzzle.slots.size() - 1};
    const int extras{extra_count(random_engine)};
    for (int i{}; i < extras; ++i)
        puzzle.pieces.push_back(puzzle.slots[choice(random_engine)].shape);
    std::shuffle(puzzle.pieces.begin(), puzzle.pieces.end(), random_engine);
    uniform_real_distribution<float> random_color{0.15f, 0.85f};
    for (auto& piece : puzzle.pieces) {
        const auto [low, high]{LocalBounds(piece)};
        uniform_real_distribution<float> x{kSourceMin.x - low.x, kSourceMax.x - high.x};
        uniform_real_distribution<float> y{kSourceMin.y - low.y, kSourceMax.y - high.y};
        piece.position = {x(random_engine), y(random_engine)};
        piece.color = {random_color(random_engine), random_color(random_engine), random_color(random_engine)};
    }
}

void BeginDrag(const vec2& point) {
    if (puzzle.drag)
        return;
    for (size_t i{puzzle.pieces.size()}; i > 0; --i) {
        auto& piece{puzzle.pieces[i - 1]};
        if (piece.slot_index && BoardComplete(puzzle.slots[*piece.slot_index].board_index))
            continue;
        if (!Contains(piece, point))
            continue;
        puzzle.drag = DragState{i - 1, piece.position, piece.position - point, piece.slot_index};
        if (piece.slot_index)
            puzzle.slots[*piece.slot_index].occupied = false;
        piece.slot_index.reset();
        return;
    }
}

optional<size_t> EndDrag(const vec2& point) {
    if (!puzzle.drag)
        return nullopt;
    const DragState drag{*puzzle.drag};
    auto& piece{puzzle.pieces[drag.piece_index]};
    piece.position = point + drag.offset;
    optional<size_t> closest;
    float distance{kSnapDistance};
    for (size_t i{}; i < puzzle.slots.size(); ++i) {
        const auto& slot{puzzle.slots[i]};
        if (slot.occupied || !Matches(piece, slot.shape))
            continue;
        const float d{glm::length(piece.position - slot.shape.position)};
        if (d <= distance) {
            closest = i;
            distance = d;
        }
    }
    optional<size_t> completed_board;
    if (closest) {
        auto& slot{puzzle.slots[*closest]};
        piece.position = slot.shape.position;
        piece.slot_index = closest;
        slot.occupied = true;
        if (BoardComplete(slot.board_index))
            completed_board = slot.board_index;
    } else {
        const auto [low, high]{LocalBounds(piece)};
        if (!glm::all(glm::greaterThanEqual(piece.position + low, kSourceMin)) ||
            !glm::all(glm::lessThanEqual(piece.position + high, kSourceMax))) {
            piece.position = drag.start_position;
            piece.slot_index = drag.start_slot_index;
            if (piece.slot_index)
                puzzle.slots[*piece.slot_index].occupied = true;
        }
    }
    puzzle.drag.reset();
    return completed_board;
}

vec2 CursorPosition(GLFWwindow* window) {
    double x{}, y{};
    int width{}, height{};
    glfwGetCursorPos(window, &x, &y);
    glfwGetWindowSize(window, &width, &height);
    const int side{std::max(1, std::min(width, height))};
    return {static_cast<float>((x - (width - side) / 2.0) / side * 2.0 - 1.0),
        static_cast<float>(1.0 - (y - (height - side) / 2.0) / side * 2.0)};
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;
    const vec2 point{CursorPosition(window)};
    if (action == GLFW_PRESS)
        BeginDrag(point);
    else if (action == GLFW_RELEASE)
        if (const auto completed_board{EndDrag(point)})
            cout << kBoards[*completed_board].name << " complete!\n";
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
    PollKeyPressedOnce(GLFW_KEY_R, []() { ResetPuzzle(); });
    PollKeyPressedOnce(GLFW_KEY_Q, [window]() { glfwSetWindowShouldClose(window, GL_TRUE); });
    if (puzzle.drag) {
        const auto& drag{*puzzle.drag};
        puzzle.pieces[drag.piece_index].position = CursorPosition(window) + drag.offset;
    }
}

void DrawObject(const Object& object, bool outline = false) {
    glUniform2f(position_location, object.position.x, object.position.y);
    glUniform2f(scale_location, object.scale.x, object.scale.y);
    glUniform1f(rotation_location, object.rotation);
    glUniform3f(color_location, object.color.r, object.color.g, object.color.b);
    const Mesh& mesh{meshes.at(object.mesh_id)};
    glBindVertexArray(mesh.vao);
    glDrawElements(outline ? GL_LINE_LOOP : mesh.primitive, mesh.index_count, GL_UNSIGNED_INT, nullptr);
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program_id);
    DrawObject({"quad", {}, {0.015f, 9.5f}, 0.0f, {0.75f, 0.8f, 0.85f}});
    for (size_t i{}; i < kBoards.size(); ++i) {
        const vec3 color{BoardComplete(i) ? vec3{0.1f, 0.65f, 0.25f} : vec3{0.8f, 0.84f, 0.88f}};
        DrawObject({"quad", kBoards[i].center, {1.9f, 2.2f}, 0.0f, color}, true);
    }
    for (const auto& slot : puzzle.slots)
        DrawObject(slot.shape, true);
    for (size_t i{}; i < puzzle.pieces.size(); ++i)
        if (!puzzle.drag || i != puzzle.drag->piece_index)
            DrawObject(puzzle.pieces[i]);
    if (puzzle.drag)
        DrawObject(puzzle.pieces[puzzle.drag->piece_index]);
}

void Initialize() {
    position_location = glGetUniformLocation(shader_program_id, "uPosition");
    scale_location = glGetUniformLocation(shader_program_id, "uScale");
    rotation_location = glGetUniformLocation(shader_program_id, "uRotation");
    color_location = glGetUniformLocation(shader_program_id, "uColor");
    ResetPuzzle();
}
