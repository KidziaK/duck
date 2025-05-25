#include "DuckAnimator.h"
#include <algorithm>
#include <iostream>
#include <cmath>

DuckAnimator::DuckAnimator(float waterSurfaceSize, int numCtrlPoints)
    : splineParameter_t(0.0f),
      numControlPoints(numCtrlPoints),
      rng(std::random_device{}()),
      dist(-waterSurfaceSize / 2.0f, waterSurfaceSize / 2.0f),
      currentOscillationTime(0.0f),
      baseSplineSpeed(0.25f),
      speedAmplitude(0.15f),
      speedOscillationFrequency(1.0f),
      lastCalculatedSplineAdvancementSpeed(0.25f)
{
    controlPoints.reserve(numCtrlPoints);
    for (int i = 0; i < numCtrlPoints; ++i) {
        controlPoints.push_back(glm::vec3(dist(rng), 0.0f, dist(rng)));
    }
}

void DuckAnimator::setSpeedParameters(float baseSpeed, float amplitude, float frequency) {
    baseSplineSpeed = baseSpeed;
    speedAmplitude = amplitude;
    speedOscillationFrequency = frequency;
}

void DuckAnimator::update(float app_deltaTime) {
    int numSegments = numControlPoints;
    if (numSegments < 4) return;

    currentOscillationTime += app_deltaTime;

    float currentSpeed = baseSplineSpeed + speedAmplitude * std::sin(speedOscillationFrequency * currentOscillationTime);
    currentSpeed = std::max(0.05f, currentSpeed);

    lastCalculatedSplineAdvancementSpeed = currentSpeed;

    splineParameter_t += currentSpeed * app_deltaTime;

    if (splineParameter_t >= static_cast<float>(numSegments)) {
        splineParameter_t -= static_cast<float>(numSegments);
    }
     if (splineParameter_t < 0.0f) {
        splineParameter_t += static_cast<float>(numSegments);
    }
}

float DuckAnimator::getCurrentSplineAdvancementSpeed() const {
    return lastCalculatedSplineAdvancementSpeed;
}

glm::mat4 DuckAnimator::getDuckTransform(float fixedWaterHeight, const glm::vec3& fixedWaterNormal) const {
    glm::vec3 positionOnSpline = getBSplinePoint(splineParameter_t);
    glm::vec3 tangent = getBSplineTangent(splineParameter_t);
    glm::vec3 duckPosition = glm::vec3(positionOnSpline.x, fixedWaterHeight + 0.05f, positionOnSpline.z);
    glm::vec3 targetDir = glm::normalize(tangent);
    glm::vec3 upDir = glm::normalize(fixedWaterNormal);
    if (glm::length(targetDir) < 0.001f) targetDir = glm::vec3(1.0f,0.0f,0.0f);
    if (glm::abs(glm::dot(targetDir, upDir)) > 0.99f) upDir = glm::vec3(0.0f,1.0f,0.0f);
    glm::vec3 newRight = glm::normalize(glm::cross(targetDir, upDir));
    glm::vec3 newUp = glm::normalize(glm::cross(newRight, targetDir));
    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    rotationMatrix[0] = glm::vec4(-targetDir, 0.0f);
    rotationMatrix[1] = glm::vec4(newUp, 0.0f);
    rotationMatrix[2] = glm::vec4(-newRight, 0.0f);
    rotationMatrix[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float scale = getDuckScale();
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    return glm::translate(glm::mat4(1.0f), duckPosition) * rotationMatrix * scaleMatrix;
}

glm::vec3 DuckAnimator::getBSplinePoint(float t_global) const {
    if (controlPoints.size() < 4) return glm::vec3(0.0f);
    int numActualPoints = controlPoints.size();
    int i = static_cast<int>(floor(t_global));
    float u = t_global - i;
    glm::vec3 p0 = controlPoints[(i + 0) % numActualPoints];
    glm::vec3 p1 = controlPoints[(i + 1) % numActualPoints];
    glm::vec3 p2 = controlPoints[(i + 2) % numActualPoints];
    glm::vec3 p3 = controlPoints[(i + 3) % numActualPoints];
    float u1 = 1.0f - u;
    float b0 = u1*u1*u1 / 6.0f;
    float b1 = (3*u*u*u - 6*u*u + 4) / 6.0f;
    float b2 = (-3*u*u*u + 3*u*u + 3*u + 1) / 6.0f;
    float b3 = u*u*u / 6.0f;
    return b0*p0 + b1*p1 + b2*p2 + b3*p3;
}

glm::vec3 DuckAnimator::getBSplineTangent(float t_global) const {
    if (controlPoints.size() < 4) return glm::vec3(1.0f, 0.0f, 0.0f);
    int numActualPoints = controlPoints.size();
    int i = static_cast<int>(floor(t_global));
    float u = t_global - i;
    glm::vec3 p0 = controlPoints[(i + 0) % numActualPoints];
    glm::vec3 p1 = controlPoints[(i + 1) % numActualPoints];
    glm::vec3 p2 = controlPoints[(i + 2) % numActualPoints];
    glm::vec3 p3 = controlPoints[(i + 3) % numActualPoints];
    float u1 = 1.0f - u;
    glm::vec3 t = -0.5f * u1 * u1 * p0 +
                   0.5f * (3 * u * u - 4 * u) * p1 +
                   0.5f * (-3 * u * u + 2 * u + 1) * p2 +
                   0.5f * u * u * p3;

    if (glm::length(t) < 1e-6) {
        if (numActualPoints > 0) {
            int current_idx = i % numActualPoints;
            int next_idx = (i + 1) % numActualPoints;
            if (numActualPoints > 1 && next_idx == current_idx) next_idx = (current_idx + 1) % numActualPoints;


            if(next_idx != current_idx) {
                 glm::vec3 next_p = controlPoints[next_idx];
                 glm::vec3 current_p = controlPoints[current_idx];
                 if (glm::length(next_p - current_p) > 1e-6) return glm::normalize(next_p - current_p);
            }
        }
        return glm::vec3(1.0f, 0.0f, 0.0f);
    }
    return glm::normalize(t);
}

glm::vec3 DuckAnimator::getCurrentPositionXZ() const {
    glm::vec3 pos = getBSplinePoint(splineParameter_t);
    return glm::vec3(pos.x, 0.0f, pos.z);
}

float DuckAnimator::getDuckScale() const {
    return 0.003f;
}