#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform sampler2D texture_diffuse1;

void main()
{
    // Fresnel effect calculation
    float fresnelPower = 13.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    float fresnelFactor = pow(1.0 - max(dot(viewDir, normalize(Normal)), 0.0), fresnelPower);

    // Texture color
    vec4 texColor = texture(texture_diffuse1, TexCoords);

    // Apply Fresnel effect to the alpha channel
    vec4 finalColor = vec4(texColor.rgb, texColor.a * fresnelFactor);

    FragColor = finalColor;
}
