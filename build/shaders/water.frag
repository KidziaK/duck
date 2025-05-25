#version 450 core
out vec4 FragColor;

in vec2 v_TexCoords;       // UVs of the water surface quad
in vec3 v_FragPos_World;   // World position of the water surface fragment

uniform sampler2D normalMap;
uniform samplerCube skyboxCube;     // For distant environment & reflections
uniform sampler2D sceneTexture;   // Texture of the scene rendered WITHOUT water (duck, walls)
uniform vec3 viewPos_world;
uniform mat4 projectionMatrix; // For converting world to screen_uv
uniform mat4 viewMatrix;       // For converting world to screen_uv


uniform float indexOfRefractionAir = 1.0;
uniform float indexOfRefractionWater = 1.33;
uniform float waterOpacity = 0.75f;
uniform float distortionStrength = 0.05; // How much refraction distorts UVs

void main()
{
    vec3 N = normalize(texture(normalMap, v_TexCoords).rgb * 2.0 - 1.0);
    vec3 V = normalize(viewPos_world - v_FragPos_World);

    float eta_ratio = indexOfRefractionAir / indexOfRefractionWater;
    if (dot(N, V) < 0.0) { // Viewing from underwater
         eta_ratio = 1.0 / eta_ratio; // Correctly handled in your provided shader
        // N = -N; // Normal should always point towards the viewer for consistent refract/reflect
    }
    N = faceforward(-N, V, N); // Ensure N points towards V for correct reflect/refract

    vec3 R_reflect = reflect(-V, N);
    vec3 R_refract = refract(-V, N, eta_ratio);

    // Fresnel Factor
    float R0 = pow((indexOfRefractionAir - indexOfRefractionWater) / (indexOfRefractionAir + indexOfRefractionWater), 2.0);
    float cosThetaView = abs(dot(V, N)); // Use abs for robust angle calculation
    float fresnelFactor = R0 + (1.0 - R0) * pow(1.0 - cosThetaView, 5.0);

    vec3 reflectionColor = texture(skyboxCube, R_reflect).rgb;
    vec3 refractionColor;

    if (length(R_refract) < 0.001) { // Total Internal Reflection
        refractionColor = reflectionColor; // Or a specific TIR color
        fresnelFactor = 1.0;
    } else {
        // --- Screen-Space Refraction Part ---
        // Project current fragment position to screen space UVs
        vec4 clipSpacePos = projectionMatrix * viewMatrix * vec4(v_FragPos_World, 1.0);
        vec2 ndcPos = clipSpacePos.xy / clipSpacePos.w;
        vec2 screenUV = ndcPos * 0.5 + 0.5;

        // Use the X and Z components of the world-space refraction vector to create an offset
        // R_refract is a direction. We need to project it onto the view plane for a 2D offset.
        // A simpler way for screen-space is to use the normal's XZ components projected.
        // Or use the deviation of the refracted ray from the view ray in screen space.

        // Simplified distortion: Use the normal's XY components (in screen space) to offset screenUV
        // For more physically based, project R_refract to screen space.
        // For now, a common cheat: use the normal's components that are perpendicular to view depth.
        // Let's use the N (surface normal) to create a 2D offset for distortion.
        // Project N onto the view plane (approximation)
        vec2 ndcNormal = (projectionMatrix * viewMatrix * vec4(N, 0.0)).xy; // Project normal
        vec2 distortionOffset = ndcNormal * distortionStrength;

        vec2 distortedScreenUV = screenUV + distortionOffset;
        distortedScreenUV = clamp(distortedScreenUV, 0.001, 0.999); // Clamp to avoid edge artifacts

        refractionColor = texture(sceneTexture, distortedScreenUV).rgb;

        // Optionally, if distortedScreenUV goes "off-screen" or hits a skybox pixel in sceneTexture,
        // you might want to fall back to skyboxCube sampling with R_refract.
        // This requires knowing if sceneTexture contains sky or actual objects at that UV.
        // For now, this will sample whatever is in sceneTexture (duck, walls).
    }

    vec3 surfaceColor = mix(refractionColor, reflectionColor, fresnelFactor);
    FragColor = vec4(surfaceColor, waterOpacity);
}