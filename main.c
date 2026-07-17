#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

//inclusao da API C do Lua
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define WIDTH 700
#define HEIGHT 700
#define DEFAULT_MAX_ITER 100

typedef struct {
    double n;
    double zr;
    double zi;
} PixelData;

typedef enum { STATE_IDLE, STATE_CALCULATING } AppState;

//argumentos passados para a thread que executa o calculo em Lua
typedef struct {
    lua_State *L;
    double offsetX;
    double offsetY;
    double zoom;
    double power;
    int maxIter;
    PixelData *fractalData;
} LuaThreadArgs;

//variaveis globais compartilhadas entre a thread principal (raylib) e a
//thread secundaria (calculo em Lua)
volatile bool lua_is_running = false;
LuaThreadArgs thread_args;


//THREAD SECUNDARIA: roda a VM do Lua sem congelar a interface grafica
void* LuaBackgroundThread(void* arg) {
    LuaThreadArgs *args = (LuaThreadArgs*)arg;
    lua_State *L = args->L;

    //busca a funcao "calcular_frame" definida em mandelbrot.lua
    lua_getglobal(L, "calcular_frame");

    //empilha os argumentos na ordem esperada pela funcao Lua
    lua_pushinteger(L, WIDTH);
    lua_pushinteger(L, HEIGHT);
    lua_pushnumber(L, args->offsetX);
    lua_pushnumber(L, args->offsetY);
    lua_pushnumber(L, args->zoom);
    lua_pushnumber(L, args->power);
    lua_pushinteger(L, args->maxIter);

    //executa a funcao Lua (7 argumentos, 1 valor de retorno esperado)
    if (lua_pcall(L, 7, 1, 0) != LUA_OK) {
        printf("Erro interno no Lua: %s\n", lua_tostring(L, -1));
    } else {
        //o retorno e uma unica string binaria contendo todos os pixels
        //ja empacotados
        size_t tamanho_buffer;
        const char* resultado_binario = lua_tolstring(L, -1, &tamanho_buffer);

        //copia os bytes direto para o buffer de pixels em C
        memcpy(args->fractalData, resultado_binario, tamanho_buffer);

        //remove a string resultante do topo da pilha Lua
        lua_pop(L, 1);
    }

    //sinaliza a thread principal que o buffer de pixels esta pronto
    lua_is_running = false;
    return NULL;
}


//converte o buffer de resultados (iteracoes + posicao final no plano
//complexo) em cores e desenha em Image
void UpdateFractalImage(Image *img, PixelData *data, int mode, float colorShift, int maxIter) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int index = y * WIDTH + x;
            double n = data[index].n;
            double zr = data[index].zr;
            double zi = data[index].zi;

            Color c = BLACK;

            if (n < maxIter) {
                float base_hue = 0.0f;
                if (mode == 0) {
                    base_hue = (n / maxIter) * 360.0f;
                } else if (mode == 1) {
                    double mag = sqrt(zr * zr + zi * zi);
                    double mu = n + 1.0 - log2(log2(mag));
                    base_hue = (mu / maxIter) * 360.0f;
                } else if (mode == 2) {
                    double angle = atan2(zi, zr) * (180.0 / PI);
                    if (angle < 0) angle += 360.0;
                    base_hue = angle;
                }

                float final_hue = fmod(base_hue + colorShift, 360.0f);
                if (final_hue < 0) final_hue += 360.0f;
                c = ColorFromHSV(final_hue, 0.85f, 1.0f);
            }
            ImageDrawPixel(img, x, y, c);
        }
    }
}

int main(int argc, char *argv[]) {
    double offsetX = -0.5, offsetY = 0.0, zoom = 1.0, power = 2.0;
    int maxIter = DEFAULT_MAX_ITER;

    if (argc >= 5) {
        offsetX = atof(argv[1]);
        offsetY = atof(argv[2]);
        zoom    = atof(argv[3]);
        power   = atof(argv[4]);
    }
    if (argc >= 6) {
        maxIter = atoi(argv[5]);
    }

    //inicializa a maquina virtual Lua
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    //carrega o script Lua
    if (luaL_dofile(L, "mandelbrot.lua") != LUA_OK) {
        printf("Erro ao compilar mandelbrot.lua: %s\n", lua_tostring(L, -1));
        return 1;
    }

    InitWindow(WIDTH, HEIGHT, "Mandelbrot Explorer - Lua + Raylib");
    SetTargetFPS(60);

    PixelData *fractalData = (PixelData *)malloc(WIDTH * HEIGHT * sizeof(PixelData));
    Image img = GenImageColor(WIDTH, HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(img);

    float colorShift = 240.0f;
    int currentMode = 1;

    AppState state = STATE_IDLE;
    pthread_t thread_id;

    float loading_time = 0.0f;
    float loading_rotation = 0.0f;

    //configura os argumentos da thread para a primeira execucao
    thread_args.L = L;
    thread_args.fractalData = fractalData;
    thread_args.offsetX = offsetX;
    thread_args.offsetY = offsetY;
    thread_args.zoom = zoom;
    thread_args.power = power;
    thread_args.maxIter = maxIter;

    state = STATE_CALCULATING;
    lua_is_running = true;
    pthread_create(&thread_id, NULL, LuaBackgroundThread, &thread_args);

    while (!WindowShouldClose()) {

        if (state == STATE_IDLE) {
            bool needsRecalc = false;
            bool needsRedraw = false;

            float moveSpeed = 0.3f / zoom;

            if (IsKeyPressed(KEY_RIGHT)) { offsetX += moveSpeed; needsRecalc = true; }
            if (IsKeyPressed(KEY_LEFT))  { offsetX -= moveSpeed; needsRecalc = true; }
            if (IsKeyPressed(KEY_DOWN))  { offsetY += moveSpeed; needsRecalc = true; }
            if (IsKeyPressed(KEY_UP))    { offsetY -= moveSpeed; needsRecalc = true; }
            if (IsKeyPressed(KEY_Z))     { zoom *= 1.5f; needsRecalc = true; }
            if (IsKeyPressed(KEY_X))     { zoom /= 1.5f; needsRecalc = true; }

            if (IsKeyPressed(KEY_W))     { power += 1.0f; needsRecalc = true; }
            if (IsKeyPressed(KEY_Q))     { power -= 1.0f; needsRecalc = true; }

            if (IsKeyDown(KEY_C))        { colorShift += 3.0f; needsRedraw = true; }
            if (IsKeyDown(KEY_V))        { colorShift -= 3.0f; needsRedraw = true; }

            if (IsKeyPressed(KEY_ONE) && currentMode != 0)   { currentMode = 0; needsRedraw = true; }
            if (IsKeyPressed(KEY_TWO) && currentMode != 1)   { currentMode = 1; needsRedraw = true; }
            if (IsKeyPressed(KEY_THREE) && currentMode != 2) { currentMode = 2; needsRedraw = true; }

            if (needsRecalc) {
                state = STATE_CALCULATING;
                loading_time = 0.0f;

                //atualiza os parametros e dispara o Lua na thread em background
                thread_args.offsetX = offsetX;
                thread_args.offsetY = offsetY;
                thread_args.zoom = zoom;
                thread_args.power = power;
                thread_args.maxIter = maxIter;

                lua_is_running = true;
                pthread_create(&thread_id, NULL, LuaBackgroundThread, &thread_args);
            }
            else if (needsRedraw) {
                UpdateFractalImage(&img, fractalData, currentMode, colorShift, maxIter);
                UpdateTexture(texture, img.data);
            }
        }
        else if (state == STATE_CALCULATING) {
            loading_time += GetFrameTime();
            loading_rotation += 250.0f * GetFrameTime();

            if (!lua_is_running) {
                pthread_join(thread_id, NULL);

                UpdateFractalImage(&img, fractalData, currentMode, colorShift, maxIter);
                UpdateTexture(texture, img.data);

                state = STATE_IDLE;
            }
        }

        BeginDrawing();

        if (state == STATE_CALCULATING) {
            DrawTexture(texture, 0, 0, GRAY);
            DrawRectangle(0, HEIGHT/2 - 60, WIDTH, 120, Fade(BLACK, 0.8f));
            DrawRing((Vector2){ WIDTH/2 - 140, HEIGHT/2 }, 15, 22, loading_rotation, loading_rotation + 270, 32, YELLOW);
            DrawText("A PROCESSAR NO LUA...", WIDTH/2 - 100, HEIGHT/2 - 15, 20, YELLOW);
            DrawText(TextFormat("Tempo decorrido: %.1f s", loading_time), WIDTH/2 - 100, HEIGHT/2 + 10, 15, LIGHTGRAY);
        } else {
            DrawTexture(texture, 0, 0, WHITE);
            DrawRectangle(10, 10, 310, 175, Fade(BLACK, 0.8f));
            DrawText("NAVEGACAO:", 20, 20, 10, LIGHTGRAY);
            DrawText("- Setas: Mover  |  Z/X: Zoom", 20, 35, 10, WHITE);
            DrawText("DEFORMADOR (Potencia):", 20, 60, 10, LIGHTGRAY);
            DrawText(TextFormat("- Q/W: Alterar Z^%.1f", power), 20, 75, 10, ORANGE);
            DrawText("ESTILIZADOR (Cores):", 20, 100, 10, LIGHTGRAY);
            DrawText("- Segure C/V para animar", 20, 115, 10, SKYBLUE);
            DrawText("MODOS (1, 2, 3):", 20, 140, 10, LIGHTGRAY);
            DrawText(currentMode == 0 ? "> 1. Escalonado" : (currentMode == 1 ? "> 2. Suavizado" : "> 3. Fase Magnetica"), 20, 155, 10, YELLOW);
        }
        EndDrawing();
    }

    if (lua_is_running) {
        pthread_join(thread_id, NULL);
    }

    UnloadTexture(texture);
    UnloadImage(img);
    free(fractalData);

    //desliga a maquina virtual Lua antes de fechar o programa
    lua_close(L);
    CloseWindow();

    return 0;
}
