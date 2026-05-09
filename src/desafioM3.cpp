/* Desafio Modulo 3 - Visualizador OBJ com Texturas
 * Extende a atividade vivencial M2 adicionando:
 *   - Coordenadas de textura (vt) como atributo dos vertices
 *   - Leitura do arquivo .MTL para obter o nome da textura (map_Kd)
 *   - Carregamento de textura via stb_image
 *   - Iluminacao Phong
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

// Prototipos
void   key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int    setupShader();
GLuint loadTexture(const string& filePath);
string parseMTL(const string& mtlPath);
int    loadSimpleOBJ(const string& filePath, int& nVertices, string& outTexturePath);

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
//  Fragment Shader - Phong + textura + highlight de selecao
// ============================================================
const GLchar* fragmentShaderSource = R"(
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
uniform float highlight;

out vec4 color;

void main() {
    vec3  lightColor  = vec3(1.0);
    vec4  objectColor = texture(texBuff, texCoord);

    // Ambiente
    vec3 ambient = ka * lightColor;

    // Difusa
    vec3  N       = normalize(fragNormal);
    vec3  L       = normalize(lightPos - fragPos);
    float diff    = max(dot(N, L), 0.0);
    vec3  diffuse = kd * diff * lightColor;

    // Especular (Phong)
    vec3  R       = reflect(-L, N);
    vec3  V       = normalize(camPos - fragPos);
    float spec    = pow(max(dot(R, V), 0.0), q);
    vec3  specular = ks * spec * lightColor;

    vec3 result = (ambient + diffuse) * vec3(objectColor) + specular;
    color = vec4(result * highlight, objectColor.a);
}
)";

// ============================================================
//  Struct do objeto 3D
// ============================================================
struct Object3D {
    GLuint VAO;
    GLuint textureID;
    int    nVertices;
    string name;
    vec3   position;
    float  scale;
    float  rotAngleX, rotAngleY, rotAngleZ;
    bool   rotateX,   rotateY,   rotateZ;
};

int              selectedObj = 0;
vector<Object3D> objects;

// Flags de teclas mantidas pressionadas
bool keyW = false, keyA = false, keyS_mv = false, keyD = false;
bool keyI = false, keyJ = false;
bool keyScaleUp = false, keyScaleDown = false;

// ============================================================
//  Helper: cria textura branca 1x1 como fallback
// ============================================================
static GLuint whiteFallbackTexture()
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    unsigned char white[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

// ============================================================
//  Helper: tenta carregar um objeto e adiciona ao vector
// ============================================================
static void addObject(const string& objPath, const string& name, vec3 position, float scale)
{
    int    nVerts = 0;
    string texPath;
    int    vao = loadSimpleOBJ(objPath, nVerts, texPath);

    if (vao == -1) {
        cerr << "Falha ao carregar: " << objPath << "\n";
        return;
    }

    Object3D obj;
    obj.VAO       = (GLuint)vao;
    obj.nVertices = nVerts;
    obj.name      = name;
    obj.position  = position;
    obj.scale     = scale;
    obj.rotAngleX = obj.rotAngleY = obj.rotAngleZ = 0.0f;
    obj.rotateX   = obj.rotateY   = obj.rotateZ   = false;

    if (!texPath.empty())
        obj.textureID = loadTexture(texPath);
    else {
        obj.textureID = whiteFallbackTexture();
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
        "Desafio M3 - Visualizador OBJ Texturizado", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD\n";
        return -1;
    }

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // Locations dos uniforms
    GLint modelLoc     = glGetUniformLocation(shaderID, "model");
    GLint viewLoc      = glGetUniformLocation(shaderID, "view");
    GLint projLoc      = glGetUniformLocation(shaderID, "projection");
    GLint highlightLoc = glGetUniformLocation(shaderID, "highlight");

    // Projecao perspectiva
    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));

    // Camera
    vec3 camPos(0.0f, 1.0f, 5.0f);
    mat4 view = lookAt(camPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));

    // Iluminacao Phong
    vec3 lightPos(3.0f, 5.0f, 4.0f);
    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "camPos"),   1, value_ptr(camPos));
    glUniform1f (glGetUniformLocation(shaderID, "ka"),  0.2f);
    glUniform1f (glGetUniformLocation(shaderID, "kd"),  0.7f);
    glUniform1f (glGetUniformLocation(shaderID, "ks"),  0.5f);
    glUniform1f (glGetUniformLocation(shaderID, "q"),   32.0f);
    glUniform1i (glGetUniformLocation(shaderID, "texBuff"), 0);

    glEnable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    // --- Carregar modelos ---
    addObject("../assets/Modelos3D/Suzanne.obj",        "Suzanne",        vec3(-2.5f, 0.0f, 0.0f), 1.0f);
    addObject("../assets/Modelos3D/SuzanneSubdiv1.obj", "SuzanneSubdiv1", vec3( 0.0f, 0.0f, 0.0f), 1.0f);
    addObject("../assets/Modelos3D/Cube.obj",           "Cube",           vec3( 2.5f, 0.0f, 0.0f), 1.0f);
    objects.back().textureID = loadTexture("../assets/Modelos3D/Suzanne.png");

    if (objects.empty()) {
        cerr << "Nenhum modelo carregado. Verifique os caminhos dos arquivos .obj\n";
        glfwTerminate();
        return -1;
    }

    cout << "\n=== CONTROLES ===\n";
    cout << "TAB     : Selecionar proximo objeto\n";
    cout << "X/Y/Z   : Toggle rotacao continua no eixo\n";
    cout << "W/S     : Transladar eixo Z\n";
    cout << "A/D     : Transladar eixo X\n";
    cout << "I/J     : Transladar eixo Y\n";
    cout << "[ / ]   : Diminuir / Aumentar escala\n";
    cout << "ESC     : Fechar\n";
    cout << "\nSelecionado: " << objects[selectedObj].name
         << "  (brilhante = selecionado)\n\n";

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

        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int i = 0; i < (int)objects.size(); i++)
        {
            Object3D& obj = objects[i];

            // Acumula angulo de rotacao
            if (obj.rotateX) obj.rotAngleX += dt;
            if (obj.rotateY) obj.rotAngleY += dt;
            if (obj.rotateZ) obj.rotAngleZ += dt;

            // Matriz model: T * Rx * Ry * Rz * S
            mat4 model = mat4(1.0f);
            model = translate(model, obj.position);
            model = rotate(model, obj.rotAngleX, vec3(1.0f, 0.0f, 0.0f));
            model = rotate(model, obj.rotAngleY, vec3(0.0f, 1.0f, 0.0f));
            model = rotate(model, obj.rotAngleZ, vec3(0.0f, 0.0f, 1.0f));
            model = scale(model, vec3(obj.scale));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

            // Objeto selecionado aparece mais brilhante
            glUniform1f(highlightLoc, (i == selectedObj) ? 1.0f : 0.55f);

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

    // Ciclar selecao com TAB
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        selectedObj = (selectedObj + 1) % (int)objects.size();
        cout << "Selecionado: " << objects[selectedObj].name << "\n";
    }

    // Toggle rotacao por eixo (exclusivos entre si)
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
        cout << "  Textura carregada: " << filePath << " (" << w << "x" << h << ")\n";
    } else {
        cerr << "  Falha ao carregar textura: " << filePath << "\n";
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

// ============================================================
//  parseMTL
//  Le o arquivo .mtl e retorna o caminho de map_Kd (textura difusa).
// ============================================================
string parseMTL(const string& mtlPath)
{
    ifstream file(mtlPath);
    if (!file.is_open()) {
        cerr << "  Aviso: nao foi possivel abrir MTL: " << mtlPath << "\n";
        return "";
    }

    // Diretorio base do .mtl para resolver caminhos relativos
    string baseDir;
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
            return baseDir + texName;
        }
    }
    return "";
}

// ============================================================
//  loadSimpleOBJ
//
//  Le um arquivo .OBJ e monta um VAO com 3 atributos por vertice:
//    location 0 : posicao  (x, y, z)    - 3 floats
//    location 1 : texCoord (s, t)        - 2 floats
//    location 2 : normal   (nx, ny, nz)  - 3 floats
//  Stride total: 8 floats por vertice
//
//  Se o OBJ referenciar um .MTL com map_Kd, devolve o caminho
//  da textura em outTexturePath.
//
//  Retorna o VAO (int) ou -1 em caso de erro.
// ============================================================
int loadSimpleOBJ(const string& filePath, int& nVertices, string& outTexturePath)
{
    vector<vec3>    positions;
    vector<vec2>    texCoords;
    vector<vec3>    normals;
    vector<GLfloat> vBuffer;

    // Diretorio base do OBJ para localizar o MTL
    string baseDir;
    size_t slash = filePath.find_last_of("/\\");
    if (slash != string::npos)
        baseDir = filePath.substr(0, slash + 1);

    outTexturePath = "";

    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Erro ao abrir OBJ: " << filePath << "\n";
        return -1;
    }

    cout << "Carregando: " << filePath << "\n";

    string line;
    while (getline(file, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "v") {
            vec3 v; ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (token == "vt") {
            vec2 vt; ss >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (token == "vn") {
            vec3 vn; ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (token == "mtllib") {
            string mtlFile; ss >> mtlFile;
            outTexturePath = parseMTL(baseDir + mtlFile);
        }
        else if (token == "f") {
            string word;
            while (ss >> word) {
                int vi = 0, ti = 0, ni = 0;
                istringstream ws(word);
                string idx;

                if (getline(ws, idx, '/')) vi = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx, '/')) ti = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx))      ni = idx.empty() ? 0 : stoi(idx) - 1;

                // Posicao
                vec3 pos = (vi >= 0 && vi < (int)positions.size()) ? positions[vi] : vec3(0.0f);
                vBuffer.push_back(pos.x);
                vBuffer.push_back(pos.y);
                vBuffer.push_back(pos.z);

                // Coordenada de textura (vt) - fallback (0,0) se nao houver
                vec2 uv = (ti >= 0 && ti < (int)texCoords.size()) ? texCoords[ti] : vec2(0.0f);
                vBuffer.push_back(uv.s);
                vBuffer.push_back(uv.t);

                // Normal - fallback (0,1,0) se nao houver
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
        return -1;
    }

    // Monta VBO e VAO
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    const GLsizei stride = 8 * sizeof(GLfloat); // pos:3 + uv:2 + normal:3

    // location 0: posicao (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // location 1: texCoord (s, t)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // location 2: normal (nx, ny, nz)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)(vBuffer.size() / 8);
    cout << "  " << nVertices << " vertices carregados\n";
    return (int)VAO;
}
