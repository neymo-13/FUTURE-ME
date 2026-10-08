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
uniform float emissiveStrength;
uniform float time; // เพิ่ม uniform time

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
uniform vec3 tintColor;

void main()
{
    vec3 albedo = texture(texture_albedo, TexCoord).rgb;

    // Tint
    float maxC = max(max(albedo.r, albedo.g), albedo.b);
    float minC = min(min(albedo.r, albedo.g), albedo.b);
    float saturation = maxC - minC;
    float brightness = maxC;

    float notColored = 1.0 - smoothstep(0.05, 0.2, saturation);
    float isBright   = smoothstep(0.6, 0.9, brightness);
    float tintMask   = notColored * isBright;

    albedo = mix(albedo, albedo * tintColor, tintMask);

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    // Directional light
    vec3 L = normalize(-dirLight.direction);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = albedo * dirLight.colour * dirLight.intensity * diff;

    // Point lights
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

    // ---------- ปรับปรุง Ambient & Aurora Reflection ----------
    vec3 ambient = lightColour * albedo * material_ao;
    
    // 1. สีแสงเหนือหลัก (เขียวมรกตอมฟ้า)
    vec3 auroraColor = vec3(0.05, 0.85, 0.45);
    
    // 2. ทำให้แสงกระเพื่อมตามเวลา เลียนแบบแสงเหนือที่พริ้วไหว
    float pulse = (sin(time * 0.8) * 0.5 + 0.5) * 0.4 + 0.6; // สว่างขึ้นลงระหว่าง 0.6 - 1.0
    
    // 3. ทิศทางตกกระทบ (สะท้อนเฉพาะพื้นผิวที่หงายขึ้นฟ้า)
    float upFacing = smoothstep(0.0, 1.0, N.y);
    
    // 4. เพิ่ม Specular (ประกายเงาสะท้อนจากฟ้า) สำหรับความมันวาวบนหิมะ/หลังคารถ
    vec3 lightDirSky = vec3(0.0, 1.0, 0.0); // สมมติว่าแสงมาจากกลางฟ้าตรงๆ
    vec3 H = normalize(lightDirSky + V);
    float specAurora = pow(max(dot(N, H), 0.0), 16.0) * 0.15; // ปรับ 0.15 เพื่อเพิ่ม/ลดความเงา
    
    // 5. รวมแสงสะท้อนแบบสว่าง (Diffuse) และแบบเงา (Specular)
    vec3 auroraReflection = (albedo * 0.4 + vec3(specAurora)) * auroraColor * upFacing * pulse;
    ambient += auroraReflection;
    // -------------------------------------------------------------

    // รวมแสง
    vec3 result = ambient + diffuse + pointSum;

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    // Emissive
    result = mix(result, albedo, emissiveStrength);

    colour = vec4(result, 1.0);
}