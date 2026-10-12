// Copyright 2026 Yoon-Seong Kim. All rights reserved.

// OpenGL Labs OBJ parser

export module gllab_obj;

import std;

export struct Vertex {
    float x, y, z;
};

export struct ObjMesh {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

std::uint32_t GetIndex(const std::string& str) {
    std::uint32_t result{};
    std::from_chars(str.data(), str.data() + str.size(), result);
    return result;
}

export std::unique_ptr<ObjMesh> LoadObj(const std::filesystem::path& file_path) {
    std::ifstream file{file_path};
    if (!file)
        return nullptr;

    auto mesh = std::make_unique<ObjMesh>();
    std::string line{};
    while (std::getline(file, line)) {
        size_t comment{line.find('#')};
        if (comment != std::string::npos)
            line.erase(comment);

        std::istringstream iss{line};
        std::string type{};
        iss >> type;
        if (type.empty())
            continue;

        if (type == "v") {
            Vertex v{};
            if (!(iss >> v.x >> v.y >> v.z))
                return nullptr;
            mesh->vertices.push_back(std::move(v));
        } else if (type == "f") {
            std::string token{};
            while (iss >> token) {
                size_t first{token.find('/')};
                if (first == std::string::npos)
                    return nullptr;

                size_t second{token.find('/', first + 1)};
                if (second != std::string::npos && token.find('/', second + 1) != std::string::npos)
                    return nullptr;

                std::uint32_t fv = GetIndex(token.substr(0, first)) - 1;
                mesh->indices.push_back(fv);
            }
        }
    }

    return mesh;
}