#include "common.h"
#include <unistd.h> // Para `usleep`
#include <cmath>
#include <algorithm>

using namespace std;

int g_scale_pct = 100;            // começa em 100 %

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
    if (!texture) {
        cerr << "Erro ao criar textura do texto!" << endl;
        SDL_FreeSurface(surface);
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
    SDL_FreeSurface(surface);
}

// ---- Helpers de contraste (WCAG) -------------------------------------------
static inline double srgb_to_linear(double c) {
    return (c <= 0.04045) ? (c/12.92) : pow((c+0.055)/1.055, 2.4);
}
static inline double rel_lum(SDL_Color c) {
    const double R = srgb_to_linear(c.r/255.0);
    const double G = srgb_to_linear(c.g/255.0);
    const double B = srgb_to_linear(c.b/255.0);
    return 0.2126*R + 0.7152*G + 0.0722*B;
}
static inline SDL_Color pick_inner_ring(SDL_Color fill) {
    // Se a tecla é clara, use borda interna preta; se é escura, borda interna branca
    return (rel_lum(fill) >= 0.5) ? SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
}

static inline SDL_Rect inflate(SDL_Rect r, int d) {
    return SDL_Rect{ r.x - d, r.y - d, r.w + 2*d, r.h + 2*d };
}

// Desenha borda dupla usando 4 retângulos por anel.
// - Anel interno (encostado na tecla): preto ou branco conforme a cor da tecla.
// - Anel externo (mais afastado): sempre preto (garante contraste com o fundo branco).
static void draw_focus_border_rects(SDL_Renderer* r, SDL_Rect key, SDL_Color keyFill) {
    // espessuras escaláveis, mínimo 2 px reais
    const int t_in  = (int)lround(SZ(5)); // anel interno
    const int t_out = (int)lround(SZ(5)); // anel externo

    auto draw_ring = [&](SDL_Color col, int offset, int thick) {
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, 255);

        // TOP
        SDL_Rect top{ key.x - offset - thick,
                      key.y - offset - thick,
                      key.w + 2*(offset + thick),
                      thick };
        // BOTTOM
        SDL_Rect bottom{ key.x - offset - thick,
                         key.y + key.h + offset,
                         key.w + 2*(offset + thick),
                         thick };
        // LEFT
        SDL_Rect left{ key.x - offset - thick,
                       key.y - offset,
                       thick,
                       key.h + 2*offset };
        // RIGHT
        SDL_Rect right{ key.x + key.w + offset,
                        key.y - offset,
                        thick,
                        key.h + 2*offset };

        SDL_RenderFillRect(r, &top);
        SDL_RenderFillRect(r, &bottom);
        SDL_RenderFillRect(r, &left);
        SDL_RenderFillRect(r, &right);
    };

    // Luminância relativa simples p/ decidir preto/branco no anel interno
    auto srgb_to_linear = [](double c) {
        return (c <= 0.04045) ? (c/12.92) : pow((c+0.055)/1.055, 2.4);
    };
    const double L =
        0.2126*srgb_to_linear(keyFill.r/255.0) +
        0.7152*srgb_to_linear(keyFill.g/255.0) +
        0.0722*srgb_to_linear(keyFill.b/255.0);
    const SDL_Color inner = (L >= 0.5)
        ? SDL_Color{0,0,0,255}          // tecla clara → anel interno preto
        : SDL_Color{255,255,255,255};   // tecla escura → anel interno branco

    // 1) Anel interno (encostado na tecla)
    draw_ring(inner, /*offset=*/0, /*thick=*/t_in);

    // 2) Anel externo preto, afastado por t_in
    draw_ring(SDL_Color{0,0,0,255}, /*offset=*/t_in, /*thick=*/t_out);
}

// Renderiza as teclas na tela com bordas e efeito de destaque
void render_keys(SDL_Renderer* renderer, TTF_Font* font, int highlighted_index) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    int winW, winH;
    SDL_GetRendererOutputSize(renderer, &winW, &winH);

    const int keyWSmall = 100, keyWBig = 180, keyH = 50, gap = 20;
    int keyWSmallx  = static_cast<int>(lround(SZ(keyWSmall)));
    int keyWBigx  = static_cast<int>(lround(SZ(keyWBig)));
    int keyHx  = static_cast<int>(lround(SZ(keyH)));
    int gapx = static_cast<int>(lround(SZ(gap)));
    const int cols = 5, rows = 3;

    int totalWidth = cols * keyWSmallx + (cols - 1) * gapx;   // 5 botões pequenos, 4 espaços

    int totalHeight = rows * keyHx + (rows - 1) * gapx;       // 3 linhas, 2 espaços

    int x0 = (winW - totalWidth) / 2;
    int y0 = (winH - totalHeight) / 2;

    int x = x0, y = y0;
    for (size_t i = 0; i < keys.size(); i++) {
        int width = (i >= 10) ? keyWBigx : keyWSmallx;
        keys[i].rect = {x, y, width, keyHx};

        // Preenchimento da tecla
        SDL_SetRenderDrawColor(renderer, keys[i].color.r, keys[i].color.g, keys[i].color.b, keys[i].color.a);
        SDL_RenderFillRect(renderer, &keys[i].rect);

        if (i < 10) SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);  // Cinza médio para botões numéricos
        else SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Preto para botões de ação
        SDL_RenderDrawRect(renderer, &keys[i].rect);

        // Cor do texto: Branco para botões numéricos, Preto para botões de ação
        draw_text(renderer, font, keys[i].label, keys[i].rect, (i < 10) ? WHITE : BLACK);

        // Destacar tecla selecionada com borda dupla de alto contraste
        if (highlighted_index == static_cast<int>(i)) {
            draw_focus_border_rects(renderer, keys[i].rect, keys[i].color);
        }

        x += width + gapx;
        if ((i + 1) % 5 == 0) {
            x = x0;
            y += keyHx + gapx;
        }
    }

    SDL_RenderPresent(renderer);
}