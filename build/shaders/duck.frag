#version 450 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos_World;
in vec3 Normal_World;    // World-space normal
in vec3 Tangent_World;   // World-space tangent
in vec3 Bitangent_World; // World-space bitangent

uniform sampler2D texture_diffuse1;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Anisotropic parameters (can be uniforms later)
uniform float shininess = 32.0;          // Base shininess
uniform float anisotropyStrength = 0.7;  // 0 for isotropic, towards 1 for more anisotropic
uniform float anisotropicShininessFactor = 5.0; // How much shininess is affected by anisotropy

void main()
{
    vec3 N = normalize(Normal_World);
    vec3 T = normalize(Tangent_World);
    // Bitangent B can be re-derived: vec3 B = normalize(cross(N, T));
    // Or use the passed in Bitangent_World if confident about its orthogonality and handedness.
    // For simplicity, let's assume passed Bitangent_World is good enough or re-derive.
    vec3 B = normalize(Bitangent_World);


    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;

    // Ambient
    float ambientStrength = 0.25f;
    vec3 ambient = ambientStrength * lightColor * texColor; // Apply texture to ambient

    // Diffuse
    vec3 L = normalize(lightPos - FragPos_World);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor * texColor; // Apply texture to diffuse

    // Anisotropic Specular (Blinn-Phong based)
    vec3 V = normalize(viewPos - FragPos_World);
    vec3 H = normalize(L + V); // Halfway vector

    float dotNH = max(0.0, dot(N, H));

    // Anisotropic term:
    // We want the highlight to be stretched along the bitangent (perpendicular to tangent T)
    // So, when H aligns with T, the highlight should be tighter (higher effective shininess).
    // When H aligns with B, the highlight should be broader (lower effective shininess).
    float dotTH = dot(T, H);
    // float dotBH = dot(B, H); // Can also use dotBH if preferred

    // Shift shininess based on alignment with tangent.
    // (1.0 - abs(dotTH)) is larger when H is perpendicular to T (aligned with B)
    // abs(dotTH) is larger when H is aligned with T
    float anisoTerm = (1.0 - anisotropyStrength) + anisotropyStrength * (1.0 - dotTH * dotTH); // Ranges from (1-anisoStr) to 1
    // Or: (1.0 - anisotropyStrength * (1.0 - dotTH*dotTH) )
    // Let's try: shininess_eff = shininess / (1.0 - anisotropy * (1 - dot(T,H)^2))

    // A common model: make shininess dependent on direction relative to tangent
    // Highlight is stretched along direction s.t. shininess is lower.
    // If T is groove direction, highlight is perpendicular to T.
    // Effective shininess:
    // float effectiveShininess = shininess * (1.0 + anisotropicShininessFactor * (1.0 - abs(dotTH)));
    // This makes it broader (lower shininess) when H is perpendicular to T.

    // Let's use a common formulation for anisotropic Blinn-Phong:
    // Shift the halfway vector H along the tangent direction before dotting with N
    // Or, more directly, modify the specular exponent
    // The "Shifted Tangent" approach:
    // vec3 shiftedH = H - T * dot(T, H); // Project H onto plane defined by N and B
    // shiftedH = normalize(shiftedH + T * dot(T, H) * (1.0 - anisotropyStrength)); // Scale tangent component
    // float spec = pow(max(dot(N, shiftedH), 0.0), shininess);

    // Simpler: Modulate shininess
    // We want highlight stretched perpendicular to T.
    // So, when H is perpendicular to T (i.e., H is more along B), shininess should be lower.
    // When H is aligned with T, shininess should be higher.
    float finalShininess = shininess;
    if (anisotropyStrength > 0.0) {
        // dotTH^2 is 1 when H is along T, 0 when H is along B.
        // We want higher shininess when H is along T.
        // So, shininess_factor = lerp(1.0, anisotropicShininessFactor, dotTH*dotTH)
        // finalShininess = shininess * mix(1.0, anisotropicShininessFactor, abs(dotTH)); // Stretches along T
        // To stretch along B (perpendicular to T):
        finalShininess = shininess * mix(anisotropicShininessFactor, 1.0, abs(dotTH));
    }

    float spec = pow(dotNH, finalShininess);
    vec3 specular = anisotropyStrength > 0.0 ? spec * lightColor : pow(dotNH, shininess) * lightColor; // Use base shininess if no anisotropy
    if (anisotropyStrength <= 0.0) specular = pow(dotNH, shininess) * lightColor;


    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}