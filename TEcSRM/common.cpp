#include "common.h"
#include <unistd.h> // Para `usleep`
#include <cmath>
#include <algorithm>

using namespace std;

int g_scale_pct = 100;            // começa em 100 %

int g_scan_speed = 3; // padrão = 3

// Definição única do vetor keys (evita múltiplas definições)
vector<Key> keys = {
    {"0", BLACK}, {"1", BLACK}, {"2", BLACK}, {"3", BLACK}, {"4", BLACK},
    {"5", BLACK}, {"6", BLACK}, {"7", BLACK}, {"8", BLACK}, {"9", BLACK},
    {"Confirma", CONFIRMA_GREEN}, {"Corrige", CORRIGE_ORANGE}, {"Branco", WHITE}
};

// Cor de fundo (parte da paleta). Começa branco na paleta padrão.
SDL_Color g_bg_color = {255, 255, 255, 255};

void clear_with_bg(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, g_bg_color.r, g_bg_color.g, g_bg_color.b, 255);
    SDL_RenderClear(r);
}

int g_hc_option = 1;  // começa na paleta padrão

void set_palette_default() {
    // Reaplica as cores padrão às teclas
    for (auto &k : keys) {
        if (k.label == "Confirma") {
            k.color = CONFIRMA_GREEN;      // #50A25D
        } else if (k.label == "Corrige") {
            k.color = CORRIGE_ORANGE;      // #E96501
        } else if (k.label == "Branco") {
            k.color = WHITE;
        } else {
            // dígitos 0–9 (e outras neutras)
            k.color = BLACK;
        }
    }
    // Fundo faz parte da paleta
    g_bg_color = WHITE;

    g_hc_option = 1;
    SDL_Log("Paleta aplicada: Padrão (inclui fundo branco).");
}

void set_palette_inverted() {
    // Teclas: inverter preto↔branco; manter Confirma/Corrige.
    for (auto &k : keys) {
        if (k.label == "Confirma") {
            k.color = CONFIRMA_GREEN;          // mantém
        } else if (k.label == "Corrige") {
            k.color = CORRIGE_ORANGE;          // mantém
        } else if (k.label == "Branco") {
            k.color = BLACK;                   // era branco → vira preto
        } else {
            // dígitos 0–9 (eram pretos) → viram brancos
            k.color = WHITE;
        }
    }
    // Fundo da paleta invertida: preto
    g_bg_color = BLACK;

    g_hc_option = 2;
    SDL_Log("Paleta: Alto Contraste 2 (invertida) aplicada.");
}

void set_palette_hc3() {
    // Teclas: manter cores de ação; mapear preto→HC3_DARK, branco→HC3_LIGHT
    for (auto &k : keys) {
        if (k.label == "Confirma")      k.color = CONFIRMA_GREEN;
        else if (k.label == "Corrige")  k.color = CORRIGE_ORANGE;
        else if (k.label == "Branco")   k.color = HC3_LIGHT; // substituto do branco
        else                            k.color = HC3_DARK;  // dígitos etc. (substituto do preto)
    }
    // Fundo claro (substituto do branco)
    g_bg_color = HC3_LIGHT;

    g_hc_option = 3;
    SDL_Log("Paleta: Alto Contraste 3 (colorido) aplicada.");
}

Uint32 scan_interval_ms() {
    // 1→1750ms, 2→1500ms, 3→1250ms, 4→1000ms, 5→750ms
    int v = std::clamp(g_scan_speed, 1, 5);
    switch (v) {
        case 1: return 1750;
        case 2: return 1500;
        case 3: return 1250;
        case 4: return 1000;
        default: return  750; // v == 5
    }
}

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

static inline SDL_Color best_text_bw_for_fill(SDL_Color fill) {
    const double L = rel_lum(fill);
    const double cBlack = (L + 0.05) / 0.05;        // contraste do PRETO sobre 'fill'
    const double cWhite = 1.05 / (L + 0.05);        // contraste do BRANCO sobre 'fill'
    return (cBlack >= cWhite) ? SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
}

static SDL_Color best_bw_for_bg(SDL_Color bg) {
    // escolhe preto ou branco com maior contraste contra o fundo
    const double Lbg = rel_lum(bg);
    const double cBlack = (std::max(Lbg, 0.0) + 0.05) / (0.0 + 0.05);  // Lpreto=0
    const double cWhite = (1.0 + 0.05) / (std::min(Lbg, 1.0) + 0.05);  // Lbranco=1
    return (cBlack >= cWhite) ? SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
}

static inline SDL_Color pick_inner_ring(SDL_Color fill) {
    // Se a tecla é clara, use borda interna preta; se é escura, borda interna branca
    return (rel_lum(fill) >= 0.5) ? SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
}

static inline SDL_Rect inflate(SDL_Rect r, int d) {
    return SDL_Rect{ r.x - d, r.y - d, r.w + 2*d, r.h + 2*d };
}

static inline double contrast_ratio(SDL_Color a, SDL_Color b) {
    const double La = rel_lum(a), Lb = rel_lum(b);
    const double Lmax = std::max(La, Lb), Lmin = std::min(La, Lb);
    return (Lmax + 0.05) / (Lmin + 0.05);
}

// Escolhe entre (dark, light) a que entrega MAIOR contraste contra `base`
static inline SDL_Color best_from_pair_for(SDL_Color base, SDL_Color dark, SDL_Color light) {
    return (contrast_ratio(base, dark) >= contrast_ratio(base, light)) ? dark : light;
}

// Texto sobre a tecla (preenche melhor contraste):
// - HC3: escolhe entre (HC3_DARK, HC3_LIGHT)
// - HC1/HC2: escolhe entre (preto, branco)
static inline SDL_Color pick_text_for_fill(SDL_Color fill) {
    if (g_hc_option == 3) {
        return best_from_pair_for(fill, HC3_DARK, HC3_LIGHT);
    }
    // fallback PB
    return best_text_bw_for_fill(fill);
}

static inline bool same_rgb(SDL_Color a, SDL_Color b) {
    return a.r==b.r && a.g==b.g && a.b==b.b;
}

static inline SDL_Color other_in_pair(SDL_Color c, SDL_Color dark, SDL_Color light) {
    return same_rgb(c, light) ? dark : light;
}

// Anel interno do realce: adapta à cor da tecla
static inline SDL_Color pick_inner_ring_color(SDL_Color keyFill) {
    if (g_hc_option == 3) {
        // HC3 usa o par colorido
        return best_from_pair_for(keyFill, HC3_DARK, HC3_LIGHT);
    }
    // HC1/HC2: PB
    return best_from_pair_for(keyFill, SDL_Color{0,0,0,255}, SDL_Color{255,255,255,255});
}

// Outline que contrasta com o fundo atual
static inline SDL_Color pick_outline_vs_bg(SDL_Color bg) {
    if (g_hc_option == 3) {
        return best_from_pair_for(bg, HC3_DARK, HC3_LIGHT);
    }
    return best_from_pair_for(bg, SDL_Color{0,0,0,255}, SDL_Color{255,255,255,255});
}

// Borda dupla por 4 retângulos (interno + externo).
// Interno: preto/branco conforme sua lógica (ou fixa branca, se você decidiu assim).
// EXTERNO: preto ou branco, escolhido para maximizar contraste com o FUNDO atual (paleta).
static void draw_focus_border_rects(SDL_Renderer* r, SDL_Rect key, SDL_Color keyFill) {
    const int t_in  = (int)std::lround(SZ(5));
    const int t_out = (int)std::lround(SZ(5));

    auto draw_ring = [&](SDL_Color col, int offset, int thick) {
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, 255);

        SDL_Rect top    { key.x - offset - thick, key.y - offset - thick,
                          key.w + 2*(offset + thick), thick };
        SDL_Rect bottom { key.x - offset - thick, key.y + key.h + offset,
                          key.w + 2*(offset + thick), thick };
        SDL_Rect left   { key.x - offset - thick, key.y - offset,
                          thick, key.h + 2*offset };
        SDL_Rect right  { key.x + key.w + offset, key.y - offset,
                          thick, key.h + 2*offset };

        SDL_RenderFillRect(r, &top);
        SDL_RenderFillRect(r, &bottom);
        SDL_RenderFillRect(r, &left);
        SDL_RenderFillRect(r, &right);
    };

    // 1) ANEL INTERNO (encostado na tecla)
    SDL_Color inner;
    if (g_hc_option == 3) {
        // HC3: anel interno se adapta à cor da tecla com o par (HC3_DARK, HC3_LIGHT)
        inner = best_from_pair_for(keyFill, HC3_DARK, HC3_LIGHT);
    } else {
        // HC1/HC2: anel interno se adapta com o par (preto, branco)
        inner = best_from_pair_for(keyFill, SDL_Color{0,0,0,255}, SDL_Color{255,255,255,255});
    }
    draw_ring(inner, /*offset=*/0,    /*thick=*/t_in);

    // 2) ANEL EXTERNO — escolhe preto OU branco p/ maximizar contraste com o FUNDO atual
    SDL_Color outer;
    if (g_hc_option == 3) {
        // HC3: anel externo contrasta com o FUNDO usando (HC3_DARK, HC3_LIGHT)
        outer = best_from_pair_for(g_bg_color, HC3_DARK, HC3_LIGHT);
    } else {
        // HC1/HC2: anel externo contrasta com o FUNDO usando (preto, branco)
        outer = best_from_pair_for(g_bg_color, SDL_Color{0,0,0,255}, SDL_Color{255,255,255,255});
    }
    draw_ring(outer, /*offset=*/t_in, /*thick=*/t_out);
}

// Renderiza as teclas na tela com bordas e efeito de destaque
void render_keys(SDL_Renderer* renderer, TTF_Font* font, int highlighted_index) {
    clear_with_bg(renderer);

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

        SDL_Color uiFG = best_bw_for_bg(g_bg_color); // melhor preto/branco contra o fundo atual

        // --- Outline externo (contorno fino do retângulo da tecla) -----------------
        if (i < 10) {
            // Teclas numéricas: mantém cinza médio
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        } else {
            // Teclas de ação: Confirma, Corrige, Branco
            SDL_Color outline;

            if (highlighted_index == static_cast<int>(i)) {
                // Em DESTAQUE: contraste com a BORDA INTERNA do realce.
                // 1) Obtém a cor do anel interno (já adaptada à cor da tecla):
                SDL_Color inner = pick_inner_ring_color(keys[i].color);

                // 2) Escolhe a "outra" cor do par para o outline externo:
                if (g_hc_option == 3) {
                    // HC3: usa par (HC3_DARK, HC3_LIGHT)
                    outline = other_in_pair(inner, HC3_DARK, HC3_LIGHT);
                } else {
                    // HC1/HC2: PB
                    outline = other_in_pair(inner, SDL_Color{0,0,0,255}, SDL_Color{255,255,255,255});
                }
            } else {
                // SEM DESTAQUE: contraste com o FUNDO (fundo claro → outline escuro; fundo escuro → outline claro)
                outline = pick_outline_vs_bg(g_bg_color);
            }

            SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, 255);
        }
        SDL_RenderDrawRect(renderer, &keys[i].rect);

        // Cor do texto coerente com a paleta atual (HC3 usa HC3_DARK↔HC3_LIGHT)
        const SDL_Color textColor = pick_text_for_fill(keys[i].color);
        draw_text(renderer, font, keys[i].label.c_str(), keys[i].rect, textColor);


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