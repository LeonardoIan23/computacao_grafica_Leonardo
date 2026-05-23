/* Atividade Vivencial M4 - Iluminacao de Tres Pontos (Three-Point Lighting)
 * Extende o desafioM4 adicionando:
 *   - 3 fontes de luz pontuais: Key Light, Fill Light e Back Light
 *   - Posicionamento automatico das luzes a partir da posicao e escala do objeto principal
 *   - Fator de atenuacao (constante, linear e quadratico) na componente difusa
 *   - Toggle individual de cada luz via teclas 1, 2, 3
 *
 * Controles:
 *   TAB        : Selecionar proximo objeto
 *   WASD       : Transladar objeto selecionado (X e Z)
 *   I / J      : Transladar objeto selecionado (Y)
 *   X / Y / Z  : Toggle rotacao continua no eixo
 *   [ / ]      : Diminuir / Aumentar escala
 *   1          : Ligar/Desligar Key Light  (luz principal)
 *   2          : Ligar/Desligar Fill Light (luz de preenchimento)
 *   3          : Ligar/Desligar Back Light (luz de fundo)
 *   ESC        : Fechar
 *
 * Leonardo Ian de Oliveira
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ============================================================
//  Structs
// ============================================================
struct Material {
    vec3  Ka = vec3(0.2f);
    vec3  Kd = vec3(0.8f);
    vec3  Ks = vec3(0.5f);
    float Ns = 32.0f;
    string texturePath;
};

struct Object3D {
    GLuint   VAO;
    GLuint   textureID;
    int      nVertices;
    string   name;
    Material mat;
    vec3     position;
    float    scale;
    float    rotAngleX, rotAngleY, rotAngleZ;
    bool     rotateX,   rotateY,   rotateZ;
};

// ============================================================
//  Prototipos
// ============================================================
void     key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int      setupShader();
GLuint   loadTexture(const string& filePath);
Material parseMTL(const string& mtlPath);
int      loadSimpleOBJ(const string& filePath, int& nVertices, Material& outMat);
void     updateLights(GLuint shaderID, const Object3D& mainObj);

const GLuint WIDTH = 1000, HEIGHT = 1000;

// ============================================================
//  Vertex Shader
//  layout 0: posicao (x,y,z)
//  layout 1: texCoord (s,t)
//  layout 2: normal (nx,ny,nz)
// ============================================================
const GLchar* vertexShaderSource = R"(
#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoordIn;
layout (location = 2) in vec3 normalIn;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 texCoord;
out vec3 fragNormal;
out vec3 fragPos;

void main() {
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position   = projection * view * worldPos;
    fragPos       = vec3(worldPos);
    fragNormal    = mat3(transpose(inverse(model))) * normalIn;
    texCoord      = texCoordIn;
}
)";

// ============================================================
//  Fragment Shader - Phong com 3 luzes pontuais e atenuacao
//
//  Para cada luz habilitada:
//    att      = 1 / (Kc + Kl*d + Kq*d^2)
//    difusa   = Kd * max(dot(N,L), 0) * texColor * lightColor * att
//    especular= Ks * pow(max(dot(R,V), 0), Ns) * lightColor
//
//  Ambiente: calculado uma vez (independente das luzes)
// ============================================================
const GLchar* fragmentShaderSource = R"(
#version 450
in vec2 texCoord;
in vec3 fragNormal;
in vec3 fragPos;

uniform sampler2D texBuff;
uniform vec3  camPos;

// Material (lido do .MTL)
uniform vec3  Ka;
uniform vec3  Kd;
uniform vec3  Ks;
uniform float Ns;

// Tres fontes de luz pontuais
uniform vec3 lightPos[3];    // posicao de cada luz
uniform vec3 lightColor[3];  // cor/intensidade de cada luz
uniform int  lightOn[3];     // 1 = ligada, 0 = desligada

// Coeficientes de atenuacao
uniform float Kc;  // constante
uniform float Kl;  // linear
uniform float Kq;  // quadratico

uniform float highlight;

out vec4 color;

void main() {
    vec4 texColor = texture(texBuff, texCoord);
    vec3 texRGB   = vec3(texColor);

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camPos - fragPos);

    // Componente ambiente (uma vez, independente das luzes)
    vec3 result = Ka * texRGB;

    // Contribuicao de cada luz pontual
    for (int i = 0; i < 3; i++) {
        if (lightOn[i] == 0) continue;

        vec3  L    = normalize(lightPos[i] - fragPos);
        float dist = length(lightPos[i] - fragPos);

        // Fator de atenuacao
        float att = 1.0 / (Kc + Kl * dist + Kq * dist * dist);

        // Componente difusa (com atenuacao)
        float diff    = max(dot(N, L), 0.0);
        vec3  diffuse = Kd * diff * texRGB * lightColor[i] * att;

        // Componente especular (Phong)
        vec3  R    = reflect(-L, N);
        float spec = pow(max(dot(R, V), 0.0), max(Ns, 1.0));
        vec3  specular = Ks * spec * lightColor[i];

        result += diffuse + specular;
    }

    color = vec4(result * highlight, texColor.a);
}
)";

// ============================================================
//  Estado global
// ============================================================
int              selectedObj = 0;
vector<Object3D> objects;

// Toggle das 3 luzes (true = ligada)
bool lightEnabled[3] = {true, true, true};

// Flags de teclas mantidas pressionadas
bool keyW = false, keyA = false, keyS_mv = false, keyD = false;
bool keyI = false, keyJ = false;
bool keyScaleUp = false, keyScaleDown = false;

// ============================================================
//  Calcula e envia as posicoes das 3 luzes ao shader
//  a partir da posicao e escala do objeto principal (objects[0])
//
//  Configuracao Three-Point Lighting:
//    Key Light  : frente-esquerda, acima     — mais intensa
//    Fill Light : frente-direita,  levemente acima — mais suave
//    Back Light : atras,           acima     — separa objeto do fundo
// ============================================================
void updateLights(GLuint shaderID, const Object3D& mainObj)
{
    vec3  P = mainObj.position;
    float S = mainObj.scale;

    // Posicoes relativas ao objeto principal (escalonadas pelo tamanho dele)
    vec3 positions[3] = {
        P + vec3(-2.0f,  3.0f,  2.5f) * S,   // Key  Light: esq, cima, frente
        P + vec3( 2.5f,  1.5f,  2.0f) * S,   // Fill Light: dir, leve cima, frente
        P + vec3( 0.0f,  2.5f, -3.0f) * S,   // Back Light: centro, cima, atras
    };

    // Cores/intensidades calibradas por funcao:
    //   Key  → branco quente brilhante  (luz principal)
    //   Fill → branco neutro suave      (preenche sombras, ~40% da key)
    //   Back → azulado medio            (separa objeto do fundo)
    vec3 colors[3] = {
        vec3(1.0f, 0.95f, 0.85f),   // Key  — branco levemente quente
        vec3(0.4f, 0.42f, 0.45f),   // Fill — branco suave/neutro
        vec3(0.5f, 0.55f, 0.8f),    // Back — azulado frio
    };

    // Aplica toggle (zera a cor das luzes desligadas)
    vec3 effectiveColors[3];
    int  onFlags[3];
    for (int i = 0; i < 3; i++) {
        effectiveColors[i] = lightEnabled[i] ? colors[i] : vec3(0.0f);
        onFlags[i]         = lightEnabled[i] ? 1 : 0;
    }

    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"),   3, &positions[0].x);
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 3, &effectiveColors[0].x);
    glUniform1iv(glGetUniformLocation(shaderID, "lightOn"),    3, onFlags);
}

// ============================================================
//  Helper: textura branca 1x1 como fallback
// ============================================================
static GLuint whiteFallback()
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    unsigned char white[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

// ============================================================
//  Helper: carrega OBJ e adiciona ao vector
// ============================================================
static void addObject(const string& objPath, const string& name,
                      vec3 position, float scale)
{
    Material mat;
    int nVerts = 0;
    int vao = loadSimpleOBJ(objPath, nVerts, mat);
    if (vao == -1) { cerr << "Falha ao carregar: " << objPath << "\n"; return; }

    Object3D obj;
    obj.VAO       = (GLuint)vao;
    obj.nVertices = nVerts;
    obj.name      = name;
    obj.mat       = mat;
    obj.position  = position;
    obj.scale     = scale;
    obj.rotAngleX = obj.rotAngleY = obj.rotAngleZ = 0.0f;
    obj.rotateX   = obj.rotateY   = obj.rotateZ   = false;

    if (!mat.texturePath.empty())
        obj.textureID = loadTexture(mat.texturePath);
    else {
        obj.textureID = whiteFallback();
        cout << "  Sem textura no MTL - usando fallback branco\n";
    }
    objects.push_back(obj);
}

// ============================================================
//  MAIN
// ============================================================
int main()
{
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Atividade Vivencial M4 - Three-Point Lighting", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD\n"; return -1;
    }

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // Uniforms
    GLint modelLoc     = glGetUniformLocation(shaderID, "model");
    GLint viewLoc      = glGetUniformLocation(shaderID, "view");
    GLint projLoc      = glGetUniformLocation(shaderID, "projection");
    GLint highlightLoc = glGetUniformLocation(shaderID, "highlight");
    GLint KaLoc        = glGetUniformLocation(shaderID, "Ka");
    GLint KdLoc        = glGetUniformLocation(shaderID, "Kd");
    GLint KsLoc        = glGetUniformLocation(shaderID, "Ks");
    GLint NsLoc        = glGetUniformLocation(shaderID, "Ns");

    // Projecao e camera (fixas)
    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));

    vec3 camPos(0.0f, 1.0f, 7.0f);
    mat4 view = lookAt(camPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));

    glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1, value_ptr(camPos));
    glUniform1i (glGetUniformLocation(shaderID, "texBuff"), 0);

    // Coeficientes de atenuacao
    glUniform1f(glGetUniformLocation(shaderID, "Kc"), 1.0f);
    glUniform1f(glGetUniformLocation(shaderID, "Kl"), 0.05f);
    glUniform1f(glGetUniformLocation(shaderID, "Kq"), 0.01f);

    glEnable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Carregar modelos
    addObject("../assets/Modelos3D/Suzanne.obj",        "Suzanne",        vec3( 0.0f, 0.0f, 0.0f), 1.0f);
    addObject("../assets/Modelos3D/SuzanneSubdiv1.obj", "SuzanneSubdiv1", vec3(-2.8f, 0.0f, 0.0f), 1.0f);
    addObject("../assets/Modelos3D/Cube.obj",           "Cube",           vec3( 2.8f, 0.0f, 0.0f), 1.0f);
    objects.back().textureID = loadTexture("../assets/Modelos3D/Suzanne.png");

    if (objects.empty()) {
        cerr << "Nenhum modelo carregado.\n"; glfwTerminate(); return -1;
    }

    cout << "\n=== CONTROLES ===\n";
    cout << "TAB      : Selecionar proximo objeto\n";
    cout << "WASD     : Transladar objeto selecionado (X/Z)\n";
    cout << "I / J    : Transladar objeto selecionado (Y)\n";
    cout << "X/Y/Z    : Toggle rotacao continua no eixo\n";
    cout << "[ / ]    : Diminuir / Aumentar escala\n";
    cout << "1        : Ligar/Desligar Key Light  (luz principal)\n";
    cout << "2        : Ligar/Desligar Fill Light (luz de preenchimento)\n";
    cout << "3        : Ligar/Desligar Back Light (luz de fundo)\n";
    cout << "ESC      : Fechar\n";
    cout << "\nObjeto principal (luzes orbitam): " << objects[0].name << "\n";
    cout << "Selecionado: " << objects[selectedObj].name << "\n\n";

    const float MOVE_SPEED  = 2.5f;
    const float SCALE_SPEED = 1.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt  = now - lastFrame;
        lastFrame = now;

        glfwPollEvents();

        // Transformacoes no objeto selecionado
        Object3D& sel = objects[selectedObj];
        if (keyA)         sel.position.x -= MOVE_SPEED * dt;
        if (keyD)         sel.position.x += MOVE_SPEED * dt;
        if (keyW)         sel.position.z -= MOVE_SPEED * dt;
        if (keyS_mv)      sel.position.z += MOVE_SPEED * dt;
        if (keyI)         sel.position.y += MOVE_SPEED * dt;
        if (keyJ)         sel.position.y -= MOVE_SPEED * dt;
        if (keyScaleUp)   sel.scale = std::min(sel.scale + SCALE_SPEED * dt, 5.0f);
        if (keyScaleDown) sel.scale = std::max(sel.scale - SCALE_SPEED * dt, 0.1f);

        // Atualiza posicoes das luzes a partir do objeto principal (objects[0])
        updateLights(shaderID, objects[0]);

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int i = 0; i < (int)objects.size(); i++)
        {
            Object3D& obj = objects[i];

            if (obj.rotateX) obj.rotAngleX += dt;
            if (obj.rotateY) obj.rotAngleY += dt;
            if (obj.rotateZ) obj.rotAngleZ += dt;

            mat4 model = mat4(1.0f);
            model = translate(model, obj.position);
            model = rotate(model, obj.rotAngleX, vec3(1.0f, 0.0f, 0.0f));
            model = rotate(model, obj.rotAngleY, vec3(0.0f, 1.0f, 0.0f));
            model = rotate(model, obj.rotAngleZ, vec3(0.0f, 0.0f, 1.0f));
            model = scale(model, vec3(obj.scale));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

            glUniform3fv(KaLoc, 1, value_ptr(obj.mat.Ka));
            glUniform3fv(KdLoc, 1, value_ptr(obj.mat.Kd));
            glUniform3fv(KsLoc, 1, value_ptr(obj.mat.Ks));
            glUniform1f (NsLoc, obj.mat.Ns);
            glUniform1f (highlightLoc, (i == selectedObj) ? 1.0f : 0.85f);

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

// ============================================================
//  key_callback
// ============================================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Toggle das luzes com feedback no console
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        lightEnabled[0] = !lightEnabled[0];
        cout << "Key Light  (1): " << (lightEnabled[0] ? "LIGADA" : "DESLIGADA") << "\n";
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        lightEnabled[1] = !lightEnabled[1];
        cout << "Fill Light (2): " << (lightEnabled[1] ? "LIGADA" : "DESLIGADA") << "\n";
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        lightEnabled[2] = !lightEnabled[2];
        cout << "Back Light (3): " << (lightEnabled[2] ? "LIGADA" : "DESLIGADA") << "\n";
    }

    // Selecao de objeto
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        selectedObj = (selectedObj + 1) % (int)objects.size();
        cout << "Selecionado: " << objects[selectedObj].name << "\n";
    }

    // Rotacao do objeto selecionado
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

    if (key == GLFW_KEY_A)             keyA        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_D)             keyD        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_W)             keyW        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_S)             keyS_mv     = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_I)             keyI        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_J)             keyJ        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_RIGHT_BRACKET) keyScaleUp   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_LEFT_BRACKET)  keyScaleDown = (action != GLFW_RELEASE);
}

// ============================================================
//  setupShader
// ============================================================
int setupShader()
{
    auto compile = [](GLenum type, const GLchar* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, NULL);
        glCompileShader(s);
        GLint ok; GLchar log[512];
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { glGetShaderInfoLog(s, 512, NULL, log); cerr << "Shader error:\n" << log << "\n"; }
        return s;
    };
    GLuint vs   = compile(GL_VERTEX_SHADER,   vertexShaderSource);
    GLuint fs   = compile(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; GLchar log[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, NULL, log); cerr << "Link error:\n" << log << "\n"; }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// ============================================================
//  loadTexture
// ============================================================
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
        cout << "  Textura carregada: " << filePath << "\n";
    } else { cerr << "  Falha ao carregar textura: " << filePath << "\n"; }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

// ============================================================
//  parseMTL - extrai Ka, Kd, Ks, Ns e map_Kd
// ============================================================
Material parseMTL(const string& mtlPath)
{
    Material mat;
    ifstream file(mtlPath);
    if (!file.is_open()) {
        cerr << "  Aviso: nao foi possivel abrir MTL: " << mtlPath << "\n";
        return mat;
    }
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
    cout << "  MTL: Ka(" << mat.Ka.r << ") Ks(" << mat.Ks.r << ") Ns(" << mat.Ns << ")\n";
    return mat;
}

// ============================================================
//  loadSimpleOBJ
//  Stride: pos(3) + uv(2) + normal(3) = 8 floats por vertice
// ============================================================
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
        if (t == "v") {
            vec3 v; ss >> v.x >> v.y >> v.z; positions.push_back(v);
        } else if (t == "vt") {
            vec2 vt; ss >> vt.s >> vt.t; texCoords.push_back(vt);
        } else if (t == "vn") {
            vec3 vn; ss >> vn.x >> vn.y >> vn.z; normals.push_back(vn);
        } else if (t == "mtllib") {
            string mtlFile; ss >> mtlFile; outMat = parseMTL(baseDir + mtlFile);
        } else if (t == "f") {
            string word;
            while (ss >> word) {
                int vi = 0, ti = 0, ni = 0;
                istringstream ws(word); string idx;
                if (getline(ws, idx, '/')) vi = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx, '/')) ti = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx))      ni = idx.empty() ? 0 : stoi(idx) - 1;

                vec3 pos = (vi >= 0 && vi < (int)positions.size()) ? positions[vi] : vec3(0.0f);
                vec2 uv  = (ti >= 0 && ti < (int)texCoords.size()) ? texCoords[ti] : vec2(0.0f);
                vec3 nrm = (ni >= 0 && ni < (int)normals.size())   ? normals[ni]   : vec3(0.0f,1.0f,0.0f);

                vBuffer.push_back(pos.x); vBuffer.push_back(pos.y); vBuffer.push_back(pos.z);
                vBuffer.push_back(uv.s);  vBuffer.push_back(uv.t);
                vBuffer.push_back(nrm.x); vBuffer.push_back(nrm.y); vBuffer.push_back(nrm.z);
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
    cout << "  " << nVertices << " vertices carregados\n";
    return (int)VAO;
}
