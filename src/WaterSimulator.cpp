#include "WaterSimulator.h"
#include <iostream>
#include <algorithm>
#include <cmath>

WaterSimulator::WaterSimulator(int gridN, float physicalSize) :
    N(gridN),
    size(physicalSize),
    rng(std::random_device{}()),
    distN(0, gridN),
    distProb(0.0f, 1.0f) {

    h = size / (static_cast<float>(N));
    C_const = 1.0f;
    dt_sim = (1.0f / static_cast<float>(N));

    float dt_sim_sq = dt_sim * dt_sim;
    float h_sq = h * h;
    float C_const_sq = C_const * C_const;

    A_const = (C_const_sq * dt_sim_sq) / h_sq;
    B_const = 2.0f - 4.0f * A_const;

    std::cout << "WaterSim N=" << N << ", size=" << size << ", h=" << h << ", dt_sim=" << dt_sim << std::endl;
    std::cout << "WaterSim A=" << A_const << ", B=" << B_const << std::endl;
    float stability_check = (C_const_sq * dt_sim_sq) / h_sq;
    std::cout << "Stability Check (c^2*dt^2/h^2) = " << stability_check << " (should be <= 0.5)" << std::endl;
    if (stability_check > 0.5f) {
        std::cerr << "WARNING: Simulation might be unstable!" << std::endl;
    }

    currentHeights.resize(N * N, 0.0f);
    previousHeights.resize(N * N, 0.0f);
    dampingFactors.resize(N * N, 1.0f);
    normals.resize(N * N, glm::vec3(0.0f, 1.0f, 0.0f));
    normalmapData.resize(N * N * 4, 0);

    initializeGrid();
    setupTextures();
}

WaterSimulator::~WaterSimulator() {
    glDeleteTextures(1, &heightmapTexture);
    glDeleteTextures(1, &normalmapTexture);
}

void WaterSimulator::initializeGrid() {
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            getHeight(currentHeights, r, c) = 0.0f;
            getHeight(previousHeights, r, c) = 0.0f;
        }
    }
    initializeDampingFactors();
}

void WaterSimulator::initializeDampingFactors() {
    float square_half_size = size / 2.0f;

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            float norm_x = static_cast<float>(c) / (N - 1);
            float norm_y = static_cast<float>(r) / (N - 1);

            float phys_x = -square_half_size + norm_x * size;
            float phys_y_coord = -square_half_size + norm_y * size;

            float dist_to_left_edge   = std::abs(phys_x - (-square_half_size));
            float dist_to_right_edge  = std::abs(phys_x - square_half_size);
            float dist_to_bottom_edge = std::abs(phys_y_coord - (-square_half_size));
            float dist_to_top_edge    = std::abs(phys_y_coord - square_half_size);

            float l = std::min({dist_to_left_edge, dist_to_right_edge, dist_to_bottom_edge, dist_to_top_edge});

            getDamping(r, c) = 0.95f * std::min(1.0f, l / 0.2f);
            if (r == 0 || r == N - 1 || c == 0 || c == N - 1) {
                 getDamping(r,c) = 0.95f;
            }
        }
    }
}

void WaterSimulator::setupTextures() {
    glGenTextures(1, &heightmapTexture);
    glBindTexture(GL_TEXTURE_2D, heightmapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, N, N, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &normalmapTexture);
    glBindTexture(GL_TEXTURE_2D, normalmapTexture);

    std::fill(normalmapData.begin(), normalmapData.end(), 0);
    for(size_t i = 0; i < N * N; ++i) {
        normalmapData[i*4 + 0] = 127;
        normalmapData[i*4 + 1] = 255;
        normalmapData[i*4 + 2] = 127;
        normalmapData[i*4 + 3] = 255;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, normalmapData.data()); // [cite: 9]
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void WaterSimulator::updateTextures() {
    glBindTexture(GL_TEXTURE_2D, heightmapTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N, N, GL_RED, GL_FLOAT, currentHeights.data());

    glBindTexture(GL_TEXTURE_2D, normalmapTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N, N, GL_RGBA, GL_UNSIGNED_BYTE, normalmapData.data());

    glBindTexture(GL_TEXTURE_2D, 0);
}


void WaterSimulator::simulateWaterSurface() {
    std::vector<float> nextHeights = currentHeights;

    for (int r = 1; r < N - 1; ++r) {
        for (int c = 1; c < N - 1; ++c) {
            float sum_neighbors = getHeight(currentHeights, r + 1, c) +
                                  getHeight(currentHeights, r - 1, c) +
                                  getHeight(currentHeights, r, c + 1) +
                                  getHeight(currentHeights, r, c - 1);

            float new_h = A_const * sum_neighbors +
                          B_const * getHeight(currentHeights, r, c) -
                          getHeight(previousHeights, r, c);

            getHeight(nextHeights, r, c) = new_h * getDamping(r,c);
        }
    }

    previousHeights = currentHeights;
    currentHeights = nextHeights;
}

void WaterSimulator::calculateNormals() {
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            float grad_x = (getHeight(currentHeights, r, std::min(N - 1, c + 1)) - getHeight(currentHeights, r, std::max(0, c - 1))) / (2.0f*h);
            float grad_z = (getHeight(currentHeights, std::min(N - 1, r + 1), c) - getHeight(currentHeights, std::max(0, r - 1), c)) / (2.0f*h);

            glm::vec3 normal = glm::normalize(glm::vec3(-grad_x, 1.0f, -grad_z));

            normals[r * N + c] = normal;

            normalmapData[(r * N + c) * 4 + 0] = static_cast<unsigned char>((normal.x * 0.5f + 0.5f) * 255.0f);
            normalmapData[(r * N + c) * 4 + 1] = static_cast<unsigned char>((normal.y * 0.5f + 0.5f) * 255.0f);
            normalmapData[(r * N + c) * 4 + 2] = static_cast<unsigned char>((normal.z * 0.5f + 0.5f) * 255.0f);
            normalmapData[(r * N + c) * 4 + 3] = 255;
        }
    }
}

void WaterSimulator::createRaindrop() {
    if (distProb(rng) < raindropProbability) {
        int r = distN(rng);
        int c = distN(rng);

        r = std::max(1, std::min(N - 2, r));
        c = std::max(1, std::min(N - 2, c));

        getHeight(currentHeights, r, c) += raindropMagnitude;
    }
}

void WaterSimulator::updateSimulation() {
    simulateWaterSurface();
    calculateNormals();
    updateTextures();
}

void WaterSimulator::createDisturbance(float worldX, float worldZ, float magnitude) {
    float normX = (worldX + size / 2.0f) / size;
    float normZ = (worldZ + size / 2.0f) / size;

    int c = static_cast<int>(normX * (N - 1));
    int r = static_cast<int>(normZ * (N - 1));

    r = std::max(1, std::min(N - 2, r));
    c = std::max(1, std::min(N - 2, c));

    if (r >= 0 && r < N && c >= 0 && c < N) {
        getHeight(currentHeights, r, c) += magnitude;
    }
}

float WaterSimulator::getHeightAt(float worldX, float worldZ) const {
    float normX = (worldX + size / 2.0f) / size;
    float normZ = (worldZ + size / 2.0f) / size;

    float c_float = normX * (N - 1);
    float r_float = normZ * (N - 1);

    int r0 = static_cast<int>(floor(r_float));
    int c0 = static_cast<int>(floor(c_float));

    r0 = std::max(0, std::min(N - 2, r0));
    c0 = std::max(0, std::min(N - 2, c0));

    int r1 = r0 + 1;
    int c1 = c0 + 1;

    float tx = c_float - c0;
    float ty = r_float - r0;

    float h00 = getHeight(currentHeights, r0, c0);
    float h10 = getHeight(currentHeights, r0, c1);
    float h01 = getHeight(currentHeights, r1, c0);
    float h11 = getHeight(currentHeights, r1, c1);

    float height = (1 - tx) * (1 - ty) * h00 +
                   tx * (1 - ty) * h10 +
                   (1 - tx) * ty * h01 +
                   tx * ty * h11;
    return height;
}

glm::vec3 WaterSimulator::getNormalAt(float worldX, float worldZ) const {
    float normX = (worldX + size / 2.0f) / size;
    float normZ = (worldZ + size / 2.0f) / size;

    float c_float = normX * (N - 1);
    float r_float = normZ * (N - 1);

    int r0 = static_cast<int>(floor(r_float));
    int c0 = static_cast<int>(floor(c_float));

    r0 = std::max(0, std::min(N - 2, r0));
    c0 = std::max(0, std::min(N - 2, c0));

    int r1 = r0 + 1;
    int c1 = c0 + 1;

    float tx = c_float - c0;
    float ty = r_float - r0;

    const glm::vec3& n00 = normals[r0 * N + c0];
    const glm::vec3& n10 = normals[r0 * N + c1];
    const glm::vec3& n01 = normals[r1 * N + c0];
    const glm::vec3& n11 = normals[r1 * N + c1];

    glm::vec3 interpolatedNormal;
    interpolatedNormal.x = (1 - tx) * (1 - ty) * n00.x +
                           tx * (1 - ty) * n10.x +
                           (1 - tx) * ty * n01.x +
                           tx * ty * n11.x;

    interpolatedNormal.y = (1 - tx) * (1 - ty) * n00.y +
                           tx * (1 - ty) * n10.y +
                           (1 - tx) * ty * n01.y +
                           tx * ty * n11.y;

    interpolatedNormal.z = (1 - tx) * (1 - ty) * n00.z +
                           tx * (1 - ty) * n10.z +
                           (1 - tx) * ty * n01.z +
                           tx * ty * n11.z;

    return glm::normalize(interpolatedNormal);
}
