# Computação Gráfica — CGCCHibrido

**Aluno:** Leonardo Ian de Oliveira  
**Disciplina:** Processamento Gráfico / Computação Gráfica — Unisinos  

Repositório com os exercícios e desafios desenvolvidos ao longo da disciplina usando **OpenGL**, **GLFW**, **GLM** e **stb_image**.

---

## Dependências

Todas as dependências são baixadas automaticamente pelo CMake via `FetchContent`:

| Biblioteca | Uso |
|---|---|
| [GLFW 3.4](https://www.glfw.org/) | Janela e entrada de teclado/mouse |
| [GLM](https://github.com/g-truc/glm) | Matemática (vetores, matrizes) |
| [stb_image](https://github.com/nothings/stb) | Carregamento de texturas |
| GLAD (local) | Carregamento das funções OpenGL |

---

## Como compilar

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Os executáveis são gerados dentro da pasta `build/`.

> **Importante:** execute os programas sempre a partir da pasta `build/`, pois os caminhos dos modelos e texturas são relativos a ela (ex: `../assets/Modelos3D/Suzanne.obj`).

```bash
cd build
.\desafioM2.exe
.\desafioM3.exe
.\desafioM4.exe
.\desafioM5.exe
.\atividade_vivencial_M2.exe
```

---

## Modelos 3D disponíveis

Localizados em `assets/Modelos3D/`:

| Arquivo | Descrição |
|---|---|
| `Suzanne.obj` | Cabeça da macaca do Blender (baixa resolução) |
| `SuzanneSubdiv1.obj` | Suzanne subdividida (mais suave) |
| `Cube.obj` | Cubo simples |
| `Suzanne.png` | Textura usada pela Suzanne e pelo Cube |

---

## Projetos entregues

---

### desafioM2 — Visualizador 3D com Cubos

**Arquivo:** `src/desafioM2.cpp`

Visualizador de cubos coloridos com transformações interativas. Cada cubo tem cor diferente por face (vermelho, verde, azul, amarelo, ciano, magenta) e pode ser manipulado individualmente.

**O que implementa:**
- Geometria hard-coded de cubo com cores por face
- Projeção perspectiva com câmera fixa
- 3 cubos na cena com transformações independentes
- Seleção por teclas numéricas

**Controles:**

| Tecla | Ação |
|---|---|
| `1` / `2` / `3` | Selecionar cubo 1, 2 ou 3 |
| `X` / `Y` / `Z` | Toggle rotação contínua no eixo |
| `W` / `S` | Transladar no eixo Z |
| `A` / `D` | Transladar no eixo X |
| `I` / `J` | Transladar no eixo Y |
| `[` / `]` | Diminuir / Aumentar escala |
| `ESC` | Fechar |

---

### atividade_vivencial_M2 — Múltiplos Modelos OBJ

**Arquivo:** `src/atividade_vivencial_M2.cpp`

Extensão do desafio M2: carrega múltiplos modelos `.obj` reais na cena, com seleção por ciclagem e transformações individuais. Sem textura ou iluminação (visual flat-shaded por cor sólida).

**O que implementa:**
- Parser de arquivo `.OBJ` (leitura de `v`, `vt`, `vn`, `f`)
- 3 modelos na cena: Suzanne (vermelho), SuzanneSubdiv1 (verde), Cube (azul)
- Seleção por ciclagem com `TAB`
- Destaque visual: objeto selecionado aparece em brilho total, outros em 50%
- `struct Object3D` com posição, escala e rotação individuais

**Modelos carregados:**

| Modelo | Cor | Posição |
|---|---|---|
| Suzanne | Vermelho | Esquerda |
| SuzanneSubdiv1 | Verde | Centro |
| Cube | Azul | Direita |

**Controles:**

| Tecla | Ação |
|---|---|
| `TAB` | Selecionar próximo objeto (cicla pela lista) |
| `X` / `Y` / `Z` | Toggle rotação contínua no eixo |
| `W` / `S` | Transladar no eixo Z |
| `A` / `D` | Transladar no eixo X |
| `I` / `J` | Transladar no eixo Y |
| `[` / `]` | Diminuir / Aumentar escala |
| `ESC` | Fechar |

---

### desafioM3 — Texturas via OBJ + MTL

**Arquivo:** `src/desafioM3.cpp`

Extende a atividade vivencial M2 adicionando suporte completo a texturas. Lê o arquivo `.MTL` referenciado no `.OBJ` para obter o nome da imagem de textura (`map_Kd`) e a carrega com `stb_image`.

**O que implementa:**
- Leitura de coordenadas de textura `vt` do `.OBJ` e armazenamento no VAO
- Parser de `.MTL` extraindo `map_Kd` (caminho da textura difusa)
- Carregamento de textura com `stb_image` e geração de mipmaps
- Layout do vértice: `pos(3) + uv(2) + normal(3)` = 8 floats
- Iluminação Phong básica com coeficientes fixos
- Fallback: textura branca 1×1 quando o MTL não define textura

**Controles:** idênticos à atividade vivencial M2 (`TAB`, `X/Y/Z`, `WASD`, `I/J`, `[/]`, `ESC`)

---

### desafioM4 — Modelo de Iluminação de Phong

**Arquivo:** `src/desafioM4.cpp`

Extende o desafioM3 implementando o modelo de Phong completo com coeficientes lidos do arquivo `.MTL` por objeto. Cada modelo recebe seus próprios `Ka`, `Kd`, `Ks` e `Ns`.

**O que implementa:**
- `struct Material` com `Ka`, `Kd`, `Ks` (vec3) e `Ns` (float)
- Parser de `.MTL` expandido: lê `Ka`, `Kd`, `Ks`, `Ns` além do `map_Kd`
- Coeficientes enviados como uniforms ao fragment shader **por objeto**
- Fragment shader com Phong completo:
  - **Ambiente:** `Ka × texColor`
  - **Difusa:** `Kd × max(dot(N,L), 0) × texColor`
  - **Especular:** `Ks × pow(max(dot(R,V), 0), Ns) × lightColor`
- Matriz normal correta `transpose(inverse(model))` para escalas não uniformes

**Controles:** idênticos à atividade vivencial M2 (`TAB`, `X/Y/Z`, `WASD`, `I/J`, `[/]`, `ESC`)

---

### atividade_vivencial_M4 — Iluminação de Três Pontos

**Arquivo:** `src/atividade_vivencial_M4.cpp`

Extende o desafioM4 adicionando um sistema de **iluminação de três pontos** (*three-point lighting*) com fontes de luz pontuais e atenuação por distância.

**O que implementa:**
- **3 fontes de luz pontuais** com papéis clássicos de estúdio:
  - **Key Light** — luz principal, iluminação dominante
  - **Fill Light** — luz de preenchimento, suaviza sombras
  - **Back Light** — luz de fundo (contorno/rim), separa o objeto do fundo
- Posicionamento automático das luzes relativo à posição e escala do objeto principal
- Fator de atenuação por distância em cada luz: `1 / (Kc + Kl×d + Kq×d²)`
- Fragment shader com Phong acumulado sobre as 3 luzes (componente ambiente calculada uma vez)
- Toggle individual de cada luz em tempo real

**Controles:**

| Tecla | Ação |
|---|---|
| `TAB` | Selecionar próximo objeto |
| `X` / `Y` / `Z` | Toggle rotação contínua no eixo |
| `W` / `S` | Transladar no eixo Z |
| `A` / `D` | Transladar no eixo X |
| `I` / `J` | Transladar no eixo Y |
| `[` / `]` | Diminuir / Aumentar escala |
| `1` | Ligar / Desligar Key Light |
| `2` | Ligar / Desligar Fill Light |
| `3` | Ligar / Desligar Back Light |
| `ESC` | Fechar |

---

### desafioM5 — Câmera em Primeira Pessoa

**Arquivo:** `src/desafioM5.cpp`

Extende o desafioM4 adicionando uma câmera interativa em primeira pessoa implementada como uma **classe** com atributos e métodos encapsulados.

**O que implementa:**

**Classe `Camera`:**

| Atributo | Descrição |
|---|---|
| `position` | Posição da câmera no mundo |
| `front`, `right`, `up` | Vetores de direção (recalculados a cada rotação) |
| `yaw` | Ângulo horizontal (rotação em Y) |
| `pitch` | Ângulo vertical (rotação em X), limitado a ±89° |
| `speed` | Velocidade de deslocamento (unidades/segundo) |
| `sensitivity` | Sensibilidade do mouse (graus/pixel) |

| Método | Descrição |
|---|---|
| `moveForward/Back/Left/Right/Up/Down(dt)` | Move a câmera nas direções cardinais |
| `rotate(xOff, yOff)` | Atualiza yaw/pitch e recalcula os vetores de direção |
| `getViewMatrix()` | Retorna `lookAt(position, position + front, up)` |

- Mouse capturado pela janela (`GLFW_CURSOR_DISABLED`)
- `cursor_callback` computa o delta do mouse e chama `camera.rotate()`
- Flag `firstMouse` evita salto na primeira captura
- View matrix e posição da câmera atualizadas a cada frame no shader

**Controles:**

| Tecla / Entrada | Ação |
|---|---|
| **Mouse** | Girar câmera (yaw / pitch) |
| `W` / `A` / `S` / `D` | Mover câmera (frente / esq / trás / dir) |
| `Espaço` | Câmera sobe |
| `C` | Câmera desce |
| `TAB` | Selecionar próximo objeto |
| `Seta ←` / `Seta →` | Transladar objeto no eixo X |
| `Seta ↑` / `Seta ↓` | Transladar objeto no eixo Z |
| `Page Up` / `Page Down` | Transladar objeto no eixo Y |
| `X` / `Y` / `Z` | Toggle rotação contínua no eixo |
| `[` / `]` | Diminuir / Aumentar escala |
| `ESC` | Fechar |

---

## Estrutura do repositório

```
CGCCHibrido/
├── src/
│   ├── desafioM2.cpp              # Cubos com transformações (geometria hard-coded)
│   ├── atividade_vivencial_M2.cpp # Múltiplos OBJs, seleção, sem textura
│   ├── desafioM3.cpp              # OBJ + MTL + textura
│   ├── desafioM4.cpp              # Phong com coeficientes do MTL
│   ├── atividade_vivencial_M4.cpp # Three-point lighting com atenuação
│   ├── desafioM5.cpp              # Câmera em primeira pessoa
│   ├── Hello3D.cpp                # Pirâmide com rotação (exercício base)
│   ├── TriangleTex.cpp            # Triângulos texturizados (exercício base)
│   └── SpherePhong.cpp            # Esfera com Phong procedural (exercício base)
├── assets/
│   └── Modelos3D/
│       ├── Suzanne.obj / .mtl / .png
│       ├── SuzanneSubdiv1.obj / .mtl
│       ├── SuzanneUV.png
│       └── Cube.obj / .mtl
├── include/
│   └── glad/
├── common/
│   └── glad.c
├── Code snippets/
│   ├── LoadSimpleOBJ.cpp          # Função base de leitura OBJ (referência do professor)
│   └── LoadSimpleOBJ.md
└── CMakelists.txt
```

---

## Progressão dos desafios

```
desafioM2               → geometria hard-coded, sem OBJ
        ↓
atividade_vivencial_M2  → leitura de OBJ, múltiplos objetos, sem textura
        ↓
desafioM3               → + textura (MTL map_Kd + stb_image)
        ↓
desafioM4               → + Phong completo (Ka, Kd, Ks, Ns do MTL)
        ↓
atividade_vivencial_M4  → + three-point lighting com atenuação por distância
        ↓
desafioM5               → + câmera em primeira pessoa (classe Camera)
```
