#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <string>
#include <cmath>

using namespace std;

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

// Declaração das funções definidas em Bpart0.cpp e Cpart0.cpp
extern int run_scanning(Display* display, Window target_window, SDL_Renderer* renderer);
extern int run_selection(Display* display, Window target_window, SDL_Renderer* renderer);
extern int run_settings(SDL_Renderer* renderer);

bool g_is_fullscreen = false;   // estado global do F11

const char* FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";
TTF_Font* font = nullptr;          // torna-se GLOBAL neste arquivo
const int BASE_FONT_PX = 24;

bool reload_font()                 // reabre a fonte no tamanho correto
{
    if (font)  TTF_CloseFont(font);
    font = TTF_OpenFont(FONT_PATH, static_cast<int>(BASE_FONT_PX * g_scale_pct / 100.0));
    return font != nullptr;
}

// Exibe o menu principal
int show_main_menu(SDL_Renderer* renderer)
{
    bool needs_redraw = true;
    SDL_Event ev;

    /* Retângulos dos botões serão recalculados sempre que a janela mudar */
    SDL_Rect optionB{}, optionC{}, settings{};

    while (true) {
        /* -----------------------------------------------------------------
         * (re)desenha apenas quando realmente necessário
         * -----------------------------------------------------------------*/
        if (needs_redraw) {
            int winW, winH;
            SDL_GetRendererOutputSize(renderer, &winW, &winH);

            const int btnW = static_cast<int>(lround(SZ(500))), btnH = static_cast<int>(lround(SZ(60))), gap = static_cast<int>(lround(SZ(30)));
            int totalH   = 3 * btnH + 2 * gap;
            int startY   = (winH - totalH) / 2;
            int startX   = (winW - btnW)  / 2;

            optionB  = {startX,                 startY,               btnW, btnH};
            optionC  = {startX, startY + btnH + gap,                 btnW, btnH};
            settings = {startX, startY + 2 * (btnH + gap),           btnW, btnH};

            clear_with_bg(renderer);

            SDL_Color uiOutline = pick_outline_vs_bg(g_bg_color);

            SDL_SetRenderDrawColor(renderer, uiOutline.r, uiOutline.g, uiOutline.b, 255);
            SDL_RenderDrawRect(renderer, &optionB);
            SDL_RenderDrawRect(renderer, &optionC);
            SDL_RenderDrawRect(renderer, &settings);

            draw_text(renderer, font, "Solução B (Varredura Automática)", optionB,  uiOutline);
            draw_text(renderer, font, "Solução C (Mouse Adaptado)",       optionC,  uiOutline);
            draw_text(renderer, font, "Configurações",                    settings, uiOutline);

            SDL_RenderPresent(renderer);
            needs_redraw = false;
        }

        /* -----------------------------------------------------------------
         * trata eventos
         * -----------------------------------------------------------------*/
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)      return -1;

            if (ev.type == SDL_WINDOWEVENT &&
                ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                needs_redraw = true;                         // redimensionou

            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F11) {
                extern bool g_is_fullscreen;
                g_is_fullscreen = !g_is_fullscreen;
                SDL_SetWindowFullscreen(SDL_GetWindowFromID(1),
                        g_is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                needs_redraw = true;                         // tela mudou → redesenhar
            }

            if (ev.type == SDL_MOUSEBUTTONDOWN &&
                ev.button.button == SDL_BUTTON_LEFT) {
                int mx = ev.button.x, my = ev.button.y;
                if (mx >= optionB.x && mx <= optionB.x + optionB.w &&
                    my >= optionB.y && my <= optionB.y + optionB.h) return 1;
                if (mx >= optionC.x && mx <= optionC.x + optionC.w &&
                    my >= optionC.y && my <= optionC.y + optionC.h) return 2;
                if (mx >= settings.x && mx <= settings.x + settings.w &&
                    my >= settings.y && my <= settings.y + settings.h) return 3;
            }
        }
        SDL_Delay(10);  // evita usar 100 % da CPU
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("TEcSRM - Terminal do Eleitor com Severa Restrição de Mobilidade",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        700, 350,
        SDL_WINDOW_RESIZABLE);                // <- agora redimensionável
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!reload_font()) { cerr << "Falha ao abrir fonte\n"; return 1; }

    // === Cursor maior mantendo o desenho do tema (Xcursor) =================
    Cursor xcur = 0;       // guardaremos para liberar no final
    Display* dpy_for_cursor = nullptr;

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (SDL_GetWindowWMInfo(window, &wmi) && wmi.subsystem == SDL_SYSWM_X11) {
        dpy_for_cursor = wmi.info.x11.display;
        ::Window xwin  = wmi.info.x11.window;

        // Tamanho desejado (experimente 48, 64, 72…)
        XcursorSetDefaultSize(dpy_for_cursor, 72);
        xcur = XcursorLibraryLoadCursor(dpy_for_cursor, "left_ptr");
        if (xcur) {
            XDefineCursor(dpy_for_cursor, xwin, xcur);
            XFlush(dpy_for_cursor);
        }
    }
    // ======================================================================

    Display* display = XOpenDisplay(NULL);
    if (!display) {
        cerr << "Erro ao abrir a conexão com o servidor X." << endl;
        return -1;
    }

    Window target_window = 0x600003;

    int choice;

    do {
        choice = show_main_menu(renderer);
        if (choice == 1) choice = run_scanning(display, target_window, renderer);
        else if (choice == 2) choice = run_selection(display, target_window, renderer);
        else if (choice == 3) choice = run_settings(renderer);
    } while (choice != -1);

    // Libera cursor custom, se criado
    if (xcur && dpy_for_cursor) XFreeCursor(dpy_for_cursor, xcur);

    XCloseDisplay(display);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
