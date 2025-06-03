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

// Estrutura de teclas e suas cores
struct Key {
    string label;
    SDL_Color color;
    SDL_Rect rect;
};

constexpr SDL_Color BLACK = {0, 0, 0, 255};
constexpr SDL_Color WHITE = {255, 255, 255, 255};
constexpr SDL_Color GREEN = {0, 255, 0, 255};
constexpr SDL_Color RED = {255, 0, 0, 255};

vector<Key> keys = {
    {"0", BLACK}, {"1", BLACK}, {"2", BLACK}, {"3", BLACK}, {"4", BLACK},
    {"5", BLACK}, {"6", BLACK}, {"7", BLACK}, {"8", BLACK}, {"9", BLACK},
    {"Confirma", GREEN}, {"Corrige", RED}, {"Branco", WHITE}
};

// Enviar eventos de tecla para a urna eletrônica simulada
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

    // KeyPress
    event.xkey.type = KeyPress;
    XSendEvent(display, target_window, True, KeyPressMask, &event);
    XFlush(display);

    usleep(50000);

    // KeyRelease
    event.xkey.type = KeyRelease;
    XSendEvent(display, target_window, True, KeyReleaseMask, &event);
    XFlush(display);
}

// Desenha texto centralizado dentro de uma tecla
void draw_text(SDL_Renderer* renderer, TTF_Font* font, const string& text, SDL_Rect rect, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
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
void render_keys(SDL_Renderer* renderer, TTF_Font* font, int highlighted_index = -1) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    int x = 50, y = 50;
    for (size_t i = 0; i < keys.size(); i++) {
        int width = (i >= 10) ? 180 : 100;
        keys[i].rect = {x, y, width, 50};

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
            SDL_Rect bottom_border = {x - border_thickness, y + 50, width + 2 * border_thickness, border_thickness};
            SDL_Rect left_border = {x - border_thickness, y, border_thickness, 50};
            SDL_Rect right_border = {x + width, y, border_thickness, 50};

            SDL_RenderFillRect(renderer, &top_border);
            SDL_RenderFillRect(renderer, &bottom_border);
            SDL_RenderFillRect(renderer, &left_border);
            SDL_RenderFillRect(renderer, &right_border);
        }

        x += width + 20;
        if ((i + 1) % 5 == 0) {
            x = 50;
            y += 70;
        }
    }

    SDL_RenderPresent(renderer);
}

// Processa clique do mouse e simula pressionamento de tecla
void handle_mouse_click(Display* display, Window target_window, int x, int y, SDL_Renderer* renderer, TTF_Font* font) {
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
                SDL_RenderPresent(renderer);
                SDL_Delay(500);

                render_keys(renderer, font, i);
                SDL_RenderPresent(renderer);
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

// Inicializa a interface gráfica
void run_interface(Display* display, Window target_window) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("Solução C - Mouse adaptado", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 680, 290, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);

    if (!font) {
        cerr << "Erro ao carregar a fonte!" << endl;
        return;
    }

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) return;
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                handle_mouse_click(display, target_window, event.button.x, event.button.y, renderer, font);
            }
        }
        render_keys(renderer, font);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

int main() {
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        cerr << "Erro ao abrir a conexão com o servidor X." << endl;
        return -1;
    }

    Window target_window = 0x600003;
    run_interface(display, target_window);

    XCloseDisplay(display);
    return 0;
}
