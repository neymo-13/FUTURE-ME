#version 330 core

in vec2 vUV;
out vec4 colour;

uniform float time;

// ----------------------------------------
// Hash & Noise Functions
// ----------------------------------------
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p)
{
    float val = 0.0;
    float amp = 0.5;
    for(int i = 0; i < 4; i++)
    {
        val += amp * noise(p);
        p *= 2.02;
        amp *= 0.5;
    }
    return val;
}

// ----------------------------------------
// ฟังก์ชันสร้างดาว (Procedural Stars)
// ----------------------------------------
float stars(vec2 uv, float t)
{
    // แบ่งหน้าจอเป็นตารางเล็กๆ เพื่อสุ่มตำแหน่งดาวในแต่ละช่อง
    vec2 gridUV = uv * 60.0; // เพิ่ม/ลด เลขนี้เพื่อปรับความหนาแน่นของดาว
    vec2 id = floor(gridUV);
    vec2 gUv = fract(gridUV) - 0.5;

    // สุ่มตำแหน่งจุดศูนย์กลางของดาวในแต่ละตาราง
    vec2 pos = vec2(hash(id) - 0.5, hash(id + 99.0) - 0.5) * 0.7;
    float dist = length(gUv - pos);

    // สร้างจุดดวงดาวสว่างแบบจุดนุ่มๆ
    float star = smoothstep(0.08, 0.0, dist);

    // ทำให้ดาวบางดวงระยิบระยับ (Twinkle Effect) ด้วยเวลา
    float twinkle = sin(t * 3.0 + hash(id) * 6.28) * 0.5 + 0.5;
    star *= mix(0.3, 1.0, twinkle);

    // สุ่มความสว่างของดาวแต่ละดวงให้ไม่เท่ากัน
    star *= step(0.85, hash(id + 42.0)); // ปรับ 0.85 (ค่านิ่ง ยิ่งเยอะดาวยิ่งน้อยลง)

    return star;
}

// ----------------------------------------
// Main Shader
// ----------------------------------------
void main()
{
    if (gl_FrontFacing) discard;
    
    vec2 uv = vUV;
    float t = time * 0.1;

    // 1. Domain Warping
    vec2 warpUV = uv;
    warpUV.x += fbm(vec2(uv.x * 2.2 + t * 0.4, uv.y * 0.8)) * 0.45 - 0.22;
    
    // 2. Vertical Rays (ริ้วแสงแนวตั้ง)
    float rayNoise = fbm(vec2(warpUV.x * 12.0, warpUV.y * 2.0 - t * 0.6));
    
    // 3. Aurora Shape & Position
    float curtainCenter = 0.53 + sin(warpUV.x * 3.14 + t * 1.2) * 0.10;
    float dist = abs(uv.y - curtainCenter);
    float curtainShape = smoothstep(0.28, 0.0, dist);
    
    // 4. Combine Density
    float density = curtainShape * (0.25 + 0.75 * rayNoise);
    float verticalFade = smoothstep(0.20, 0.45, uv.y) * (1.0 - smoothstep(0.70, 0.88, uv.y));
    density *= verticalFade;

    // 5. Colors (ไล่เฉดเขียวมรกต -> ฟ้า -> ม่วง)
    vec3 colBase   = vec3(0.05, 0.95, 0.45);
    vec3 colMid    = vec3(0.08, 0.75, 0.95);
    vec3 colTop    = vec3(0.60, 0.20, 0.85);

    float colorMix = smoothstep(curtainCenter - 0.1, curtainCenter + 0.25, uv.y);
    vec3 auroraColor = mix(colBase, colMid, smoothstep(0.0, 0.5, colorMix));
    auroraColor = mix(auroraColor, colTop, smoothstep(0.5, 1.0, colorMix));

    vec3 auroraFinal = auroraColor * (density * 1.4 + pow(density, 2.2) * 1.0);
    float auroraAlpha = clamp(density * 0.85, 0.0, 0.82);

    // ----------------------------------------------------
    // 6. วาดดวงดาว และรวมเข้ากับแสงเหนือ
    // ----------------------------------------------------
    float starVal = stars(uv, time);

    // ให้ดาวปรากฏเฉพาะบนฟ้า (uv.y > 0.35) และจางลงเมื่อจมเข้าไปในแสงเหนือที่หนาแน่น
    float starFade = smoothstep(0.3, 0.6, uv.y) * (1.0 - density * 0.8);
    vec3 starColor = vec3(1.0, 0.98, 0.9) * starVal * starFade;

    // ผสมสีดาว + สีแสงเหนือ
    vec3 finalColor = auroraFinal + starColor;
    float finalAlpha = max(auroraAlpha, starVal * starFade * 0.9);

    colour = vec4(finalColor, finalAlpha);
}