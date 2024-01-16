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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <forms.h>

#include <common/frontconf.h>
#include <common/game.h>

static const int *board = NULL;
static int size = 0;

static int wWidth = 0, wHeight = 0;

static FL_FORM *form = NULL;
static FL_OBJECT *flagsLeftLabel = NULL;
static FL_OBJECT **buttons = NULL;
static FL_OBJECT **numbers = NULL;

static void
updateButtons() {
    static char buff[256];

    /* Show flags left */
    snprintf(buff, 256, "%d", gameGetFlagsLeft());
    fl_set_object_label(flagsLeftLabel, buff);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            /* Variables stuff */
            int btni = (x * size) + y;

            /* If clear, hide the button, count surrounding cells and print
                n of mines */
            if (CHECK_CLEAR(BOARDXY(x, y))) {
                fl_hide_object(buttons[btni]);
                int n = gameGetSurroundingMines(x, y);

                if (n) {
                    snprintf(buff, 256, "%d", n);

                    /* Set text */
                    fl_set_object_label(numbers[btni], buff);

                    /* Set color */
                    switch (n) {
                        case 1: fl_set_object_lcolor(numbers[btni],
                            FL_BLUE); break;
                        case 2: fl_set_object_lcolor(numbers[btni],
                            FL_GREEN);break;
                        case 3: fl_set_object_lcolor(numbers[btni],
                            FL_RED); break;
                        case 4: fl_set_object_lcolor(numbers[btni],
                            FL_DARKCYAN); break;
                        case 5: fl_set_object_lcolor(numbers[btni],
                            FL_INDIANRED); break;
                        case 6: fl_set_object_lcolor(numbers[btni],
                            FL_CYAN); break;
                        case 7: fl_set_object_lcolor(numbers[btni],
                            FL_BLACK); break;
                        case 8: fl_set_object_lcolor(numbers[btni],
                            FL_INACTIVE); break;
                    }

                    /* Show text */
                    fl_show_object(numbers[btni]);
                }
            }
            /* If not clear, check flag and draw it */
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                fl_set_object_lcolor(buttons[btni], FL_RED);
            }
            /* Otherwise just a tile */
            else {
                /* Clear flag if applicable */
                fl_set_object_lcolor(buttons[btni], FL_COL1);
            }
        }
    }

    /* Check state */
    switch (gameGetState()) {
        case STATE_LOST: {
            FL_POPUP *popup = fl_popup_add(fl_root, "game over");
            fl_popup_add_entries(popup, TXT_LOST);
            fl_popup_do(popup);
            fl_finish();
            exit(0);
        } break;
        case STATE_WON: {
            FL_POPUP *popup = fl_popup_add(fl_root, "game over");
            fl_popup_add_entries(popup, TXT_WON);
            fl_popup_do(popup);
            fl_finish();
            exit(0);
        } break;
    }
}

static void
buttonCallback(FL_OBJECT *btn, long btni) {
    /* Mouse press on button */
    const XEvent *e = fl_last_event();
    if (e->type != ButtonRelease) return;

    switch (e->xbutton.button) {
        case 1: { /* Left */
            gameClearCell(btni / size, btni % size);
        } break;
        case 3: { /* Right */
            gameFlagCell(btni / size, btni % size);
        } break;
    }

    updateButtons();
}

int
xformsStart(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    wWidth = (2 * W_MARGIN) + (size * CELL_SIZE) + ((size - 1) * CELL_MARGIN);
    wHeight = HEADER_HEIGHT + W_MARGIN + (size * CELL_SIZE) +
        ((size - 1) * CELL_MARGIN);

    char *argv[2] = { "arfminesweeper",  NULL};
    int argc = 1;

    /* Initialize XForms */
    fl_initialize(&argc, argv, 0, 0, 0);

    /* Create form */
    form = fl_bgn_form(FL_UP_BOX, wWidth, wHeight);

    /* Add title */
    FL_OBJECT *title = fl_add_text(FL_NORMAL_TEXT, 2, 2, 120, 20, TXT_TITLE);
    /* Add flags left */
    flagsLeftLabel = fl_add_text(FL_NORMAL_TEXT, wWidth - 30, 25, 25, 20, "");

    /* Allocate object arrays */
    buttons = malloc(sizeof(FL_OBJECT*) * size * size);
    numbers = malloc(sizeof(FL_OBJECT*) * size * size);

    /* Add UI matrix */
    int btni = 0;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            /* Variables stuff */
            btni = (size * y) + x;
            int cX = W_MARGIN + (x * (CELL_SIZE + CELL_MARGIN));
            int cY = HEADER_HEIGHT + (y * (CELL_SIZE + CELL_MARGIN));

            /* Button */
            buttons[btni] = fl_add_bitmapbutton(FL_NORMAL_BUTTON,
                cX, cY, CELL_SIZE, CELL_SIZE, "");
            fl_set_bitmapbutton_file(buttons[btni], FLAG_XBM_PATH);
            fl_set_object_color(buttons[btni], FL_COL1, FL_COL1);
            fl_set_object_lcolor(buttons[btni], FL_COL1);
            fl_set_object_callback(buttons[btni],
                buttonCallback, btni);

            /* Text hidden by default */
            numbers[btni] = fl_add_text(FL_NORMAL_TEXT,
                cX, cY, CELL_SIZE, CELL_SIZE, "");
            fl_hide_object(numbers[btni]);
        }
    }

    /* End form, then show it and run the loop */
    fl_end_form();
 
    updateButtons();

    fl_show_form(form, FL_PLACE_MOUSE, FL_FULLBORDER, TXT_TITLE);
    fl_do_forms();
    return 0;
}

void
xformsDestroy() {
    fl_hide_form(form);
    fl_finish();
}
