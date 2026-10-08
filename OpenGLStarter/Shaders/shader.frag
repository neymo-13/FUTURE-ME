#version 330 core

in vec4 vCol;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 colour;

uniform sampler2D texture_albedo;
uniform sampler2D texture_metallic;
uniform sampler2D texture_roughness;
uniform sampler2D texture_normal;

uniform vec3 lightColour;
uniform vec3 viewPos;

uniform float emissiveStrength;   // 0 = ปกติ, 1 = เรืองแสงเต็มที่

struct DirLight {
    vec3 direction;
    vec3 colour;
    float intensity;
};
uniform DirLight dirLight;

struct PointLight {
    vec3 position;
    vec3 colour;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight pointLights[4];
uniform int numPointLights;

uniform float material_ao;

void main()
{
    // ---------- Sample textures ----------
    vec3  albedo    = texture(texture_albedo,    TexCoord).rgb;
    float metallic  = texture(texture_metallic,  TexCoord).r;
    float roughness = texture(texture_roughness, TexCoord).r;

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    // ---------- Directional light (Lambert) ----------
    vec3 L = normalize(-dirLight.direction);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = albedo * dirLight.colour * dirLight.intensity * diff;

    // ---------- Point lights (Lambert + attenuation) ----------
    vec3 pointSum = vec3(0.0);
    for (int i = 0; i < numPointLights && i < 4; i++)
    {
        vec3  toLight  = pointLights[i].position - FragPos;
        float distance = length(toLight);
        vec3  Lp       = toLight / max(distance, 0.0001);
        float diffP    = max(dot(N, Lp), 0.0);

        float denom = pointLights[i].constant
                    + pointLights[i].linear    * distance
                    + pointLights[i].quadratic * distance * distance;

        float attenuation = 1.0 / max(denom, 0.0001);

        pointSum += albedo * pointLights[i].colour * diffP * attenuation;
    }

    // ---------- Ambient ----------
    vec3 ambient = lightColour * albedo * material_ao;

    // ---------- รวมแสง ----------
    vec3 result = ambient + diffuse + pointSum;

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    // ---------- Emissive (สำหรับกองไฟ) ----------
    // emissiveStrength = 0 → ใช้สีปกติ
    // emissiveStrength = 1 → ใช้สี texture ดิบ (เรืองแสง)
    result = mix(result, albedo, emissiveStrength);

    colour = vec4(result, 1.0);
}