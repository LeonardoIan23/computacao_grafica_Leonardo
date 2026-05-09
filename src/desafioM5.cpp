/* Desafio Modulo 5 - Camera em Primeira Pessoa
 * Extende o desafioM4 adicionando:
 *   - Classe Camera com atributos proprios (posicao, direcao, yaw, pitch)
 *   - Metodos de movimento (moveForward, moveBack, moveLeft, moveRight)
 *   - Metodo de rotacao via mouse (rotate)
 *   - Mouse capturado pela janela para controle da camera
 *
 * Controles:
 *   Mouse      : Girar camera (yaw / pitch)
 *   W/A/S/D    : Mover camera (frente / esquerda / tras / direita)
 *   Espaço / C : Mover camera cima / baixo
 *   TAB        : Selecionar proximo objeto
 *   Setas      : Transladar objeto selecionado (X e Z)
 *   PgUp/PgDn  : Transladar objeto selecionado (Y)
 *   X / Y / Z  : Toggle rotacao continua no eixo
 *   [ / ]      : Diminuir / Aumentar escala
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

// ============================================================
//  Classe Camera (primeira pessoa)
// ============================================================
class Camera
{
public:
    vec3  position;
    vec3  front;
    vec3  up;
    vec3  right;
    vec3  worldUp;

    float yaw;         // angulo horizontal (graus)
    float pitch;       // angulo vertical   (graus)
    float speed;       // unidades por segundo
    float sensitivity; // graus por pixel do mouse

    Camera(vec3  startPos  = vec3(0.0f, 1.0f, 7.0f),
           vec3  worldUp   = vec3(0.0f, 1.0f, 0.0f),
           float startYaw  = -90.0f,  // aponta para -Z (cena)
           float startPitch = 0.0f)
        : position(startPos)
        , worldUp(worldUp)
        , yaw(startYaw)
        , pitch(startPitch)
        , speed(5.0f)
        , sensitivity(0.1f)
    {
        updateVectors();
    }

    // Retorna a view matrix (lookAt a partir da posicao e direcao atuais)
    mat4 getViewMatrix() const
    {
        return lookAt(position, position + front, up);
    }

    // Movimentos cardinais - chamados no game loop com deltaTime
    void moveForward(float dt) { position += front  * speed * dt; }
    void moveBack   (float dt) { position -= front  * speed * dt; }
    void moveLeft   (float dt) { position -= right  * speed * dt; }
    void moveRight  (float dt) { position += right  * speed * dt; }
    void moveUp     (float dt) { position += worldUp * speed * dt; }
    void moveDown   (float dt) { position -= worldUp * speed * dt; }

    // Rotacao pela diferenca do mouse (xOff = delta horizontal, yOff = delta vertical)
    void rotate(float xOff, float yOff)
    {
        yaw   += xOff * sensitivity;
        pitch += yOff * sensitivity;

        // Clamp no pitch para evitar inversao da camera
        pitch = std::max(-89.0f, std::min(89.0f, pitch));

        updateVectors();
    }

private:
    // Recalcula front, right e up a partir de yaw e pitch
    void updateVectors()
    {
        vec3 f;
        f.x   = cos(radians(yaw)) * cos(radians(pitch));
        f.y   = sin(radians(pitch));
        f.z   = sin(radians(yaw)) * cos(radians(pitch));
        front = normalize(f);
        right = normalize(cross(front, worldUp));
        up    = normalize(cross(right, front));
    }
};

// ============================================================
//  Structs de material e objeto 3D
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
void     cursor_callback(GLFWwindow* window, double xpos, double ypos);
int      setupShader();
GLuint   loadTexture(const string& filePath);
Material parseMTL(const string& mtlPath);
int      loadSimpleOBJ(const string& filePath, int& nVertices, Material& outMat);

const GLuint WIDTH = 1000, HEIGHT = 1000;

// ============================================================
//  Vertex Shader
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
//  Fragment Shader - Phong + textura + highlight
// ============================================================
const GLchar* fragmentShaderSource = R"(
#version 450
in vec2 texCoord;
in vec3 fragNormal;
in vec3 fragPos;

uniform sampler2D texBuff;
uniform vec3  lightPos;
uniform vec3  camPos;
uniform vec3  Ka;
uniform vec3  Kd;
uniform vec3  Ks;
uniform float Ns;
uniform float highlight;

out vec4 color;

void main() {
    vec3 lightColor = vec3(1.0);
    vec4 texColor   = texture(texBuff, texCoord);
    vec3 texRGB     = vec3(texColor);

    // Ambiente
    vec3 ambient = Ka * texRGB;

    // Difusa
    vec3  N    = normalize(fragNormal);
    vec3  L    = normalize(lightPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    vec3  diffuse = Kd * diff * texRGB;

    // Especular (Phong)
    vec3  R    = reflect(-L, N);
    vec3  V    = normalize(camPos - fragPos);
    float spec = pow(max(dot(R, V), 0.0), max(Ns, 1.0));
    vec3  specular = Ks * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * highlight;
    color = vec4(result, texColor.a);
}
)";

// ============================================================
//  Estado global
// ============================================================
Camera           camera;
int              selectedObj = 0;
vector<Object3D> objects;

// Flags de teclas da camera
bool camW = false, camA = false, camS = false, camD = false;
bool camUp = false, camDown = false;

// Flags de teclas dos objetos
bool objLeft = false, objRight = false;
bool objFwd  = false, objBack  = false;
bool objPgUp = false, objPgDn  = false;
bool keyScaleUp = false, keyScaleDown = false;

// Mouse
float lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f;
bool  firstMouse = true;

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
        "Desafio M5 - Camera Primeira Pessoa", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Captura o cursor para controle de camera
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD\n"; return -1;
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
    GLint camPosLoc    = glGetUniformLocation(shaderID, "camPos");
    GLint highlightLoc = glGetUniformLocation(shaderID, "highlight");
    GLint KaLoc        = glGetUniformLocation(shaderID, "Ka");
    GLint KdLoc        = glGetUniformLocation(shaderID, "Kd");
    GLint KsLoc        = glGetUniformLocation(shaderID, "Ks");
    GLint NsLoc        = glGetUniformLocation(shaderID, "Ns");

    // Projecao perspectiva (fixa - nao muda com a camera)
    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));

    // Luz fixa no mundo
    vec3 lightPos(3.0f, 5.0f, 4.0f);
    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, value_ptr(lightPos));
    glUniform1i (glGetUniformLocation(shaderID, "texBuff"), 0);

    glEnable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Carregar modelos
    addObject("../assets/Modelos3D/Suzanne.obj",        "Suzanne",        vec3(-2.5f, 0.0f, 0.0f), 1.0f);
    addObject("../assets/Modelos3D/SuzanneSubdiv1.obj", "SuzanneSubdiv1", vec3( 0.0f, 0.0f, 0.0f), 1.0f);
    addObject("../assets/Modelos3D/Cube.obj",           "Cube",           vec3( 2.5f, 0.0f, 0.0f), 1.0f);
    objects.back().textureID = loadTexture("../assets/Modelos3D/Suzanne.png");

    if (objects.empty()) {
        cerr << "Nenhum modelo carregado.\n"; glfwTerminate(); return -1;
    }

    cout << "\n=== CONTROLES ===\n";
    cout << "Mouse      : Girar camera (yaw/pitch)\n";
    cout << "W/A/S/D    : Mover camera\n";
    cout << "Espaco / C : Mover camera cima / baixo\n";
    cout << "TAB        : Selecionar proximo objeto\n";
    cout << "Setas      : Transladar objeto selecionado (X e Z)\n";
    cout << "PgUp/PgDn  : Transladar objeto selecionado (Y)\n";
    cout << "X/Y/Z      : Toggle rotacao continua no eixo\n";
    cout << "[ / ]      : Diminuir / Aumentar escala\n";
    cout << "ESC        : Fechar\n";
    cout << "\nSelecionado: " << objects[selectedObj].name << "\n\n";

    const float OBJ_SPEED  = 2.5f;
    const float SCALE_SPEED = 1.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt  = now - lastFrame;
        lastFrame = now;

        glfwPollEvents();

        // --- Mover camera ---
        if (camW)    camera.moveForward(dt);
        if (camS)    camera.moveBack   (dt);
        if (camA)    camera.moveLeft   (dt);
        if (camD)    camera.moveRight  (dt);
        if (camUp)   camera.moveUp     (dt);
        if (camDown) camera.moveDown   (dt);

        // --- Transformacoes no objeto selecionado ---
        Object3D& sel = objects[selectedObj];
        if (objLeft)  sel.position.x -= OBJ_SPEED * dt;
        if (objRight) sel.position.x += OBJ_SPEED * dt;
        if (objFwd)   sel.position.z -= OBJ_SPEED * dt;
        if (objBack)  sel.position.z += OBJ_SPEED * dt;
        if (objPgUp)  sel.position.y += OBJ_SPEED * dt;
        if (objPgDn)  sel.position.y -= OBJ_SPEED * dt;
        if (keyScaleUp)   sel.scale = std::min(sel.scale + SCALE_SPEED * dt, 5.0f);
        if (keyScaleDown) sel.scale = std::max(sel.scale - SCALE_SPEED * dt, 0.1f);

        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Atualiza view matrix e posicao da camera no shader
        mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
        glUniform3fv(camPosLoc, 1, value_ptr(camera.position));

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
            glUniform1f (highlightLoc, (i == selectedObj) ? 1.0f : 0.6f);

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

    // Camera - teclas pressionadas continuamente
    if (key == GLFW_KEY_W)      camW    = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_S)      camS    = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_A)      camA    = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_D)      camD    = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_SPACE)  camUp   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_C)      camDown = (action != GLFW_RELEASE);

    // Selecao de objeto
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        selectedObj = (selectedObj + 1) % (int)objects.size();
        cout << "Selecionado: " << objects[selectedObj].name << "\n";
    }

    // Translacao do objeto selecionado (setas + PgUp/PgDn)
    if (key == GLFW_KEY_LEFT)      objLeft  = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_RIGHT)     objRight = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_UP)        objFwd   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_DOWN)      objBack  = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_PAGE_UP)   objPgUp  = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_PAGE_DOWN) objPgDn  = (action != GLFW_RELEASE);

    // Rotacao do objeto selecionado (toggle por eixo)
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

    // Escala do objeto selecionado
    if (key == GLFW_KEY_RIGHT_BRACKET) keyScaleUp   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_LEFT_BRACKET)  keyScaleDown = (action != GLFW_RELEASE);
}

// ============================================================
//  cursor_callback - rotacao da camera via mouse
// ============================================================
void cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
        return;
    }

    float xOffset =  (float)xpos - lastX;
    float yOffset =  lastY - (float)ypos; // invertido: y cresce para baixo na tela
    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.rotate(xOffset, yOffset);
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
    cout << "  MTL: Ka(" << mat.Ka.r << ") Kd(" << mat.Kd.r
         << ") Ks(" << mat.Ks.r << ") Ns(" << mat.Ns << ")\n";
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
