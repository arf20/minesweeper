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

    fbdev.c: fbdev frontend

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>

#include <common/frontconf.h>
#include <common/game.h>
#include "fbdev.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static const int *board = NULL;
static int size = 0;

static int wWidth = 0, wHeight = 0;

static int fbfd = 0, screensize = 0;

typedef struct {
    char b, g, r, a;
} bgra_t;

static int sWidth = 0, fbWidth = 0, sHeight = 0;
static bgra_t *fbp = NULL; /* assume 32bpp */
#define FB_XY(x, y)  fbp[((y) * fbWidth) + (x)]

/* assume 9x15 font that starts at space */
const unsigned char *fontimg = NULL;
int fontiw = 0, fontih = 0;
#define FONT_W  9
#define FONT_H  15
#define FONT_FIRSTC 0x20

#define TXT_OFFX    5
#define TXT_OFFY    4

/* flag */
const unsigned char *flag = NULL;
int flagw = 0, flagh = 0;

/* term stuff */
struct termios raw;
struct termios orig;

/* cursor */
int curx = 0, cury = 0;

/* assume 32bpp */
bgra_t fbColor(char r, char g, char b, char a) {
    bgra_t c;
    c.r = r; c.g = g; c.b = b; c.a = a;
    return c;
}

#define FB_TRANS    fbColor(0x00, 0x00, 0x00, 0x00)
#define FB_WHITE    fbColor(0xff, 0xff, 0xff, 0xff)
#define FB_BLACK    fbColor(0x00, 0x00, 0x00, 0xff)
#define FB_BLUE     fbColor(0x00, 0x00, 0xff, 0xff)
#define FB_GREEN    fbColor(0x00, 0xff, 0x00, 0xff)
#define FB_RED      fbColor(0xff, 0x00, 0x00, 0xff)
#define FB_DARKBLUE fbColor(0x00, 0x00, 0x8b, 0xff)
#define FB_DARKRED  fbColor(0x8b, 0x00, 0x00, 0xff)
#define FB_DARKCYAN fbColor(0x00, 0x8b, 0x8b, 0xff)
#define FB_DARKGREY fbColor(0xa9, 0xa9, 0xa9, 0xff)

/* fb utils */
void
fbClear() {
    //for (int y = 0; y < sHeight; y++)
    //    for (int x = 0; x < sWidth; x++)
    //        FB_XY(x, y) = FB_BLACK;
    memset(fbp, 0x00, sizeof(int) * sWidth * sHeight);
}

void
fbFillRect(int ix, int iy, int w, int h, bgra_t c) {
    if (ix < 0 || iy < 0 || ix + w >= sWidth || iy + h >= sHeight) return;
    for (int y = iy; y < iy + h; y++)
        for (int x = ix; x < ix + w; x++)
            FB_XY(x, y) = c;
}

void
fbHLine(int ix, int l, int y, bgra_t c) {
    if (ix < 0 || y < 0 || ix + l >= sWidth || y >= sHeight) return;
    for (int x = ix; x < ix + l; x++)
        FB_XY(x, y) = c;
}

void
fbVLine(int x, int l, int iy, bgra_t c) {
    if (x < 0 || iy < 0 || x >= sWidth || iy + l >= sHeight) return;
    for (int y = iy; y < iy + l; y++)
        FB_XY(x, y) = c;
}

void
fbRenderFont(int dx, int dy, int sx, int sy, int w, int h, bgra_t fg) {
    if (dx < 0 || dy < 0 || dx + w > sWidth || dy + h > sHeight) return;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            FB_XY(dx + x, dy + y)
                = fontimg[((sy + y) * fontiw) + (sx + x)] > 0 ? fg : FB_TRANS;
}

void
fbDrawString(int ix, int iy, bgra_t fg, const char *str, size_t len) {
    const char *ptr = str;
    int i = 0;
    while (*ptr && (i < len)) {
        i = ptr - str;
        if (*ptr < FONT_FIRSTC) { ptr++; continue; }
        fbRenderFont((FONT_W * i) + ix, iy, FONT_W * (*ptr - FONT_FIRSTC), 0,
            FONT_W, FONT_H, fg);
        ptr++;
    }
}

static void
drawTextMultiline(int x, int y, const char *str) {
    const char *line = str, *next = NULL;
    int len = strlen(str), i = 0;
    while (line < str + len) {
        next = strchr(line, '\n');
        fbDrawString(x, y + (i * FONT_H), FB_WHITE, line, next - line);
        line = next + 1;
        i++;
    }
}

static void
drawFlag(int ix, int iy) {
    for (int y = 0; y < flagh; y++)
        for (int x = 0; x < flagw; x++)
            FB_XY(ix + x + 2, iy + y + 2)
                = flag[(flagw * y) + x] > 0 ? FB_RED : FB_WHITE;
}

static void
render() {
    fbClear();
    fbDrawString(5, 15, FB_WHITE, TXT_TITLE, sizeof(TXT_TITLE));

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
    fbDrawString(wWidth - 25, 35, FB_WHITE, buff, strlen(buff));

    bgra_t c;
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
                        case 1: c = FB_BLUE; break;
                        case 2: c = FB_GREEN; break;
                        case 3: c = FB_RED; break;
                        case 4: c = FB_DARKBLUE; break;
                        case 5: c = FB_DARKRED; break;
                        case 6: c = FB_DARKCYAN; break;
                        case 7: c = FB_BLACK; break;
                        case 8: c = FB_DARKGREY; break;
                    }
                    fbDrawString(cX + TXT_OFFX, cY + TXT_OFFY,
                        c, buff, strlen(buff));
                }
            }
            /* If not clear, check flag and draw it */
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                fbFillRect(cX, cY, CELL_SIZE, CELL_SIZE, FB_WHITE);
                drawFlag(cX, cY);
            }
            /* Otherwise just a tile */
            else {
                fbFillRect(cX, cY, CELL_SIZE, CELL_SIZE, FB_WHITE);
            }

            /* draw cursor */
            if (x == curx && y == cury) {
                fbHLine(cX, CELL_SIZE, cY, FB_RED);
                fbHLine(cX, CELL_SIZE, cY + CELL_SIZE - 1, FB_RED);
                fbVLine(cX, CELL_SIZE, cY, FB_RED);
                fbVLine(cX + CELL_SIZE - 1, CELL_SIZE, cY, FB_RED);
            }
        }
    }
}

int
fbdevStart(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    wWidth = (2 * W_MARGIN) + (size * CELL_SIZE) + ((size - 1) * CELL_MARGIN);
    wHeight = HEADER_HEIGHT + W_MARGIN + (size * CELL_SIZE) +
        ((size - 1) * CELL_MARGIN);

    /* Read bitmap font */
    int ch;
    fontimg = stbi_load(FONT_BMP_PATH, &fontiw, &fontih, &ch, 1);
    if (!fontimg) {
        printf("Error loading bitmap font: " FONT_BMP_PATH "\n");
        return -1;
    }
    printf("Bitmap font: %dx%d, %dch\n", fontiw, fontih, ch);

    /* Read flag bitmap */
    flag = stbi_load(FLAG_PNG_PATH, &flagw, &flagh, &ch, 1);
    if (!fontimg) {
        printf("Error loading bitmap font: " FLAG_PNG_PATH "\n");
        return -1;
    }
    printf("Bitmap flag: %dx%d, %dch\n", fontiw, fontih, ch);

    /* Through the power of linux magic, open the framebuffer device and map it
        to memory */
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        printf("Error opening /dev/fb0: %s\n", strerror(errno));
        return -1;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        printf("Error reading fb variable information: %s\n", strerror(errno));
        return -1;
    }

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        printf("Error reading fb variable information: %s\n", strerror(errno));
        return -1;
    }

    sWidth = vinfo.xres; sHeight = vinfo.yres;
    fbWidth = finfo.line_length / sizeof(int);
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    const int PADDING = 4096;
    int mmapsize = (screensize + PADDING - 1) & ~(PADDING-1);

    fbp = (bgra_t*)mmap(0, mmapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbp == MAP_FAILED) {
        printf("Error mapping fbdev to memory: %s\n", strerror(errno));
        return -1;
    }

    printf("fb0: %d(%d)x%d, %dbpp\n", vinfo.xres, fbWidth, vinfo.yres,
        vinfo.bits_per_pixel);

    /* Set terminal to non-canonical non-blocking mode mode, this may fuck it up */
    tcgetattr(STDIN_FILENO, &orig);
    raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flags);

    render();
    char input[8] = { 0 };
    int bytesread;
    while (1) {
        while ((bytesread = read(STDIN_FILENO, &input, 8)) > 0) {
            for (int i = 0; i < bytesread; i++) {
                if (input[i] == '\033') {
                    if (strncmp(input + i, "\033[A", 3) == 0) cury--;
                    if (strncmp(input + i, "\033[B", 3) == 0) cury++;
                    if (strncmp(input + i, "\033[C", 3) == 0) curx++;
                    if (strncmp(input + i, "\033[D", 3) == 0) curx--;
                    i += 2;
                } else if (isalpha(input[i])) {
                    input[i] = tolower(input[i]);
                    switch (input[i]) {
                        case 'a': curx--; break;
                        case 'd': curx++; break;
                        case 'w': cury--; break;
                        case 's': cury++; break;
                        case 'f': gameFlagCell(curx, cury); break;
                        case 'c': gameClearCell(curx, cury); break;
                    }
                }
                if (curx < 0) curx = size - 1;
                if (cury < 0) cury = size - 1;
                if (curx >= size) curx = 0;
                if (cury >= size) cury = 0;
            }
            render();
            memset(input, 0, 8);
        }
        //render();
        //usleep(25000);
    }
}

void
fbdevDestroy() {
    free((void*)fontimg);
    munmap(fbp, screensize);
    close(fbfd);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}
