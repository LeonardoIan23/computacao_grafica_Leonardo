/* Visualizador 3D - Leitura OBJ + MTL + Textura
 *
 * Baseado no projeto de Rossana Baptista Queiroz - Unisinos
 * Estende a função loadSimpleOBJ para incluir:
 *   - Coordenadas de textura (vt) como atributo dos vértices
 *   - Leitura do arquivo .MTL para obter o nome da textura (map_Kd)
 *   - Carregamento da textura via stb_image
 *   - Iluminação Phong
 *   - Controles: rotação (X/Y/Z), translação (WASD/IJ), escala ([/])
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ============================================================
//  Protótipos
// ============================================================
void   key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int    setupShader();
GLuint loadTexture(const string &filePath);
string parseMTL(const string &mtlPath);
GLuint loadSimpleOBJ(const string &filePath, int &nVertices, string &outTexturePath);

// ============================================================
//  Dimensões da janela
// ============================================================
const GLuint WIDTH = 1000, HEIGHT = 1000;

// ============================================================
//  Vertex Shader
//  Atributos: location 0 = posição, 1 = texCoord, 2 = normal
// ============================================================
const GLchar *vertexShaderSource = R"(
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

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position   = projection * view * worldPos;
    fragPos       = vec3(worldPos);
    // Matriz normal correta para escala não uniforme
    fragNormal    = mat3(transpose(inverse(model))) * normalIn;
    texCoord      = texCoordIn;
}
)";

// ============================================================
//  Fragment Shader  (Phong + textura)
// ============================================================
const GLchar *fragmentShaderSource = R"(
#version 450
in vec2 texCoord;
in vec3 fragNormal;
in vec3 fragPos;

uniform sampler2D texBuff;
uniform vec3  lightPos;
uniform vec3  camPos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;

out vec4 color;

void main()
{
    vec3 lightColor  = vec3(1.0);
    vec4 objectColor = texture(texBuff, texCoord);

    // Componente ambiente
    vec3 ambient = ka * lightColor;

    // Componente difusa
    vec3 N    = normalize(fragNormal);
    vec3 L    = normalize(lightPos - fragPos);
    float d   = max(dot(N, L), 0.0);
    vec3 diffuse = kd * d * lightColor;

    // Componente especular (Phong)
    vec3 R    = reflect(-L, N);
    vec3 V    = normalize(camPos - fragPos);
    float s   = pow(max(dot(R, V), 0.0), q);
    vec3 specular = ks * s * lightColor;

    vec3 result = (ambient + diffuse) * vec3(objectColor) + specular;
    color = vec4(result, objectColor.a);
}
)";

// ============================================================
//  Estado de entrada (teclas)
// ============================================================
bool keyW = false, keyA = false, keyS = false, keyD = false;
bool keyI = false, keyJ = false;
bool keyScaleUp = false, keyScaleDown = false;
bool rotX = false, rotY = false, rotZ = false;

float angleX = 0.0f, angleY = 0.0f, angleZ = 0.0f;
vec3  objPos(0.0f);
float objScale = 1.0f;

// ============================================================
//  MAIN
// ============================================================
int main(int argc, char *argv[])
{
    // Caminho do OBJ — pode ser passado como argumento
    // Exemplo: ./programa ../assets/obj/Suzanne.obj
    string objPath = (argc > 1) ? argv[1] : "../assets/obj/Suzanne.obj";

    // ---- GLFW ----
    glfwInit();
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Visualizador OBJ Texturizado", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD\n";
        return -1;
    }

    cout << "Renderer : " << glGetString(GL_RENDERER) << "\n";
    cout << "OpenGL   : " << glGetString(GL_VERSION)  << "\n\n";
    cout << "=== CONTROLES ===\n";
    cout << "X / Y / Z  : Rotacionar no eixo\n";
    cout << "A / D      : Mover eixo X\n";
    cout << "W / S      : Mover eixo Z\n";
    cout << "I / J      : Mover eixo Y\n";
    cout << "[ / ]      : Diminuir / Aumentar escala\n";
    cout << "ESC        : Fechar\n\n";

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    // ---- Compila shaders ----
    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // ---- Carrega OBJ (já lê o MTL internamente) ----
    int    nVertices   = 0;
    string texturePath = "";
    GLuint VAO         = loadSimpleOBJ(objPath, nVertices, texturePath);

    if (VAO == 0) {
        cerr << "Falha ao carregar o arquivo OBJ: " << objPath << "\n";
        glfwTerminate();
        return -1;
    }
    cout << "OBJ carregado: " << nVertices << " vértices\n";

    // ---- Carrega textura (ou cria textura branca de fallback) ----
    GLuint texID = 0;
    if (!texturePath.empty()) {
        texID = loadTexture(texturePath);
        cout << "Textura: " << texturePath << "\n";
    } else {
        // Textura 1×1 branca — o objeto ficará com cor branca
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        unsigned char white[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        cout << "Nenhuma textura encontrada no MTL — usando fallback branco.\n";
    }

    // ---- Uniforms fixos ----
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glUniform1f(glGetUniformLocation(shaderID, "ka"),  0.15f);
    glUniform1f(glGetUniformLocation(shaderID, "kd"),  0.7f);
    glUniform1f(glGetUniformLocation(shaderID, "ks"),  0.5f);
    glUniform1f(glGetUniformLocation(shaderID, "q"),   32.0f);

    vec3 lightPos(2.0f, 4.0f, 3.0f);
    vec3 camPos  (0.0f, 1.0f, 3.5f);
    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "camPos"),   1, value_ptr(camPos));

    // ---- Matrizes câmera / projeção ----
    mat4 view       = lookAt(camPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"),       1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_DEPTH_TEST);

    float lastFrame = 0.0f;

    // ============================================================
    //  Game loop
    // ============================================================
    while (!glfwWindowShouldClose(window))
    {
        float now       = (float)glfwGetTime();
        float deltaTime = now - lastFrame;
        lastFrame       = now;

        glfwPollEvents();

        const float MOVE  = 1.5f;
        const float SCALE = 0.5f;
        const float ROT   = 1.5f;

        if (keyA)         objPos.x  -= MOVE  * deltaTime;
        if (keyD)         objPos.x  += MOVE  * deltaTime;
        if (keyW)         objPos.z  -= MOVE  * deltaTime;
        if (keyS)         objPos.z  += MOVE  * deltaTime;
        if (keyI)         objPos.y  += MOVE  * deltaTime;
        if (keyJ)         objPos.y  -= MOVE  * deltaTime;
        if (keyScaleUp)   objScale   = std::min(objScale + SCALE * deltaTime, 5.0f);
        if (keyScaleDown) objScale   = std::max(objScale - SCALE * deltaTime, 0.05f);
        if (rotX)         angleX    += ROT   * deltaTime;
        if (rotY)         angleY    += ROT   * deltaTime;
        if (rotZ)         angleZ    += ROT   * deltaTime;

        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Monta matriz model: T * Rx * Ry * Rz * S
        mat4 model = mat4(1.0f);
        model = translate(model, objPos);
        model = rotate(model, angleX, vec3(1, 0, 0));
        model = rotate(model, angleY, vec3(0, 1, 0));
        model = rotate(model, angleZ, vec3(0, 0, 1));
        model = scale(model, vec3(objScale));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

        glBindTexture(GL_TEXTURE_2D, texID);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

// ============================================================
//  key_callback
// ============================================================
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Rotação — toggle por eixo
    if (key == GLFW_KEY_X && action == GLFW_PRESS) { rotX = !rotX; rotY = false; rotZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { rotX = false; rotY = !rotY; rotZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { rotX = false; rotY = false; rotZ = !rotZ; }

    // Movimento contínuo
    if (key == GLFW_KEY_A) keyA = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_D) keyD = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_W) keyW = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_S) keyS = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_I) keyI = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_J) keyJ = (action != GLFW_RELEASE);

    if (key == GLFW_KEY_RIGHT_BRACKET) keyScaleUp   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_LEFT_BRACKET)  keyScaleDown = (action != GLFW_RELEASE);
}

// ============================================================
//  setupShader
// ============================================================
int setupShader()
{
    auto compile = [](GLenum type, const GLchar *src) -> GLuint {
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
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; GLchar log[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, NULL, log); cerr << "Link error:\n" << log << "\n"; }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ============================================================
//  loadTexture  (stb_image)
// ============================================================
GLuint loadTexture(const string &filePath)
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
    unsigned char *data = stbi_load(filePath.c_str(), &w, &h, &ch, 0);
    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        cerr << "Falha ao carregar textura: " << filePath << "\n";
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

// ============================================================
//  parseMTL
//  Lê um arquivo .mtl e retorna o caminho de map_Kd (textura difusa).
//  O caminho retornado é relativo ao diretório do próprio .mtl.
// ============================================================
string parseMTL(const string &mtlPath)
{
    ifstream file(mtlPath);
    if (!file.is_open()) {
        cerr << "Aviso: não foi possível abrir o MTL: " << mtlPath << "\n";
        return "";
    }

    // Diretório base do .mtl para resolver caminhos relativos
    string baseDir = "";
    size_t slash = mtlPath.find_last_of("/\\");
    if (slash != string::npos)
        baseDir = mtlPath.substr(0, slash + 1);

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "map_Kd") {
            string texName;
            ss >> texName;
            return baseDir + texName;   // retorna caminho completo
        }
    }
    return "";  // MTL não tem map_Kd
}

// ============================================================
//  loadSimpleOBJ
//
//  Lê um arquivo .OBJ e monta um VAO com 3 atributos por vértice:
//    location 0 : posição  (x, y, z)        — 3 floats
//    location 1 : texCoord (s, t)            — 2 floats
//    location 2 : normal   (nx, ny, nz)      — 3 floats
//
//  Stride total: 8 floats por vértice.
//
//  Se o OBJ referenciar um arquivo .MTL com map_Kd, o caminho
//  da textura é devolvido em outTexturePath.
//
//  Retorna o identificador do VAO (ou 0 em caso de erro).
// ============================================================
GLuint loadSimpleOBJ(const string &filePath, int &nVertices, string &outTexturePath)
{
    // ---- Listas temporárias de dados brutos ----
    vector<vec3> positions;
    vector<vec2> texCoords;
    vector<vec3> normals;

    // Buffer final que vai para o VBO (posição + uv + normal = 8 floats)
    vector<GLfloat> vBuffer;

    // Diretório base do OBJ (para localizar o MTL)
    string baseDir = "";
    size_t slash = filePath.find_last_of("/\\");
    if (slash != string::npos)
        baseDir = filePath.substr(0, slash + 1);

    outTexturePath = "";

    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Erro ao abrir o arquivo OBJ: " << filePath << "\n";
        return 0;
    }

    string line;
    while (getline(file, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;

        // ---- Vértice de posição ----
        if (token == "v") {
            vec3 v;
            ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        // ---- Coordenada de textura ----
        else if (token == "vt") {
            vec2 vt;
            ss >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        // ---- Normal ----
        else if (token == "vn") {
            vec3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        // ---- Referência ao arquivo MTL ----
        else if (token == "mtllib") {
            string mtlFile;
            ss >> mtlFile;
            string mtlPath = baseDir + mtlFile;
            outTexturePath = parseMTL(mtlPath);
        }
        // ---- Face (triângulo: v/vt/vn  v/vt/vn  v/vt/vn) ----
        else if (token == "f")
        {
            // Cada palavra da linha é um índice "vi/ti/ni"
            // O OBJ pode ter faces com mais de 3 vértices (polígonos)
            // mas aqui assumimos apenas triângulos (exportar com triangulate no Blender)
            string word;
            while (ss >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                istringstream ws(word);
                string idx;

                // Extrai índice de posição
                if (getline(ws, idx, '/')) vi = idx.empty() ? 0 : stoi(idx) - 1;
                // Extrai índice de texCoord (pode ser vazio: v//vn)
                if (getline(ws, idx, '/')) ti = idx.empty() ? 0 : stoi(idx) - 1;
                // Extrai índice de normal
                if (getline(ws, idx))      ni = idx.empty() ? 0 : stoi(idx) - 1;

                // Posição
                vec3 pos = (vi >= 0 && vi < (int)positions.size()) ? positions[vi] : vec3(0.0f);
                vBuffer.push_back(pos.x);
                vBuffer.push_back(pos.y);
                vBuffer.push_back(pos.z);

                // Coordenada de textura
                // Se não houver vt no arquivo, usa (0,0)
                vec2 uv = (ti >= 0 && ti < (int)texCoords.size()) ? texCoords[ti] : vec2(0.0f);
                vBuffer.push_back(uv.s);
                vBuffer.push_back(uv.t);

                // Normal
                // Se não houver vn, usa (0,1,0) como fallback
                vec3 nrm = (ni >= 0 && ni < (int)normals.size()) ? normals[ni] : vec3(0.0f, 1.0f, 0.0f);
                vBuffer.push_back(nrm.x);
                vBuffer.push_back(nrm.y);
                vBuffer.push_back(nrm.z);
            }
        }
    }

    file.close();

    if (vBuffer.empty()) {
        cerr << "OBJ vazio ou sem faces: " << filePath << "\n";
        return 0;
    }

    // ============================================================
    //  Monta VBO e VAO
    // ============================================================
    cout << "Gerando VAO/VBO...\n";

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Stride = 8 floats por vértice (pos:3 + uv:2 + normal:3)
    const GLsizei stride = 8 * sizeof(GLfloat);

    // location 0: posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // location 1: coordenada de textura (s, t)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // location 2: normal (nx, ny, nz)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 8 valores por vértice: x y z  s t  nx ny nz
    nVertices = (int)(vBuffer.size() / 8);

    return VAO;
}