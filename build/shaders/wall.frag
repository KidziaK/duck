#version 450 core
out vec4 FragColor;

in vec3 v_WorldPos; // Interpolated world-space position of the fragment on the wall

uniform samplerCube environmentMap; // The skybox cubemap

void main()
{
    // The direction vector for cubemap sampling is from the center of the cube (origin)
    // to the fragment's position on the cube surface.
    // v_WorldPos is this position. Normalizing it gives the direction.
    FragColor = texture(environmentMap, normalize(v_WorldPos));
    // FragColor.a = 1.0; // Ensure walls are opaque
}