#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

out vec2 TexCoords;
out vec3 FragPos_World;
out vec3 Normal_World;
out vec3 Tangent_World;
out vec3 Bitangent_World;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos_World = vec3(model * vec4(aPos, 1.0));

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    Normal_World = normalize(normalMatrix * aNormal);
    Tangent_World = normalize(normalMatrix * aTangent);
    Tangent_World = normalize(Tangent_World - dot(Tangent_World, Normal_World) * Normal_World);

    Bitangent_World = normalize(cross(Normal_World, Tangent_World));

    TexCoords = vec2(aTexCoords.x, 1.0 - aTexCoords.y);

    gl_Position = projection * view * vec4(FragPos_World, 1.0);
}
