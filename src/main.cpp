#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "WaterSimulator.h"
#include "Model.h"
#include "DuckAnimator.h"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadCubemap(std::vector<std::string> faces);
void generateWaterGrid(std::vector<float>& vertices, std::vector<unsigned int>& indices, int N_vertices_edge, float total_size);
unsigned int loadTexture(const char* path);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;

const int WATER_GRID_N = 256;
const float WATER_SURFACE_SIZE = 4.0f;
const unsigned int HEIGHT_MAP_RESOLUTION = WATER_GRID_N;

Camera camera(glm::vec3(0.0f, 0.5f, 0.0f), 3.0f);

bool leftMouseButtonPressed = false;
bool rightMouseButtonPressed = false;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightPos(3.0f, 4.0f, 3.0f);

unsigned int sceneFBO;
unsigned int sceneColorTextureOutput;
unsigned int sceneDepthTextureOutput;


int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Duck Water Sim - SSR Reflections", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader waterShader("shaders/water.vert", "shaders/water.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
    Shader duckShader("shaders/duck.vert", "shaders/duck.frag");
    Shader wallShader("shaders/wall.vert", "shaders/wall.frag");

    WaterSimulator waterSimulator(WATER_GRID_N, WATER_SURFACE_SIZE);

    std::vector<float> waterVertices;
    std::vector<unsigned int> waterIndices;
    generateWaterGrid(waterVertices, waterIndices, WATER_GRID_N, WATER_SURFACE_SIZE);

    unsigned int waterVAO, waterVBO, waterEBO;
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glGenBuffers(1, &waterEBO);
    glBindVertexArray(waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(float), &waterVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterIndices.size() * sizeof(unsigned int), &waterIndices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::vector<std::string> faces {
        "textures/skybox/posx.jpg", "textures/skybox/negx.jpg",
        "textures/skybox/posy.jpg", "textures/skybox/negy.jpg",
        "textures/skybox/posz.jpg", "textures/skybox/negz.jpg"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    Model duckModel("textures/duck.txt");
    unsigned int duckTexture = loadTexture("textures/ducktex.jpg");
    DuckAnimator duckAnimator(WATER_SURFACE_SIZE);

    float scaledWallVertices[] = {
        -2.0f, -2.0f, -2.0f,  0.0f,  0.0f,  1.0f,  2.0f, -2.0f, -2.0f,  0.0f,  0.0f,  1.0f,  2.0f,  2.0f, -2.0f,  0.0f,  0.0f,  1.0f,
        -2.0f, -2.0f, -2.0f,  0.0f,  0.0f,  1.0f,  2.0f,  2.0f, -2.0f,  0.0f,  0.0f,  1.0f, -2.0f,  2.0f, -2.0f,  0.0f,  0.0f,  1.0f,
        -2.0f, -2.0f,  2.0f,  0.0f,  0.0f,  1.0f,  2.0f,  2.0f,  2.0f,  0.0f,  0.0f,  1.0f,  2.0f, -2.0f,  2.0f,  0.0f,  0.0f,  1.0f,
        -2.0f, -2.0f,  2.0f,  0.0f,  0.0f,  1.0f, -2.0f,  2.0f,  2.0f,  0.0f,  0.0f,  1.0f,  2.0f,  2.0f,  2.0f,  0.0f,  0.0f,  1.0f,
        -2.0f, -2.0f, -2.0f, -1.0f,  0.0f,  0.0f, -2.0f,  2.0f,  2.0f, -1.0f,  0.0f,  0.0f, -2.0f, -2.0f,  2.0f, -1.0f,  0.0f,  0.0f,
        -2.0f, -2.0f, -2.0f, -1.0f,  0.0f,  0.0f, -2.0f,  2.0f, -2.0f, -1.0f,  0.0f,  0.0f, -2.0f,  2.0f,  2.0f, -1.0f,  0.0f,  0.0f,
         2.0f, -2.0f, -2.0f,  1.0f,  0.0f,  0.0f,  2.0f, -2.0f,  2.0f,  1.0f,  0.0f,  0.0f,  2.0f,  2.0f,  2.0f,  1.0f,  0.0f,  0.0f,
         2.0f, -2.0f, -2.0f,  1.0f,  0.0f,  0.0f,  2.0f,  2.0f,  2.0f,  1.0f,  0.0f,  0.0f,  2.0f,  2.0f, -2.0f,  1.0f,  0.0f,  0.0f,
        -2.0f, -2.0f, -2.0f,  0.0f,  1.0f,  0.0f,  2.0f, -2.0f,  2.0f,  0.0f,  1.0f,  0.0f,  2.0f, -2.0f, -2.0f,  0.0f,  1.0f,  0.0f,
        -2.0f, -2.0f, -2.0f,  0.0f,  1.0f,  0.0f, -2.0f, -2.0f,  2.0f,  0.0f,  1.0f,  0.0f,  2.0f, -2.0f,  2.0f,  0.0f,  1.0f,  0.0f,
        -2.0f,  2.0f, -2.0f,  0.0f,  1.0f,  0.0f,  2.0f,  2.0f, -2.0f,  0.0f,  1.0f,  0.0f,  2.0f,  2.0f,  2.0f,  0.0f,  1.0f,  0.0f,
        -2.0f,  2.0f, -2.0f,  0.0f,  1.0f,  0.0f,  2.0f,  2.0f,  2.0f,  0.0f,  1.0f,  0.0f, -2.0f,  2.0f,  2.0f,  0.0f,  1.0f,  0.0f
    };
    unsigned int sceneWallVAO, sceneWallVBO;
    glGenVertexArrays(1, &sceneWallVAO);
    glGenBuffers(1, &sceneWallVBO);
    glBindVertexArray(sceneWallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sceneWallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(scaledWallVertices), scaledWallVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneColorTextureOutput);
    glBindTexture(GL_TEXTURE_2D, sceneColorTextureOutput);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTextureOutput, 0);

    glGenTextures(1, &sceneDepthTextureOutput);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTextureOutput);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTextureOutput, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Scene FBO is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float heightScale = 0.1f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        waterSimulator.createRaindrop();
        waterSimulator.updateSimulation();

        duckAnimator.update(deltaTime);
        glm::vec3 currentDuckSplinePos = duckAnimator.getCurrentPositionXZ();

        glm::mat4 duckTransform = duckAnimator.getDuckTransform(-0.075f, glm::vec3(0.0f, 1.0f, 0.0f));

        float currentSplineSpeed = duckAnimator.getCurrentSplineAdvancementSpeed();
        float actualWakeMagnitude = currentSplineSpeed;
        waterSimulator.createDisturbance(currentDuckSplinePos.x, currentDuckSplinePos.z, actualWakeMagnitude);

        glm::mat4 projection = camera.GetProjectionMatrix((float)SCR_WIDTH / (float)SCR_HEIGHT);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 identityModel = glm::mat4(1.0f);

        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_BACK);

        wallShader.use();
        wallShader.setMat4("model", identityModel);
        wallShader.setMat4("view", view);
        wallShader.setMat4("projection", projection);
        wallShader.setVec3("wallColor", glm::vec3(0.5f, 0.5f, 0.5f));
        wallShader.setVec3("lightPos", lightPos);
        wallShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        wallShader.setVec3("viewPos", camera.Position);
        glBindVertexArray(sceneWallVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        duckShader.use();
        duckShader.setMat4("model", duckTransform);
        duckShader.setMat4("view", view);
        duckShader.setMat4("projection", projection);
        duckShader.setVec3("lightPos", lightPos);
        duckShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.9f));
        duckShader.setVec3("viewPos", camera.Position);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, duckTexture);
        duckShader.setInt("texture_diffuse1", 0);
        duckModel.Draw();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(view)));
        skyboxShader.setMat4("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        skyboxShader.setInt("skybox", 0);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        wallShader.use();
        wallShader.setMat4("model", identityModel);
        wallShader.setMat4("view", view);
        wallShader.setMat4("projection", projection);
        wallShader.setVec3("wallColor", glm::vec3(0.5f, 0.5f, 0.5f));
        wallShader.setVec3("lightPos", lightPos);
        wallShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        wallShader.setVec3("viewPos", camera.Position);
        glBindVertexArray(sceneWallVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDisable(GL_CULL_FACE);

        duckShader.use();
        duckShader.setMat4("model", duckTransform);
        duckShader.setMat4("view", view);
        duckShader.setMat4("projection", projection);
        duckShader.setVec3("lightPos", lightPos);
        duckShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.9f));
        duckShader.setVec3("viewPos", camera.Position);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, duckTexture);
        duckShader.setInt("texture_diffuse1", 0);
        duckModel.Draw();



        waterShader.use();
        waterShader.setMat4("model", identityModel);
        waterShader.setMat4("view", view);
        waterShader.setMat4("projection", projection);
        waterShader.setVec3("viewPos_world", camera.Position);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneColorTextureOutput);
        waterShader.setInt("uSceneColor", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sceneDepthTextureOutput);
        waterShader.setInt("uSceneDepth", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        waterShader.setInt("uSkybox", 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, waterSimulator.getHeightmapTextureID());
        waterShader.setInt("uHeightMap", 3);

        waterShader.setFloat("uHeightScale", heightScale);
        waterShader.setFloat("uWaterSurfaceSize", WATER_SURFACE_SIZE);
        waterShader.setVec2("uTexelSize", 1.0f / (float)waterSimulator.getGridN(), 1.0f / (float)waterSimulator.getGridN());

        glBindVertexArray(waterVAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(waterIndices.size()), GL_UNSIGNED_INT, 0);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteBuffers(1, &waterEBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteTextures(1, &cubemapTexture);
    glDeleteTextures(1, &duckTexture);
    glDeleteVertexArrays(1, &sceneWallVAO);
    glDeleteBuffers(1, &sceneWallVBO);
    glDeleteFramebuffers(1, &sceneFBO);
    glDeleteTextures(1, &sceneColorTextureOutput);
    glDeleteTextures(1, &sceneDepthTextureOutput);

    glfwTerminate();
    return 0;
}

void generateWaterGrid(std::vector<float>& vertices, std::vector<unsigned int>& indices, int N_vertices_edge, float total_size) {
    vertices.reserve(N_vertices_edge * N_vertices_edge * 5);
    indices.reserve(N_vertices_edge * N_vertices_edge * 6);
    int N_segments_edge = N_vertices_edge - 1;
    float segment_size = total_size / static_cast<float>(N_segments_edge);
    float half_size = total_size / 2.0f;
    for (int j = 0; j < N_vertices_edge; ++j) {
        for (int i = 0; i < N_vertices_edge; ++i) {
            float x = -half_size + static_cast<float>(i) * segment_size;
            float y = 0.0f;
            float z = -half_size + static_cast<float>(j) * segment_size;
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
            float u = static_cast<float>(i) / static_cast<float>(N_segments_edge);
            float v = static_cast<float>(j) / static_cast<float>(N_segments_edge);
            vertices.push_back(u); vertices.push_back(v);
        }
    }
    for (int j = 0; j < N_segments_edge; ++j) {
        for (int i = 0; i < N_segments_edge; ++i) {
            int topLeft = j * N_vertices_edge + i;
            int topRight = topLeft + 1;
            int bottomLeft = (j + 1) * N_vertices_edge + i;
            int bottomRight = bottomLeft + 1;
            indices.push_back(topLeft); indices.push_back(bottomRight); indices.push_back(topRight);
            indices.push_back(topLeft); indices.push_back(bottomLeft); indices.push_back(bottomRight);
        }
    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    if (sceneColorTextureOutput) {
        glBindTexture(GL_TEXTURE_2D, sceneColorTextureOutput);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
    if (sceneDepthTextureOutput) {
        glBindTexture(GL_TEXTURE_2D, sceneDepthTextureOutput);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) leftMouseButtonPressed = true;
        else if (action == GLFW_RELEASE) leftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) rightMouseButtonPressed = true;
        else if (action == GLFW_RELEASE) rightMouseButtonPressed = false;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos; lastY = ypos; firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    if (leftMouseButtonPressed) camera.ProcessOrbit(xoffset, yoffset);
    if (rightMouseButtonPressed) camera.ProcessZoom(yoffset * camera.ZoomSensitivityRMB);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessZoom(static_cast<float>(yoffset) * camera.ZoomSensitivityScroll);
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}

unsigned int loadTexture(char const * path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        else { std::cerr << "Texture failed to load (unsupported components): " << path << std::endl; stbi_image_free(data); return 0; }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
    }
    return textureID;
}