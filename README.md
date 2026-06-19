# Computação Gráfica — CGCCHibrido

**Aluno:** Leonardo Ian de Oliveira  
**Disciplina:** Processamento Gráfico / Computação Gráfica — Unisinos

Repositório com os exercícios e atividades desenvolvidos ao longo da disciplina usando **OpenGL 4.5**, **GLFW 3.4**, **GLM**, **stb_image** e **nlohmann/json**.

---

## Dependências

Todas as dependências são baixadas automaticamente pelo CMake via `FetchContent`:

| Biblioteca | Uso |
|---|---|
| [GLFW 3.4](https://www.glfw.org/) | Janela e entrada de teclado/mouse |
| [GLM](https://github.com/g-truc/glm) | Matemática (vetores, matrizes) |
| [stb_image](https://github.com/nothings/stb) | Carregamento de texturas |
| [nlohmann/json](https://github.com/nlohmann/json) | Leitura de arquivos JSON (cenaFinal e desafioM6) |
| GLAD (local em `common/`) | Carregamento das funções OpenGL |

---

## Como compilar

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Os executáveis são gerados dentro da pasta `build/`.

> **Importante:** execute os programas sempre a partir da pasta `build/`, pois os caminhos dos modelos e texturas são relativos a ela.

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
- Layout do vértice: `pos(3) + uv(2) + normal(3)` = 8 floats por vértice
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
- Fator de atenuação por distância: `1 / (Kc + Kl×d + Kq×d²)`
- Fragment shader com Phong acumulado sobre as 3 luzes (ambiente calculado uma vez)
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

### desafioM6 — Arquitetura OOP e Trajetórias Cíclicas

**Executável:** `build/desafioM6.exe`  
**Arquivos:** `src/desafioM6.cpp` + módulos em `src/` e `include/`

Refatoração completa em arquitetura orientada a objetos. Mantém todas as funcionalidades do desafioM5 com maior modularidade e adiciona suporte a trajetórias cíclicas multi-ponto.

**Arquitetura (módulos):**

| Classe / Módulo | Arquivo | Responsabilidade |
|---|---|---|
| `Shader` | `src/Shader.cpp` | Compilação, linkagem e envio de uniforms GLSL |
| `Mesh` | `src/Mesh.cpp` | VAO/VBO, atributos de vértice, `glDrawArrays` |
| `Model` | `src/Model.cpp` | Parser OBJ/MTL, textura com stb_image, draw |
| `Camera` | `src/Camera.cpp` | Câmera FPS: lookAt, perspectiva, input de mouse |
| `Bezier` | `src/Bezier.cpp` | Curva de Bézier cúbica `B(t)`, update por dt |
| `Trajectory` | `src/Trajectory.cpp` | Trajetória cíclica com N waypoints (interpolação linear) |
| `SceneObject` | `src/SceneObject.cpp` | Objeto com transformações TRS e animações opcionais |
| `SceneLoader` | `src/SceneLoader.cpp` | Parser do `scene.json` via nlohmann/json |

**O que implementa:**
- Curva de Bézier cúbica: `B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃`
- Trajetória cíclica com N pontos, t percorrendo `[0, N-1]` ciclicamente
- Cena configurada via `scene.json` (câmera, luzes, lista de objetos, animações)

---

## cenaFinal — Avaliação Final: Diorama da Estação de Metrô

> **Este é o executável avaliado pelo professor.**

**Executável:** `build/cenaFinal.exe`  
**Arquivo principal:** `src/cenaFinal.cpp` (arquivo único, auto-contido, ~950 linhas)  
**Configuração:** `assets/scene.json`

### Como compilar e executar

```powershell
mkdir build
cd build
cmake ..
cmake --build . --target cenaFinal
cd build
.\cenaFinal.exe
```

O programa lê a cena a partir de `../assets/scene.json` por padrão, ou de um arquivo JSON passado como argumento:

```powershell
.\cenaFinal.exe ../assets/scene.json
```

---

### O que implementa

O cenaFinal integra todos os conceitos desenvolvidos ao longo do semestre em um único arquivo de código C++:

**Câmera e visualização:**
- Câmera em primeira pessoa (FPS) com controle por mouse e WASD
- Projeção perspectiva configurável via JSON (`fov`, `near`, `far`)
- View matrix recalculada a cada frame

**Modelos 3D:**
- Carregamento de arquivos `.OBJ` com triangulação por fan
- Parser de `.MTL`: `Ka`, `Kd`, `Ks`, `Ns` e `map_Kd` (textura difusa)
- Layout de vértice: `pos(3) + uv(2) + normal(3)` = 8 floats por vértice
- Textura branca 1×1 como fallback quando MTL não define `map_Kd`

**Iluminação — Modelo de Phong com 3 luzes:**
- **Ambiente:** `Ka × texColor` (calculado uma única vez)
- **Difusa:** `Kd × max(dot(N,L), 0) × texColor × lightColor[i]`
- **Especular:** `Ks × pow(max(dot(R,V), 0), Ns) × lightColor[i]`
- Acumulação sobre **3 luzes independentes** (Key / Fill / Back Light)
- Luzes, posições e intensidades configuradas via `scene.json`

**Geometria procedural:**
- **Plataforma elevada** (H = 0,8 m) em toda a extensão da estação
- **2 trilhos metálicos** com material de alta especularidade (`Ns=80`) e **travessas** a cada 0,6 m
- **Aberturas de túnel** nas paredes laterais (onde o trem passa), cada parede dividida em 3 quads com um quad escuro de fundo atrás da abertura
- **Caixas de luminária** emissivas nas posições reais das lanternas dos postes (calculadas a partir dos vértices do `.OBJ`)

**Animação:**
- Curva de Bézier cúbica aplicada ao trem: `B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃`
- Percurso cíclico com `t ∈ [0,1]`, reiniciando automaticamente
- Pontos de controle e velocidade configuráveis via `scene.json`

**Seleção e edição de objetos:**
- `TAB` cicla pelos objetos da cena
- Objeto selecionado recebe highlight visual (`uniform float highlight`)
- Translação, rotação contínua por eixo e escala em tempo real
- Translação manual desativada automaticamente em objetos com animação ativa

**Configuração via scene.json:**
- Janela (`width`, `height`, `title`)
- Câmera (`position`, `yaw`, `pitch`, `fov`, `near`, `far`)
- Texturas do ambiente (`floor_texture`, `wall_texture`)
- Array de luzes (`lights`): `name`, `position`, `color`, `intensity`, `enabled`
- Array de objetos (`objects`): `name`, `obj`, `position`, `rotation`, `scale`, `animation`

---

### Controles

| Tecla / Entrada | Ação |
|---|---|
| **Mouse** | Girar câmera (yaw / pitch) |
| `W` / `A` / `S` / `D` | Mover câmera (frente / esq / trás / dir) |
| `Espaço` | Câmera sobe |
| `C` | Câmera desce |
| `TAB` | Selecionar próximo objeto |
| `Seta ←` / `Seta →` | Transladar objeto selecionado (eixo X) |
| `Seta ↑` / `Seta ↓` | Transladar objeto selecionado (eixo Z) |
| `Page Up` / `Page Down` | Transladar objeto selecionado (eixo Y) |
| `X` / `Y` / `Z` | Toggle rotação contínua no eixo |
| `[` / `]` | Diminuir / Aumentar escala |
| `P` | Pausar / retomar animações |
| `ESC` | Fechar |

> Translação manual é desativada automaticamente para objetos com animação ativa (ex.: o trem).

---

### Objetos na cena

| Objeto | Quantidade | Posição na cena |
|---|---|---|
| Trem | 1 | Animado por Bézier, atravessa a estação no eixo X |
| Banco | 5 | Sobre a plataforma, espaçados ao longo do eixo X |
| Poste | 4 | Alinhados na borda traseira da plataforma |
| Lixeira | 1 | Sobre a plataforma, próxima à extremidade esquerda |

---

### Assets utilizados — Modelos 3D

| Modelo | Autor | Link | Licença |
|---|---|---|---|
| Trem (lowpoly) | Ajaya Tamang Moktan (@ajaa) | [Sketchfab](https://sketchfab.com/3d-models/lowpoly-3d-train-eefc996de2be46fa8eccc6853c765aa4) | CC-BY 4.0 |
| Banco de parque | tadeus | [Sketchfab](https://sketchfab.com/3d-models/low-poly-park-bench-3a6019602c234c13a376c55a6c52c0b8) | CC-BY 4.0 |
| Poste de luz | Memorie | [Sketchfab](https://sketchfab.com/3d-models/low-poly-lamp-post-c466684e819a4428b6d8ed50537615e4) | CC-BY 4.0 |
| Lixeira | katykatе | [Sketchfab](https://sketchfab.com/3d-models/trash-can-low-poly-6dfba42794e445719010caf0a1ceca7c) | CC-BY 4.0 |

### Assets utilizados — Texturas

| Textura | Uso | Fonte | Link | Licença |
|---|---|---|---|---|
| Concrete Floor | Piso e plataforma | Poly Haven | [polyhaven.com](https://polyhaven.com/a/concrete_floor) | CC0 |
| Brick Wall 005 | Paredes | Poly Haven | [polyhaven.com](https://polyhaven.com/a/brick_wall_005) | CC0 |

---

### Estrutura de arquivos do cenaFinal

```
assets/
├── scene.json              # Configuração da cena (câmera, luzes, objetos)
├── texturas/
│   ├── concrete.jpg        # Textura do piso e plataforma
│   └── brick.jpg           # Textura das paredes
├── trem/
│   ├── trem.obj
│   └── trem.mtl
├── banco/
│   ├── banco.obj
│   └── banco.mtl
├── poste/
│   ├── poste.obj
│   └── poste.mtl
└── lixeira/
    ├── lixeira.obj
    └── lixeira.mtl
```

---

## Estrutura completa do repositório

```
CGCCHibrido/
├── src/
│   ├── cenaFinal.cpp              # Avaliacao final — diorama completo (arquivo unico)
│   ├── desafioM2.cpp              # Cubos com transformacoes (geometria hard-coded)
│   ├── atividade_vivencial_M2.cpp # Multiplos OBJs, selecao, sem textura
│   ├── desafioM3.cpp              # OBJ + MTL + textura
│   ├── desafioM4.cpp              # Phong com coeficientes do MTL
│   ├── atividade_vivencial_M4.cpp # Three-point lighting com atenuacao
│   ├── desafioM5.cpp              # Camera em primeira pessoa
│   ├── desafioM6.cpp              # Arquitetura OOP + trajetorias ciclicas
│   ├── Shader.cpp / Camera.cpp / Mesh.cpp / Model.cpp
│   ├── Bezier.cpp / Trajectory.cpp / SceneObject.cpp / SceneLoader.cpp
│   ├── Hello3D.cpp                # Piramide com rotacao (exercicio base)
│   ├── TriangleTex.cpp            # Triangulos texturizados (exercicio base)
│   └── SpherePhong.cpp            # Esfera com Phong procedural (exercicio base)
├── assets/
│   ├── scene.json                 # Configuracao da cena final
│   ├── texturas/                  # Texturas do ambiente (concreto, tijolo)
│   ├── trem/ banco/ poste/ lixeira/   # Modelos 3D dos objetos
│   └── Modelos3D/                 # Suzanne, Cube (exercicios anteriores)
├── include/
│   ├── glad/                      # GLAD (glad.h)
│   ├── nlohmann/json.hpp          # nlohmann/json header-only
│   ├── Shader.h / Camera.h / Mesh.h / Model.h
│   ├── Bezier.h / Trajectory.h / SceneObject.h / SceneLoader.h
│   └── stb_image.h
├── common/
│   └── glad.c
├── Code snippets/
│   ├── LoadSimpleOBJ.cpp
│   └── LoadSimpleOBJ.md
└── CMakelists.txt
```

---

## Progressão dos desafios

```
desafioM2               → geometria hard-coded, sem OBJ
        ↓
atividade_vivencial_M2  → leitura de OBJ, multiplos objetos, sem textura
        ↓
desafioM3               → + textura (MTL map_Kd + stb_image)
        ↓
desafioM4               → + Phong completo (Ka, Kd, Ks, Ns do MTL)
        ↓
atividade_vivencial_M4  → + three-point lighting com atenuacao por distancia
        ↓
desafioM5               → + camera em primeira pessoa (classe Camera)
        ↓
desafioM6               → + arquitetura OOP modular + trajetorias ciclicas
        ↓
cenaFinal               → Avaliacao Final: diorama completo auto-contido,
                          3 luzes Phong, Bezier cubica, geometria procedural,
                          configuracao via JSON
```

---

## Referências

- [LearnOpenGL](https://learnopengl.com/) — tutoriais de OpenGL moderno
- [GLM Documentation](https://glm.g-truc.net/) — biblioteca matemática
- [OpenGL Reference Pages](https://registry.khronos.org/OpenGL-Refpages/gl4/) — documentação oficial
- [nlohmann/json](https://github.com/nlohmann/json) — parser JSON header-only
- [stb_image](https://github.com/nothings/stb) — carregamento de imagens
- [Poly Haven](https://polyhaven.com/) — texturas PBR gratuitas (CC0)
- [Sketchfab](https://sketchfab.com/) — modelos 3D (CC-BY 4.0)
