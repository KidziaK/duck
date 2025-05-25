#version 450 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos_World;
in vec3 Normal_World;
in vec3 Tangent_World;
in vec3 Bitangent_World;

uniform sampler2D texture_diffuse1;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Anisotropic parameters
uniform float shininess = 32.0;
uniform float anisotropyStrength = 1.0;
uniform float anisotropicShininessFactor = 5.0;

void main()
{
    vec3 N = normalize(Normal_World);
    vec3 T = normalize(Tangent_World);
    vec3 B = normalize(Bitangent_World);


    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;

    // Ambient
    float ambientStrength = 0.25f;
    vec3 ambient = ambientStrength * lightColor * texColor;

    // Diffuse
    vec3 L = normalize(lightPos - FragPos_World);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor * texColor;

    // Anisotropic Specular (Blinn-Phong based)
    vec3 V = normalize(viewPos - FragPos_World);
    vec3 H = normalize(L + V);

    float dotNH = max(0.0, dot(N, H));
    float dotTH = dot(T, H);
    float anisoTerm = (1.0 - anisotropyStrength) + anisotropyStrength * (1.0 - dotTH * dotTH);

    float finalShininess = shininess;
    if (anisotropyStrength > 0.0) {
        finalShininess = shininess * mix(anisotropicShininessFactor, 1.0, abs(dotTH));
    }

    float spec = pow(dotNH, finalShininess);
    vec3 specular = anisotropyStrength > 0.0 ? spec * lightColor : pow(dotNH, shininess) * lightColor;
    if (anisotropyStrength <= 0.0) specular = pow(dotNH, shininess) * lightColor;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
