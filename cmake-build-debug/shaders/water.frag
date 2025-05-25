#version 450 core

in VS_OUT {
    vec3 worldPos_no_displacement;
    vec3 worldPos; // Interpolated world position (with vertex displacement)
    vec2 texCoord;
    vec4 fragPosClipSpace;
} fs_in;

out vec4 FragColor;

uniform sampler2D heightMap;         // For fine-grained height adjustment
uniform sampler2D normalMap;         // Normals for lighting and reflection/refraction
uniform sampler2D sceneColorTexture; // Color buffer of the scene (without water)
uniform sampler2D sceneDepthTexture; // Depth buffer of the scene (without water)
uniform samplerCube skyboxCube;      // Skybox for rays that miss scene objects

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 inverseViewMatrix;
uniform mat4 inverseProjectionMatrix;

uniform vec3 cameraPosition_world;
uniform vec3 lightPos_world; // For basic water surface lighting (optional)

uniform vec2 viewportSize; // SCR_WIDTH, SCR_HEIGHT

uniform float indexOfRefractionAir;   // e.g., 1.0
uniform float indexOfRefractionWater; // e.g., 1.33
uniform float waterOpacity;           // e.g., 0.8 - 1.0
uniform float heightScale;            // Should match vertex shader if used for refinement

// SSR Parameters
uniform int reflectionMaxRaySteps;       // e.g., 32-64
uniform float reflectionRayStepSize;     // e.g., 0.05 - 0.2 in screen space UVs, or world units
uniform float reflectionThickness;       // e.g., 0.1-0.5 world units for depth comparison
uniform int refractionMaxRaySteps;       // e.g., 16-32
uniform float refractionRayStepSize;
uniform float refractionThickness;
uniform float refractionMaxDistance;     // Max world distance for refraction ray
uniform float distortionStrength;        // For simple normal-based distortion of refraction

// Function to reconstruct view-space position from depth
vec3 reconstructViewPos(vec2 uv, float depthSample) {
    vec4 clipSpacePos = vec4(uv * 2.0 - 1.0, depthSample * 2.0 - 1.0, 1.0);
    vec4 viewSpacePos = inverseProjectionMatrix * clipSpacePos;
    return viewSpacePos.xyz / viewSpacePos.w;
}

// Function to reconstruct world-space position from depth
vec3 reconstructWorldPos(vec2 uv, float depthSample) {
    vec4 viewSpacePos = vec4(reconstructViewPos(uv, depthSample), 1.0);
    vec4 worldSpacePos = inverseViewMatrix * viewSpacePos;
    return worldSpacePos.xyz;
}

// Fresnel calculation (Schlick's approximation)
float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Screen Space Raymarching function
// Returns color, alpha component of vec4 indicates hit (1.0) or miss (0.0)
vec4 traceScreenSpaceRay(
vec3 rayOrigin_world,
vec3 rayDir_world,
int maxSteps,
float stepSizeFactor, // Multiplier for screen-space step based on world step projection
float depthThickness,
float maxTraceDistanceWorld,
bool isReflection // To handle skybox sampling on miss for reflections
) {
    float currentRayDistance = 0.0;

    for (int i = 0; i < maxSteps; ++i) {
        // Current point on the ray in world space
        vec3 P_world = rayOrigin_world + rayDir_world * currentRayDistance;

        // Project current ray point to screen space
        vec4 P_clip = projectionMatrix * viewMatrix * vec4(P_world, 1.0);
        if (P_clip.w <= 0.001) break; // Behind or too close to near plane

        vec2 P_uv = (P_clip.xy / P_clip.w) * 0.5 + 0.5; // To [0,1] screen UV

        // Check if ray is outside screen bounds
        if (P_uv.x < 0.0 || P_uv.x > 1.0 || P_uv.y < 0.0 || P_uv.y > 1.0) {
            if (isReflection) return vec4(texture(skyboxCube, rayDir_world).rgb, 1.0); // Hit screen edge, sample skybox
            else return vec4(0.0); // Miss for refraction
        }

        // Get depth of the scene at this screen UV
        float sceneDepthSample = texture(sceneDepthTexture, P_uv).r;

        // If scene depth is at far plane (sky), skip or handle for reflection
        if (sceneDepthSample >= 0.99999) {
            if (isReflection) {
                // Continue stepping, maybe something is in front of skybox
                // Or, if this is the "closest" we get to a non-sky hit, sample skybox later
            } else {
                // For refraction, hitting sky means we see through water to sky (unlikely if objects are present)
                // Or it means we hit nothing within the scene.
            }
        }

        // Reconstruct view space Z for scene and ray point
        // This is more robust than world positions for depth comparison usually
        float scene_Z_view = reconstructViewPos(P_uv, sceneDepthSample).z;
        float ray_P_Z_view = (viewMatrix * vec4(P_world, 1.0)).z;

        // Intersection test: Compare depth of ray point with scene depth
        // In OpenGL view space, Z is negative. Larger Z (less negative) is closer.
        // A hit occurs if the ray point is at or "behind" the opaque surface.
        // ray_P_Z_view >= scene_Z_view - depthThickness
        if (ray_P_Z_view > scene_Z_view - depthThickness && sceneDepthSample < 0.9999) { // sceneDepthSample check avoids matching with far plane
            return vec4(texture(sceneColorTexture, P_uv).rgb, 1.0); // Hit
        }

        // Advance ray
        // Estimate a reasonable world step. Projecting a fixed world step to screen space can give variable screen steps.
        // This stepSizeFactor could be dynamic or a fixed small world unit.
        // Let's use a world step size that is a fraction of maxTraceDistanceWorld
        float worldStep = maxTraceDistanceWorld / float(maxSteps);
        currentRayDistance += worldStep; // stepSizeFactor; // Adjust stepSizeFactor based on desired detail/performance

        if (currentRayDistance > maxTraceDistanceWorld) break;
    }

    if (isReflection) return vec4(texture(skyboxCube, rayDir_world).rgb, 1.0); // Ray missed, sample skybox
    return vec4(0.0); // Miss for refraction (could be a default deep water color)
}


void main() {
    // Screen UV from gl_FragCoord
    vec2 screenUV = gl_FragCoord.xy / viewportSize;

    // Fine-tune world position with heightmap (if vertex displacement wasn't enough or for per-pixel detail)
    // This assumes fs_in.worldPos is the position on the base grid + vertex displacement.
    // For consistency, ensure height sampling is similar to vertex shader or only do it here.
    // Let's use fs_in.worldPos as the starting point (already displaced by vertex shader).
    // If more precision is needed, one could use fs_in.worldPos_no_displacement + normal * height_from_map_here.
    vec3 surfaceWorldPos = fs_in.worldPos;

    // Sample normal map (assuming it's in world space, or TBN is setup)
    // For simplicity, assume normalMap provides world-space normals.
    // If it's tangent space, you'd need TBN matrix (from vertex shader or calculated here).
    vec3 N_surface = normalize(texture(normalMap, fs_in.texCoord).rgb * 2.0 - 1.0);
    // If normal map is already [-1, 1] range, then just normalize(texture(normalMap, fs_in.texCoord).rgb);
    // The WaterSimulator likely provides normals ready for use.
    // Let's assume it's already a proper world normal:
    N_surface = normalize(texture(normalMap, fs_in.texCoord).rgb);


    vec3 V = normalize(cameraPosition_world - surfaceWorldPos); // View vector

    // --- Reflection ---
    vec3 R_world = reflect(-V, N_surface);
    vec4 reflectionSample = traceScreenSpaceRay(surfaceWorldPos + N_surface * 0.01, R_world, reflectionMaxRaySteps, reflectionRayStepSize, reflectionThickness, 50.0, true); // Offset origin slightly
    vec3 reflectionColor = reflectionSample.rgb;
    if (reflectionSample.a == 0.0) { // Should be handled by skybox sampling inside traceScreenSpaceRay
        reflectionColor = texture(skyboxCube, R_world).rgb;
    }

    // --- Refraction ---
    float eta = indexOfRefractionAir / indexOfRefractionWater;
    vec3 T_world_incident = refract(-V, N_surface, eta); // Ray from air into water
    vec3 refractionColor = vec3(0.0, 0.1, 0.2); // Default deep water / miss color

    // Simple distortion for refraction based on normals (applied to screenUV for sceneColorTexture lookup)
    vec2 distortion = N_surface.xz * distortionStrength; // Use XZ components of normal for distortion
    vec2 distortedScreenUV = screenUV + distortion;
    distortedScreenUV = clamp(distortedScreenUV, 0.0, 1.0);

    if (dot(T_world_incident, T_world_incident) > 0.001) { // Check for total internal reflection (refract returns 0 vector)
        // Trace ray for refraction
        // The ray starts at the water surface and goes "into" the scene.
        vec4 refractionSample = traceScreenSpaceRay(surfaceWorldPos - N_surface * 0.01, T_world_incident, refractionMaxRaySteps, refractionRayStepSize, refractionThickness, refractionMaxDistance, false); // Offset origin slightly

        if (refractionSample.a > 0.0) { // Hit something
            refractionColor = refractionSample.rgb;

            // Optional: Apply absorption based on distance
            // float hitDistance = distance(surfaceWorldPos, P_world_of_hit); // P_world_of_hit would need to be returned by traceRay
            // vec3 absorptionColor = vec3(0.7, 0.85, 0.9); // How much light of each color remains after 1 unit
            // refractionColor *= pow(absorptionColor, hitDistance);
        } else {
            // If SSR for refraction misses, we could sample sceneColorTexture at distorted UVs without raymarching,
            // as if looking directly through the water with some distortion.
            // This is a fallback if SSR fails to find anything.
            float sceneDepthAtDistortedUV = texture(sceneDepthTexture, distortedScreenUV).r;
            if(sceneDepthAtDistortedUV < 0.9999){ // Check if not sky
                refractionColor = texture(sceneColorTexture, distortedScreenUV).rgb;
            } else {
                // If distorted UV points to sky, or SSR missed, use a default deep water color or skybox sample from below.
                // For simplicity, let's stick to the default miss color or a skybox sample if appropriate.
                // Refracting sky is also possible:
                // refractionColor = texture(skyboxCube, T_world_incident).rgb;
            }
        }
    } else { // Total internal reflection, reflection is dominant
        // Fresnel will handle this, but we can ensure refractionColor is not used or is black.
        refractionColor = vec3(0.0);
    }

    // --- Fresnel ---
    float F0_water = pow((indexOfRefractionAir - indexOfRefractionWater) / (indexOfRefractionAir + indexOfRefractionWater), 2.0);
    float fresnelFactor = fresnelSchlick(max(dot(V, N_surface), 0.0), F0_water);

    // --- Combine ---
    vec3 finalColor = mix(refractionColor, reflectionColor, fresnelFactor);

    // Optional: Add some basic lighting to the water surface itself
    vec3 lightDir_world = normalize(lightPos_world - surfaceWorldPos);
    float diffuseLight = max(dot(N_surface, lightDir_world), 0.0);
    vec3 H = normalize(lightDir_world + V); // Blinn-Phong half-vector
    float specularLight = pow(max(dot(N_surface, H), 0.0), 64.0); // Shininess = 64

    vec3 waterSurfaceColor = vec3(0.1, 0.2, 0.3); // Base color of water body
    vec3 ambient = vec3(0.1) * waterSurfaceColor;
    vec3 diffuse = waterSurfaceColor * diffuseLight * 0.5;
    vec3 specular = vec3(0.3) * specularLight; // White specular highlights

    // Blend lighting with SSR result. Could add lighting to reflection/refraction components separately.
    // For now, add it as an overlay or to the non-Fresnel part.
    // finalColor = finalColor * (ambient + diffuse) + specular; // Modulate SSR by diffuse, add specular
    // Or simply add some specular highlights on top:
    finalColor += specular * 0.5; // Add some specular highlights

    FragColor = vec4(finalColor, waterOpacity);
}
