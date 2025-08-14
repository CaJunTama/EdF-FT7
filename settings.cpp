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

/* ===== Helpers locais (quadrados) ======================================== */
static inline bool hit_rect(int mx, int my, const SDL_Rect& r) {
    return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
}

// Desenha botão quadrado com:
// - borda (anel retangular)
// - miolo (quadrado interno)
// - outline externo preto (fino)
// - outline interno preto (fino, na fronteira borda↔miolo)
static void draw_square_button(SDL_Renderer* r,
                               const SDL_Rect& outer,
                               SDL_Color fill, SDL_Color border,
                               int border_thick,
                               int outline_ext_thick,   // p.ex. lround(SZ(1))
                               int outline_int_thick)   // p.ex. lround(SZ(1))
{
    // 1) Preenche a borda (anel) como um retângulo cheio
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
    SDL_RenderFillRect(r, &outer);

    // 2) Miolo (quadrado menor "cavado" pela espessura da borda)
    SDL_Rect inner{
        outer.x + border_thick,
        outer.y + border_thick,
        std::max(0, outer.w - 2*border_thick),
        std::max(0, outer.h - 2*border_thick)
    };
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
    if (inner.w > 0 && inner.h > 0) SDL_RenderFillRect(r, &inner);

    // 3) Outline interno preto (fino) — na borda do miolo
    if (outline_int_thick > 0 && inner.w > 0 && inner.h > 0) {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        for (int k = 0; k < outline_int_thick; ++k) {
            SDL_Rect ir{
                inner.x + k, inner.y + k,
                std::max(0, inner.w - 2*k), std::max(0, inner.h - 2*k)
            };
            if (ir.w > 0 && ir.h > 0) SDL_RenderDrawRect(r, &ir);
        }
    }

    // 4) Outline externo preto (fino) — ao redor do quadrado todo
    if (outline_ext_thick > 0) {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        for (int k = 0; k < outline_ext_thick; ++k) {
            SDL_Rect orr{
                outer.x - k, outer.y - k,
                outer.w + 2*k, outer.h + 2*k
            };
            if (orr.w > 0 && orr.h > 0) SDL_RenderDrawRect(r, &orr);
        }
    }
}

/* ===== Tela =============================================================== */
int run_settings(SDL_Renderer* renderer)
{
    /* encontra o índice atual na tabela */
    int idx = 0;
    while (idx < (int)SCALES.size() && SCALES[idx] < g_scale_pct) ++idx;

    bool needs_redraw = true;
    SDL_Event ev;

    /* tamanhos base (antes da função SZ) */
    const int BTN_SIZE0    = 40;   // altura/largura dos botões + e −
    const int GAP_H0       = 12;   // gap horizontal
    const int GAP_V0       = 12;   // gap vertical entre linhas
    const int GAP_ROWS0    = 24;   // espaço entre a linha de Zoom e a de Alto Contraste
    const int BOX_BORDER0  = 5;    // espessura base da borda do quadrado
    const int BOX_OUTLINE0 = 1;  // 1 px em 100%

    SDL_Rect btnPlus{}, btnMinus{}, txtZoomBox{};
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
            const int zoomGroupH = std::max(zH, BTN_SIZE);

            // a altura total do bloco (linha zoom + espaçamento + linha HC)
            const string hcStr = "Alto Contraste: ";
            int hcWText = 0, hcHText = 0;
            TTF_SizeUTF8(font, hcStr.c_str(), &hcWText, &hcHText);

            const int gapBoxes = GAP_H; // espaçamento entre quadrados
            const int hcLineW  = hcWText + GAP_H + 3*BTN_SIZE + 2*gapBoxes;
            const int hcLineH  = std::max(hcHText, BTN_SIZE);

            const int totalH   = zoomGroupH + GAP_ROWS + hcLineH;
            const int baseY    = (winH - totalH) / 2;

            // ---- posicionamento Zoom (centralizado) ----
            const int zoomStartX = (winW - zoomGroupW) / 2;
            const int zoomStartY = baseY;

            txtZoomBox = { zoomStartX, zoomStartY + (zoomGroupH - zH) / 2, zW, zH };
            btnPlus    = { txtZoomBox.x + txtZoomBox.w + GAP_H, zoomStartY, BTN_SIZE, BTN_SIZE };
            btnMinus   = { btnPlus.x + btnPlus.w + GAP_H,       zoomStartY, BTN_SIZE, BTN_SIZE };

            // ---- posicionamento linha Alto Contraste (centralizado) ----
            const int hcStartX = (winW - hcLineW) / 2;
            const int hcStartY = baseY + zoomGroupH + GAP_ROWS;

            txtHCBox = { hcStartX, hcStartY + (hcLineH - hcHText)/2, hcWText, hcHText };

            const int qY = hcStartY + (hcLineH - BTN_SIZE)/2;
            const int q1x = hcStartX + hcWText + GAP_H;
            const int q2x = q1x + BTN_SIZE + gapBoxes;
            const int q3x = q2x + BTN_SIZE + gapBoxes;

            q1 = { q1x, qY, BTN_SIZE, BTN_SIZE };
            q2 = { q2x, qY, BTN_SIZE, BTN_SIZE };
            q3 = { q3x, qY, BTN_SIZE, BTN_SIZE };

            /* ------------------------ desenho -------------------------------- */
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderClear(renderer);

            // linha 1 (zoom)
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_RenderDrawRect(renderer, &btnPlus);
            SDL_RenderDrawRect(renderer, &btnMinus);
            draw_text(renderer, font, zoomStr.c_str(), txtZoomBox, BLACK);
            draw_text(renderer, font, "+", btnPlus,  BLACK);
            draw_text(renderer, font, "−", btnMinus, BLACK);

            // linha 2 (alto contraste) - texto sem borda
            draw_text(renderer, font, hcStr.c_str(), txtHCBox, BLACK);

            // três botões quadrados (placeholders)
            // 1) miolo branco, borda preta
            draw_square_button(renderer, q1, WHITE, BLACK, BOX_BORDER,
                               BOX_OUTLINE, BOX_OUTLINE);

            // 2) miolo preto, borda branca
            draw_square_button(renderer, q2, BLACK, WHITE, BOX_BORDER,
                               BOX_OUTLINE, BOX_OUTLINE);

            // 3) miolo branco, borda branca
            draw_square_button(renderer, q3, WHITE, WHITE, BOX_BORDER,
                               BOX_OUTLINE, BOX_OUTLINE);

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

                // Placeholders Alto Contraste (por enquanto, só logam)
                if (hit_rect(mx, my, q1)) {
                    SDL_Log("HC placeholder 1 (miolo branco / borda preta)");
                } else if (hit_rect(mx, my, q2)) {
                    SDL_Log("HC placeholder 2 (miolo preto / borda branca)");
                } else if (hit_rect(mx, my, q3)) {
                    SDL_Log("HC placeholder 3 (miolo branco / borda branca)");
                }
            }
        }
        SDL_Delay(10);
    }
}
