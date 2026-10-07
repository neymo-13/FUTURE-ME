#version 330 core

in vec4 vCol;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 colour;

struct Material
{
    float specularStrength;
    float shininess;
};

struct PointLight
{
    vec3 position;
    vec3 colour;
    float constant;
    float linear;
    float quadratic;
};

struct DirLight
{
    vec3 direction;
    vec3 colour;
    float intensity;
};

uniform vec3 lightColour;
uniform vec3 viewPos;

uniform Material material;
uniform float emissiveStrength;   // 0 = normal, 1 = full glow (texture colour as-is)

uniform PointLight pointLights[4];
uniform int numPointLights;
uniform DirLight dirLight;

uniform sampler2D texture2D;

vec3 ambientLight()
{
    float ambientStrength = 0.2f;
    vec3 ambient = lightColour * ambientStrength;

    return ambient;
}

// Person A
vec3 CalcDirLight(DirLight light, vec3 norm, vec3 viewDir)
{
    return vec3(0.0);
}

// Person B
vec3 CalcPointLight(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir)
{
    return vec3(0.0);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 lighting = ambientLight() + CalcDirLight(dirLight, norm, viewDir);
    for (int i = 0; i < numPointLights && i < 4; i++)
        lighting += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    vec4 texColour = texture(texture2D, TexCoord);
    colour = vec4(mix(texColour.rgb * lighting, texColour.rgb, emissiveStrength), texColour.a);
}
