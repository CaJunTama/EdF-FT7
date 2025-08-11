// settings.cpp – tela de Configurações com botão "Alto Contraste"
#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <array>
#include <cmath>

using namespace std;

/* --- escalas disponíveis -------------------------------------------------- */
constexpr array<int,5> SCALES = {100, 125, 150, 175, 200};

int run_settings(SDL_Renderer* renderer)
{
    /* encontra o índice atual na tabela */
    int idx = 0;
    while (idx < (int)SCALES.size() && SCALES[idx] < g_scale_pct) ++idx;

    bool needs_redraw = true;
    SDL_Event ev;

    /* tamanhos base (antes da função SZ) */
    const int BTN_SIZE0 = 40;   // altura dos botões
    const int GAP_H0    = 12;   // gap horizontal
    const int GAP_V0    = 12;   // gap vertical

    SDL_Rect btnPlus{}, btnMinus{}, txtBox{}, btnContrast{};

    while (true) {
        /* ===================== REDESENHO =================================== */
        if (needs_redraw) {
            int winW, winH;
            SDL_GetRendererOutputSize(renderer, &winW, &winH);

            /* aplica o fator de escala global */
            int BTN_SIZE = static_cast<int>(lround(SZ(BTN_SIZE0)));
            int GAP_H    = static_cast<int>(lround(SZ(GAP_H0)));
            int GAP_V    = static_cast<int>(lround(SZ(GAP_V0)));

            /* ------------ layout da área de zoom (+/−) --------------------- */
            string zoomStr = "Zoom: " + to_string(g_scale_pct) + "%";
            int txtW, txtH;  TTF_SizeUTF8(font, zoomStr.c_str(), &txtW, &txtH);

            int groupW  = txtW + GAP_H + BTN_SIZE;              // texto + gap + botão
            int totalH  = 2*BTN_SIZE + 2*GAP_V + BTN_SIZE;      // +/-    + gaps + contraste
            int startX  = (winW - groupW) / 2;
            int startY  = (winH - totalH) / 2;

            /* botão + */
            btnPlus  = {startX + txtW + GAP_H, startY,                BTN_SIZE, BTN_SIZE};
            /* botão − */
            btnMinus = {btnPlus.x,              btnPlus.y + BTN_SIZE + GAP_V,
                                             BTN_SIZE, BTN_SIZE};
            /* caixa do texto de zoom */
            int midY  = (btnPlus.y + BTN_SIZE + btnMinus.y) / 2;
            txtBox   = {startX, midY - txtH / 2, txtW, txtH};

            /* botão Alto Contraste, logo abaixo */
            btnContrast = {startX, btnMinus.y + BTN_SIZE + GAP_V, groupW, BTN_SIZE};

            /* desenha elementos ------------------------------------------------- */
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_RenderDrawRect(renderer, &btnPlus);
            SDL_RenderDrawRect(renderer, &btnMinus);
            SDL_RenderDrawRect(renderer, &btnContrast);

            draw_text(renderer, font, zoomStr.c_str(),  txtBox,      BLACK);
            draw_text(renderer, font, "+",                btnPlus,   BLACK);
            draw_text(renderer, font, "−",                btnMinus,  BLACK);
            draw_text(renderer, font, "Alto Contraste",   btnContrast, BLACK);

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
                if (ev.key.keysym.sym == SDLK_ESCAPE) return 0;      // voltar ao menu
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
                int mx = ev.button.x, my = ev.button.y;
                bool hitPlus     = (mx > btnPlus.x     && mx < btnPlus.x     + btnPlus.w     &&
                                    my > btnPlus.y    && my < btnPlus.y    + btnPlus.h);
                bool hitMinus    = (mx > btnMinus.x    && mx < btnMinus.x    + btnMinus.w    &&
                                    my > btnMinus.y   && my < btnMinus.y   + btnMinus.h);
                bool hitContrast = (mx > btnContrast.x && mx < btnContrast.x + btnContrast.w &&
                                    my > btnContrast.y&& my < btnContrast.y+ btnContrast.h);

                if (hitPlus && idx < (int)SCALES.size() - 1) {
                    ++idx; g_scale_pct = SCALES[idx]; reload_font(); needs_redraw = true;
                }
                if (hitMinus && idx > 0) {
                    --idx; g_scale_pct = SCALES[idx]; reload_font(); needs_redraw = true;
                }
                if (hitContrast) {
                    /* ação futura: alternar modo de alto contraste */
                    SDL_Log("Botão 'Alto Contraste' pressionado – funcionalidade futura.");
                }
            }
        }
        SDL_Delay(10);
    }
}
