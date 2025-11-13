#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <X11/Xlib.h>

using namespace std;

// Declaração das funções definidas em Bpart0.cpp e Cpart0.cpp
extern int run_scanning(Display* display, Window target_window, SDL_Renderer* renderer, TTF_Font* font);
extern int run_selection(Display* display, Window target_window, SDL_Renderer* renderer, TTF_Font* font);

bool g_is_fullscreen = false;   // estado global do F11

// Exibe o menu principal
int show_main_menu(SDL_Renderer* renderer, TTF_Font* font) {
    /* --- tamanho atual da janela --- */
    int winW, winH;
    SDL_GetRendererOutputSize(renderer, &winW, &winH);

    const int btnW = 500, btnH = 60, gap = 30;

    /* 3 botões empilhados: altura total = 3*btnH + 2*gap */
    int totalH = 3 * btnH + 2 * gap;
    int startY = (winH - totalH) / 2;          // centraliza verticalmente
    int startX = (winW - btnW) / 2;            // centraliza horizontalmente

    SDL_Rect optionB = {startX, startY, btnW, btnH};
    SDL_Rect optionC = {startX, startY + btnH + gap, btnW, btnH};
    SDL_Rect settings = {startX, startY + 2*(btnH+gap), btnW, btnH};

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &optionB);
    SDL_RenderDrawRect(renderer, &optionC);
    SDL_RenderDrawRect(renderer, &settings);

    draw_text(renderer, font, "Solução B (Varredura Automática)", optionB, BLACK);
    draw_text(renderer, font, "Solução C (Mouse Adaptado)",       optionC, BLACK);
    draw_text(renderer, font, "Configurações", settings, BLACK);

    SDL_RenderPresent(renderer);

    SDL_Event event;
    while (true) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return -1;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_F11) {
                    g_is_fullscreen = !g_is_fullscreen;
                    SDL_SetWindowFullscreen(SDL_GetWindowFromID(1),
                            g_is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    /* recalcule positions depois de mudar o tamanho */
                    return 0;   // força redesenho imediato
                }
            }
            /* novo: clique do mouse escolhe a opção ------------------ */
            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT) {

                int mx = event.button.x, my = event.button.y;
                if (mx >= optionB.x && mx <= optionB.x + optionB.w &&
                    my >= optionB.y && my <= optionB.y + optionB.h)
                    return 1;   // Solução B

                if (mx >= optionC.x && mx <= optionC.x + optionC.w &&
                    my >= optionC.y && my <= optionC.y + optionC.h)
                    return 2;   // Solução C
                if (mx >= settings.x && mx <= settings.x + settings.w &&
                    my >= settings.y && my <= settings.y + settings.h) {
                    /* Configurações ainda não implementado */
                    cout << "opa" << endl;
                    continue;   // por ora, botão não faz nada
                }
            }
        }
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("TEcSRM - Terminal do Eleitor com Severa Restrição de Mobilidade",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        700, 350,
        SDL_WINDOW_RESIZABLE);                // <- agora redimensionável
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
        if (choice == 1) choice = run_scanning(display, target_window, renderer, font);
        else if (choice == 2) choice = run_selection(display, target_window, renderer, font);
    } while (choice != -1);

    XCloseDisplay(display);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
