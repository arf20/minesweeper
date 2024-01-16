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

    win32.c: Win32 ComCtl API frontend

*/

#include <stdio.h>

#include <windows.h>

#include <common/frontconf.h>
#include "win32.h"
#include <common/game.h>


static const int* board = NULL;
static int size = 0;

static int wWidth = 0, wHeight = 0;

static HWND mainhWnd = NULL;
static HWND titleLabel = NULL;
static HWND flagsLeftLabel = NULL;
static HWND *buttons = NULL;
static HWND *labels = NULL;

static HANDLE flag = NULL;

static WNDPROC OldButtonProc;

#define RGB_RED         255, 0, 0
#define RGB_GREEN       0, 255, 0
#define RGB_BLUE        0, 0, 255
#define RGB_DBLUE       0, 0, 139
#define RGB_DRED        139, 0, 0
#define RGB_DCYAN       0, 139, 139
#define RGB_DGREY       169, 169, 169

static void
updateButtons(HWND hWnd) {
    static char buff[256];

    /* Check game state*/
    switch (gameGetState()) {
        case STATE_LOST: {
            MessageBox(hWnd, TXT_LOST, "Game Over", MB_OK | MB_ICONSTOP | MB_APPLMODAL);
            return;
        } break;
        case STATE_WON: {
            MessageBox(hWnd, TXT_WON, "Game Over", MB_OK | MB_ICONWARNING | MB_APPLMODAL);
            return;
        } break;
    }

    /* Show flags left */
    snprintf(buff, 256, "%d", gameGetFlagsLeft());
    SendMessage(flagsLeftLabel, WM_SETTEXT, 0, (LPARAM)buff);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            /* Variables stuff */
            int btni = (x * size) + y;

            /* If clear, hide the button, count surrounding cells and print
                n of mines */
            if (CHECK_CLEAR(BOARDXY(x, y))) {
                ShowWindow(buttons[btni], SW_HIDE);
                ShowWindow(labels[btni], SW_SHOW);
            }
            /* If not clear, check flag and draw it */
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                SendMessage(buttons[btni], BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)flag);
                printf("sendmessage: %d\n", GetLastError());
            }
            /* Otherwise just a tile */
            else {
                /* Clear flag if applicable */
                SendMessage(buttons[btni], BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)NULL);
                ShowWindow(buttons[btni], SW_SHOW);
                ShowWindow(labels[btni], SW_HIDE);
            }
        }
    }

    RedrawWindow(mainhWnd, NULL, NULL, RDW_ALLCHILDREN);
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
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW));
            EndPaint(hWnd, &ps);
        } break;
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, TRANSPARENT);

            int btni = -1;
            for (int i = 0; i < size * size; i++) if (labels[i] == (HWND)lParam) btni = i;
            int n = gameGetSurroundingMines(btni / size, btni % size);
            switch (n) {
                case 1: SetTextColor(hdcStatic, RGB(0, 0, 255)); break;
                case 2: SetTextColor(hdcStatic, RGB(0, 255, 0)); break;
                case 3: SetTextColor(hdcStatic, RGB(255, 0, 0)); break;
                case 4: SetTextColor(hdcStatic, RGB(0, 0, 139)); break;
                case 5: SetTextColor(hdcStatic, RGB(139, 0, 0)); break;
                case 6: SetTextColor(hdcStatic, RGB(0, 139, 139)); break;
                case 7: SetTextColor(hdcStatic, RGB(0, 0, 0)); break;
                case 8: SetTextColor(hdcStatic, RGB(169, 169, 169)); break;
            }
            return (LRESULT)GetSysColorBrush(COLOR_MENU);
        }
        default: return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

static LRESULT CALLBACK
ButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_LBUTTONUP: {
            int btni = (int)GetProp(hWnd, TEXT("btni"));
            gameClearCell(btni / size, btni % size);
            updateButtons(hWnd);
            printf("left click %d\n", btni);
            return CallWindowProc((WNDPROC)OldButtonProc, hWnd, uMsg, wParam, lParam);
        } break;
        case WM_RBUTTONUP: {
            int btni = (int)GetProp(hWnd, TEXT("btni"));
            gameFlagCell(btni / size, btni % size);
            updateButtons(hWnd);
            printf("right click %d\n", btni);
            return CallWindowProc((WNDPROC)OldButtonProc, hWnd, uMsg, wParam, lParam);
        } break;
        case WM_DESTROY: {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)OldButtonProc);
        } break;
        default: return CallWindowProc((WNDPROC)OldButtonProc, hWnd, uMsg, wParam, lParam);
    }
}

int
Win32Start(const int *lboard, int lsize) {
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
    RECT crect = { 0, 0, wWidth, wHeight};
    AdjustWindowRectEx(&crect, WS_OVERLAPPEDWINDOW, 0, 0);

    mainhWnd = CreateWindowEx(0, className, TXT_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, crect.right - crect.left, crect.bottom - crect.top,
        NULL, 0, hInstance, NULL);
    if (!mainhWnd) {
        printf("Error creating window: %d\n", GetLastError());
    }
    //SetWindowTheme(mainhWnd, L" ", L" ");

    

    titleLabel = CreateWindowEx(0, "STATIC", TXT_TITLE, WS_CHILD | WS_VISIBLE,
        5, 2, 110, 20, mainhWnd, NULL, hInstance, NULL);

    flagsLeftLabel = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
        wWidth - 25, 25, 20, 20, mainhWnd, NULL, hInstance, NULL);

    /* Load flag image */
    flag = LoadImage(NULL, FLAG_ICO_PATH, IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    if (!flag) {
        printf("Error loading " FLAG_ICO_PATH ": %d\n", GetLastError());
    }

    /* Allocate object arrays */
    buttons = malloc(sizeof(HWND) * size * size);
    labels = malloc(sizeof(HWND) * size * size);
    
    /* Add UI matrix */
    char buff[256];
    int btni = 0;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            /* Variables stuff */
            btni = (size * y) + x;
            int cX = W_MARGIN + (x * (CELL_SIZE + CELL_MARGIN));
            int cY = HEADER_HEIGHT + (y * (CELL_SIZE + CELL_MARGIN));

            /* Button */
            buttons[btni] = CreateWindowEx(0, "BUTTON", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_BITMAP,
                cX, cY, CELL_SIZE, CELL_SIZE, mainhWnd, (HMENU)btni, hInstance, NULL);
            OldButtonProc = (WNDPROC)SetWindowLongPtr(buttons[btni], GWLP_WNDPROC, (LONG_PTR)ButtonProc);
            SetProp(buttons[btni], TEXT("btni"), (HANDLE)btni);

            /* Label */
            int n = gameGetSurroundingMines(y, x);
            if (n) snprintf(buff, 256, "%d", n);
            else *buff = 0;
            labels[btni] = CreateWindowEx(0, "STATIC", buff, WS_VISIBLE | WS_CHILD | SS_CENTER,
                cX, cY, CELL_SIZE, CELL_SIZE, mainhWnd, NULL, hInstance, NULL);
        }
    }

    updateButtons(mainhWnd);

    ShowWindow(mainhWnd, SW_SHOW);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

void
Win32Destroy() {
    
}
