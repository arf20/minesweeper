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

    gdi.c: Win32 GDI frontend

*/

#include <stdio.h>

#include <windows.h>
#include <Windowsx.h>

#include <common/frontconf.h>
#include "gdi.h"
#include <common/game.h>


static const int* board = NULL;
static int size = 0;

static int wWidth = 0, wHeight = 0;

static HWND mainhWnd = NULL;

static HBITMAP hFlag;

#define TXT_OFFX 5
#define TXT_OFFY 1

#define C_WHITE  255, 255, 255
#define C_BLACK  0, 0, 0
#define C_YELLOW 255, 255, 0
#define C_RED    255, 0, 0
#define C_GREEN  0, 255, 0
#define C_BLUE   0, 0, 255
#define C_DBLUE  0, 0, 139
#define C_DRED   139, 0, 0
#define C_DCYAN  0, 139, 139
#define C_DGREY  169, 169, 169

static void
render(HDC hdc) {
    printf("render\n");
    /* Set font */
    HFONT hFont, hOldFont;
    hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
    SelectObject(hdc, hFont);

    /* Draw title */
    TextOut(hdc, 5, 5, TXT_TITLE, sizeof(TXT_TITLE));

    /* Check game state */
    switch (gameGetState()) {
        case STATE_LOST: {
            MessageBox(mainhWnd, TXT_LOST, "Game Over", MB_OK | MB_ICONSTOP | MB_APPLMODAL);
            return;
        } break;
        case STATE_WON: {
            MessageBox(mainhWnd, TXT_WON, "Game Over", MB_OK | MB_ICONWARNING | MB_APPLMODAL);
            return;
        } break;
    }

    /* Print flags left */
    static char buff[256];
    snprintf(buff, 256, "%d", gameGetFlagsLeft());
    TextOut(hdc, wWidth - 25, 35, buff, strlen(buff));

    /* For drawing flags */
    BITMAP bmFlag;
    HDC hdcMem = CreateCompatibleDC(hdc);
    SelectObject(hdcMem, hFlag);
    GetObject(hFlag, sizeof(bmFlag), &bmFlag);

    /* Render cell matrix */
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
                        case 1: SetTextColor(hdc, RGB(0, 0, 255)); break;
                        case 2: SetTextColor(hdc, RGB(0, 255, 0)); break;
                        case 3: SetTextColor(hdc, RGB(255, 0, 0)); break;
                        case 4: SetTextColor(hdc, RGB(0, 0, 139)); break;
                        case 5: SetTextColor(hdc, RGB(139, 0, 0)); break;
                        case 6: SetTextColor(hdc, RGB(0, 139, 139)); break;
                        case 7: SetTextColor(hdc, RGB(0, 0, 0)); break;
                        case 8: SetTextColor(hdc, RGB(169, 169, 169)); break;
                    }
                    TextOut(hdc, cX + TXT_OFFX, cY + TXT_OFFY, buff, strlen(buff));
                }
            }
            /* If not clear, check flag and draw it */
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                RECT cellrect = { cX, cY, cX + CELL_SIZE, cY + CELL_SIZE };
                FillRect(hdc, &cellrect, (HBRUSH)(COLOR_GRAYTEXT + 1));
                //renderTexture(flag, CELL_SIZE, CELL_SIZE, cX, cY);
                //BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                //AlphaBlend(hdc, cX, cY, CELL_SIZE, CELL_SIZE, hdcMem, 0, 0, 16, 16, blend);
                BitBlt(hdc, cX, cY, 16, 16, hdcMem, 0, 0, 16, 16, SRCCOPY);
            }
            /* Otherwise just a tile */
            else {
                RECT cellrect = { cX, cY, cX + CELL_SIZE, cY + CELL_SIZE };
                FillRect(hdc, &cellrect, (HBRUSH)(COLOR_GRAYTEXT + 1));
            }
        }
    }

}

static LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE: DestroyWindow(hWnd); break;
    case WM_DESTROY: {
        PostQuitMessage(0);
    } break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
        render(hdc);
        EndPaint(hWnd, &ps);
    } break;
    case WM_LBUTTONUP:  /* fall down */
    case WM_RBUTTONUP: {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int ix = (xPos - W_MARGIN) /
            (CELL_SIZE + CELL_MARGIN);
        int iy = (yPos - HEADER_HEIGHT) /
            (CELL_SIZE + CELL_MARGIN);
        if (ix < 0 || ix >= size || iy < 0 || iy >= size) break;

        if (uMsg == WM_LBUTTONUP) {
            gameClearCell(ix, iy);
            printf("left click %d, %d\n", ix, iy);
        }
        else {
            gameFlagCell(ix, iy);
            printf("right click %d, %d\n", ix, iy);
        }

        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
    } break;
    default: return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

int
gdiStart(const int* lboard, int lsize) {
    board = lboard;
    size = lsize;

    wWidth = (2 * W_MARGIN) + (size * CELL_SIZE) + ((size - 1) * CELL_MARGIN);
    wHeight = HEADER_HEIGHT + W_MARGIN + (size * CELL_SIZE) +
        ((size - 1) * CELL_MARGIN);

    /* Win32 stuff */
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS winclass = { 0 };

    const char className[] = TXT_TITLE;

    winclass.lpfnWndProc = WindowProc;
    winclass.hInstance = hInstance;
    winclass.lpszClassName = className;
    winclass.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&winclass);

    /* Create window */
    RECT crect = { 0, 0, wWidth, wHeight };
    AdjustWindowRectEx(&crect, WS_OVERLAPPEDWINDOW, 0, 0);

    mainhWnd = CreateWindowEx(0, className, TXT_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, crect.right - crect.left, crect.bottom - crect.top,
        NULL, 0, hInstance, NULL);
    if (!mainhWnd) {
        printf("Error creating window: %d\n", GetLastError());
    }

    hFlag = (HBITMAP)LoadImage(NULL, FLAG_BMP_PATH, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    ShowWindow(mainhWnd, SW_SHOW);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

void
gdiDestroy() {

}
