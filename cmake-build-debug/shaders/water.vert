#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out VS_OUT {
    vec3 worldPos_no_displacement; // World position of the base grid vertex
    vec3 worldPos;                 // World position after vertex displacement
    vec2 texCoord;
    vec4 fragPosClipSpace;         // For easy access to screen UVs via gl_FragCoord
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D heightMap;
uniform float heightScale; // e.g., 0.1 or 0.2, controls wave height

void main() {
    vs_out.texCoord = aTexCoords;
    vs_out.worldPos_no_displacement = vec3(model * vec4(aPos, 1.0));

    // Sample heightmap for vertex displacement (coarse displacement)
    // Fragment shader can do more fine-grained displacement if needed.
    float height = texture(heightMap, aTexCoords).r * heightScale;
    vec3 displacedPos = aPos + vec3(0.0, height, 0.0); // Assuming water plane is Y-up

    vs_out.worldPos = vec3(model * vec4(displacedPos, 1.0));

    vs_out.fragPosClipSpace = projection * view * model * vec4(displacedPos, 1.0);
    gl_Position = vs_out.fragPosClipSpace;
}
