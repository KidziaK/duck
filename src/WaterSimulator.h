#ifndef WATERSIMULATOR_H
#define WATERSIMULATOR_H

#include <vector>
#include <glad.h>
#include <glm/glm.hpp>
#include <string>
#include <random>

class WaterSimulator {
public:
    WaterSimulator(int gridN = 256, float physicalSize = 2.0f);
    ~WaterSimulator();

    void updateSimulation();
    void createRaindrop();

    void createDisturbance(float worldX, float worldZ, float magnitude);
    float getHeightAt(float worldX, float worldZ) const;

    GLuint getHeightmapTextureID() const { return heightmapTexture; }
    GLuint getNormalmapTextureID() const { return normalmapTexture; }

    glm::vec3 getNormalAt(float worldX, float worldZ) const;

    int getGridN() const { return N; }

private:
    int N;
    float size;
    float h;
    float dt_sim;
    float C_const;
    float A_const;
    float B_const;

    std::vector<float> currentHeights;
    std::vector<float> previousHeights;
    std::vector<float> dampingFactors;
    std::vector<glm::vec3> normals;
    std::vector<unsigned char> normalmapData;

    GLuint heightmapTexture;
    GLuint normalmapTexture;

    std::mt19937 rng;
    std::uniform_int_distribution<int> distN;
    std::uniform_real_distribution<float> distProb;
    const float raindropProbability = 0.05f;
    const float raindropMagnitude = 1.1f;

    void initializeGrid();
    void initializeDampingFactors();
    void simulateWaterSurface();
    void calculateNormals();
    void setupTextures();
    void updateTextures();

    float& getHeight(std::vector<float>& heights, int r, int c) {
        return heights[r * N + c];
    }

    const float& getHeight(const std::vector<float>& heights, int r, int c) const {
        return heights[r * N + c];
    }

    float& getDamping(int r, int c) {
        return dampingFactors[r * N + c];
    }
};

#endif // WATERSIMULATOR_H
