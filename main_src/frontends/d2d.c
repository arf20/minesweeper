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

    d2d.c: Win32 Direct2D frontend

*/

#include <stdio.h>

#include <windows.h>


#include <d2d1.h>

#include <common/frontconf.h>
#include "gdi.h"
#include <common/game.h>


static const int* board = NULL;
static int size = 0;

static int wWidth = 0, wHeight = 0;

static HWND mainhWnd = NULL;

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


static LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE: DestroyWindow(hWnd); break;
    case WM_DESTROY: {
        PostQuitMessage(0);
    } break;
    default: return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

int
d2dStart(const int* lboard, int lsize) {
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
        return -1;
    }

    ShowWindow(mainhWnd, SW_SHOW);
    UpdateWindow(mainhWnd);

    ID2D1Factory *d2dFactory;
    D2D1_FACTORY_OPTIONS d2dOpt = { D2D1_DEBUG_LEVEL_NONE };
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, &d2dOpt, &d2dFactory);
    if (hr != S_OK) {
        printf("D2D1CreateFactory error\n");
        return -1;
    }

    /* CreateHwndRenderTarget */
    

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

void
d2dDestroy() {

}
