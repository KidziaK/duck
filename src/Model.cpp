#include "Model.h"

Model::Model(const std::string& path) : indexCount(0) {
    loadModel(path);
    if (!vertices.empty() && !indices.empty()) {
        setupMesh();
    }
}

Model::~Model() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Model::loadModel(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR::MODEL::COULD_NOT_OPEN_FILE: " << path << std::endl;
        return;
    }

    std::string line;
    int numVertices;
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        ss >> numVertices;
    } else {
        return;
    }

    vertices.resize(numVertices);
    for (int i = 0; i < numVertices; ++i) {
        if (std::getline(file, line)) {
            std::stringstream ss(line);
            vertices[i].Tangent = glm::vec3(0.0f);
            ss >> vertices[i].Position.x >> vertices[i].Position.y >> vertices[i].Position.z;
            ss >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
            ss >> vertices[i].TexCoords.x >> vertices[i].TexCoords.y;
        } else {
            return;
        }
    }

    int numTriangles;
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        ss >> numTriangles;
    } else {
        return;
    }

    indices.reserve(numTriangles * 3);
    indexCount = 0;
    for (int i = 0; i < numTriangles; ++i) {
        if (std::getline(file, line)) {
            std::stringstream ss(line);
            unsigned int v1, v2, v3;
            ss >> v1 >> v2 >> v3;
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);
        } else {
            return;
        }
    }
    indexCount = indices.size();
    file.close();

    calculateTangents();

    std::cout << "Model loaded: " << path << " (Vertices: " << vertices.size() << ", Indices: " << indices.size() << ")" << std::endl;
}


void Model::calculateTangents() {
    if (vertices.empty() || indices.empty()) return;

    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].Tangent = glm::vec3(0.0f);
    }

    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i+1]];
        Vertex& v2 = vertices[indices[i+2]];

        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;

        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        if (std::isinf(f) || std::isnan(f)) {
            f = 0.0f;
        }

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        v0.Tangent += tangent;
        v1.Tangent += tangent;
        v2.Tangent += tangent;
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        const glm::vec3& n = vertices[i].Normal;
        glm::vec3& t = vertices[i].Tangent;

        if (glm::length(t) > 1e-6f) {
            t = glm::normalize(t - glm::dot(t, n) * n);
        } else {
            glm::vec3 c1 = glm::cross(n, glm::vec3(0.0, 0.0, 1.0));
            glm::vec3 c2 = glm::cross(n, glm::vec3(0.0, 1.0, 0.0));
            if (glm::length(c1) > glm::length(c2)) {
                t = glm::normalize(c1);
            } else {
                t = glm::normalize(c2);
            }
            if (glm::length(t) < 1e-6f) {
                 t = glm::vec3(1.0, 0.0, 0.0);
            }
        }
    }
}


void Model::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

    // Vertex Normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // Vertex Texture Coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    // Vertex Tangent (New for Phase 4)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    glBindVertexArray(0);
}

void Model::Draw() {
    if (indexCount == 0) return;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
