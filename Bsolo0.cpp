#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>  // Para usleep()

using namespace std;

// Estrutura de teclas e suas cores
struct Key {
    string label;
    SDL_Color color;
};

// Lista de teclas
vector<Key> keys = {
    {"0", {0, 0, 0, 255}}, {"1", {0, 0, 0, 255}}, {"2", {0, 0, 0, 255}},
    {"3", {0, 0, 0, 255}}, {"4", {0, 0, 0, 255}}, {"5", {0, 0, 0, 255}},
    {"6", {0, 0, 0, 255}}, {"7", {0, 0, 0, 255}}, {"8", {0, 0, 0, 255}},
    {"9", {0, 0, 0, 255}}, {"Confirma", {0, 255, 0, 255}}, {"Corrige", {255, 0, 0, 255}}, {"Branco", {255, 255, 255, 255}}
};

// Variáveis globais
int current_index = 0;
bool running = true;
Uint32 interval = 1000;  // Intervalo de 1 segundo por tecla

// Função para enviar eventos de tecla
void send_key(Display* display, Window target_window, KeySym keysym) {
    KeyCode keycode = XKeysymToKeycode(display, keysym);
    if (keycode == 0) {
        cerr << "Erro: KeySym inválido." << endl;
        return;
    }

    XEvent event = {};
    event.xkey.type = KeyPress;
    event.xkey.display = display;
    event.xkey.window = target_window;
    event.xkey.root = DefaultRootWindow(display);
    event.xkey.keycode = keycode;

    // Enviar KeyPress
    XSendEvent(display, target_window, True, KeyPressMask, &event);
    XFlush(display);

    usleep(50000);  // Atraso entre KeyPress e KeyRelease

    event.xkey.type = KeyRelease;
    // Enviar KeyRelease
    XSendEvent(display, target_window, True, KeyReleaseMask, &event);
    XFlush(display);
}

// Enviar comandos de acordo com a tecla
void process_key(Display* display, Window target_window, const string& key) {
    XSetInputFocus(display, target_window, RevertToParent, CurrentTime);

    if (key >= "0" && key <= "9") {
        send_key(display, target_window, XStringToKeysym(key.c_str()));
    } else if (key == "Confirma") {
        send_key(display, target_window, XK_Return);
    } else if (key == "Corrige") {
        send_key(display, target_window, XK_BackSpace);
    } else if (key == "Branco") {
        send_key(display, target_window, XK_KP_Multiply);
    }

    cout << "Tecla enviada: " << key << endl;
}

// Desenhar texto centralizado
void draw_text(SDL_Renderer* renderer, TTF_Font* font, const string& text, SDL_Rect rect, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    int text_width = surface->w;
    int text_height = surface->h;
    SDL_Rect text_rect = {
        rect.x + (rect.w - text_width) / 2,
        rect.y + (rect.h - text_height) / 2,
        text_width,
        text_height
    };

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_DestroyTexture(texture);
}

// Desenhar teclas na tela
void render_keys(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    int x = 50, y = 50;
    for (int i = 0; i < keys.size(); i++) {
        int width = (i >= 10) ? 180 : 100;
        SDL_Rect rect = {x, y, width, 50};

        // Preencher tecla
        SDL_SetRenderDrawColor(renderer, keys[i].color.r, keys[i].color.g, keys[i].color.b, keys[i].color.a);
        SDL_RenderFillRect(renderer, &rect);

        // Bordas pretas nos botões de ação
        if (i >= 10) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }

        SDL_Color text_color = (i < 10) ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255};
        draw_text(renderer, font, keys[i].label, rect, text_color);

        // Destacar tecla atual
        if (i == current_index) {
            int border_thickness = 5;  // Espessura da borda
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

// Lógica principal da varredura
void run_scanning(Display* display, Window target_window, SDL_Renderer* renderer, TTF_Font* font) {
    Uint32 last_time = SDL_GetTicks();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                process_key(display, target_window, keys[current_index].label);
                current_index = 0;
                last_time = SDL_GetTicks();
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        // Atualizar varredura
        if (SDL_GetTicks() - last_time >= interval) {
            current_index = (current_index + 1) % keys.size();
            last_time = SDL_GetTicks();
        }

        render_keys(renderer, font);
    }
}

// Função principal
void scanning_software() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window* window = SDL_CreateWindow("Software de Varredura", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 680, 290, 0);
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