#version 330 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;

uniform sampler2D uTexture;
uniform vec3 uCamPos;

// Material (lido do .mtl)
uniform vec3  uKa;
uniform vec3  uKd;
uniform vec3  uKs;
uniform float uNs;

// === 3 PONTOS DE LUZ (Key, Fill, Back) ===
struct Light {
    vec3  position;
    vec3  color;
    float intensity;
    bool  enabled;
};
uniform Light uLights[3];

// Highlight do objeto selecionado
uniform float uHighlight;

out vec4 FragColor;

// === CÁLCULO DE PHONG POR FONTE DE LUZ ===
vec3 calcPhong(Light light, vec3 N, vec3 fragPos, vec3 V, vec3 texColor) {
    if (!light.enabled) return vec3(0.0);
    vec3 L   = normalize(light.position - fragPos);
    vec3 R   = reflect(-L, N);

    // Componente ambiente
    vec3 amb = uKa * texColor;

    // Componente difusa (Lei de Lambert)
    vec3 dif = uKd * max(dot(N, L), 0.0) * texColor * light.color * light.intensity;

    // Componente especular (Phong)
    vec3 spe = uKs * pow(max(dot(V, R), 0.0), max(uNs, 1.0)) * light.color * light.intensity;

    return amb + dif + spe;
}

void main() {
    vec4 texSample = texture(uTexture, vTexCoord);
    if (texSample.a < 0.1) discard;
    vec3 texColor = vec3(texSample);
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vFragPos);

    // === CÁLCULO DE PHONG TOTAL (soma das 3 fontes) ===
    vec3 result = vec3(0.0);
    for (int i = 0; i < 3; i++)
        result += calcPhong(uLights[i], N, vFragPos, V, texColor);

    result  = clamp(result, 0.0, 1.0);
    result *= uHighlight; // highlight objeto selecionado
    FragColor = vec4(result, texSample.a);
}
