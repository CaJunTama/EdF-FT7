#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>  // Para usleep()
#include <random>

using namespace std;

extern bool g_is_fullscreen;
extern TTF_Font* font;          //  fonte global carregada em TEcSRM.cpp

// Estrutura de controle global
struct Scanner {
    int current_index;
    mt19937 rng;
    uniform_int_distribution<int> dist;

    Scanner() : rng(random_device{}()), dist(0, keys.size() - 1) {
        reset_index();
    }

    void reset_index() {
        current_index = dist(rng);
    }
};

// Ponteiro global para o scanner (inicializado em `run_solution_b`)
Scanner* scanner = nullptr;

// Enviar comandos de acordo com a tecla e fazer a borda piscar 3 vezes
void process_key(Display* display, Window target_window, const string& key, SDL_Renderer* renderer) {
    XSetInputFocus(display, target_window, RevertToParent, CurrentTime);

    if (key >= "0" && key <= "9") send_key(display, target_window, XStringToKeysym(key.c_str()));
    else if (key == "Confirma") send_key(display, target_window, XK_Return);
    else if (key == "Corrige") send_key(display, target_window, XK_BackSpace);
    else if (key == "Branco") send_key(display, target_window, XK_KP_Multiply);

    // Antes do efeito de piscar, limpar a fila de cliques para evitar eventos antigos
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_MOUSEBUTTONDOWN) continue;
    }

    // Bloquear cliques durante o efeito de piscar
    SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_DISABLE);

    // Efeito de piscar a borda azul na tecla escolhida
    for (int i = 0; i < 3; i++) {  // Piscar 3 vezes
        render_keys(renderer, font, -1);
        SDL_Delay(500);

        render_keys(renderer, font, scanner->current_index);
        SDL_Delay(500);
    }

    // Após o efeito de piscar, limpar qualquer clique extra que possa ter ficado na fila
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_MOUSEBUTTONDOWN) continue;
    }

    // Reativar cliques após garantir que eventos antigos não sejam processados
    SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);

    scanner->reset_index();
}

// Lógica principal da varredura
int run_scanning(Display* display, Window target_window, SDL_Renderer* renderer) {
    // Inicializa `scanner` corretamente
    scanner = new Scanner();

    send_key(display, target_window, XK_f);  // Enviar tecla neutra F para inicializar

    // Definir `current_index` como um valor aleatório dentro do intervalo das teclas ao iniciar o programa
    scanner->reset_index();

    Uint32 last_time = SDL_GetTicks(); 

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return -1;
            /* atalho global: F11 alterna fullscreen */
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11) {
                g_is_fullscreen = !g_is_fullscreen;
                Uint32 flag = g_is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
                SDL_SetWindowFullscreen(SDL_GetWindowFromID(1), flag);
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                delete scanner;  // Libera a memória corretamente
                return 0;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                process_key(display, target_window, keys[scanner->current_index].label, renderer);
                last_time = SDL_GetTicks();
            }
        }

        if (SDL_GetTicks() - last_time >= scan_interval_ms()) {
            scanner->current_index = (scanner->current_index + 1) % keys.size();
            last_time = SDL_GetTicks();
        }

        render_keys(renderer, font, scanner->current_index);
    }
}



