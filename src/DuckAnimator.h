#ifndef DUCKANIMATOR_H
#define DUCKANIMATOR_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <random>

class DuckAnimator {
public:
    DuckAnimator(float waterSurfaceSize, int numCtrlPoints = 10);

    void update(float app_deltaTime);
    glm::mat4 getDuckTransform(float fixedWaterHeight, const glm::vec3& fixedWaterNormal) const;
    glm::vec3 getCurrentPositionXZ() const;
    glm::vec3 getBSplineTangent(float t_global) const;
    float getDuckScale() const;
    void setSpeedParameters(float baseSpeed, float amplitude, float frequency);
    float getCurrentSplineAdvancementSpeed() const;

    float splineParameter_t;

private:
    std::vector<glm::vec3> controlPoints;

    int numControlPoints;

    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;

    float currentOscillationTime;
    float baseSplineSpeed;
    float speedAmplitude;
    float speedOscillationFrequency;

    float lastCalculatedSplineAdvancementSpeed;

    glm::vec3 getBSplinePoint(float t_global) const;
};

#endif
