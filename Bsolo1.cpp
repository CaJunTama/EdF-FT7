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

// Estrutura de teclas e suas cores
struct Key {
    string label;
    SDL_Color color;
};

// Lista de teclas
constexpr SDL_Color BLACK = {0, 0, 0, 255};
constexpr SDL_Color WHITE = {255, 255, 255, 255};
constexpr SDL_Color GREEN = {0, 255, 0, 255};
constexpr SDL_Color RED = {255, 0, 0, 255};

const vector<Key> keys = {
    {"0", BLACK}, {"1", BLACK}, {"2", BLACK}, {"3", BLACK}, {"4", BLACK},
    {"5", BLACK}, {"6", BLACK}, {"7", BLACK}, {"8", BLACK}, {"9", BLACK},
    {"Confirma", GREEN}, {"Corrige", RED}, {"Branco", WHITE}
};

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
} scanner;

constexpr Uint32 INTERVAL = 1000;  // Intervalo de 1 segundo por tecla

// Função para enviar eventos de tecla
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

    usleep(50000);  // Atraso entre KeyPress e KeyRelease

    // Enviar KeyRelease
    event.xkey.type = KeyRelease;
    XSendEvent(display, target_window, True, KeyReleaseMask, &event);
    XFlush(display);
}

// Desenhar texto centralizado
void draw_text(SDL_Renderer* renderer, TTF_Font* font, const string& text, SDL_Rect rect, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        cerr << "Erro: Falha ao renderizar o texto na superfície." << endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        cerr << "Erro: Falha ao criar textura do texto." << endl;
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

// Desenhar teclas na tela
void render_keys(SDL_Renderer* renderer, TTF_Font* font, bool show_highlight) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    int x = 50, y = 50;
    for (size_t i = 0; i < keys.size(); i++) {
        int width = (i >= 10) ? 180 : 100;
        SDL_Rect rect = {x, y, width, 50};

        // Preencher tecla
        SDL_SetRenderDrawColor(renderer, keys[i].color.r, keys[i].color.g, keys[i].color.b, keys[i].color.a);
        SDL_RenderFillRect(renderer, &rect);

        // Bordas cinza nas teclas numéricas para melhor contraste
        if (i < 10) SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);  // Cinza médio para botões numéricos
        else SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Preto para botões de ação
        SDL_RenderDrawRect(renderer, &rect);

        draw_text(renderer, font, keys[i].label, rect, (i < 10) ? WHITE : BLACK);

        // Destacar tecla atual com borda azul (se permitido)
        if (show_highlight && i == scanner.current_index) {
            int border_thickness = 5;
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);  // Azul

            SDL_Rect top_border = {rect.x - border_thickness, rect.y - border_thickness, rect.w + 2 * border_thickness, border_thickness};
            SDL_Rect bottom_border = {rect.x - border_thickness, rect.y + rect.h, rect.w + 2 * border_thickness, border_thickness};
            SDL_Rect left_border = {rect.x - border_thickness, rect.y, border_thickness, rect.h};
            SDL_Rect right_border = {rect.x + rect.w, rect.y, border_thickness, rect.h};

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

// Enviar comandos de acordo com a tecla e fazer a borda piscar 3 vezes
void process_key(Display* display, Window target_window, const string& key, SDL_Renderer* renderer, TTF_Font* font) {
    XSetInputFocus(display, target_window, RevertToParent, CurrentTime);

    if (key >= "0" && key <= "9") send_key(display, target_window, XStringToKeysym(key.c_str()));
    else if (key == "Confirma") send_key(display, target_window, XK_Return);
    else if (key == "Corrige") send_key(display, target_window, XK_BackSpace);
    else if (key == "Branco") send_key(display, target_window, XK_KP_Multiply);

    cout << "Tecla enviada: " << key << endl;

    // Antes do efeito de piscar, limpar a fila de cliques para evitar eventos antigos
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_MOUSEBUTTONDOWN) continue;
    }

    // Bloquear cliques durante o efeito de piscar
    SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_DISABLE);

    // Efeito de piscar a borda azul na tecla escolhida
    for (int i = 0; i < 3; i++) {  // Piscar 3 vezes
        render_keys(renderer, font, false);
        SDL_RenderPresent(renderer);
        SDL_Delay(500);

        render_keys(renderer, font, true);
        SDL_RenderPresent(renderer);
        SDL_Delay(500);
    }

    // Após o efeito de piscar, limpar qualquer clique extra que possa ter ficado na fila
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_MOUSEBUTTONDOWN) continue;
    }

    // Reativar cliques após garantir que eventos antigos não sejam processados
    SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);

    scanner.reset_index();
}

// Lógica principal da varredura
void run_scanning(Display* display, Window target_window, SDL_Renderer* renderer, TTF_Font* font) {
    Uint32 last_time = SDL_GetTicks(); 

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) return;
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                process_key(display, target_window, keys[scanner.current_index].label, renderer, font);
                last_time = SDL_GetTicks();
            }
        }

        if (SDL_GetTicks() - last_time >= INTERVAL) {
            scanner.current_index = (scanner.current_index + 1) % keys.size();
            last_time = SDL_GetTicks();
        }

        render_keys(renderer, font, true);
    }
}

void scanning_software() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("Solução B - Varredura", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 680, 290, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);

    if (!font) {
        cerr << "Erro ao carregar a fonte." << endl;
        return;
    }

    Display* display = XOpenDisplay(NULL);
    if (!display) {
        cerr << "Erro ao abrir a conexão com o servidor X." << endl;
        return;
    }

    Window target_window = 0x600003;
    send_key(display, target_window, XK_f);  // Enviar tecla neutra F para inicializar

    // Definir `current_index` como um valor aleatório dentro do intervalo das teclas ao iniciar o programa
    scanner.reset_index();

    run_scanning(display, target_window, renderer, font);

    XCloseDisplay(display);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

int main() {
    cout << "Iniciando o software de varredura..." << endl;
    scanning_software();
    return 0;
}


