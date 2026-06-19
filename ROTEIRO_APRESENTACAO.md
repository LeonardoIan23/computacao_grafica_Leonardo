# Roteiro de Apresentação — Diorama Estação de Metrô

**Aluno:** Leonardo Ian de Oliveira  
**Projeto:** cenaFinal — Computação Gráfica / Unisinos

> **Como usar este arquivo:**  
> `[FALAR]` → texto de narração (leia em voz alta)  
> `[FAZER]` → ação na tela  
> `[CÓDIGO]` → linha exata no arquivo para apontar  
> `[PERGUNTA DO PROF]` → resposta preparada para a defesa presencial

---

## PARTE 1 — Introdução (30 segundos)

**[FAZER]** Abra o terminal na pasta `build/` e execute:
```
.\cenaFinal.exe
```

**[FALAR]**
> "Olá, meu nome é Leonardo Ian de Oliveira, aluno da disciplina de Computação Gráfica da Unisinos. Vou apresentar meu projeto final: um diorama 3D de uma estação de metrô, implementado em C++ com OpenGL 4.5. O projeto integra todos os conceitos trabalhados ao longo do semestre: carregamento de modelos OBJ, iluminação de Phong com três fontes de luz, câmera em primeira pessoa, animação por curva de Bézier e geometria procedural."

---

## PARTE 2 — Câmera em Primeira Pessoa

**[FALAR]**
> "A câmera funciona em modo primeira pessoa. O mouse controla a direção do olhar, e as teclas W, A, S, D movem a câmera pelo espaço. Espaço sobe e C desce."

**[FAZER]**
- Mova o mouse lentamente para a esquerda e direita
- Pressione W para andar em direção à plataforma
- Pressione S para recuar
- Pressione Espaço para subir e ver a cena de cima
- Pressione C para descer de volta

---

## PARTE 3 — Navegação e Seleção de Objetos

**[FALAR]**
> "A cena possui múltiplos objetos. Pressionar TAB cicla a seleção entre eles. O objeto selecionado aparece em destaque visual — é o uniform 'highlight' no shader que altera o brilho."

**[FAZER]**
- Pressione TAB algumas vezes lentamente, mostrando como o destaque muda de objeto

---

## PARTE 4 — Translação, Rotação e Escala

**[FALAR]**
> "Com um objeto selecionado que não seja o trem, posso transladá-lo com as setas do teclado nos eixos X e Z, e com Page Up e Page Down no eixo Y."

**[FAZER]**
- Selecione um banco com TAB
- Pressione seta direita/esquerda para mover no X
- Pressione Page Up para mover para cima, Page Down para baixo

**[FALAR]**
> "As teclas X, Y e Z ativam rotação contínua no respectivo eixo. Os colchetes [ e ] diminuem e aumentam a escala do objeto selecionado."

**[FAZER]**
- Pressione Y — o banco começa a girar
- Pressione ] algumas vezes para aumentar a escala
- Pressione [ para reduzir de volta
- Pressione Y novamente para parar a rotação

---

## PARTE 5 — Materiais e Texturas

**[FALAR]**
> "Cada objeto na cena tem seus próprios coeficientes de material lidos do arquivo MTL: Ka para o componente ambiente, Kd para o difuso e Ks para o especular, mais o expoente Ns. O chão usa textura de concreto, as paredes usam textura de tijolos. Os modelos OBJ têm suas próprias texturas definidas nos arquivos MTL."

**[FAZER]**
- Posicione a câmera olhando para o chão (textura de concreto)
- Gire para mostrar as paredes (textura de tijolos)
- Aproxime a câmera de um banco ou poste para mostrar a textura do modelo

**[FALAR]**
> "A geometria procedural — plataforma, trilhos, travessas e caixas de luminária — usa materiais definidos diretamente no código C++. Os trilhos têm especularidade alta para simular metal, e as caixas de luminária têm Ka máximo e Kd zero para parecerem auto-iluminadas."

---

## PARTE 6 — Iluminação de Phong com 3 Luzes

**[FALAR]**
> "A iluminação implementa o modelo de Phong com três fontes de luz configuradas no arquivo scene.json: Key Light, Fill Light e Back Light. O fragment shader acumula a contribuição difusa e especular de cada luz individualmente, calculando o componente ambiente apenas uma vez."

**[FAZER]**
- Mova a câmera para uma posição ampla que mostre bem a iluminação da cena
- Aponte para os postes com caixas de luz (luminária emissiva)
- Mova devagar para mostrar como a especularidade dos trilhos brilha com a câmera se movendo

**[FALAR]**
> "As três luzes são: Key Light na posição central à frente, Fill Light à esquerda com tom azulado para suavizar sombras, e Back Light ao fundo com tom quente para dar profundidade. Cada uma tem posição, cor e intensidade definidas no JSON."

---

## PARTE 7 — Animação por Curva de Bézier

**[FALAR]**
> "O trem percorre a cena seguindo uma curva de Bézier cúbica, com quatro pontos de controle que definem a trajetória de entrada ao lado esquerdo até a saída pelo lado direito. O ciclo se repete continuamente."

**[FAZER]**
- Posicione a câmera de frente para ver o trem atravessar a estação
- Aguarde o trem aparecer ou caminhe com a câmera para acompanhá-lo

**[FALAR]**
> "Pressionar P pausa e retoma a animação."

**[FAZER]**
- Pressione P para pausar com o trem em algum ponto visível
- Mostre o trem parado
- Pressione P novamente para retomar

---

## PARTE 8 — Arquivo de Configuração de Cena (JSON)

**[FALAR]**
> "Toda a cena é carregada de um arquivo JSON externo: assets/scene.json. Vou abrir para mostrar a estrutura."

**[FAZER]**
- Abra `assets/scene.json` no editor
- Role o arquivo lentamente

**[FALAR]**
> "O JSON define a janela, os parâmetros da câmera, as três fontes de luz com nome, posição, cor, intensidade e flag de habilitado, e a lista de objetos com caminho do OBJ, posição, rotação, escala — e para o trem, o bloco de animação com tipo Bézier, velocidade e os quatro pontos de controle. Isso torna o visualizador reutilizável para qualquer cena sem recompilar."

---

## PARTE 9 — Encerramento do Vídeo

**[FALAR]**
> "O projeto está disponível no repositório no GitHub com o README completo, instruções de compilação, créditos dos assets e referências. Todos os modelos são do Sketchfab sob licença CC-BY 4.0, e as texturas do Poly Haven sob CC0. O maior desafio técnico foi a geometria procedural das aberturas de túnel e o cálculo correto da posição das luminárias a partir dos vértices do OBJ do poste. Obrigado."

---

---

# PARTE 10 — Defesa Presencial: Perguntas do Professor

> Abra `src/cenaFinal.cpp` na IDE antes de entrar. Deixe estas seções marcadas com bookmarks ou divida a tela.

---

### P1: "Onde está o parser do arquivo de configuração de cena?"

**[CÓDIGO]** → `src/cenaFinal.cpp`, linha **619**

```
bool setupScene(const json& j)         // linha 619 — início do parser
```

**[FALAR]**
> "A função setupScene, que começa na linha 619, recebe o objeto JSON já lido do arquivo scene.json. Ela lê a câmera nas linhas 622 a 628, depois as luzes a partir da linha 631 — iterando o array 'lights' e carregando posição, cor e intensidade de cada uma. A partir da linha 666 percorre o array 'objects': lê caminho do OBJ, posição, rotação, escala, chama loadSimpleOBJ para criar o VAO, e na linha 708 lê o bloco de animação se existir — carregando os quatro pontos de controle do Bézier."

---

### P2: "Onde ocorre a passagem de uniforms ao shader?"

**[CÓDIGO]** → `src/cenaFinal.cpp`, linhas **789 a 801** (localização dos uniforms), depois **829 a 831** (envio das luzes) e **885 a 887** (view matrix por frame) e **925 a 936** (model matrix + material por objeto)

**[FALAR]**
> "As localizações dos uniforms são obtidas uma vez após a compilação do shader, nas linhas 789 a 801, usando glGetUniformLocation. Os dados das três luzes são enviados na linha 829 com glUniform3fv passando o array completo de posições e cores. A cada frame, na linha 885, a view matrix é atualizada com glUniformMatrix4fv usando a posição atual da câmera. Para cada objeto renderizado, nas linhas 925 a 936, a model matrix é construída e enviada, seguida dos coeficientes Ka, Kd, Ks e Ns lidos do MTL."

---

### P3: "Onde está o cálculo de iluminação no Fragment Shader?"

**[CÓDIGO]** → `src/cenaFinal.cpp`, linhas **180 a 213**

**[FALAR]**
> "O fragment shader está definido como string no código, começando na linha 180. Na linha 202 o componente ambiente é calculado uma única vez: Ka multiplicado pela cor da textura. A partir da linha 204 um loop percorre as três luzes. Para cada luz: na linha 205 calcula o vetor L de direção da luz, na linha 206 o vetor R de reflexão. Na linha 207 acumula o componente difuso: Kd vezes o produto escalar entre N e L, multiplicado pela cor da textura e pela cor da luz. Na linha 208 acumula o especular: Ks vezes o cosseno do ângulo de reflexão elevado ao expoente Ns."

---

### P4: "Onde está a manipulação das matrizes Model e View?"

**[CÓDIGO — Model]** → `src/cenaFinal.cpp`, linhas **925 a 931**

```cpp
mat4 model = mat4(1.f);                          // linha 925 — identidade
model = translate(model, obj.position);          // linha 926 — translação
model = rotate(model, obj.rotAngleX, ...);       // linhas 927-929 — rotação XYZ
model = scale(model, obj.scaleXYZ);              // linha 930 — escala
glUniformMatrix4fv(modelLoc, 1, ...);            // linha 931 — envia ao shader
```

**[CÓDIGO — View]** → `src/cenaFinal.cpp`, linha **885**

```cpp
mat4 view = camera.getViewMatrix();              // linha 885
glUniformMatrix4fv(viewLoc, 1, ...);             // linha 886
```

**[CÓDIGO — Normal Matrix]** → vertex shader, linha **175**

```glsl
fragNormal = mat3(transpose(inverse(model))) * normalIn;   // linha 175
```

**[FALAR]**
> "A model matrix de cada objeto é construída a cada frame nas linhas 925 a 930: começa como identidade, aplica translação, depois três rotações nos eixos X, Y e Z, e por último a escala. Essa ordem garante que a escala não afete as rotações e que o objeto seja posicionado corretamente no mundo. A view matrix é obtida na linha 885 chamando camera.getViewMatrix(), que internamente usa lookAt com a posição, posição mais front e o vetor up da câmera. No vertex shader, na linha 175, a normal é transformada pela matriz inversa transposta da model para lidar corretamente com escalas não uniformes."

---

### P5: "Como funciona a animação de Bézier?"

**[CÓDIGO]** → `src/cenaFinal.cpp`, linhas **148 a 155** (função) e **919 a 922** (atualização no loop)

**[FALAR]**
> "A função bezierCubic nas linhas 148 a 155 implementa a fórmula da curva cúbica: B de t é igual a u³ vezes P0, mais 3 vezes u² vezes t vezes P1, mais 3 vezes u vezes t² vezes P2, mais t³ vezes P3, onde u é 1 menos t. No loop de renderização, nas linhas 919 a 922, o parâmetro t é incrementado por dt multiplicado pela velocidade, e ao ultrapassar 1 é subtraído de 1 para criar o ciclo. A posição do objeto é então recalculada chamando bezierCubic com os quatro pontos de controle lidos do JSON."

---

### P6: "Como a textura é aplicada?"

**[CÓDIGO]** → `src/cenaFinal.cpp`, linha **705** (carregamento) e linha **939** (bind por objeto) e fragment shader linha **195**

**[FALAR]**
> "A textura de cada objeto é carregada na linha 705 usando loadTexture, que internamente usa stb_image para ler a imagem e gera mipmaps. O caminho da textura vem do campo map_Kd do arquivo MTL. No loop de renderização, na linha 939, glBindTexture associa a textura do objeto ao slot 0 antes do draw call. No fragment shader, na linha 195, a cor da textura é amostrada com a função texture usando as coordenadas UV interpoladas pelo rasterizador."

---

### P7: "Como os coeficientes Ka, Kd, Ks são lidos do MTL?"

**[CÓDIGO]** → Procure a função `parseMTL` em `src/cenaFinal.cpp` (por volta da linha 370)

**[FALAR]**
> "A função parseMTL lê o arquivo MTL linha por linha. Quando encontra a palavra 'Ka' lê os três componentes RGB e armazena em mat.Ka. Idem para 'Kd' em mat.Kd, 'Ks' em mat.Ks, 'Ns' em mat.Ns como float, e 'map_Kd' para o caminho da textura difusa. Esses valores ficam na struct Material e são passados ao shader como uniforms antes de cada draw call."

---

### P8: "Por que você usou um único arquivo em vez de classes separadas?"

**[FALAR]**
> "O cenaFinal foi desenvolvido como arquivo único e auto-contido por uma escolha de simplicidade para a entrega final. Paralelamente desenvolvi o desafioM6 com arquitetura orientada a objetos completa — classes Shader, Mesh, Model, Camera, Bezier, Trajectory, SceneObject e SceneLoader em arquivos separados. O cenaFinal reutiliza os mesmos conceitos implementados de forma monolítica para facilitar a avaliação: um único arquivo compila e executa sem dependências de headers externos além das bibliotecas."

---

# Checklist de verificação antes de gravar / antes da defesa

- [ ] `.\cenaFinal.exe` abre sem erros no terminal
- [ ] A cena carrega com trem animado, 5 bancos, 4 postes, lixeira
- [ ] Mouse rotaciona a câmera
- [ ] W/A/S/D movem a câmera
- [ ] TAB cicla entre objetos com destaque visual
- [ ] Setas transladam objeto selecionado (banco)
- [ ] Y ativa rotação, ] aumenta escala, [ diminui
- [ ] P pausa e retoma o trem
- [ ] Trem atravessa de um lado ao outro e reinicia
- [ ] IDE com `src/cenaFinal.cpp` aberto e `assets/scene.json` aberto em aba separada
- [ ] Linha 148 visível (Bézier), linha 202 visível (Fragment Shader), linha 619 visível (JSON parser)
