#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vFragPos;

void main() {
    // === CONSTRUÇÃO DA MATRIZ MODEL E TRANSFORMAÇÃO PARA WORLD SPACE ===
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position   = uProjection * uView * worldPos;
    vFragPos      = vec3(worldPos);

    // Matriz Normal: transposta da inversa da model (corrige escala não-uniforme)
    vNormal   = mat3(transpose(inverse(uModel))) * aNormal;
    vTexCoord = aTexCoord;
}
