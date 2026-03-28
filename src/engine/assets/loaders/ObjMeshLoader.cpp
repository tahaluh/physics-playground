#include "engine/assets/loaders/ObjMeshLoader.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
int parseFaceIndex(const std::string &token)
{
    const size_t slashPosition = token.find('/');
    const std::string indexText = slashPosition == std::string::npos ? token : token.substr(0, slashPosition);
    return std::stoi(indexText) - 1;
}
}

bool ObjMeshLoader::loadFromFile(const std::string &path, Mesh3D &mesh)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    Mesh3D loadedMesh;
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream stream(line);
        std::string type;
        stream >> type;

        if (type == "v")
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            stream >> x >> y >> z;
            loadedMesh.vertices.emplace_back(x, y, z);
        }
        else if (type == "f")
        {
            std::vector<int> indices;
            std::string token;
            while (stream >> token)
            {
                indices.push_back(parseFaceIndex(token));
            }

            if (indices.size() < 3)
            {
                continue;
            }

            for (size_t i = 1; i + 1 < indices.size(); ++i)
            {
                loadedMesh.triangles.push_back({{{indices[0], indices[i], indices[i + 1]}}, 0xFFBDE0FE});
            }
        }
    }

    for (const auto &triangle : loadedMesh.triangles)
    {
        loadedMesh.edges.push_back({triangle.indices[0], triangle.indices[1]});
        loadedMesh.edges.push_back({triangle.indices[1], triangle.indices[2]});
        loadedMesh.edges.push_back({triangle.indices[2], triangle.indices[0]});
    }

    mesh = loadedMesh;
    return !mesh.vertices.empty() && !mesh.triangles.empty();
}
