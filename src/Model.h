#ifndef MODEL_H
#define MODEL_H

#include <glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
};

class Model {
public:
    Model(const std::string& path);
    ~Model();
    void Draw();

private:
    GLuint VAO, VBO, EBO;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    size_t indexCount;

    void loadModel(const std::string& path);
    void calculateTangents();
    void setupMesh();
};

#endif
