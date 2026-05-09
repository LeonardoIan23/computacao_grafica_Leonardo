
/* Visualizador 3D - Cubos com transformaÃ§Ãµes
 *
 * Baseado no projeto de Rossana Baptista Queiroz
 * ImplementaÃ§Ã£o: cubo colorido, translaÃ§Ã£o, escala e mÃºltiplas instÃ¢ncias
 * Leonardo Ian de Oliveira
 */

#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

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

// ProtÃ³tipo da funÃ§Ã£o de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// ProtÃ³tipos das funÃ§Ãµes
int setupShader();
int setupGeometry();

// DimensÃµes da janela
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Vertex Shader â€” inclui view e projection para perspectiva real
const GLchar *vertexShaderSource =
    "#version 450\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "    finalColor = vec4(color, 1.0);\n"
    "}\0";

// Fragment Shader
const GLchar *fragmentShaderSource =
    "#version 450\n"
    "in vec4 finalColor;\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "    color = finalColor;\n"
    "}\n\0";

// --- Teclas pressionadas (para movimento contÃ­nuo) ---
bool keyW = false, keyA = false, keyS = false, keyD = false;
bool keyI = false, keyJ = false;
bool keyScaleUp = false, keyScaleDown = false;

// Estrutura para armazenar o estado de cada cubo
struct Cube
{
    vec3 position;
    float scale;
    bool rotateX, rotateY, rotateZ;
    float rotAngleX, rotAngleY, rotAngleZ;
};

// Ãndice do cubo selecionado
int selectedCube = 0;

// Vetor de cubos na cena
vector<Cube> cubes;

int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Visualizador 3D - Cubos", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported: " << version << endl;
    cout << "\n=== CONTROLES ===" << endl;
    cout << "X / Y / Z   : Rotacionar o cubo selecionado no eixo correspondente" << endl;
    cout << "W / S       : Mover no eixo Z (frente/tras)" << endl;
    cout << "A / D       : Mover no eixo X (esquerda/direita)" << endl;
    cout << "I / J       : Mover no eixo Y (cima/baixo)" << endl;
    cout << "[ / ]       : Diminuir / Aumentar escala" << endl;
    cout << "1 / 2 / 3   : Selecionar cubo 1, 2 ou 3" << endl;
    cout << "ESC         : Fechar\n"
         << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();
    GLuint VAO = setupGeometry();

    glUseProgram(shaderID);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projectionLoc = glGetUniformLocation(shaderID, "projection");

    // --- ProjeÃ§Ã£o perspectiva (45 graus de FOV) ---
    mat4 projection = perspective(radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));

    // --- CÃ¢mera posicionada acima e atrÃ¡s, olhando para a origem ---
    mat4 view = lookAt(
        vec3(0.0f, 1.5f, 4.5f), // posiÃ§Ã£o da cÃ¢mera
        vec3(0.0f, 0.0f, 0.0f), // ponto alvo
        vec3(0.0f, 1.0f, 0.0f)  // vetor up
    );
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));

    glEnable(GL_DEPTH_TEST);

    // --- 3 cubos com rotaÃ§Ã£o inicial para jÃ¡ aparecerem em Ã¢ngulo 3D ---
    cubes.push_back({vec3(-1.8f, 0.0f, 0.0f), 0.8f, false, false, false, radians(25.0f), radians(35.0f), 0.0f});
    cubes.push_back({vec3(0.0f, 0.0f, 0.0f), 0.8f, false, false, false, radians(25.0f), radians(35.0f), 0.0f});
    cubes.push_back({vec3(1.8f, 0.0f, 0.0f), 0.8f, false, false, false, radians(25.0f), radians(35.0f), 0.0f});

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // --- Velocidades escaladas por deltaTime para movimento suave ---
        const float MOVE_SPEED = 1.5f;
        const float SCALE_SPEED = 0.5f;

        Cube &c = cubes[selectedCube];

        if (keyA)
            c.position.x -= MOVE_SPEED * deltaTime;
        if (keyD)
            c.position.x += MOVE_SPEED * deltaTime;
        if (keyW)
            c.position.z -= MOVE_SPEED * deltaTime;
        if (keyS)
            c.position.z += MOVE_SPEED * deltaTime;
        if (keyI)
            c.position.y += MOVE_SPEED * deltaTime;
        if (keyJ)
            c.position.y -= MOVE_SPEED * deltaTime;
        if (keyScaleUp)
            c.scale = std::min(c.scale + SCALE_SPEED * deltaTime, 3.0f);
        if (keyScaleDown)
            c.scale = std::max(c.scale - SCALE_SPEED * deltaTime, 0.1f);

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO);

        for (int i = 0; i < (int)cubes.size(); i++)
        {
            Cube &cube = cubes[i];

            // Acumula Ã¢ngulo de rotaÃ§Ã£o contÃ­nua
            if (cube.rotateX)
                cube.rotAngleX += deltaTime;
            if (cube.rotateY)
                cube.rotAngleY += deltaTime;
            if (cube.rotateZ)
                cube.rotAngleZ += deltaTime;

            mat4 model = mat4(1.0f);
            model = translate(model, cube.position);
            model = rotate(model, cube.rotAngleX, vec3(1.0f, 0.0f, 0.0f));
            model = rotate(model, cube.rotAngleY, vec3(0.0f, 1.0f, 0.0f));
            model = rotate(model, cube.rotAngleZ, vec3(0.0f, 0.0f, 1.0f));
            model = scale(model, vec3(cube.scale));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

            glDrawArrays(GL_TRIANGLES, 0, 36);

            glPointSize(5);
            glDrawArrays(GL_POINTS, 0, 36);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        selectedCube = 0;
        cout << "Cubo 1 selecionado" << endl;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        selectedCube = 1;
        cout << "Cubo 2 selecionado" << endl;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        selectedCube = 2;
        cout << "Cubo 3 selecionado" << endl;
    }

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        Cube &c = cubes[selectedCube];
        c.rotateX = !c.rotateX;
        c.rotateY = false;
        c.rotateZ = false;
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        Cube &c = cubes[selectedCube];
        c.rotateX = false;
        c.rotateY = !c.rotateY;
        c.rotateZ = false;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        Cube &c = cubes[selectedCube];
        c.rotateX = false;
        c.rotateY = false;
        c.rotateZ = !c.rotateZ;
    }

    if (key == GLFW_KEY_A)
        keyA = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_D)
        keyD = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_W)
        keyW = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_S)
        keyS = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_I)
        keyI = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_J)
        keyJ = (action != GLFW_RELEASE);

    if (key == GLFW_KEY_RIGHT_BRACKET)
        keyScaleUp = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_LEFT_BRACKET)
        keyScaleDown = (action != GLFW_RELEASE);
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "VERTEX ERROR:\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "FRAGMENT ERROR:\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "LINK ERROR:\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

int setupGeometry()
{
    /*
     * Cubo unitÃ¡rio centrado na origem.
     * 6 faces x 2 triÃ¢ngulos x 3 vÃ©rtices = 36 vÃ©rtices.
     * Cores por face:
     *   Frente   - Vermelho
     *   TrÃ¡s     - Verde
     *   Esquerda - Azul
     *   Direita  - Amarelo
     *   Cima     - Ciano
     *   Baixo    - Magenta
     */
    GLfloat vertices[] = {
        // ---- FRENTE (z = +0.5) - Vermelho ----
        -0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        -0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        -0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,

        // ---- TRÃS (z = -0.5) - Verde ----
        0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,

        // ---- ESQUERDA (x = -0.5) - Azul ----
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        1.0f,

        // ---- DIREITA (x = +0.5) - Amarelo ----
        0.5f,
        -0.5f,
        0.5f,
        1.0f,
        1.0f,
        0.0f,
        0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        -0.5f,
        1.0f,
        1.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.5f,
        1.0f,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        -0.5f,
        1.0f,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        0.5f,
        1.0f,
        1.0f,
        0.0f,

        // ---- CIMA (y = +0.5) - Ciano ----
        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        1.0f,
        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        1.0f,
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        1.0f,

        // ---- BAIXO (y = -0.5) - Magenta ----
        -0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        1.0f,
        0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        1.0f,
        0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        1.0f,
        0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        1.0f,
    };

    GLuint VBO, VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo posiÃ§Ã£o (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Atributo cor (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}
