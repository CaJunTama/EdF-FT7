#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <X11/Xlib.h>

using namespace std;

// Declaração das funções definidas em Bpart0.cpp e Cpart0.cpp
extern void run_scanning(Display* display, Window target_window, SDL_Renderer* renderer, TTF_Font* font);
extern void run_selection(Display* display, Window target_window, SDL_Renderer* renderer, TTF_Font* font);

// Exibe o menu principal
int show_main_menu(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    SDL_Rect optionB = {10, 50, 680, 60};
    SDL_Rect optionC = {50, 150, 600, 60};
    SDL_Rect exit = {150, 250, 400, 60};

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &optionB);
    SDL_RenderDrawRect(renderer, &optionC);
    SDL_RenderDrawRect(renderer, &exit);

    draw_text(renderer, font, "Pressione 1: Solução B (Varredura Automática)", optionB, BLACK);
    draw_text(renderer, font, "Pressione 2: Solução C (Mouse Adaptado)", optionC, BLACK);
    draw_text(renderer, font, "Pressione ESC para sair", exit, BLACK);

    SDL_RenderPresent(renderer);

    SDL_Event event;
    while (true) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return -1;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_1) return 1;
                if (event.key.keysym.sym == SDLK_2) return 2;
                if (event.key.keysym.sym == SDLK_ESCAPE) return -1;
            }
        }
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("TEcSRM - Terminal do Eleitor com Severa Restrição de Mobilidade", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 700, 350, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);

    if (!font) {
        cerr << "Erro ao carregar a fonte!" << endl;
        return -1;
    }

    Display* display = XOpenDisplay(NULL);
    if (!display) {
        cerr << "Erro ao abrir a conexão com o servidor X." << endl;
        return -1;
    }

    Window target_window = 0x600003;

    int choice;
    do {
        choice = show_main_menu(renderer, font);
        if (choice == 1) run_scanning(display, target_window, renderer, font);
        else if (choice == 2) run_selection(display, target_window, renderer, font);
    } while (choice != -1);

    XCloseDisplay(display);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
