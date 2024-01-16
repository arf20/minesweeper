/*

    arfminesweeper: Cross-plataform multi-frontend game
    Copyright (C) 2023 arf20 (Ángel Ruiz Fernandez)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    sdl1.c: SDL1 frontend

*/

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
//#include <SDL/SDL_image.h>

#include <common/frontconf.h>
#include "sdl1.h"
#include <common/game.h>


static const int *board = NULL;
static int size = 0;

#define CI_WHITE  0xffffffff
#define CI_BLACK  0x00000000
#define C_WHITE  255, 255, 255, 255
#define C_BLACK  0, 0, 0, 0
#define C_YELLOW 255, 255, 0, 255
#define C_RED    255, 0, 0, 255
#define C_GREEN  0, 255, 0, 255
#define C_BLUE   0, 0, 255, 255
#define C_DBLUE  0, 0, 139, 255
#define C_DRED   139, 0, 0, 255
#define C_DCYAN  0, 139, 139, 255
#define C_DGREY  169, 169, 169, 255

#define TEXT_CENTERX    (unsigned int)1
#define TEXT_CENTERY    (unsigned int)2

static SDL_Surface *screen;
static SDL_Surface *flag;
static TTF_Font *font;
static int run = 1;

static int wWidth = 0, wHeight = 0;

static SDL_Color
SDLColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    struct SDL_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    return c;
}

static void
renderText(const char *str, TTF_Font* f, int x, int y, unsigned int flags,
    SDL_Color c)
{
    SDL_Surface* surfaceText = TTF_RenderText_Blended(f, str, c);

    SDL_Rect rectText;  // create a rect
    rectText.x = x;     // controls the rect's x coordinate 
    rectText.y = y;     // controls the rect's y coordinte
    rectText.w = 0;     // controls the width of the rect
    rectText.h = 0;     // controls the height of the rect
    int w, h;

    TTF_SizeText(f, str, &w, &h);
    rectText.w = w; rectText.h = h;

    if (flags & TEXT_CENTERX) rectText.x -= rectText.w / 2;
    if (flags & TEXT_CENTERY) rectText.y -= rectText.h / 2;

    SDL_BlitSurface(surfaceText, NULL, screen, &rectText);

    SDL_FreeSurface(surfaceText);
}

static void
renderSurface(SDL_Surface *s, int w, int h, int x, int y) {
    SDL_Rect rect;
    rect.h = h; rect.w = w; rect.x = x; rect.y = y;
    SDL_BlitSurface(s, NULL, screen, &rect);
}

static void
render() {
    static char buff[256];

    SDL_FillRect(screen, NULL, 0x000000);

    /* Draw title */
    renderText(TXT_TITLE, font, 5, 5, 0, SDLColor(C_WHITE));

    /* Check game state*/
    switch (gameGetState()) {
        case STATE_LOST: {
            //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
            //    "Game Over", TXT_LOST, w);
            run = 0;
            return;
        } break;
        case STATE_WON: {
            //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
            //    "Game Over", TXT_WON, w);
            run = 0;
        } break;
    }

    /* Print flags left */
    snprintf(buff, 256, "%d", gameGetFlagsLeft());
    renderText(buff, font, wWidth - 25, 35, 0, SDLColor(C_WHITE));

    /* Render cell matrix */
    SDL_Color c;
    int cX, cY, n;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            cX = W_MARGIN + (x * (CELL_SIZE + CELL_MARGIN));
            cY = HEADER_HEIGHT + (y * (CELL_SIZE + CELL_MARGIN));
            /* If clear, count surrounding cells and print n of mines */
            if (CHECK_CLEAR(BOARDXY(x, y))) {
                n = gameGetSurroundingMines(x, y);
                if (n) {
                    snprintf(buff, 256, "%d", n);
                    switch (n) {
                        case 1: c = SDLColor(C_BLUE); break;
                        case 2: c = SDLColor(C_GREEN); break;
                        case 3: c = SDLColor(C_RED); break;
                        case 4: c = SDLColor(C_DBLUE); break;
                        case 5: c = SDLColor(C_DRED); break;
                        case 6: c = SDLColor(C_DCYAN); break;
                        case 7: c = SDLColor(C_BLACK); break;
                        case 8: c = SDLColor(C_DGREY); break;
                    }
                    renderText(buff, font, cX, cY, 0, c);
                }
            }
            /* If not clear, check flag and draw it */
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                SDL_Rect cellRect = {cX, cY, CELL_SIZE, CELL_SIZE};
                SDL_FillRect(screen, &cellRect, CI_WHITE);
                renderSurface(flag, CELL_SIZE, CELL_SIZE, cX, cY);
            }
            /* Otherwise just a tile */
            else {
                SDL_Rect cellRect = {cX, cY, CELL_SIZE, CELL_SIZE};
                SDL_FillRect(screen, &cellRect, CI_WHITE);
            }
        }
    }

    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

int
SDL1Start(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    wWidth = (2 * W_MARGIN) + (size * CELL_SIZE) + ((size - 1) *
        CELL_MARGIN);
    wHeight = HEADER_HEIGHT + W_MARGIN + (size * CELL_SIZE) +
        ((size - 1) * CELL_MARGIN);

    /* Initialise SDL2 */
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        printf("SDL_Init failed: %s\n", SDL_GetError());

    if (TTF_Init() < 0)
        printf("TTF_Init failed: %s\n", TTF_GetError());

    /* Create stuff */
    if ((screen = SDL_SetVideoMode(wWidth, wHeight, 0,
        SDL_SWSURFACE)) == NULL)
        printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());

    SDL_WM_SetCaption(TXT_TITLE, TXT_TITLE);

    
    if (!(font = TTF_OpenFont(FONT_TTF_PATH, 16)))
        printf("Error opening font: %s\n", TTF_GetError());

    if (!(flag = SDL_LoadBMP(FLAG_PNG_PATH)))
        printf("Error loading texture: %s\n", SDL_GetError());

    /* SDL event loop */
    SDL_Event e;
    while (run) {
        render();
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_KEYDOWN: {
                    //event.key.keysym.sym
                } break;
                case SDL_MOUSEBUTTONDOWN: {
                    /* Coordinates */
                    int ix = (e.button.x - W_MARGIN) /
                    (CELL_SIZE + CELL_MARGIN);
                    int iy = (e.button.y - HEADER_HEIGHT) /
                        (CELL_SIZE + CELL_MARGIN);
                    if (ix < 0 || ix >= size || iy < 0 || iy >= size) continue;

                    switch (e.button.button) {
                        case SDL_BUTTON_LEFT: {
                            gameClearCell(ix, iy);
                        } break;
                        case SDL_BUTTON_RIGHT: {
                            gameFlagCell(ix, iy);
                        } break;
                    }
                } break;
                case SDL_QUIT: {
                    run = 0;
                } break;
            }
        }
    }
}

void
SDL1Destroy() {
    SDL_FreeSurface(flag);
    TTF_Quit();
    SDL_Quit();
}
