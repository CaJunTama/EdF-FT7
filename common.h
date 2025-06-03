#ifndef COMMON_H
#define COMMON_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

// Estrutura de teclas e suas cores
struct Key {
    string label;
    SDL_Color color;
    SDL_Rect rect;
};

// Definição das cores
constexpr SDL_Color BLACK = {0, 0, 0, 255};
constexpr SDL_Color WHITE = {255, 255, 255, 255};
constexpr SDL_Color GREEN = {0, 255, 0, 255};
constexpr SDL_Color RED = {255, 0, 0, 255};

// Declaração do vetor keys
extern vector<Key> keys;

// Função para enviar eventos de tecla ao simulador da urna eletrônica
void send_key(Display* display, Window target_window, KeySym keysym);

// Função para desenhar texto centralizado
void draw_text(SDL_Renderer* renderer, TTF_Font* font, const string& text, SDL_Rect rect, SDL_Color color);

// Renderiza as teclas na tela com bordas e efeito de destaque
void render_keys(SDL_Renderer* renderer, TTF_Font* font, int highlighted_index = -1);

#endif // COMMON_H
