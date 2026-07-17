# Projeto: Fractal de Mandelbrot (Lua + C)

Visualizador interativo do fractal de Mandelbrot (e sua generalizacao, o
Multibrot, Z^n + C), onde o calculo matematico e feito em **Lua** e a
interface grafica e feita em **C**, usando a biblioteca
**raylib**.


## Estrutura do projeto

* `main.c` — interface: entrada do usuario, thread do Lua, conversao de
  resultados em cores, desenho na tela via raylib.
* `mandelbrot.lua` — calculo: algoritmo Mandelbrot/Multibrot.
* `Makefile` — compilacao (`build`) e execucao de um caso de estudo
  (`run-caso`).


## Requisitos

* raylib
* lua
* gcc


## Como compilar e executar

Gerar o executavel:
```bash
make build
```

Executar o caso de estudo:
```bash
make run-caso
```


## Comandos da interface

* **Navegacao:** setas do teclado para deslocar, `Z`/`X` para zoom.
* **Deformacao:** `Q`/`W` para alterar a potencia da formula (Z^n).
* **Estilizacao:** segure `C` ou `V` para rotacionar a paleta de cores.
* **Modos de coloracao (`1`, `2`, `3`):** escalonado, suavizado e fase magnetica.


## Autor
* Eduarda Louzada
