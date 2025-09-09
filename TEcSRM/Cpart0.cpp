#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>

using namespace std;

extern bool g_is_fullscreen;
extern TTF_Font* font;

// Processa clique do mouse e simula pressionamento de tecla
void handle_mouse_click(Display* display, Window target_window, int x, int y, SDL_Renderer* renderer) {
    XSetInputFocus(display, target_window, RevertToParent, CurrentTime);

    for (size_t i = 0; i < keys.size(); i++) {
        if (x >= keys[i].rect.x && x <= keys[i].rect.x + keys[i].rect.w &&
            y >= keys[i].rect.y && y <= keys[i].rect.y + keys[i].rect.h) {

            KeySym keysym;
            if (keys[i].label >= "0" && keys[i].label <= "9") keysym = XStringToKeysym(keys[i].label.c_str());
            else if (keys[i].label == "Confirma") keysym = XK_Return;
            else if (keys[i].label == "Corrige") keysym = XK_BackSpace;
            else if (keys[i].label == "Branco") keysym = XK_KP_Multiply;

            send_key(display, target_window, keysym);

            // Antes do efeito de piscar, limpar a fila de cliques para evitar eventos antigos
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_MOUSEBUTTONDOWN) continue;
            }

            // Bloquear cliques durante o efeito de piscar
            SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_DISABLE);
            
            // Efeito de piscar a tecla
            for (int j = 0; j < 3; j++) {
                render_keys(renderer, font, -1);
                SDL_Delay(500);

                render_keys(renderer, font, i);
                SDL_Delay(500);
            }

            // Após o efeito de piscar, limpar qualquer clique extra que possa ter ficado na fila
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_MOUSEBUTTONDOWN) continue;
            }

            // Reativar cliques após garantir que eventos antigos não sejam processados
            SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);

            break;
        }
    }
}

int run_selection(Display* display, Window target_window, SDL_Renderer* renderer) {
    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return -1;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11) {
                g_is_fullscreen = !g_is_fullscreen;
                Uint32 flag = g_is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
                SDL_SetWindowFullscreen(SDL_GetWindowFromID(1), flag);
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) return 0;
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                handle_mouse_click(display, target_window, event.button.x, event.button.y, renderer);
            }
        }
        render_keys(renderer, font);
    }
}
