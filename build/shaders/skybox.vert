#version 450 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view; // View matrix without translation

void main()
{
    TexCoords = aPos;
    // Remove translation from view matrix for skybox
    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Ensure depth is always 1.0 (far plane)
}