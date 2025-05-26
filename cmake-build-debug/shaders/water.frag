#version 450 core

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;
in vec4 ClipSpacePos;
in vec3 ViewPos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 invView;
uniform mat4 invProjection;
uniform vec3 viewPos_world;
uniform vec2 uScreenSize;

uniform sampler2D uSceneColor;
uniform sampler2D uSceneDepth;

uniform float uRefractionStrength;
uniform float uReflectionStrength;
uniform float uFresnelPower;
uniform float uWaterIOR;
uniform float uWaterTurbidity;
uniform float uWaterLevel;

uniform int uMaxSteps;
uniform float uStepSize;
uniform float uMaxDistance;
uniform float uThickness;
uniform float uRayBias;

out vec4 FragColor;

vec3 worldToView(vec3 worldPos) {
    return (view * vec4(worldPos, 1.0)).xyz;
}

vec3 viewToWorld(vec3 viewPos) {
    return (invView * vec4(viewPos, 1.0)).xyz;
}

vec4 viewToClip(vec3 viewPos) {
    return projection * vec4(viewPos, 1.0);
}

vec3 clipToView(vec4 clipPos) {
    vec4 viewPos = invProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

vec2 clipToScreen(vec4 clipPos) {
    vec3 ndc = clipPos.xyz / clipPos.w;
    return ndc.xy * 0.5 + 0.5;
}

float getViewDepth(vec2 screenUV) {
    float depth = texture(uSceneDepth, screenUV).r;
    vec4 clipPos = vec4(screenUV * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec3 viewPos = clipToView(clipPos);
    return viewPos.z;
}

bool isUnderwater() {
    return viewPos_world.y < uWaterLevel;
}

void getEffectiveParameters(out vec3 effectiveNormal, out float effectiveIOR) {
    vec3 viewDir = normalize(FragPos - viewPos_world);
    vec3 rawNormal = normalize(Normal);

    if (isUnderwater()) {
        if (dot(viewDir, rawNormal) > 0.0) {
            effectiveNormal = -rawNormal;
            effectiveIOR = uWaterIOR;
        } else {
            effectiveNormal = rawNormal;
            effectiveIOR = uWaterIOR;
        }
    } else {
        effectiveNormal = rawNormal;
        effectiveIOR = uWaterIOR;
    }
}

vec3 performSSR(vec3 worldViewDir, vec3 worldNormal) {
    vec3 viewDir = normalize(mat3(view) * worldViewDir);
    vec3 normal = normalize(mat3(view) * worldNormal);

    vec3 reflectionDir = reflect(viewDir, normal);

    vec3 startPos = ViewPos + normal * uRayBias;

    vec3 currentPos = startPos;
    float stepSize = uStepSize;

    for (int i = 0; i < uMaxSteps; i++) {
        currentPos += reflectionDir * stepSize;

        vec4 clipPos = viewToClip(currentPos);

        if (clipPos.w <= 0.0) break;

        vec2 screenUV = clipToScreen(clipPos);

        if (screenUV.x < 0.0 || screenUV.x > 1.0 ||
        screenUV.y < 0.0 || screenUV.y > 1.0) {
            break;
        }

        float sceneDepth = getViewDepth(screenUV);
        float rayDepth = currentPos.z;

        if (rayDepth <= sceneDepth && rayDepth >= sceneDepth - uThickness) {
            return texture(uSceneColor, screenUV).rgb;
        }

        if (length(currentPos - startPos) > uMaxDistance) {
            break;
        }

        stepSize *= 1.02;
    }

    return vec3(0.0);
}

vec3 performSSRefraction(vec3 worldViewDir, vec3 worldNormal, float effectiveIOR) {
    float iorRatio;
    if (isUnderwater()) {
        iorRatio = effectiveIOR;
    } else {
        iorRatio = 1.0 / effectiveIOR;
    }

    vec3 refractionDir = refract(normalize(worldViewDir), worldNormal, iorRatio);

    if (length(refractionDir) < 0.1) {
        vec2 screenUV = (ClipSpacePos.xy / ClipSpacePos.w) * 0.5 + 0.5;
        vec2 distortion = worldNormal.xz * uRefractionStrength;
        vec2 distortedUV = clamp(screenUV + distortion, 0.0, 1.0);
        return texture(uSceneColor, distortedUV).rgb;
    }

    vec3 viewRefractionDir = normalize(mat3(view) * refractionDir);
    vec3 viewNormal = normalize(mat3(view) * worldNormal);

    vec3 startPos;
    if (isUnderwater()) {
        startPos = ViewPos + viewNormal * uRayBias;
    } else {
        startPos = ViewPos - viewNormal * uRayBias;
    }

    vec3 currentPos = startPos;
    float stepSize = uStepSize * 0.5;

    for (int i = 0; i < uMaxSteps; i++) {
        currentPos += viewRefractionDir * stepSize;

        vec4 clipPos = viewToClip(currentPos);

        if (clipPos.w <= 0.0) break;

        vec2 screenUV = clipToScreen(clipPos);

        if (screenUV.x < 0.0 || screenUV.x > 1.0 ||
        screenUV.y < 0.0 || screenUV.y > 1.0) {
            break;
        }

        float sceneDepth = getViewDepth(screenUV);
        float rayDepth = currentPos.z;

        if (rayDepth <= sceneDepth && rayDepth >= sceneDepth - uThickness * 2.0) {
            vec3 sceneColor = texture(uSceneColor, screenUV).rgb;

            float distance = length(currentPos - startPos);
            float absorption = exp(-distance * uWaterTurbidity);
            return sceneColor * absorption;
        }

        if (length(currentPos - startPos) > uMaxDistance * 0.5) {
            break;
        }

        stepSize *= 1.01;
    }

    vec2 screenUV = (ClipSpacePos.xy / ClipSpacePos.w) * 0.5 + 0.5;
    vec2 distortion = worldNormal.xz * uRefractionStrength;
    vec2 distortedUV = clamp(screenUV + distortion, 0.0, 1.0);
    return texture(uSceneColor, distortedUV).rgb;
}

void main() {
    vec3 effectiveNormal;
    float effectiveIOR;
    getEffectiveParameters(effectiveNormal, effectiveIOR);

    vec3 viewDir = normalize(FragPos - viewPos_world);

    float cosTheta = max(dot(-viewDir, effectiveNormal), 0.0);
    float F0 = pow((1.0 - effectiveIOR) / (1.0 + effectiveIOR), 2.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, uFresnelPower);
    fresnel = clamp(fresnel, 0.0, 1.0);

    vec3 reflectionColor = performSSR(viewDir, effectiveNormal);

    vec3 refractionColor = performSSRefraction(viewDir, effectiveNormal, effectiveIOR);

    vec3 finalColor = mix(refractionColor, reflectionColor, fresnel * uReflectionStrength);

    FragColor = vec4(finalColor, 1.0);
}
