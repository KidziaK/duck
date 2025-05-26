#version 450 core
out vec4 FragColor;

in vec3 vFragPosWorld;
in vec3 vFragPosViewSpace;
in vec4 vFragPosClipSpace;
in vec3 vNormalViewSpace;
in vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 viewPos_world;

uniform sampler2D uSceneColor;
uniform sampler2D uSceneDepth;
uniform samplerCube uSkybox;

const int SSR_MAX_STEPS = 256;
const float SSR_INITIAL_STEP_SIZE = 0.05;
const float SSR_MAX_DIST = 30.0;
const float SSR_NDC_THICKNESS = 0.003;

void main() {
    vec3 N = normalize(vNormalViewSpace);
    vec3 V_eye_to_frag_view = normalize(vFragPosViewSpace);
    vec3 R_view = reflect(V_eye_to_frag_view, N);

    vec3 reflectionSample = vec3(0.0);
    bool hit = false;

    for (int i = 0; i < SSR_MAX_STEPS; ++i) {
        float rayLength = (float(i) + 0.5) * SSR_INITIAL_STEP_SIZE;
        if (rayLength > SSR_MAX_DIST) break;

        vec3 samplePosView = vFragPosViewSpace + R_view * rayLength;
        vec4 samplePosClip = projection * vec4(samplePosView, 1.0);

        if (samplePosClip.w <= 0.0) continue;

        vec2 sampleUV = samplePosClip.xy / samplePosClip.w * 0.5 + 0.5;
        float sampleDeviceDepth = (samplePosClip.z / samplePosClip.w) * 0.5 + 0.5;

        float sceneDeviceDepth = texture(uSceneDepth, sampleUV).r;

        if (sampleDeviceDepth > sceneDeviceDepth && sceneDeviceDepth > 1e-5 && sceneDeviceDepth < 0.9999) {
            float depthDifference = sampleDeviceDepth - sceneDeviceDepth;
            if (depthDifference < SSR_NDC_THICKNESS) {
                reflectionSample = texture(uSceneColor, sampleUV).rgb;
                hit = true;

                float screenEdgeFactor = 1.0 - (pow(sampleUV.x * 2.0 - 1.0, 6.0) + pow(sampleUV.y * 2.0 - 1.0, 6.0));
                screenEdgeFactor = clamp(screenEdgeFactor, 0.0, 1.0);
                reflectionSample *= screenEdgeFactor;

                float distFade = 1.0 - smoothstep(SSR_MAX_DIST * 0.6, SSR_MAX_DIST, rayLength);
                reflectionSample *= distFade;
                break;
            }
        }
    }

    if (!hit) {
        mat4 invView = inverse(view);
        vec3 R_world = normalize(mat3(invView) * R_view);
        reflectionSample = texture(uSkybox, R_world).rgb;
    }

    float ior_w = 2.0;
    float ior_a = 1.0;

    float cos_theta = abs(dot(N, V_eye_to_frag_view));

    float f0 = (ior_a - ior_w) * (ior_a - ior_w) / (ior_a + ior_w) / (ior_a + ior_w);
    float fresnel = f0 + (1 - f0) * pow(1 - cos_theta, 5);
    FragColor = vec4(reflectionSample, fresnel);
}
