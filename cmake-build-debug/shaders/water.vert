#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec3 vFragPosWorld;
out vec3 vFragPosViewSpace;
out vec4 vFragPosClipSpace;
out vec3 vNormalViewSpace;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D uHeightMap;
uniform float uHeightScale;
uniform vec2 uTexelSize;
uniform float uWaterSurfaceSize;

void main() {
    float height_raw = texture(uHeightMap, aTexCoords).r;
    float current_height_world = height_raw * uHeightScale;
    vec3 displacedPosModel = vec3(aPos.x, current_height_world, aPos.z);

    float h_L_raw = texture(uHeightMap, aTexCoords - vec2(uTexelSize.x, 0.0)).r;
    float h_R_raw = texture(uHeightMap, aTexCoords + vec2(uTexelSize.x, 0.0)).r;
    float h_D_raw = texture(uHeightMap, aTexCoords - vec2(0.0, uTexelSize.y)).r;
    float h_U_raw = texture(uHeightMap, aTexCoords + vec2(0.0, uTexelSize.y)).r;

    float world_delta_x = uWaterSurfaceSize * uTexelSize.x;
    float world_delta_z = uWaterSurfaceSize * uTexelSize.y;

    float slope_x = (h_R_raw - h_L_raw) * uHeightScale / (2.0 * world_delta_x);
    float slope_z = (h_U_raw - h_D_raw) * uHeightScale / (2.0 * world_delta_z);

    vec3 normalModelSpace = normalize(vec3(-slope_x, 1.0, -slope_z));

    gl_Position = projection * view * model * vec4(displacedPosModel, 1.0);

    vFragPosWorld = vec3(model * vec4(displacedPosModel, 1.0));
    vFragPosClipSpace = gl_Position;
    vFragPosViewSpace = vec3(view * model * vec4(displacedPosModel, 1.0));

    vNormalViewSpace = normalize(mat3(transpose(inverse(view * model))) * normalModelSpace);

    TexCoords = aTexCoords;
}
