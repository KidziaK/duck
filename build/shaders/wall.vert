#version 450 core
layout (location = 0) in vec3 aPos;
// Normals are still sent by main.cpp's VAO setup, but not strictly needed if walls only show cubemap
// layout (location = 1) in vec3 aNormal;

uniform mat4 model; // Should be identity if scaledWallVertices are already at world scale
uniform mat4 view;
uniform mat4 projection;

out vec3 v_WorldPos; // Pass world-space position to frag for cubemap sampling

void main()
{
    v_WorldPos = vec3(model * vec4(aPos, 1.0)); // Use world position of vertex
    gl_Position = projection * view * vec4(v_WorldPos, 1.0);
}