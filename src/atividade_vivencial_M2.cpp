/* Atividade Vivencial M2
 * Visualizador 3D com multiplos modelos OBJ, selecao e transformacoes
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

// Prototipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int  setupShader();
int  loadSimpleOBJ(const string& filePath, int& nVertices, vec3 color);

const GLuint WIDTH = 1000, HEIGHT = 1000;

// Vertex Shader - MVP + highlight para indicar objeto selecionado
const GLchar* vertexShaderSource = R"(
#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float highlight;
out vec4 finalColor;
void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    finalColor = vec4(color * highlight, 1.0);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 450
in vec4 finalColor;
out vec4 color;
void main() {
    color = finalColor;
}
)";

struct Object3D {
    GLuint VAO;
    int    nVertices;
    string name;
    vec3   position;
    float  scale;
    float  rotAngleX, rotAngleY, rotAngleZ;
    bool   rotateX,   rotateY,   rotateZ;
};

int            selectedObj = 0;
vector<Object3D> objects;

// Flags de teclas mantidas pressionadas (movimento continuo)
bool keyW = false, keyA = false, keyS_mv = false, keyD = false;
bool keyI = false, keyJ = false;
bool keyScaleUp = false, keyScaleDown = false;

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Atividade Vivencial M2 - Visualizador OBJ", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    GLint modelLoc     = glGetUniformLocation(shaderID, "model");
    GLint viewLoc      = glGetUniformLocation(shaderID, "view");
    GLint projLoc      = glGetUniformLocation(shaderID, "projection");
    GLint highlightLoc = glGetUniformLocation(shaderID, "highlight");

    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));

    mat4 view = lookAt(vec3(0.0f, 0.0f, 6.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));

    glEnable(GL_DEPTH_TEST);

    // --- Carregar modelos OBJ ---
    // Suzanne - vermelho, esquerda
    {
        int nVerts = 0;
        int vao = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVerts, vec3(1.0f, 0.35f, 0.35f));
        if (vao != -1) {
            Object3D obj;
            obj.VAO       = (GLuint)vao;
            obj.nVertices = nVerts;
            obj.name      = "Suzanne";
            obj.position  = vec3(-2.2f, 0.0f, 0.0f);
            obj.scale     = 1.0f;
            obj.rotAngleX = obj.rotAngleY = obj.rotAngleZ = 0.0f;
            obj.rotateX   = obj.rotateY   = obj.rotateZ   = false;
            objects.push_back(obj);
        }
    }

    // Cube - azul, direita
    {
        int nVerts = 0;
        int vao = loadSimpleOBJ("../assets/Modelos3D/Cube.obj", nVerts, vec3(0.35f, 0.6f, 1.0f));
        if (vao != -1) {
            Object3D obj;
            obj.VAO       = (GLuint)vao;
            obj.nVertices = nVerts;
            obj.name      = "Cube";
            obj.position  = vec3(2.2f, 0.0f, 0.0f);
            obj.scale     = 1.0f;
            obj.rotAngleX = obj.rotAngleY = obj.rotAngleZ = 0.0f;
            obj.rotateX   = obj.rotateY   = obj.rotateZ   = false;
            objects.push_back(obj);
        }
    }

    // SuzanneSubdiv1 - verde, centro
    {
        int nVerts = 0;
        int vao = loadSimpleOBJ("../assets/Modelos3D/SuzanneSubdiv1.obj", nVerts, vec3(0.35f, 1.0f, 0.45f));
        if (vao != -1) {
            Object3D obj;
            obj.VAO       = (GLuint)vao;
            obj.nVertices = nVerts;
            obj.name      = "SuzanneSubdiv1";
            obj.position  = vec3(0.0f, 0.0f, 0.0f);
            obj.scale     = 1.0f;
            obj.rotAngleX = obj.rotAngleY = obj.rotAngleZ = 0.0f;
            obj.rotateX   = obj.rotateY   = obj.rotateZ   = false;
            objects.push_back(obj);
        }
    }

    if (objects.empty()) {
        cout << "ERRO: Nenhum modelo carregado. Verifique o caminho dos arquivos .obj" << endl;
        glfwTerminate();
        return -1;
    }

    cout << "\n=== CONTROLES ===" << endl;
    cout << "TAB       : Selecionar proximo objeto (cicla pela lista)" << endl;
    cout << "X/Y/Z     : Toggle rotacao continua no eixo correspondente" << endl;
    cout << "W/S       : Transladar no eixo Z (frente/tras)" << endl;
    cout << "A/D       : Transladar no eixo X (esquerda/direita)" << endl;
    cout << "I/J       : Transladar no eixo Y (cima/baixo)" << endl;
    cout << "[ / ]     : Diminuir / Aumentar escala (uniforme)" << endl;
    cout << "ESC       : Fechar" << endl;
    cout << "\nObjeto selecionado: [" << objects[selectedObj].name << "] (brilhante = selecionado)" << endl;

    const float MOVE_SPEED  = 2.5f;
    const float SCALE_SPEED = 1.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // Aplicar transformacoes no objeto selecionado
        Object3D& sel = objects[selectedObj];
        if (keyA)         sel.position.x -= MOVE_SPEED * dt;
        if (keyD)         sel.position.x += MOVE_SPEED * dt;
        if (keyW)         sel.position.z -= MOVE_SPEED * dt;
        if (keyS_mv)      sel.position.z += MOVE_SPEED * dt;
        if (keyI)         sel.position.y += MOVE_SPEED * dt;
        if (keyJ)         sel.position.y -= MOVE_SPEED * dt;
        if (keyScaleUp)   sel.scale = std::min(sel.scale + SCALE_SPEED * dt, 5.0f);
        if (keyScaleDown) sel.scale = std::max(sel.scale - SCALE_SPEED * dt, 0.1f);

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Desenhar todos os objetos
        for (int i = 0; i < (int)objects.size(); i++)
        {
            Object3D& obj = objects[i];

            // Rotacao continua acumula angulo
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

            // Objeto selecionado aparece em brilho total; outros em 50%
            glUniform1f(highlightLoc, (i == selectedObj) ? 1.0f : 0.5f);

            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Ciclar objeto selecionado com TAB
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        selectedObj = (selectedObj + 1) % (int)objects.size();
        cout << "Selecionado: " << objects[selectedObj].name << endl;
    }

    // Toggle rotacao por eixo (X/Y/Z como exclusivos entre si)
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

    // Translacao e escala continua (mantendo tecla pressionada)
    if (key == GLFW_KEY_A)             keyA        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_D)             keyD        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_W)             keyW        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_S)             keyS_mv     = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_I)             keyI        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_J)             keyJ        = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_RIGHT_BRACKET) keyScaleUp   = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_LEFT_BRACKET)  keyScaleDown = (action != GLFW_RELEASE);
}

int setupShader()
{
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "VERTEX SHADER ERROR:\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "FRAGMENT SHADER ERROR:\n" << infoLog << endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cout << "SHADER LINK ERROR:\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// Baseado no snippet LoadSimpleOBJ.cpp do professor
// Aceita parametro de cor para diferenciar objetos visualmente
int loadSimpleOBJ(const string& filePath, int& nVertices, vec3 color)
{
    vector<vec3>    positions;
    vector<vec2>    texCoords;
    vector<vec3>    normals;
    vector<GLfloat> vBuffer;

    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo: " << filePath << endl;
        return -1;
    }

    string line;
    while (getline(file, line))
    {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "v") {
            vec3 v;
            ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (word == "vt") {
            vec2 vt;
            ss >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn") {
            vec3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (word == "f") {
            while (ss >> word) {
                int vi = 0, ti = 0, ni = 0;
                istringstream fss(word);
                string idx;

                if (getline(fss, idx, '/')) vi = !idx.empty() ? stoi(idx) - 1 : 0;
                if (getline(fss, idx, '/')) ti = !idx.empty() ? stoi(idx) - 1 : 0;
                if (getline(fss, idx))     ni = !idx.empty() ? stoi(idx) - 1 : 0;

                vBuffer.push_back(positions[vi].x);
                vBuffer.push_back(positions[vi].y);
                vBuffer.push_back(positions[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }
    file.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // location 0: posicao (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // location 1: cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)(vBuffer.size() / 6);
    cout << "Carregado: " << filePath << " | " << nVertices << " vertices" << endl;
    return (int)VAO;
}
