// ============================================================
// Diorama da Estação de Metrô — Visualizador 3D Interativo
// Computação Gráfica — Avaliação Final
// ============================================================

// GLAD deve ser incluído ANTES do GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Light.h"
#include "Mesh.h"
#include "Material.h"
#include "Model.h"
#include "Bezier.h"
#include "SceneObject.h"
#include "SceneLoader.h"

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>

// ============================================================
// VARIÁVEIS GLOBAIS DE ESTADO
// ============================================================

// Câmera e mouse
static Camera*  g_camera       = nullptr;
static double   g_lastMouseX   = 0.0;
static double   g_lastMouseY   = 0.0;
static bool     g_firstMouse   = true;

// Índice do objeto selecionado
static int      g_selectedIdx  = -1;

// Objetos da cena (ponteiro para o vetor em main)
static std::vector<SceneObject>* g_objects = nullptr;

// Luzes
static std::array<Light, 3>* g_lights = nullptr;

// ============================================================
// CALLBACKS GLFW
// ============================================================

static void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

static void mouseCallback(GLFWwindow* /*window*/, double xpos, double ypos) {
    if (!g_camera) return;
    if (g_firstMouse) {
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
        g_firstMouse = false;
    }
    double xOff = xpos - g_lastMouseX;
    double yOff = g_lastMouseY - ypos; // invertido: Y cresce para baixo
    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    g_camera->rotate(static_cast<float>(xOff), static_cast<float>(yOff));
}

// ============================================================
// PROCESSAMENTO DE INPUT POR FRAME
// ============================================================

static void processInput(GLFWwindow* window,
                         Camera& camera,
                         std::vector<SceneObject>& objects,
                         std::array<Light, 3>& lights,
                         float dt)
{
    // Fechar janela
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- Movimentação da câmera ---
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.moveBack   (dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.moveLeft   (dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.moveRight  (dt);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.moveDown   (dt);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.moveUp     (dt);

    // --- Toggle de luzes ---
    static bool f1Prev = false, f2Prev = false, f3Prev = false;
    bool f1Now = (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS);
    bool f2Now = (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS);
    bool f3Now = (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS);
    if (f1Now && !f1Prev) { lights[0].enabled = !lights[0].enabled; std::cout << "KeyLight: "  << (lights[0].enabled?"ON":"OFF") << "\n"; }
    if (f2Now && !f2Prev) { lights[1].enabled = !lights[1].enabled; std::cout << "FillLight: " << (lights[1].enabled?"ON":"OFF") << "\n"; }
    if (f3Now && !f3Prev) { lights[2].enabled = !lights[2].enabled; std::cout << "BackLight: " << (lights[2].enabled?"ON":"OFF") << "\n"; }
    f1Prev = f1Now; f2Prev = f2Now; f3Prev = f3Now;

    if (objects.empty()) return;

    // --- Seleção de objetos ---
    // TAB: próximo objeto
    static bool tabPrev = false;
    bool tabNow = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
    if (tabNow && !tabPrev) {
        if (g_selectedIdx >= 0) objects[g_selectedIdx].selected = false;
        g_selectedIdx = (g_selectedIdx + 1) % static_cast<int>(objects.size());
        objects[g_selectedIdx].selected = true;
        std::cout << "Selecionado: " << objects[g_selectedIdx].name << "\n";
    }
    tabPrev = tabNow;

    // Teclas 1-4: seleção direta
    const int directKeys[] = {GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4};
    static bool directPrev[4] = {false,false,false,false};
    for (int i = 0; i < 4; ++i) {
        bool kNow = (glfwGetKey(window, directKeys[i]) == GLFW_PRESS);
        if (kNow && !directPrev[i] && i < (int)objects.size()) {
            if (g_selectedIdx >= 0) objects[g_selectedIdx].selected = false;
            g_selectedIdx = i;
            objects[g_selectedIdx].selected = true;
            std::cout << "Selecionado: " << objects[g_selectedIdx].name << "\n";
        }
        directPrev[i] = kNow;
    }

    if (g_selectedIdx < 0 || g_selectedIdx >= (int)objects.size()) return;
    SceneObject& sel = objects[g_selectedIdx];

    // --- Translação do objeto selecionado ---
    const float moveSpeed = 2.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)           sel.translate(glm::vec3(0,0,-moveSpeed));
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)           sel.translate(glm::vec3(0,0, moveSpeed));
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)           sel.translate(glm::vec3(-moveSpeed,0,0));
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)           sel.translate(glm::vec3( moveSpeed,0,0));
    if (glfwGetKey(window, GLFW_KEY_UP)   == GLFW_PRESS)        sel.translate(glm::vec3(0, moveSpeed,0));
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)        sel.translate(glm::vec3(0,-moveSpeed,0));

    // --- Rotação do objeto selecionado ---
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        sel.rotateAxis(1, 45.0f * dt); // Y

    // --- Escala do objeto selecionado ---
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
        sel.scaleBy(1.0f + dt);
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
        sel.scaleBy(1.0f - dt);

    // --- Toggle animação Bézier (tecla P) ---
    static bool pPrev = false;
    bool pNow = (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);
    if (pNow && !pPrev) {
        // Busca o objeto com animação (trem)
        for (auto& obj : objects) {
            if (obj.hasAnimation()) {
                bool newState = !obj.isAnimationActive();
                obj.setAnimationActive(newState);
                std::cout << "Animacao do '" << obj.name << "': "
                          << (newState ? "INICIADA" : "PAUSADA") << "\n";
            }
        }
    }
    pPrev = pNow;
}

// ============================================================
// GEOMETRIA PROCEDURAL: PISO E PAREDES
// ============================================================

// Cria a geometria do piso (quad XZ plano)
static Mesh* createFloor() {
    const float H = 16.0f; // metade do tamanho (±8)
    const float T = 4.0f;  // repetição de textura

    // 2 triângulos = 6 vértices
    std::vector<Vertex> verts = {
        {{-H, 0, -H}, {0,T}, {0,1,0}},
        {{ H, 0, -H}, {T,T}, {0,1,0}},
        {{ H, 0,  H}, {T,0}, {0,1,0}},
        {{ H, 0,  H}, {T,0}, {0,1,0}},
        {{-H, 0,  H}, {0,0}, {0,1,0}},
        {{-H, 0, -H}, {0,T}, {0,1,0}},
    };
    return new Mesh(verts);
}

// Cria parede funda (XY, Z=-8)
static Mesh* createWallBack() {
    const float W = 8.0f, H = 5.0f, D = -8.0f;
    std::vector<Vertex> verts = {
        {{-W, 0, D}, {0,0}, {0,0,1}},
        {{ W, 0, D}, {1,0}, {0,0,1}},
        {{ W, H, D}, {1,1}, {0,0,1}},
        {{ W, H, D}, {1,1}, {0,0,1}},
        {{-W, H, D}, {0,1}, {0,0,1}},
        {{-W, 0, D}, {0,0}, {0,0,1}},
    };
    return new Mesh(verts);
}

// Cria parede lateral (YZ, X=-8)
static Mesh* createWallLeft() {
    const float D1 = -8.0f, D2 = 8.0f, H = 5.0f, X = -8.0f;
    std::vector<Vertex> verts = {
        {{X, 0, D2}, {0,0}, {1,0,0}},
        {{X, 0, D1}, {1,0}, {1,0,0}},
        {{X, H, D1}, {1,1}, {1,0,0}},
        {{X, H, D1}, {1,1}, {1,0,0}},
        {{X, H, D2}, {0,1}, {1,0,0}},
        {{X, 0, D2}, {0,0}, {1,0,0}},
    };
    return new Mesh(verts);
}

// Cria parede lateral (YZ, X=+8)
static Mesh* createWallRight() {
    const float D1 = -8.0f, D2 = 8.0f, H = 5.0f, X = 8.0f;
    std::vector<Vertex> verts = {
        {{X, 0, D1}, {0,0}, {-1,0,0}},
        {{X, 0, D2}, {1,0}, {-1,0,0}},
        {{X, H, D2}, {1,1}, {-1,0,0}},
        {{X, H, D2}, {1,1}, {-1,0,0}},
        {{X, H, D1}, {0,1}, {-1,0,0}},
        {{X, 0, D1}, {0,0}, {-1,0,0}},
    };
    return new Mesh(verts);
}

// ============================================================
// FUNÇÕES AUXILIARES PARA DESENHAR AMBIENTE
// ============================================================

// Desenha uma mesh de ambiente (piso/parede) com material padrão
static void drawEnvMesh(Shader& shader, Mesh* mesh, GLuint texID,
                        const glm::mat4& model)
{
    shader.setMat4("uModel", model);
    // Material padrão para ambiente
    shader.setVec3 ("uKa", glm::vec3(0.3f));
    shader.setVec3 ("uKd", glm::vec3(0.7f));
    shader.setVec3 ("uKs", glm::vec3(0.1f));
    shader.setFloat("uNs", 10.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    shader.setInt("uTexture", 0);
    shader.setFloat("uHighlight", 0.9f);

    mesh->draw();
}

// Carrega textura usando STB (somente aqui fora de Model.cpp precisamos de um helper manual)
// Como STB_IMAGE_IMPLEMENTATION já está em Model.cpp, usamos stb_image indiretamente via a
// função branca — passamos 0 se o arquivo não existir.
static GLuint createWhiteTexture() {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    unsigned char white[4] = {200, 200, 200, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texID;
}

// ============================================================
// ENVIO DAS LUZES AO SHADER
// ============================================================

static void sendLights(Shader& shader, const std::array<Light, 3>& lights) {
    for (int i = 0; i < 3; ++i) {
        std::string base = "uLights[" + std::to_string(i) + "]";
        shader.setVec3 (base + ".position",  lights[i].position);
        shader.setVec3 (base + ".color",     lights[i].color);
        shader.setFloat(base + ".intensity", lights[i].intensity);
        shader.setBool (base + ".enabled",   lights[i].enabled);
    }
}

// ============================================================
// IMPRESSÃO DOS CONTROLES NO CONSOLE
// ============================================================

static void printControls(const std::vector<SceneObject>& objects) {
    std::cout << "\n=== CONTROLES ===\n";
    std::cout << "WASD + mouse : Mover/girar camera\n";
    std::cout << "Q / E        : Descer / Subir camera\n";
    std::cout << "TAB / 1-4    : Selecionar objeto (TAB = proximo, 1-4 = direto)\n";
    std::cout << "I/K, J/L     : Transladar objeto selecionado (frente/tras, esq/dir)\n";
    std::cout << "Setas Up/Dn  : Transladar objeto selecionado (Y)\n";
    std::cout << "R            : Rotacionar objeto selecionado (Y)\n";
    std::cout << "+ / -        : Aumentar / Diminuir escala do objeto selecionado\n";
    std::cout << "F1 / F2 / F3 : Ligar / Desligar luz 1 (Key) / 2 (Fill) / 3 (Back)\n";
    std::cout << "P            : Iniciar / Pausar animacao do trem (Bezier)\n";
    std::cout << "ESC          : Sair\n";
    std::cout << "=================\n";
    std::cout << "Objetos:\n";
    for (int i = 0; i < (int)objects.size(); ++i) {
        std::cout << "  [" << (i+1) << "] " << objects[i].name << "\n";
    }
    std::cout << "\n";
}

// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[]) {
    // 1. Caminho do scene.json (argumento ou default)
    std::string scenePath = "../assets/scene.json";
    if (argc > 1) scenePath = argv[1];

    // 2. Carregar configuração da cena
    SceneConfig cfg;
    try {
        cfg = SceneLoader::load(scenePath);
    } catch (const std::exception& e) {
        std::cerr << "[main] Erro ao carregar cena: " << e.what() << "\n";
        return -1;
    }

    // 3. Inicializar GLFW
    if (!glfwInit()) {
        std::cerr << "[main] Falha ao inicializar GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        cfg.windowWidth, cfg.windowHeight,
        cfg.windowTitle.c_str(),
        nullptr, nullptr);
    if (!window) {
        std::cerr << "[main] Falha ao criar janela GLFW\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 4. Inicializar GLAD
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "[main] Falha ao inicializar GLAD\n";
        glfwTerminate();
        return -1;
    }

    // 5. Configurações OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);

    // 6. Compilar shaders
    Shader shader("../shaders/vertex.glsl", "../shaders/fragment.glsl");

    // 7. Câmera
    Camera camera(cfg.camPosition, cfg.camYaw, cfg.camPitch);
    g_camera = &camera;

    // 8. Geometria procedural do ambiente
    std::unique_ptr<Mesh> floorMesh (createFloor());
    std::unique_ptr<Mesh> wallBack  (createWallBack());
    std::unique_ptr<Mesh> wallLeft  (createWallLeft());
    std::unique_ptr<Mesh> wallRight (createWallRight());

    // Texturas do ambiente (fallback cinza se não encontrar)
    GLuint floorTex = createWhiteTexture();
    GLuint wallTex  = createWhiteTexture();

    // Matrizes model do ambiente (identidade — geometria já posicionada)
    glm::mat4 envModel = glm::mat4(1.0f);

    // 9. Expor ponteiros globais para callbacks
    g_objects = &cfg.objects;
    g_lights  = &cfg.lights;

    // 10. Imprimir controles no console
    printControls(cfg.objects);

    // === GAME LOOP ===
    float lastTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float dt          = currentTime - lastTime;
        lastTime          = currentTime;

        // Limitar dt para evitar pulos grandes (ex.: ao depurar)
        if (dt > 0.1f) dt = 0.1f;

        // Processar input
        processInput(window, camera, cfg.objects, cfg.lights, dt);

        // Limpar buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Ativar shader
        shader.use();

        // === PASSAGEM DAS MATRIZES DE CÂMERA ===
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = (fbHeight > 0) ? static_cast<float>(fbWidth) / fbHeight : 1.0f;

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(cfg.fov, aspect, cfg.nearP, cfg.farP);

        shader.setMat4("uView",       view);
        shader.setMat4("uProjection", proj);
        shader.setVec3("uCamPos",     camera.position);

        // === ENVIO DAS 3 LUZES AO SHADER ===
        sendLights(shader, cfg.lights);

        // --- Desenhar ambiente ---
        drawEnvMesh(shader, floorMesh.get(), floorTex, envModel);
        drawEnvMesh(shader, wallBack.get(),  wallTex,  envModel);
        drawEnvMesh(shader, wallLeft.get(),  wallTex,  envModel);
        drawEnvMesh(shader, wallRight.get(), wallTex,  envModel);

        // --- Desenhar objetos da cena ---
        for (auto& obj : cfg.objects) {
            obj.update(dt);
            // Destaque visual: objeto selecionado fica mais brilhante
            shader.setFloat("uHighlight", obj.selected ? 1.1f : 0.7f);
            obj.draw(shader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza
    glfwTerminate();
    return 0;
}
