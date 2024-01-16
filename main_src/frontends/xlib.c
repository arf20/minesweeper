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

    xlib.c: X11 Xlib frontend

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>

#include <common/frontconf.h>
#include <common/game.h>
#include "xlib.h"

#define TXT_OFFX    5
#define TXT_OFFY    15

static const int *board = NULL;
static int size = 0;

static int wWidth = 0, wHeight = 0;

static Display *d = NULL;
static int s = 0;
static Window w = 0;
static GC gc;
static Colormap cm;
static Font f;

static XColor blue, green, red, darkblue, darkred, darkcyan, /*black*/ darkgrey;

static void
drawFlag(int x, int y) {
    static XPoint flag[5] = {
        {CELL_SIZE/4, 2},
        {CELL_SIZE/4, CELL_SIZE-2},
        {(CELL_SIZE/4) + 2, CELL_SIZE-2},
        {(CELL_SIZE/4) + 2, CELL_SIZE/2},
        {(3*CELL_SIZE/4) + 2, (CELL_SIZE/2)-3},
    };
    static XPoint flagTrans[5];
    for (int i = 0; i < 5; i++) {
        flagTrans[i].x = flag[i].x + x;
        flagTrans[i].y = flag[i].y + y;
    }
    XFillPolygon(d, w, gc, flagTrans, 5, Complex, CoordModeOrigin);
}

static void
drawTextMultiline(int x, int y, const char *str) {
    const char *line = str, *next = NULL;
    int len = strlen(str), i = 0;
    while (line < str + len) {
        next = strchr(line, '\n');
        XDrawString(d, w, gc, x, y + (i * TXT_OFFY), line, next - line);
        line = next + 1;
        i++;
    }
}

static void
render() {
    XClearWindow(d, w);
    XSetForeground(d, gc, WhitePixel(d, s));
    XSetFont(d, gc, f);

    /* Draw title */
    XDrawString(d, w, gc, 5, 15, TXT_TITLE, strlen(TXT_TITLE));

    /* Check game state*/
    switch (gameGetState()) {
        case STATE_LOST: {
            drawTextMultiline(5, 45, TXT_LOST);
            return;
        } break;
        case STATE_WON: {
            drawTextMultiline(5, 45, TXT_WON);
            return;
        } break;
    }

    /* Print flags left */
    static char buff[256];
    snprintf(buff, 256, "%d", gameGetFlagsLeft());
    XDrawString(d, w, gc, wWidth - 25, 35, buff, strlen(buff));

    /* Render cell matrix */
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int cX = W_MARGIN + (x * (CELL_SIZE + CELL_MARGIN));
            int cY = HEADER_HEIGHT + (y * (CELL_SIZE + CELL_MARGIN));
            /* If clear, count surrounding cells and print n of mines */
            if (CHECK_CLEAR(BOARDXY(x, y))) {
                int n = gameGetSurroundingMines(x, y);
                if (n) {
                    snprintf(buff, 256, "%d", n);
                    switch (n) {
                        case 1: XSetForeground(d, gc, blue.pixel); break;
                        case 2: XSetForeground(d, gc, green.pixel); break;
                        case 3: XSetForeground(d, gc, red.pixel); break;
                        case 4: XSetForeground(d, gc, darkblue.pixel); break;
                        case 5: XSetForeground(d, gc, darkred.pixel); break;
                        case 6: XSetForeground(d, gc, darkcyan.pixel); break;
                        case 7: XSetForeground(d, gc, BlackPixel(d, s)); break;
                        case 8: XSetForeground(d, gc, darkgrey.pixel); break;
                    }
                    XDrawString(d, w, gc, cX + TXT_OFFX, cY + TXT_OFFY,
                        buff, strlen(buff));
                }
            }
            /* If not clear, check flag and draw it */
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                XSetForeground(d, gc, WhitePixel(d, s));
                XFillRectangle(d, w, gc, cX, cY, CELL_SIZE, CELL_SIZE);
                XSetForeground(d, gc, red.pixel);
                drawFlag(cX, cY);
            }
            /* Otherwise just a tile */
            else {
                XSetForeground(d, gc, WhitePixel(d, s));
                XFillRectangle(d, w, gc, cX, cY, CELL_SIZE, CELL_SIZE);
            }
        }
    }
}

int
XlibStart(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    wWidth = (2 * W_MARGIN) + (size * CELL_SIZE) + ((size - 1) * CELL_MARGIN);
    wHeight = HEADER_HEIGHT + W_MARGIN + (size * CELL_SIZE) +
        ((size - 1) * CELL_MARGIN);
    
    /* Xlib is pretty self explanatory */
    d = XOpenDisplay(NULL);
    if (d == NULL) {
        printf("Cannot open display\n");
        exit(1);
    }
    s = DefaultScreen(d);

    w = XCreateSimpleWindow(d, XRootWindow(d, s), 50, 50, wWidth, wHeight,
        0, BlackPixel(d, s), BlackPixel(d, s));

    XStoreName(d, w, TXT_TITLE);

    XSelectInput(d, w, ExposureMask | KeyPressMask | ButtonPressMask);
    XMapWindow(d, w);

    gc = XCreateGC(d, w, 0, NULL);

    cm = DefaultColormap(d, s);

    f = XLoadFont(d, "9x15");

    /* Generate colors - names defined in X11's rgb.txt */
    XAllocNamedColor(d, cm, "blue", &blue, &blue);
    XAllocNamedColor(d, cm, "green", &green, &green);
    XAllocNamedColor(d, cm, "red", &red, &red);
    XAllocNamedColor(d, cm, "darkblue", &darkblue, &darkblue);
    XAllocNamedColor(d, cm, "darkred", &darkred, &darkred);
    XAllocNamedColor(d, cm, "darkcyan", &darkcyan, &darkcyan);
    XAllocNamedColor(d, cm, "darkgrey", &darkgrey, &darkgrey);

    /* Event loop */
    XEvent e;
    while (1) {
        XNextEvent(d, &e);
        switch (e.type) {
            case Expose: {
                render();
                XFlush(d);
            } break;
            case KeyPress: {
                render();
                XFlush(d);
            } break;
            case ButtonPress: {
                int ix = (e.xbutton.x - W_MARGIN) /
                    (CELL_SIZE + CELL_MARGIN);
                int iy = (e.xbutton.y - HEADER_HEIGHT) /
                    (CELL_SIZE + CELL_MARGIN);
                if (ix < 0 || ix >= size || iy < 0 || iy >= size) continue;

                switch (e.xbutton.button) {
                    case 1: { /* Left */
                        gameClearCell(ix, iy);
                    } break;
                    case 3: { /* Right */
                        gameFlagCell(ix, iy);
                    } break;
                }
                render();
                XFlush(d);
            } break;
        }
    }
}

void
XlibDestroy() {
    XDestroyWindow(d, w);
    XCloseDisplay(d);
}
