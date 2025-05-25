#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D heightMap;

out vec2 v_TexCoords;
out vec3 v_FragPos_World; // Position of the fragment in world space

void main()
{
    v_TexCoords = aTexCoords;

    float displacement = texture(heightMap, v_TexCoords).r;
    vec3 displacedPos = aPos;
    // Apply a scale to the displacement if it's not already in world units
    // This scale factor might need adjustment based on your heightmap values.
    displacedPos.y += displacement * 0.1;

    v_FragPos_World = vec3(model * vec4(displacedPos, 1.0));
    gl_Position = projection * view * vec4(v_FragPos_World, 1.0); // Use world pos for gl_Position
}