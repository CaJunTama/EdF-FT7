// settings.cpp – tela de Configurações (Zoom + Alto Contraste com 3 botões QUADRADOS)
#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <array>
#include <cmath>
#include <algorithm>

using namespace std;

/* --- escalas disponíveis -------------------------------------------------- */
constexpr array<int,5> SCALES = {100, 125, 150, 175, 200};

// Placeholder de tamanhos do cursor (1 a 5), default = 1
constexpr array<int,5> CURSOR_SIZES = {1, 2, 3, 4, 5};

/* ===== Helpers locais (quadrados) ======================================== */
static inline bool hit_rect(int mx, int my, const SDL_Rect& r) {
    return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
}

// === Helpers de contraste p/ escolher outline coerente (PB ou HC3) =========
static inline double srgb_to_linear_01(uint8_t c) {
    const double cs = c / 255.0;
    return (cs <= 0.04045) ? (cs / 12.92) : pow((cs + 0.055) / 1.055, 2.4);
}
static inline double rel_lum(SDL_Color c) {
    const double R = srgb_to_linear_01(c.r);
    const double G = srgb_to_linear_01(c.g);
    const double B = srgb_to_linear_01(c.b);
    return 0.2126 * R + 0.7152 * G + 0.0722 * B;
}
static inline double contrast_ratio(SDL_Color a, SDL_Color b) {
    const double La = rel_lum(a), Lb = rel_lum(b);
    const double Lmax = (La > Lb) ? La : Lb;
    const double Lmin = (La > Lb) ? Lb : La;
    return (Lmax + 0.05) / (Lmin + 0.05);
}
static inline SDL_Color best_from_pair_for(SDL_Color base, SDL_Color dark, SDL_Color light) {
    return (contrast_ratio(base, dark) >= contrast_ratio(base, light)) ? dark : light;
}
static inline SDL_Color pick_outline_vs_bg(SDL_Color bg) {
    if (g_hc_option == 3) { // HC3: usa o par colorido
        return best_from_pair_for(bg, HC3_DARK, HC3_LIGHT);
    }
    // HC1/HC2: PB
    return best_from_pair_for(bg, SDL_Color{0,0,0,255}, SDL_Color{255,255,255,255});
}

// Desenha botão quadrado com:
// - borda (anel retangular)
// - miolo (quadrado interno)
// - outline externo preto (fino)
// - outline interno preto (fino, na fronteira borda↔miolo)
static void draw_square_button(SDL_Renderer* r,
    const SDL_Rect& outer, SDL_Color fill, SDL_Color border,
    int border_thick, SDL_Color outline_col, int outline_ext_thick)
{
    // 1) borda grossa
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
    SDL_RenderFillRect(r, &outer);

    // 2) miolo
    SDL_Rect inner{
        outer.x + border_thick, outer.y + border_thick,
        max(0, outer.w - 2*border_thick),
        max(0, outer.h - 2*border_thick)
    };
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
    if (inner.w > 0 && inner.h > 0) SDL_RenderFillRect(r, &inner);

    // 4) outline externo (fino) ao redor do quadrado todo
    SDL_SetRenderDrawColor(r, outline_col.r, outline_col.g, outline_col.b, 255);
    SDL_RenderDrawRect(r, &outer);
}

/* ===== Tela =============================================================== */
int run_settings(SDL_Renderer* renderer)
{
    /* encontra o índice atual na tabela */
    int idx = 0;
    while (idx < (int)SCALES.size() && SCALES[idx] < g_scale_pct) ++idx;

    int idxCursor = 0;  // default = 1 (posição 0 do array CURSOR_SIZES)

    bool needs_redraw = true;
    SDL_Event ev;

    /* tamanhos base (antes da função SZ) */
    const int BTN_SIZE0    = 40;   // altura/largura dos botões + e −
    const int GAP_H0       = 12;   // gap horizontal
    const int GAP_V0       = 12;   // gap vertical entre linhas
    const int GAP_ROWS0    = 96;   // espaço entre a linha de Zoom e a de Alto Contraste
    const int BOX_BORDER0  = 5;    // espessura base da borda do quadrado
    const int BOX_OUTLINE0 = 1;  // 1 px em 100%

    SDL_Rect btnPlus{}, btnMinus{}, txtZoomBox{};
    // Cursor (placeholder)
    SDL_Rect curPlus{}, curMinus{}, txtCurBox{};
    // Velocidade (placeholder)
    SDL_Rect btnSpdPlus{}, btnSpdMinus{}, txtSpdBox{};

    SDL_Rect txtHCBox{};
    SDL_Rect q1{}, q2{}, q3{};     // retângulos dos três botões
    

    while (true) {
        /* ===================== REDESENHO =================================== */
        if (needs_redraw) {
            int winW, winH;
            SDL_GetRendererOutputSize(renderer, &winW, &winH);

            /* aplica o fator de escala global */
            const int BTN_SIZE   = (int)lround(SZ(BTN_SIZE0));
            const int GAP_H      = (int)lround(SZ(GAP_H0));
            const int GAP_V      = (int)lround(SZ(GAP_V0));
            const int GAP_ROWS   = (int)lround(SZ(GAP_ROWS0));
            const int BOX_BORDER = (int)lround(SZ(BOX_BORDER0));
            const int BOX_OUTLINE = (int)lround(SZ(BOX_OUTLINE0));  // 1 px em 100%

            /* ------------ linha 1: Zoom (texto + “+” + “−”) ---------------- */
            string zoomStr = "Zoom: " + to_string(g_scale_pct) + "%";
            int zW = 0, zH = 0;
            TTF_SizeUTF8(font, zoomStr.c_str(), &zW, &zH);

            const int zoomGroupW = zW + GAP_H + BTN_SIZE + GAP_H + BTN_SIZE;
            const int zoomGroupH = max(zH, BTN_SIZE);

            // Cursor (placeholder) – mesmo padrão do Zoom
            string curStr = string("Tamanho do Cursor: ") + to_string(CURSOR_SIZES[idxCursor]);
            int cW = 0, cH = 0;
            TTF_SizeUTF8(font, curStr.c_str(), &cW, &cH);
            const int curGroupW = cW + GAP_H + BTN_SIZE + GAP_H + BTN_SIZE;
            const int curGroupH = max(cH, BTN_SIZE);

            // Linha 2 (placeholder): Velocidade de varredura — mesmo padrão do Zoom [texto][+][−]
            string spdStr = "Velocidade de Varredura: " + to_string(g_scan_speed);
            int spdW = 0, spdH = 0;
            TTF_SizeUTF8(font, spdStr.c_str(), &spdW, &spdH);

            const int spdGroupW = spdW + GAP_H + BTN_SIZE + GAP_H + BTN_SIZE;
            const int spdGroupH = max(spdH, BTN_SIZE);

            // a altura total do bloco (linha zoom + espaçamento + linha HC)
            const string hcStr = "Alto Contraste: ";
            int hcWText = 0, hcHText = 0;
            TTF_SizeUTF8(font, hcStr.c_str(), &hcWText, &hcHText);

            const int gapBoxes = GAP_H; // espaçamento entre quadrados
            const int hcLineW  = hcWText + GAP_H + 3*BTN_SIZE + 2*gapBoxes;
            const int hcLineH  = max(hcHText, BTN_SIZE);

            const int totalH = zoomGroupH + GAP_V + curGroupH + GAP_V + spdGroupH + GAP_ROWS + hcLineH;
            const int baseY    = (winH - totalH) / 2;

            // ---- posicionamento Zoom (centralizado) ----
            const int zoomStartX = (winW - zoomGroupW) / 2;
            const int zoomStartY = baseY;

            txtZoomBox = { zoomStartX, zoomStartY + (zoomGroupH - zH) / 2, zW, zH };
            btnPlus    = { txtZoomBox.x + txtZoomBox.w + GAP_H, zoomStartY, BTN_SIZE, BTN_SIZE };
            btnMinus   = { btnPlus.x + btnPlus.w + GAP_H,       zoomStartY, BTN_SIZE, BTN_SIZE };

            // ---- posicionamento Cursor (centralizado, abaixo do Zoom) ----
            const int curStartX = (winW - curGroupW) / 2;
            const int curStartY = zoomStartY + zoomGroupH + GAP_V;

            txtCurBox = { curStartX, curStartY + (curGroupH - cH)/2, cW, cH };
            curPlus   = { txtCurBox.x + txtCurBox.w + GAP_H, curStartY, BTN_SIZE, BTN_SIZE };
            curMinus  = { curPlus.x + curPlus.w + GAP_H,     curStartY, BTN_SIZE, BTN_SIZE };

            // Posicionamento da linha Velocidade: centralizado, abaixo do Zoom
            const int spdStartX = (winW - spdGroupW) / 2;
            const int spdStartY = curStartY + curGroupH + GAP_V;

            txtSpdBox   = { spdStartX, spdStartY + (spdGroupH - spdH)/2, spdW, spdH };
            btnSpdPlus  = { txtSpdBox.x + txtSpdBox.w + GAP_H, spdStartY, BTN_SIZE, BTN_SIZE };
            btnSpdMinus = { btnSpdPlus.x + btnSpdPlus.w + GAP_H,        spdStartY, BTN_SIZE, BTN_SIZE };

            // ---- posicionamento linha Alto Contraste (centralizado) ----
            const int hcStartX = (winW - hcLineW) / 2;
            const int hcStartY = baseY + zoomGroupH + GAP_V + curGroupH + GAP_V + spdGroupH + GAP_ROWS;

            txtHCBox = { hcStartX, hcStartY + (hcLineH - hcHText)/2, hcWText, hcHText };

            const int qY = hcStartY + (hcLineH - BTN_SIZE)/2;
            const int q1x = hcStartX + hcWText + GAP_H;
            const int q2x = q1x + BTN_SIZE + gapBoxes;
            const int q3x = q2x + BTN_SIZE + gapBoxes;

            q1 = { q1x, qY, BTN_SIZE, BTN_SIZE };
            q2 = { q2x, qY, BTN_SIZE, BTN_SIZE };
            q3 = { q3x, qY, BTN_SIZE, BTN_SIZE };

            /* ------------------------ desenho -------------------------------- */
            clear_with_bg(renderer);

            SDL_Color uiOutline = pick_outline_vs_bg(g_bg_color);

            // linha 1 (zoom)
            SDL_SetRenderDrawColor(renderer, uiOutline.r, uiOutline.g, uiOutline.b, 255);
            SDL_RenderDrawRect(renderer, &btnPlus);
            SDL_RenderDrawRect(renderer, &btnMinus);
            draw_text(renderer, font, zoomStr.c_str(), txtZoomBox, uiOutline);
            draw_text(renderer, font, "+", btnPlus,  uiOutline);
            draw_text(renderer, font, "−", btnMinus, uiOutline);

            // Desenho da linha Cursor (mesmo estilo do Zoom)
            SDL_SetRenderDrawColor(renderer, uiOutline.r, uiOutline.g, uiOutline.b, 255);
            SDL_RenderDrawRect(renderer, &curPlus);
            SDL_RenderDrawRect(renderer, &curMinus);
            draw_text(renderer, font, curStr.c_str(), txtCurBox, uiOutline);
            draw_text(renderer, font, "+", curPlus,  uiOutline);
            draw_text(renderer, font, "−", curMinus, uiOutline);

            // Desenho da linha Velocidade (mesmo estilo do Zoom)
            SDL_SetRenderDrawColor(renderer, uiOutline.r, uiOutline.g, uiOutline.b, 255);
            SDL_RenderDrawRect(renderer, &btnSpdPlus);
            SDL_RenderDrawRect(renderer, &btnSpdMinus);
            draw_text(renderer, font, spdStr.c_str(), txtSpdBox, uiOutline);
            draw_text(renderer, font, "+", btnSpdPlus,  uiOutline);
            draw_text(renderer, font, "−", btnSpdMinus, uiOutline);

            // linha 2 (alto contraste) - texto sem borda
            draw_text(renderer, font, hcStr.c_str(), txtHCBox, uiOutline);

            // três botões quadrados (placeholders)
            // 1) miolo branco, borda preta
            draw_square_button(renderer, q1, BLACK, WHITE, BOX_BORDER,
                               uiOutline, BOX_OUTLINE);

            // 2) miolo preto, borda branca
            draw_square_button(renderer, q2, WHITE, BLACK, BOX_BORDER,
                               uiOutline, BOX_OUTLINE);

            // 3) miolo branco, borda branca
            draw_square_button(renderer, q3, HC3_DARK, HC3_LIGHT, BOX_BORDER,
                               uiOutline, BOX_OUTLINE);

            SDL_RenderPresent(renderer);
            needs_redraw = false;
        }

        /* ===================== EVENTOS ===================================== */
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)          return -1;

            if (ev.type == SDL_WINDOWEVENT &&
                ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                needs_redraw = true;

            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) return 0;      // voltar
                if (ev.key.keysym.sym == SDLK_F11) {
                    extern bool g_is_fullscreen;
                    g_is_fullscreen = !g_is_fullscreen;
                    SDL_SetWindowFullscreen(SDL_GetWindowFromID(1),
                        g_is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    needs_redraw = true;
                }
            }

            if (ev.type == SDL_MOUSEBUTTONDOWN &&
                ev.button.button == SDL_BUTTON_LEFT) {
                const int mx = ev.button.x, my = ev.button.y;

                // Zoom +/-
                if (hit_rect(mx, my, btnPlus) && idx < (int)SCALES.size() - 1) {
                    ++idx; g_scale_pct = SCALES[idx]; reload_font(); needs_redraw = true;
                } else if (hit_rect(mx, my, btnMinus) && idx > 0) {
                    --idx; g_scale_pct = SCALES[idx]; reload_font(); needs_redraw = true;
                } 

                // Placeholder do Cursor (1..5)
                else if (hit_rect(mx, my, curPlus) && idxCursor < (int)CURSOR_SIZES.size() - 1) {
                    ++idxCursor; needs_redraw = true;
                } else if (hit_rect(mx, my, curMinus) && idxCursor > 0) {
                    --idxCursor; needs_redraw = true;
                }

                // Placeholder da Velocidade (1..5)
                else if (hit_rect(mx, my, btnSpdPlus) && g_scan_speed < 5) {
                    ++g_scan_speed; needs_redraw = true;
                } else if (hit_rect(mx, my, btnSpdMinus) && g_scan_speed > 1) {
                    --g_scan_speed; needs_redraw = true;
                }

                // Placeholders Alto Contraste (por enquanto, só logam)
                if (hit_rect(mx, my, q1)) {
                    set_palette_default();          // <<< ativa a paleta padrão
                    needs_redraw = true;            // força redesenho desta tela
                } else if (hit_rect(mx, my, q2)) {
                    set_palette_inverted();
                    needs_redraw = true;
                } else if (hit_rect(mx, my, q3)) {
                    set_palette_hc3();           // ativa a Opção 3 (colorido)
                    needs_redraw = true;
                }
            }
        }
        SDL_Delay(10);
    }
}
