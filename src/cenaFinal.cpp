/* cenaFinal.cpp - Visualizador de Cena Final
 * Integra todos os conceitos do semestre:
 *   - Cena carregada via arquivo JSON (../assets/scene.json)
 *   - Multiplos OBJs com textura e materiais Phong
 *   - Camera em primeira pessoa (mouse + WASD)
 *   - Selecao e transformacao de objetos via teclado
 *   - Animacao de trajetoria por Curva de Bezier cubica
 *
 * Controles:
 *   Mouse      : Girar camera
 *   W/A/S/D    : Mover camera
 *   Espaco / C : Camera cima / baixo
 *   TAB        : Selecionar proximo objeto
 *   Setas      : Transladar objeto selecionado (X/Z)
 *   PgUp/PgDn  : Transladar objeto selecionado (Y)
 *   X / Y / Z  : Toggle rotacao continua no eixo
 *   [ / ]      : Diminuir / Aumentar escala
 *   P          : Pausar/retomar animacoes
 *   ESC        : Fechar
 *
 * Leonardo Ian de Oliveira
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// ──────────────────────────────────────────────────────────────
//  Camera em primeira pessoa
// ──────────────────────────────────────────────────────────────
class Camera {
public:
    vec3  position, front, up, right, worldUp;
    float yaw, pitch, speed, sensitivity;

    Camera(vec3  pos = vec3(0.f, 2.f, 8.f),
           vec3  wUp = vec3(0.f, 1.f, 0.f),
           float y   = -90.f,
           float p   = 0.f)
        : position(pos), worldUp(wUp), yaw(y), pitch(p)
        , speed(5.f), sensitivity(0.1f)
    { updateVectors(); }

    mat4 getViewMatrix() const { return lookAt(position, position + front, up); }

    void moveForward (float dt) { position += front   * speed * dt; }
    void moveBack    (float dt) { position -= front   * speed * dt; }
    void moveLeft    (float dt) { position -= right   * speed * dt; }
    void moveRight   (float dt) { position += right   * speed * dt; }
    void moveUp      (float dt) { position += worldUp * speed * dt; }
    void moveDown    (float dt) { position -= worldUp * speed * dt; }

    void rotate(float xOff, float yOff) {
        yaw   += xOff * sensitivity;
        pitch += yOff * sensitivity;
        if (pitch >  89.f) pitch =  89.f;
        if (pitch < -89.f) pitch = -89.f;
        updateVectors();
    }

    void updateVectors() {
        vec3 f;
        f.x = cos(radians(yaw)) * cos(radians(pitch));
        f.y = sin(radians(pitch));
        f.z = sin(radians(yaw)) * cos(radians(pitch));
        front = normalize(f);
        right = normalize(cross(front, worldUp));
        up    = normalize(cross(right, front));
    }
};

// ──────────────────────────────────────────────────────────────
//  Structs
// ──────────────────────────────────────────────────────────────
struct Material {
    vec3  Ka{0.2f}, Kd{0.8f}, Ks{0.5f};
    float Ns = 32.f;
    string texturePath;
};

struct Animation {
    vector<vec3> pts;    // 4 pontos de controle (Bezier cubica)
    float speed = 0.3f;  // avanco de t por segundo
    float t     = 0.f;   // parametro atual [0,1]
    bool  active = false;
};

struct Object3D {
    GLuint   VAO = 0, textureID = 0;
    int      nVertices = 0;
    string   name;
    Material mat;
    vec3     position{0.f};
    vec3     scaleXYZ{1.f};   // suporta escala uniforme e nao-uniforme
    float    rotAngleX = 0.f, rotAngleY = 0.f, rotAngleZ = 0.f;
    bool     rotateX   = false, rotateY  = false, rotateZ  = false;
    Animation anim;
};

// Plano do ambiente (chao ou parede) – geometria procedural
struct EnvPlane {
    GLuint VAO = 0, textureID = 0;
    int    nVerts = 0;
    vec3   Ka{0.25f}, Kd{0.80f}, Ks{0.05f};
    float  Ns = 8.f;
};

// ──────────────────────────────────────────────────────────────
//  Estado global
// ──────────────────────────────────────────────────────────────
int              gWinW = 1200, gWinH = 800;
Camera           camera;
int              selectedObj = 0;
vector<Object3D> objects;
vector<EnvPlane> envPlanes;  // chao, paredes, plataforma e trilhos
GLuint           gFloorTexID = 0;          // textura do chao (reutilizada na plataforma)
vec3             gLightPos{3.f, 5.f, 4.f};
vec3             gLightColor{1.f};
bool             animPaused = false;

bool camW=false, camA=false, camS=false, camD=false, camUp=false, camDown=false;
bool objLeft=false, objRight=false, objFwd=false, objBack=false;
bool objPgUp=false, objPgDn=false, keyScaleUp=false, keyScaleDown=false;
float lastX = 600.f, lastY = 400.f;
bool  firstMouse = true;

// ──────────────────────────────────────────────────────────────
//  Curva de Bezier cubica:  B(t) = sum_i C(3,i) * (1-t)^(3-i) * t^i * P_i
// ──────────────────────────────────────────────────────────────
static vec3 bezierCubic(const vector<vec3>& p, float t)
{
    float u = 1.f - t;
    return u*u*u*p[0]
         + 3.f*u*u*t*p[1]
         + 3.f*u*t*t*p[2]
         +     t*t*t*p[3];
}

// ──────────────────────────────────────────────────────────────
//  Shaders
// ──────────────────────────────────────────────────────────────
const GLchar* vertSrc = R"(
#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoordIn;
layout(location = 2) in vec3 normalIn;

uniform mat4 model, view, projection;

out vec2 texCoord;
out vec3 fragNormal, fragPos;

void main() {
    vec4 wp     = model * vec4(position, 1.0);
    gl_Position = projection * view * wp;
    fragPos     = vec3(wp);
    fragNormal  = mat3(transpose(inverse(model))) * normalIn;
    texCoord    = texCoordIn;
}
)";

const GLchar* fragSrc = R"(
#version 450
in vec2 texCoord;
in vec3 fragNormal, fragPos;

uniform sampler2D texBuff;
uniform vec3  lightPos, lightColor, camPos;
uniform vec3  Ka, Kd, Ks;
uniform float Ns, highlight;

out vec4 color;

void main() {
    vec4 tc  = texture(texBuff, texCoord);
    if (tc.a < 0.1) discard;
    vec3 tex = vec3(tc);

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 V = normalize(camPos   - fragPos);
    vec3 R = reflect(-L, N);

    vec3 amb = Ka * tex;
    vec3 dif = Kd * max(dot(N, L), 0.0) * tex * lightColor;
    vec3 spe = Ks * pow(max(dot(R, V), 0.0), max(Ns, 1.0)) * lightColor;

    color = vec4((amb + dif + spe) * highlight, tc.a);
}
)";

// ──────────────────────────────────────────────────────────────
//  Prototipos
// ──────────────────────────────────────────────────────────────
void     key_callback   (GLFWwindow*, int, int, int, int);
void     cursor_callback(GLFWwindow*, double, double);
GLuint   setupShader    ();
GLuint   loadTexture    (const string&);
Material parseMTL       (const string&);
int      loadSimpleOBJ  (const string&, int&, Material&);
bool     setupScene     (const json&);

// ──────────────────────────────────────────────────────────────
//  Textura branca 1x1 (fallback sem MTL)
// ──────────────────────────────────────────────────────────────
static GLuint whiteTex()
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    unsigned char px[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return id;
}

// ──────────────────────────────────────────────────────────────
//  createQuadVAO – cria VAO para um quad (2 triangulos)
//  verts[4] : 4 cantos em sentido anti-horario (BL, BR, TR, TL)
//  uvs[4]   : UVs correspondentes a cada canto
//  nrm      : normal do plano (mesma para todos os vertices)
// ──────────────────────────────────────────────────────────────
static GLuint createQuadVAO(const vec3* verts, const vec2* uvs,
                            const vec3& nrm, int& outNVerts)
{
    // stride: pos(3) + uv(2) + normal(3) = 8 floats (igual ao OBJ)
    float buf[6 * 8];
    auto push = [&](int dst, int i) {
        int d = dst * 8;
        buf[d+0] = verts[i].x; buf[d+1] = verts[i].y; buf[d+2] = verts[i].z;
        buf[d+3] = uvs[i].x;   buf[d+4] = uvs[i].y;
        buf[d+5] = nrm.x;      buf[d+6] = nrm.y;      buf[d+7] = nrm.z;
    };
    push(0,0); push(1,1); push(2,2); // triangulo 1: BL, BR, TR
    push(3,0); push(4,2); push(5,3); // triangulo 2: BL, TR, TL

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    const GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    outNVerts = 6;
    return VAO;
}

// ──────────────────────────────────────────────────────────────
//  addBox – empurra as faces de um cubo/caixa em envPlanes
//  xMin/xMax, yMin/yMax, zMin/zMax: limites da caixa
//  faces: bitmask   1=TOP  2=BOTTOM  4=FRONT(Z+)  8=BACK(Z-)  16=LEFT(X-)  32=RIGHT(X+)
// ──────────────────────────────────────────────────────────────
static void addBox(vector<EnvPlane>& planes,
                   float xMin, float xMax,
                   float yMin, float yMax,
                   float zMin, float zMax,
                   GLuint texID,
                   vec3 Ka = {0.25f,0.25f,0.25f},
                   vec3 Kd = {0.70f,0.70f,0.70f},
                   vec3 Ks = {0.10f,0.10f,0.10f},
                   float Ns = 16.f,
                   int faces = 63)
{
    auto makeEP = [&](const vec3* v, const vec2* u, const vec3& n) {
        EnvPlane ep;
        ep.VAO       = createQuadVAO(v, u, n, ep.nVerts);
        ep.textureID = texID;
        ep.Ka = Ka; ep.Kd = Kd; ep.Ks = Ks; ep.Ns = Ns;
        planes.push_back(ep);
    };

    const float uvS = 0.25f;
    float W = xMax - xMin, H = yMax - yMin, D = zMax - zMin;

    // TOP (Y=yMax, normal +Y)
    if (faces & 1) {
        vec3 v[4] = {{xMin,yMax,zMax},{xMax,yMax,zMax},{xMax,yMax,zMin},{xMin,yMax,zMin}};
        vec2 u[4] = {{0,0},{W*uvS,0},{W*uvS,D*uvS},{0,D*uvS}};
        makeEP(v, u, {0.f,1.f,0.f});
    }
    // BOTTOM (Y=yMin, normal -Y)
    if (faces & 2) {
        vec3 v[4] = {{xMin,yMin,zMin},{xMax,yMin,zMin},{xMax,yMin,zMax},{xMin,yMin,zMax}};
        vec2 u[4] = {{0,0},{W*uvS,0},{W*uvS,D*uvS},{0,D*uvS}};
        makeEP(v, u, {0.f,-1.f,0.f});
    }
    // FRONT (Z=zMax, normal +Z)
    if (faces & 4) {
        vec3 v[4] = {{xMin,yMin,zMax},{xMax,yMin,zMax},{xMax,yMax,zMax},{xMin,yMax,zMax}};
        vec2 u[4] = {{0,0},{W*uvS,0},{W*uvS,H*uvS},{0,H*uvS}};
        makeEP(v, u, {0.f,0.f,1.f});
    }
    // BACK (Z=zMin, normal -Z)
    if (faces & 8) {
        vec3 v[4] = {{xMax,yMin,zMin},{xMin,yMin,zMin},{xMin,yMax,zMin},{xMax,yMax,zMin}};
        vec2 u[4] = {{0,0},{W*uvS,0},{W*uvS,H*uvS},{0,H*uvS}};
        makeEP(v, u, {0.f,0.f,-1.f});
    }
    // LEFT (X=xMin, normal -X)
    if (faces & 16) {
        vec3 v[4] = {{xMin,yMin,zMin},{xMin,yMin,zMax},{xMin,yMax,zMax},{xMin,yMax,zMin}};
        vec2 u[4] = {{0,0},{D*uvS,0},{D*uvS,H*uvS},{0,H*uvS}};
        makeEP(v, u, {-1.f,0.f,0.f});
    }
    // RIGHT (X=xMax, normal +X)
    if (faces & 32) {
        vec3 v[4] = {{xMax,yMin,zMax},{xMax,yMin,zMin},{xMax,yMax,zMin},{xMax,yMax,zMax}};
        vec2 u[4] = {{0,0},{D*uvS,0},{D*uvS,H*uvS},{0,H*uvS}};
        makeEP(v, u, {1.f,0.f,0.f});
    }
}

// ──────────────────────────────────────────────────────────────
//  setupEnvPlanes – cria chao e paredes do diorama
//  Le floor_texture e wall_texture do JSON e cria geometria procedural
// ──────────────────────────────────────────────────────────────
static void setupEnvPlanes(const json& j)
{
    string floorTex = j.value("floor_texture", "");
    string wallTex  = j.value("wall_texture",  "");

    // Carrega texturas do ambiente (floor e wall)
    cout << "Carregando chao: " << (floorTex.empty() ? "(sem textura)" : floorTex) << "\n";
    gFloorTexID = floorTex.empty() ? whiteTex() : loadTexture(floorTex);
    GLuint floorTexID = gFloorTexID;

    cout << "Carregando paredes: " << (wallTex.empty() ? "(sem textura)" : wallTex) << "\n";
    GLuint wallTexID  = wallTex.empty()  ? whiteTex() : loadTexture(wallTex);

    // Dimensoes da "caixa" do diorama (~1 unidade = 1 metro)
    const float W  = 10.f;   // metade da largura (X de -10 a +10 = 20m total)
    const float ZF =  4.f;   // limite frontal: chao/parede terminam aqui (camera esta em Z=11)
    const float ZB =  -6.f;  // parede de fundo
    const float H  =  5.f;   // altura das paredes (~5m)

    // UV: 1 tile de textura a cada 4 unidades de cena
    const float uvPerUnit = 1.f / 4.f;

    // ── CHAO (plano XZ em Y=0) ───────────────────────────────
    {
        float uW = (2*W)    * uvPerUnit; // tiles na direcao X
        float uD = (ZF-ZB)  * uvPerUnit; // tiles na direcao Z
        vec3 pts[4] = { {-W,0,ZF}, { W,0,ZF}, { W,0,ZB}, {-W,0,ZB} };
        vec2 uvs[4] = { {0,0}, {uW,0}, {uW,uD}, {0,uD} };
        vec3 nrm    = {0.f, 1.f, 0.f};

        EnvPlane ep;
        ep.VAO       = createQuadVAO(pts, uvs, nrm, ep.nVerts);
        ep.textureID = floorTexID;
        envPlanes.push_back(ep);
    }

    // ── PAREDE DE FUNDO (plano XY em Z=ZB) ──────────────────
    {
        float uW = (2*W) * uvPerUnit;
        float uH = H     * uvPerUnit;
        vec3 pts[4] = { {-W,0,ZB}, { W,0,ZB}, { W,H,ZB}, {-W,H,ZB} };
        vec2 uvs[4] = { {0,0}, {uW,0}, {uW,uH}, {0,uH} };
        vec3 nrm    = {0.f, 0.f, 1.f}; // aponta para dentro (direcao da camera)

        EnvPlane ep;
        ep.VAO       = createQuadVAO(pts, uvs, nrm, ep.nVerts);
        ep.textureID = wallTexID;
        envPlanes.push_back(ep);
    }

    // Abertura do tunel: o trem (scale=1.0, ~2.68m em Z) corre centrado em Z=1.0
    // Largura do trem em Z: 1.34m para cada lado → Z=-0.34..2.34; margens de 0.36m
    const float TUN_Z_MIN = -0.7f;  // borda Z esquerda da abertura
    const float TUN_Z_MAX =  2.7f;  // borda Z direita da abertura
    const float TUN_Y_MAX =  4.0f;  // altura da abertura (trem mede 3.42m + folga)

    // ── PAREDE LATERAL ESQUERDA (X=-W) – dividida em 3 partes + fundo de tunel ──
    {
        vec3 nrm = {1.f, 0.f, 0.f}; // aponta para dentro (+X)

        // Empurra um retangulo da parede esquerda: z0>z1 (de ZF para ZB)
        auto pushL = [&](float z0, float z1, float y0, float y1) {
            float uD = (z0 - z1) * uvPerUnit;
            float uH = (y1 - y0) * uvPerUnit;
            vec3 v[4] = {{-W,y0,z0},{-W,y0,z1},{-W,y1,z1},{-W,y1,z0}};
            vec2 u[4] = {{0,0},{uD,0},{uD,uH},{0,uH}};
            EnvPlane ep; ep.VAO = createQuadVAO(v,u,nrm,ep.nVerts);
            ep.textureID = wallTexID; envPlanes.push_back(ep);
        };
        pushL(ZF,       TUN_Z_MAX, 0.f, H);           // secao em frente ao tunel
        pushL(TUN_Z_MIN, ZB,       0.f, H);            // secao atras do tunel
        pushL(TUN_Z_MAX, TUN_Z_MIN, TUN_Y_MAX, H);    // secao acima da abertura

        // Quad escuro atras da parede para dar profundidade ao tunel
        EnvPlane dark;
        vec3 dv[4] = {{-W-0.3f,0.f,TUN_Z_MIN},{-W-0.3f,0.f,TUN_Z_MAX},
                      {-W-0.3f,TUN_Y_MAX,TUN_Z_MAX},{-W-0.3f,TUN_Y_MAX,TUN_Z_MIN}};
        vec2 du[4] = {{0,0},{1,0},{1,1},{0,1}};
        dark.VAO = createQuadVAO(dv, du, {1.f,0.f,0.f}, dark.nVerts);
        dark.textureID = whiteTex();
        dark.Ka = {0.04f,0.04f,0.05f}; dark.Kd = {0.f,0.f,0.f};
        dark.Ks = {0.f,0.f,0.f};       dark.Ns = 1.f;
        envPlanes.push_back(dark);
    }

    // ── PAREDE LATERAL DIREITA (X=+W) – dividida em 3 partes + fundo de tunel ──
    {
        vec3 nrm = {-1.f, 0.f, 0.f}; // aponta para dentro (-X)

        // Empurra um retangulo da parede direita: z0<z1 (de ZB para ZF)
        auto pushR = [&](float z0, float z1, float y0, float y1) {
            float uD = (z1 - z0) * uvPerUnit;
            float uH = (y1 - y0) * uvPerUnit;
            vec3 v[4] = {{W,y0,z0},{W,y0,z1},{W,y1,z1},{W,y1,z0}};
            vec2 u[4] = {{0,0},{uD,0},{uD,uH},{0,uH}};
            EnvPlane ep; ep.VAO = createQuadVAO(v,u,nrm,ep.nVerts);
            ep.textureID = wallTexID; envPlanes.push_back(ep);
        };
        pushR(ZB,       TUN_Z_MIN, 0.f, H);            // secao atras do tunel
        pushR(TUN_Z_MAX, ZF,       0.f, H);             // secao em frente ao tunel
        pushR(TUN_Z_MIN, TUN_Z_MAX, TUN_Y_MAX, H);    // secao acima da abertura

        // Quad escuro atras da parede para dar profundidade ao tunel
        EnvPlane dark;
        vec3 dv[4] = {{W+0.3f,0.f,TUN_Z_MIN},{W+0.3f,0.f,TUN_Z_MAX},
                      {W+0.3f,TUN_Y_MAX,TUN_Z_MAX},{W+0.3f,TUN_Y_MAX,TUN_Z_MIN}};
        vec2 du[4] = {{0,0},{1,0},{1,1},{0,1}};
        dark.VAO = createQuadVAO(dv, du, {-1.f,0.f,0.f}, dark.nVerts);
        dark.textureID = whiteTex();
        dark.Ka = {0.04f,0.04f,0.05f}; dark.Kd = {0.f,0.f,0.f};
        dark.Ks = {0.f,0.f,0.f};       dark.Ns = 1.f;
        envPlanes.push_back(dark);
    }

    cout << "Diorama: " << envPlanes.size()
         << " planos criados (1 chao + 3 paredes)" << endl;
}

// ──────────────────────────────────────────────────────────────
//  setupPlatform – cria bloco elevado da plataforma de embarque
//  A plataforma fica no lado do passageiro (Z negativo),
//  elevada 80 cm acima do chao (Y=0 a Y=0.8).
//  O mobiliario (bancos, postes, lixeira) deve ter Y += 0.8 no JSON.
// ──────────────────────────────────────────────────────────────
static void setupPlatform()
{
    const float PLATFORM_H  =  0.80f;  // altura em metros
    const float PLAT_X_MIN  =  -9.f;
    const float PLAT_X_MAX  =   9.f;
    const float PLAT_Z_NEAR =   0.f;   // borda frontal (lado da via)
    const float PLAT_Z_FAR  =  -5.5f;  // borda traseira

    // Material cinza concreto (textura concreta do chao reutilizada)
    vec3  Ka = {0.25f, 0.25f, 0.25f};
    vec3  Kd = {0.70f, 0.70f, 0.70f};
    vec3  Ks = {0.08f, 0.08f, 0.08f};
    float Ns = 12.f;

    // 61 = todas as faces exceto BOTTOM (face de baixo oculta no chao)
    addBox(envPlanes,
           PLAT_X_MIN, PLAT_X_MAX,
           0.f, PLATFORM_H,
           PLAT_Z_FAR, PLAT_Z_NEAR,
           gFloorTexID, Ka, Kd, Ks, Ns, 61);

    cout << "Plataforma criada: Y=0.0 a Y=" << PLATFORM_H
         << "  X=[" << PLAT_X_MIN << "," << PLAT_X_MAX << "]"
         << "  Z=[" << PLAT_Z_FAR << "," << PLAT_Z_NEAR << "]" << endl;
}

// ──────────────────────────────────────────────────────────────
//  setupRails – cria trilhos e dormentes procedurais
//  O trem se move na direcao X (rotacao Y=90 no JSON).
//  Trilhos paralelos ao eixo X, separados por ~1.4 m (bitola).
//  Centro da via: Z=1.0  → trilho esquerdo Z=0.3, direito Z=1.7
// ──────────────────────────────────────────────────────────────
static void setupRails()
{
    // Extensao dos trilhos (um pouco alem do trajeto do trem)
    const float RAIL_X_MIN  = -11.f;
    const float RAIL_X_MAX  =   9.f;
    const float RAIL_Y_TOP  =  0.15f;  // altura do trilho
    const float RAIL_WIDTH  =  0.08f;  // largura do perfil do trilho
    const float RAIL_Z_L    =  0.30f;  // trilho esquerdo
    const float RAIL_Z_R    =  1.70f;  // trilho direito

    // Material aco: cinza metalico brilhante
    vec3  railKa = {0.30f, 0.30f, 0.35f};
    vec3  railKd = {0.50f, 0.50f, 0.55f};
    vec3  railKs = {0.60f, 0.60f, 0.65f};
    float railNs = 80.f;
    GLuint railTex = whiteTex();

    // Trilho esquerdo
    addBox(envPlanes,
           RAIL_X_MIN, RAIL_X_MAX,
           0.f, RAIL_Y_TOP,
           RAIL_Z_L - RAIL_WIDTH*0.5f, RAIL_Z_L + RAIL_WIDTH*0.5f,
           railTex, railKa, railKd, railKs, railNs, 61);

    // Trilho direito
    addBox(envPlanes,
           RAIL_X_MIN, RAIL_X_MAX,
           0.f, RAIL_Y_TOP,
           RAIL_Z_R - RAIL_WIDTH*0.5f, RAIL_Z_R + RAIL_WIDTH*0.5f,
           railTex, railKa, railKd, railKs, railNs, 61);

    // Dormentes (cross-ties): madeira marrom, a cada 0.6 m ao longo de X
    const float TIE_STEP   = 0.60f;
    const float TIE_HALF_W = 0.12f;  // metade da largura em X
    const float TIE_H      = 0.08f;  // altura menor que o trilho
    const float TIE_Z_MIN  = RAIL_Z_L - 0.30f;
    const float TIE_Z_MAX  = RAIL_Z_R + 0.30f;

    vec3  tieKa = {0.30f, 0.20f, 0.10f};
    vec3  tieKd = {0.60f, 0.40f, 0.20f};
    vec3  tieKs = {0.05f, 0.05f, 0.05f};
    float tieNs = 8.f;
    GLuint tieTex = whiteTex();

    int nTies = 0;
    for (float x = RAIL_X_MIN; x <= RAIL_X_MAX + 0.01f; x += TIE_STEP) {
        addBox(envPlanes,
               x - TIE_HALF_W, x + TIE_HALF_W,
               0.f, TIE_H,
               TIE_Z_MIN, TIE_Z_MAX,
               tieTex, tieKa, tieKd, tieKs, tieNs, 61);
        nTies++;
    }

    cout << "Trilhos criados: 2 trilhos + " << nTies << " dormentes"
         << "  (Z=[" << RAIL_Z_L << "," << RAIL_Z_R << "])" << endl;
}

// ──────────────────────────────────────────────────────────────
//  setupPostLamps – luminarias auto-iluminadas no topo de cada poste
//  Posicoes devem coincidir com os postes no scene.json.
//  Ka alto + Kd=0 cria efeito "emissivo" independente da luz da cena.
// ──────────────────────────────────────────────────────────────
static void setupPostLamps()
{
    // Escala dos postes no scene.json
    const float POSTE_SCALE = 0.85f;

    // Centro do vidro da luminaria em espaco OBJ (analisado via bounding box dos vertices)
    // Cluster lantern: X=-0.916..−0.308 (ctr=-0.612), Y=3.372..3.932 (ctr=3.652), Z=±0.215 (ctr=0)
    const float LAMP_OBJ_X = -0.612f;
    const float LAMP_OBJ_Y =  3.652f;

    // Posicao em mundo para poste com position=[postX, 0.8, postZ] e scale=0.85
    // world = position + OBJ_coord * scale
    const float LAMP_DX = LAMP_OBJ_X * POSTE_SCALE;           // -0.520 m em X
    const float LAMP_Y  = 0.8f + LAMP_OBJ_Y * POSTE_SCALE;    // 3.904 m de altura

    // Dimensoes levemente menores que o vidro da luminaria para ficar interno
    // Vidro em mundo: X=0.517m  Y=0.476m  Z=0.366m
    const float LW = 0.35f;  // largura em X
    const float LH = 0.30f;  // altura em Y
    const float LD = 0.20f;  // profundidade em Z

    // Amarelo quente, auto-iluminado (Kd=0 → nao depende da luz direcional)
    vec3  Ka  = {1.0f, 0.88f, 0.55f};
    vec3  Kd  = {0.0f, 0.0f,  0.0f};
    vec3  Ks  = {0.0f, 0.0f,  0.0f};
    GLuint tex = whiteTex();

    // Posicoes XZ dos postes (devem coincidir com scene.json)
    const float postX[] = {-7.f, -2.f, 3.f, 9.f};
    const float postZ   = -4.5f;
    const int   nPosts  = 4;

    for (int i = 0; i < nPosts; i++) {
        float cx = postX[i] + LAMP_DX;  // desloca em X ate o centro do vidro
        float cz = postZ;
        addBox(envPlanes,
               cx - LW*0.5f, cx + LW*0.5f,
               LAMP_Y - LH*0.5f, LAMP_Y + LH*0.5f,
               cz - LD*0.5f, cz + LD*0.5f,
               tex, Ka, Kd, Ks, 1.f);
    }

    cout << "Luminarias: " << nPosts << " lampadas"
         << "  (X_offset=" << LAMP_DX << ", Y=" << LAMP_Y << ")" << endl;
}

// ──────────────────────────────────────────────────────────────
//  setupScene – configura camera, luz e objetos a partir do JSON
//  (deve ser chamado apos inicializacao do GLAD)
// ──────────────────────────────────────────────────────────────
bool setupScene(const json& j)
{
    // Camera
    if (j.contains("camera")) {
        const auto& c = j["camera"];
        if (c.contains("position"))
            camera.position = vec3(c["position"][0], c["position"][1], c["position"][2]);
        if (c.contains("yaw"))   camera.yaw   = c["yaw"].get<float>();
        if (c.contains("pitch")) camera.pitch = c["pitch"].get<float>();
        camera.updateVectors();
    }

    // Luz – usa a primeira luz ativa do array "lights" do JSON
    if (j.contains("lights") && j["lights"].is_array()) {
        for (const auto& l : j["lights"]) {
            if (!l.value("enabled", true)) continue;
            if (l.contains("position"))
                gLightPos = vec3(l["position"][0], l["position"][1], l["position"][2]);
            if (l.contains("color")) {
                float intensity = l.value("intensity", 1.0f);
                gLightColor = vec3(l["color"][0].get<float>() * intensity,
                                   l["color"][1].get<float>() * intensity,
                                   l["color"][2].get<float>() * intensity);
            }
            break; // shader suporta 1 luz; usa a primeira ativa
        }
    }
    // retrocompatibilidade com campo "light" singular
    else if (j.contains("light")) {
        const auto& l = j["light"];
        if (l.contains("position"))
            gLightPos   = vec3(l["position"][0], l["position"][1], l["position"][2]);
        if (l.contains("color"))
            gLightColor = vec3(l["color"][0],    l["color"][1],    l["color"][2]);
    }

    // Objetos
    if (!j.contains("objects") || !j["objects"].is_array()) {
        cerr << "JSON: campo 'objects' ausente ou invalido\n";
        return false;
    }

    for (const auto& jo : j["objects"]) {
        string objPath = jo.value("obj",  "");
        string name    = jo.value("name", "sem_nome");

        vec3 pos{0.f};
        if (jo.contains("position"))
            pos = vec3(jo["position"][0], jo["position"][1], jo["position"][2]);

        vec3 rotDeg{0.f};
        if (jo.contains("rotation"))
            rotDeg = vec3(jo["rotation"][0], jo["rotation"][1], jo["rotation"][2]);

        // "scale" aceita numero (uniforme) ou array [x,y,z] (nao-uniforme)
        vec3 scaleVec{1.f};
        if (jo.contains("scale")) {
            if (jo["scale"].is_array())
                scaleVec = vec3(jo["scale"][0], jo["scale"][1], jo["scale"][2]);
            else
                scaleVec = vec3(jo["scale"].get<float>());
        }

        if (objPath.empty()) { cerr << "  Objeto '" << name << "' sem campo 'obj'\n"; continue; }

        Material mat;
        int nVerts = 0;
        int vao = loadSimpleOBJ(objPath, nVerts, mat);
        if (vao == -1) { cerr << "  Falha ao carregar: " << objPath << "\n"; continue; }

        Object3D obj;
        obj.VAO       = (GLuint)vao;
        obj.nVertices = nVerts;
        obj.name      = name;
        obj.mat       = mat;
        obj.position  = pos;
        obj.scaleXYZ  = scaleVec;
        obj.rotAngleX = radians(rotDeg.x);
        obj.rotAngleY = radians(rotDeg.y);
        obj.rotAngleZ = radians(rotDeg.z);

        obj.textureID = mat.texturePath.empty() ? whiteTex() : loadTexture(mat.texturePath);

        // Animacao Bezier cubica
        if (jo.contains("animation")) {
            const auto& a    = jo["animation"];
            string      type = a.value("type", "");
            if (type == "bezier" && a.contains("control_points")) {
                const auto& cp = a["control_points"];
                if (cp.size() == 4) {
                    for (const auto& pt : cp)
                        obj.anim.pts.push_back(vec3(pt[0], pt[1], pt[2]));
                    obj.anim.speed  = a.value("speed", 0.3f);
                    obj.anim.active = true;
                    cout << "  [Bezier] " << name << " (speed=" << obj.anim.speed << ")\n";
                } else {
                    cerr << "  Aviso: bezier precisa de exatamente 4 pontos (" << name << ")\n";
                }
            }
        }

        objects.push_back(obj);
    }

    return !objects.empty();
}

// ──────────────────────────────────────────────────────────────
//  MAIN
// ──────────────────────────────────────────────────────────────
int main()
{
    // Fase 1: ler o JSON para extrair config da janela antes de criá-la
    const string scenePath = "../assets/scene.json";
    json sceneJson;
    {
        ifstream f(scenePath);
        if (!f.is_open()) {
            cerr << "Erro: nao encontrou " << scenePath << "\n"
                 << "Execute o programa a partir do diretorio build/\n";
            return -1;
        }
        try { f >> sceneJson; }
        catch (const json::exception& e) {
            cerr << "JSON invalido: " << e.what() << "\n"; return -1;
        }
    }

    string winTitle = "Cena Final - Leonardo Ian de Oliveira";
    if (sceneJson.contains("window")) {
        const auto& w = sceneJson["window"];
        if (w.contains("width"))  gWinW     = w["width"].get<int>();
        if (w.contains("height")) gWinH     = w["height"].get<int>();
        if (w.contains("title"))  winTitle  = w["title"].get<string>();
    }
    lastX = gWinW / 2.f;
    lastY = gWinH / 2.f;

    // Fase 2: criar janela e inicializar OpenGL
    if (!glfwInit()) { cerr << "Falha ao inicializar GLFW\n"; return -1; }

    GLFWwindow* window = glfwCreateWindow(gWinW, gWinH, winTitle.c_str(), nullptr, nullptr);
    if (!window) { cerr << "Falha ao criar janela\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback      (window, key_callback);
    glfwSetCursorPosCallback(window, cursor_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD\n"; return -1;
    }

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);

    GLuint shader = setupShader();
    glUseProgram(shader);

    // Uniform locations
    GLint modelLoc     = glGetUniformLocation(shader, "model");
    GLint viewLoc      = glGetUniformLocation(shader, "view");
    GLint projLoc      = glGetUniformLocation(shader, "projection");
    GLint camPosLoc    = glGetUniformLocation(shader, "camPos");
    GLint lightPosLoc  = glGetUniformLocation(shader, "lightPos");
    GLint lightColLoc  = glGetUniformLocation(shader, "lightColor");
    GLint highlightLoc = glGetUniformLocation(shader, "highlight");
    GLint KaLoc        = glGetUniformLocation(shader, "Ka");
    GLint KdLoc        = glGetUniformLocation(shader, "Kd");
    GLint KsLoc        = glGetUniformLocation(shader, "Ks");
    GLint NsLoc        = glGetUniformLocation(shader, "Ns");

    glUniform1i(glGetUniformLocation(shader, "texBuff"), 0);

    // Projecao perspectiva
    float fov = 45.f, nearP = 0.1f, farP = 100.f;
    if (sceneJson.contains("camera")) {
        const auto& c = sceneJson["camera"];
        if (c.contains("fov"))  fov   = c["fov"].get<float>();
        if (c.contains("near")) nearP = c["near"].get<float>();
        if (c.contains("far"))  farP  = c["far"].get<float>();
    }
    mat4 projection = perspective(radians(fov), (float)gWinW / gWinH, nearP, farP);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));

    // Fase 3: carregar cena (camera, luz, objetos)
    if (!setupScene(sceneJson)) {
        cerr << "Falha ao carregar cena (nenhum objeto carregado)\n";
        glfwTerminate(); return -1;
    }

    // Fase 4: criar geometria procedural de chao, paredes, plataforma, trilhos e luzes
    setupEnvPlanes(sceneJson);
    setupPlatform();
    setupRails();
    setupPostLamps();

    // Luz (enviada uma vez; nao muda em tempo real neste projeto)
    glUniform3fv(lightPosLoc, 1, value_ptr(gLightPos));
    glUniform3fv(lightColLoc, 1, value_ptr(gLightColor));

    cout << "\n=== CONTROLES ===\n"
         << "Mouse      : Girar camera\n"
         << "W/A/S/D    : Mover camera\n"
         << "Espaco / C : Camera cima / baixo\n"
         << "TAB        : Selecionar proximo objeto\n"
         << "Setas      : Transladar objeto (X/Z)  [desativado em objetos animados]\n"
         << "PgUp/PgDn  : Transladar objeto (Y)\n"
         << "X/Y/Z      : Toggle rotacao continua no eixo\n"
         << "[ / ]      : Diminuir / Aumentar escala\n"
         << "P          : Pausar/retomar animacoes\n"
         << "ESC        : Fechar\n"
         << "\nSelecionado: " << objects[selectedObj].name << "\n\n";

    const float OBJ_SPEED   = 2.5f;
    const float SCALE_SPEED = 1.0f;
    float lastFrame = 0.f;

    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt  = now - lastFrame;
        lastFrame = now;

        glfwPollEvents();

        // Movimento da camera
        if (camW)    camera.moveForward(dt);
        if (camS)    camera.moveBack   (dt);
        if (camA)    camera.moveLeft   (dt);
        if (camD)    camera.moveRight  (dt);
        if (camUp)   camera.moveUp     (dt);
        if (camDown) camera.moveDown   (dt);

        // Transformacoes no objeto selecionado
        Object3D& sel = objects[selectedObj];

        // Translacao manual so funciona em objetos sem animacao
        if (!sel.anim.active) {
            if (objLeft)  sel.position.x -= OBJ_SPEED * dt;
            if (objRight) sel.position.x += OBJ_SPEED * dt;
            if (objFwd)   sel.position.z -= OBJ_SPEED * dt;
            if (objBack)  sel.position.z += OBJ_SPEED * dt;
        }
        // Y sempre disponivel
        if (objPgUp)      sel.position.y += OBJ_SPEED * dt;
        if (objPgDn)      sel.position.y -= OBJ_SPEED * dt;
        if (keyScaleUp)   sel.scaleXYZ *= (1.f + SCALE_SPEED * dt);
        if (keyScaleDown) sel.scaleXYZ *= std::max(1.f - SCALE_SPEED * dt, 0.01f);

        glClearColor(0.10f, 0.10f, 0.13f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
        glUniform3fv(camPosLoc, 1, value_ptr(camera.position));

        // ── Renderiza geometria ambiental: chao, paredes, plataforma, trilhos ──
        if (!envPlanes.empty()) {
            mat4 identity = mat4(1.f);
            glUniformMatrix4fv(modelLoc,     1, GL_FALSE, value_ptr(identity));
            glUniform1f       (highlightLoc, 1.f);

            for (const auto& ep : envPlanes) {
                // cada plano tem seu proprio material (chao/parede/trilho/plataforma)
                glUniform3fv(KaLoc, 1, value_ptr(ep.Ka));
                glUniform3fv(KdLoc, 1, value_ptr(ep.Kd));
                glUniform3fv(KsLoc, 1, value_ptr(ep.Ks));
                glUniform1f (NsLoc, ep.Ns);
                glBindTexture (GL_TEXTURE_2D, ep.textureID);
                glBindVertexArray(ep.VAO);
                glDrawArrays  (GL_TRIANGLES, 0, ep.nVerts);
            }
            glBindVertexArray(0);
        }

        // ── Renderiza objetos OBJ ──────────────────────────────
        for (int i = 0; i < (int)objects.size(); i++)
        {
            Object3D& obj = objects[i];

            // Rotacao continua (toggle por eixo)
            if (obj.rotateX) obj.rotAngleX += dt;
            if (obj.rotateY) obj.rotAngleY += dt;
            if (obj.rotateZ) obj.rotAngleZ += dt;

            // Animacao de trajetoria por Bezier
            if (obj.anim.active && !animPaused) {
                obj.anim.t += dt * obj.anim.speed;
                if (obj.anim.t > 1.f) obj.anim.t -= 1.f;
                obj.position = bezierCubic(obj.anim.pts, obj.anim.t);
            }

            mat4 model = mat4(1.f);
            model = translate(model, obj.position);
            model = rotate(model, obj.rotAngleX, vec3(1.f, 0.f, 0.f));
            model = rotate(model, obj.rotAngleY, vec3(0.f, 1.f, 0.f));
            model = rotate(model, obj.rotAngleZ, vec3(0.f, 0.f, 1.f));
            model = scale(model, obj.scaleXYZ);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

            glUniform3fv(KaLoc, 1, value_ptr(obj.mat.Ka));
            glUniform3fv(KdLoc, 1, value_ptr(obj.mat.Kd));
            glUniform3fv(KsLoc, 1, value_ptr(obj.mat.Ks));
            glUniform1f (NsLoc, obj.mat.Ns);
            glUniform1f (highlightLoc, (i == selectedObj) ? 1.f : 0.6f);

            glBindTexture(GL_TEXTURE_2D, obj.textureID);
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// ──────────────────────────────────────────────────────────────
//  key_callback
// ──────────────────────────────────────────────────────────────
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    (void)scancode; (void)mode;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Camera (estado continuo)
    if (key == GLFW_KEY_W)     camW    = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_S)     camS    = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_A)     camA    = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_D)     camD    = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_SPACE) camUp   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_C)     camDown = (action != GLFW_RELEASE);

    // Selecao de objeto
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        selectedObj = (selectedObj + 1) % (int)objects.size();
        const auto& obj = objects[selectedObj];
        cout << "Selecionado: " << obj.name
             << (obj.anim.active ? " [ANIMADO]" : "") << "\n";
    }

    // Translacao do objeto selecionado
    if (key == GLFW_KEY_LEFT)      objLeft  = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_RIGHT)     objRight = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_UP)        objFwd   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_DOWN)      objBack  = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_PAGE_UP)   objPgUp  = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_PAGE_DOWN) objPgDn  = (action != GLFW_RELEASE);

    // Rotacao continua (toggle por eixo)
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        objects[selectedObj].rotateX = !objects[selectedObj].rotateX;
        objects[selectedObj].rotateY = false;
        objects[selectedObj].rotateZ = false;
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
        objects[selectedObj].rotateX = false;
        objects[selectedObj].rotateY = !objects[selectedObj].rotateY;
        objects[selectedObj].rotateZ = false;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        objects[selectedObj].rotateX = false;
        objects[selectedObj].rotateY = false;
        objects[selectedObj].rotateZ = !objects[selectedObj].rotateZ;
    }

    // Escala
    if (key == GLFW_KEY_RIGHT_BRACKET) keyScaleUp   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_LEFT_BRACKET)  keyScaleDown = (action != GLFW_RELEASE);

    // Pausar/retomar animacoes
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        animPaused = !animPaused;
        cout << "Animacoes: " << (animPaused ? "PAUSADAS" : "ATIVAS") << "\n";
    }
}

// ──────────────────────────────────────────────────────────────
//  cursor_callback – rotacao da camera via mouse
// ──────────────────────────────────────────────────────────────
void cursor_callback(GLFWwindow* /*window*/, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = (float)xpos; lastY = (float)ypos;
        firstMouse = false; return;
    }
    float xOff =  (float)xpos - lastX;
    float yOff =  lastY - (float)ypos;  // invertido: y cresce para baixo na tela
    lastX = (float)xpos; lastY = (float)ypos;
    camera.rotate(xOff, yOff);
}

// ──────────────────────────────────────────────────────────────
//  setupShader
// ──────────────────────────────────────────────────────────────
GLuint setupShader()
{
    auto compile = [](GLenum type, const GLchar* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; GLchar log[512];
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { glGetShaderInfoLog(s, 512, nullptr, log); cerr << "Shader error:\n" << log; }
        return s;
    };
    GLuint vs   = compile(GL_VERTEX_SHADER,   vertSrc);
    GLuint fs   = compile(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; GLchar log[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, nullptr, log); cerr << "Link error:\n" << log; }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// ──────────────────────────────────────────────────────────────
//  loadTexture  (stb_image)
// ──────────────────────────────────────────────────────────────
GLuint loadTexture(const string& filePath)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &ch, 0);
    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        cout << "  Textura: " << filePath << "\n";
    } else {
        cerr << "  Falha textura: " << filePath << "\n";
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

// ──────────────────────────────────────────────────────────────
//  parseMTL  – extrai Ka, Kd, Ks, Ns e map_Kd
// ──────────────────────────────────────────────────────────────
Material parseMTL(const string& mtlPath)
{
    Material mat;
    ifstream file(mtlPath);
    if (!file.is_open()) { cerr << "  MTL nao encontrado: " << mtlPath << "\n"; return mat; }

    string baseDir;
    size_t sl = mtlPath.find_last_of("/\\");
    if (sl != string::npos) baseDir = mtlPath.substr(0, sl + 1);

    string line;
    while (getline(file, line)) {
        istringstream ss(line); string t; ss >> t;
        if      (t == "Ka")     ss >> mat.Ka.r >> mat.Ka.g >> mat.Ka.b;
        else if (t == "Kd")     ss >> mat.Kd.r >> mat.Kd.g >> mat.Kd.b;
        else if (t == "Ks")     ss >> mat.Ks.r >> mat.Ks.g >> mat.Ks.b;
        else if (t == "Ns")     ss >> mat.Ns;
        else if (t == "map_Kd") { string n; ss >> n; mat.texturePath = baseDir + n; }
    }
    return mat;
}

// ──────────────────────────────────────────────────────────────
//  loadSimpleOBJ a
//  Stride: pos(3) + uv(2) + normal(3) = 8 floats por vertice
// ──────────────────────────────────────────────────────────────
int loadSimpleOBJ(const string& filePath, int& nVertices, Material& outMat)
{
    vector<vec3>    positions;
    vector<vec2>    texCoords;
    vector<vec3>    normals;
    vector<GLfloat> vBuffer;

    string baseDir;
    size_t sl = filePath.find_last_of("/\\");
    if (sl != string::npos) baseDir = filePath.substr(0, sl + 1);

    ifstream file(filePath);
    if (!file.is_open()) { cerr << "Erro ao abrir OBJ: " << filePath << "\n"; return -1; }

    cout << "Carregando: " << filePath << "\n";
    string line;
    while (getline(file, line)) {
        istringstream ss(line); string t; ss >> t;
        if      (t == "v")  { vec3 v;  ss >> v.x  >> v.y  >> v.z;  positions.push_back(v);  }
        else if (t == "vt") { vec2 vt; ss >> vt.s >> vt.t;         texCoords.push_back(vt); }
        else if (t == "vn") { vec3 vn; ss >> vn.x >> vn.y >> vn.z; normals.push_back(vn);   }
        else if (t == "mtllib") { string m; ss >> m; outMat = parseMTL(baseDir + m); }
        else if (t == "f") {
            // Coleta todos os vertices da face (suporta triangulos, quads e n-gons)
            struct FaceVert { vec3 pos; vec2 uv; vec3 nrm; };
            vector<FaceVert> faceVerts;
            string word;
            while (ss >> word) {
                int vi = 0, ti = 0, ni = 0;
                istringstream ws(word); string idx;
                if (getline(ws, idx, '/')) vi = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx, '/')) ti = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx))      ni = idx.empty() ? 0 : stoi(idx) - 1;

                FaceVert fv;
                fv.pos = (vi >= 0 && vi < (int)positions.size()) ? positions[vi] : vec3(0.f);
                fv.uv  = (ti >= 0 && ti < (int)texCoords.size()) ? texCoords[ti] : vec2(0.f);
                fv.nrm = (ni >= 0 && ni < (int)normals.size())   ? normals[ni]   : vec3(0.f, 1.f, 0.f);
                faceVerts.push_back(fv);
            }
            // Fan triangulation: (0,1,2), (0,2,3), (0,3,4) ...
            auto push = [&](const FaceVert& fv) {
                vBuffer.push_back(fv.pos.x); vBuffer.push_back(fv.pos.y); vBuffer.push_back(fv.pos.z);
                vBuffer.push_back(fv.uv.s);  vBuffer.push_back(fv.uv.t);
                vBuffer.push_back(fv.nrm.x); vBuffer.push_back(fv.nrm.y); vBuffer.push_back(fv.nrm.z);
            };
            for (int k = 1; k + 1 < (int)faceVerts.size(); k++) {
                push(faceVerts[0]);
                push(faceVerts[k]);
                push(faceVerts[k + 1]);
            }
        }
    }
    file.close();

    if (vBuffer.empty()) { cerr << "OBJ vazio: " << filePath << "\n"; return -1; }

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    const GLsizei stride = 8 * sizeof(GLfloat);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)(vBuffer.size() / 8);
    cout << "  " << nVertices << " vertices\n";
    return (int)VAO; 
}
