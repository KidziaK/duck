#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent; // New tangent attribute

out vec2 TexCoords;
out vec3 FragPos_World;
out vec3 Normal_World;
out vec3 Tangent_World;   // Pass tangent to fragment shader
out vec3 Bitangent_World; // Pass bitangent to fragment shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos_World = vec3(model * vec4(aPos, 1.0));

    // Normal matrix for transforming normals, tangents, bitangents
    // (Handles non-uniform scaling)
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    Normal_World = normalize(normalMatrix * aNormal);
    Tangent_World = normalize(normalMatrix * aTangent);

    // Re-orthogonalize tangent with normal in case of non-uniform scaling issues
    Tangent_World = normalize(Tangent_World - dot(Tangent_World, Normal_World) * Normal_World);

    // Calculate bitangent (must be orthogonal to normal and tangent)
    Bitangent_World = normalize(cross(Normal_World, Tangent_World));
    // Ensure handedness (optional, depends on UVs and tangent calculation)
    // If UVs are flipped, bitangent might need to be flipped:
    // if (dot(cross(Normal_World, Tangent_World), normalMatrix * aBitangent_from_model_if_available) < 0.0)
    //    Bitangent_World *= -1.0;
    // For now, simple cross product is usually sufficient if tangents are good.

    // UV Fix for duck texture (beak on butt)
    TexCoords = vec2(aTexCoords.x, 1.0 - aTexCoords.y);
    // TexCoords = aTexCoords; // If you fixed UVs in model or texture loading

    gl_Position = projection * view * vec4(FragPos_World, 1.0);
}