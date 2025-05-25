#version 450 core
out vec4 FragColor;

in vec3 v_WorldPos;

uniform samplerCube environmentMap;

void main()
{
    FragColor = texture(environmentMap, normalize(v_WorldPos));
}
