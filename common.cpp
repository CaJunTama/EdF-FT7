#include "common.h"
#include <unistd.h> // Para `usleep`

using namespace std;

// Definição única do vetor keys (evita múltiplas definições)
vector<Key> keys = {
    {"0", BLACK}, {"1", BLACK}, {"2", BLACK}, {"3", BLACK}, {"4", BLACK},
    {"5", BLACK}, {"6", BLACK}, {"7", BLACK}, {"8", BLACK}, {"9", BLACK},
    {"Confirma", GREEN}, {"Corrige", RED}, {"Branco", WHITE}
};

// Implementação da função `send_key`
void send_key(Display* display, Window target_window, KeySym keysym) {
    KeyCode keycode = XKeysymToKeycode(display, keysym);
    if (keycode == 0) {
        cerr << "Erro: KeySym inválido." << endl;
        return;
    }

    XEvent event = {};
    event.xkey.display = display;
    event.xkey.window = target_window;
    event.xkey.keycode = keycode;

    // Enviar KeyPress
    event.xkey.type = KeyPress;
    XSendEvent(display, target_window, True, KeyPressMask, &event);
    XFlush(display);

    usleep(50000); // Atraso entre KeyPress e KeyRelease

    // Enviar KeyRelease
    event.xkey.type = KeyRelease;
    XSendEvent(display, target_window, True, KeyReleaseMask, &event);
    XFlush(display);
}

// Implementação da função `draw_text`
void draw_text(SDL_Renderer* renderer, TTF_Font* font, const string& text, SDL_Rect rect, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
    if (!surface) {
        cerr << "Erro ao renderizar o texto na superfície!" << endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        cerr << "Erro ao criar textura do texto!" << endl;
        return;
    }

    SDL_Rect text_rect = {
        rect.x + (rect.w - surface->w) / 2,
        rect.y + (rect.h - surface->h) / 2,
        surface->w,
        surface->h
    };
    
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_DestroyTexture(texture);
}

// Renderiza as teclas na tela com bordas e efeito de destaque
void render_keys(SDL_Renderer* renderer, TTF_Font* font, int highlighted_index) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    int winW, winH;
    SDL_GetRendererOutputSize(renderer, &winW, &winH);

    const int keyWSmall = 100, keyWBig = 180, keyH = 50, gap = 20;
    const int cols = 5, rows = 3;

    int totalWidth = cols * keyWSmall + (cols - 1) * gap;   // 5 botões pequenos, 4 espaços

    int totalHeight = rows * keyH + (rows - 1) * gap;       // 3 linhas, 2 espaços

    int x0 = (winW - totalWidth) / 2;
    int y0 = (winH - totalHeight) / 2;

    int x = x0, y = y0;
    for (size_t i = 0; i < keys.size(); i++) {
        int width = (i >= 10) ? keyWBig : keyWSmall;
        keys[i].rect = {x, y, width, keyH};

        // Preenchimento da tecla
        SDL_SetRenderDrawColor(renderer, keys[i].color.r, keys[i].color.g, keys[i].color.b, keys[i].color.a);
        SDL_RenderFillRect(renderer, &keys[i].rect);

        if (i < 10) SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);  // Cinza médio para botões numéricos
        else SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Preto para botões de ação
        SDL_RenderDrawRect(renderer, &keys[i].rect);

        // Cor do texto: Branco para botões numéricos, Preto para botões de ação
        draw_text(renderer, font, keys[i].label, keys[i].rect, (i < 10) ? WHITE : BLACK);

        // Destacar tecla selecionada com borda azul
        if (highlighted_index == static_cast<int>(i)) {
            int border_thickness = 5;
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

            SDL_Rect top_border = {x - border_thickness, y - border_thickness, width + 2 * border_thickness, border_thickness};
            SDL_Rect bottom_border = {x - border_thickness, y + keyH, width + 2 * border_thickness, border_thickness};
            SDL_Rect left_border = {x - border_thickness, y, border_thickness, keyH};
            SDL_Rect right_border = {x + width, y, border_thickness, keyH};

            SDL_RenderFillRect(renderer, &top_border);
            SDL_RenderFillRect(renderer, &bottom_border);
            SDL_RenderFillRect(renderer, &left_border);
            SDL_RenderFillRect(renderer, &right_border);
        }

        x += width + gap;
        if ((i + 1) % 5 == 0) {
            x = x0;
            y += keyH + gap;
        }
    }

    SDL_RenderPresent(renderer);
}