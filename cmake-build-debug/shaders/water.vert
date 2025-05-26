#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D uHeightMap;
uniform float uHeightScale;
uniform float uWaterSurfaceSize;
uniform vec2 uTexelSize;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;
out vec4 ClipSpacePos;
out vec3 ViewPos;

vec3 calculateNormal(vec2 texCoord) {
    // Sample heightmap at current position and neighbors
    float heightL = texture(uHeightMap, texCoord + vec2(-uTexelSize.x, 0.0)).r;
    float heightR = texture(uHeightMap, texCoord + vec2(uTexelSize.x, 0.0)).r;
    float heightD = texture(uHeightMap, texCoord + vec2(0.0, -uTexelSize.y)).r;
    float heightU = texture(uHeightMap, texCoord + vec2(0.0, uTexelSize.y)).r;

    // Calculate gradients
    float dx = (heightR - heightL) * uHeightScale;
    float dz = (heightU - heightD) * uHeightScale;

    // Calculate normal vector
    vec3 normal = normalize(vec3(-dx, 2.0 * uTexelSize.x * uWaterSurfaceSize, -dz));

    return normal;
}

void main() {
    TexCoord = aTexCoord;

    // Sample height from heightmap
    float height = texture(uHeightMap, TexCoord).r * uHeightScale;

    // Apply height displacement
    vec3 displacedPos = aPos + vec3(0.0, height, 0.0);

    // Calculate world position
    FragPos = vec3(model * vec4(displacedPos, 1.0));

    // Calculate normal from heightmap
    Normal = mat3(transpose(inverse(model))) * calculateNormal(TexCoord);

    // Calculate clip space position for screen space calculations
    ClipSpacePos = projection * view * vec4(FragPos, 1.0);

    // Calculate view space position
    ViewPos = vec3(view * vec4(FragPos, 1.0));

    gl_Position = ClipSpacePos;
}