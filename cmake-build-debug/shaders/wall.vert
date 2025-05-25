#version 450 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 v_WorldPos;

void main()
{
    v_WorldPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(v_WorldPos, 1.0);
}
