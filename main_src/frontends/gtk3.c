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

    gtk3.c: Gtk3 frontend

*/

#include <gtk/gtk.h>

#include <common/frontconf.h>
#include <common/game.h>

static const int *board = NULL;
static int size = 0;

static GtkWidget *window, *flaglabel = NULL, **flagimages = NULL, **buttons = NULL, **numbers = NULL;

static void
updateButtons() {
    static char buff[256];

    /* Show flags left */
    snprintf(buff, 256, "%d", gameGetFlagsLeft());
    gtk_label_set_label(GTK_LABEL(flaglabel), buff);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            /* Variables stuff */
            int btni = (y * size) + x;

            /* If clear, hide the button, count surrounding cells and print
                n of mines */
            if (CHECK_CLEAR(BOARDXY(x, y))) {
                gtk_widget_hide(buttons[btni]);

                int n = gameGetSurroundingMines(x, y);
                if (n) {
                    snprintf(buff, 256, "%d", n);

                    /* Set text */
                    gtk_label_set_text(GTK_LABEL(numbers[btni]), buff);
                    
                    /* Set color - names the same has X11 */
                    GdkRGBA c;
                    switch (n) {
                        case 1: gdk_rgba_parse(&c, "blue"); break;
                        case 2: gdk_rgba_parse(&c, "green"); break;
                        case 3: gdk_rgba_parse(&c, "red"); break;
                        case 4: gdk_rgba_parse(&c, "darkblue"); break;
                        case 5: gdk_rgba_parse(&c, "darkred"); break;
                        case 6: gdk_rgba_parse(&c, "darkcyan"); break;
                        case 7: gdk_rgba_parse(&c, "black"); break;
                        case 8: gdk_rgba_parse(&c, "darkgrey"); break;
                    }

                    gtk_widget_override_color(numbers[btni], GTK_STATE_FLAG_NORMAL, &c);

                    /* Show text */
                    gtk_widget_show(numbers[btni]);
                }
            }
            /* If not clear, check flag and draw it */
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                //gtk_button_set_label(GTK_BUTTON(buttons[btni]), NULL);
                gtk_button_set_image(GTK_BUTTON(buttons[btni]), flagimages[btni]);
            }
            /* Otherwise just a tile */
            else {
                /* Clear flag if applicable */
                gtk_button_set_image(GTK_BUTTON(buttons[btni]), NULL);
                //gtk_button_set_label(GTK_BUTTON(buttons[btni]), " ");
            }
        }
    }

    /* Check state */
    GtkWidget *dialog;
    switch (gameGetState()) {
        case STATE_LOST: {
            dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, TXT_LOST);
            gtk_dialog_run(GTK_DIALOG (dialog));
            gtk_widget_destroy(dialog);
            gtk_window_close(GTK_WINDOW(window));
        } break;
        case STATE_WON: {
            dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, TXT_WON);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            gtk_window_close(GTK_WINDOW(window));
        } break;
    }
}

static gboolean
buttonCallback(GtkWidget *btn, GdkEventButton *event, gpointer data) {
    int btni = *(int*)data;
    int x = btni % size;
    int y = btni / size;

    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
        /* 3 is right mouse btn */
        gameFlagCell(x, y);
    }
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 1) {
        /* 1 is left mouse btn */
        gameClearCell(x, y);
    }

    updateButtons();

    return 0;
}

static void
activate(GtkApplication* app, gpointer user_data) {
    GtkWidget *grid, *titlelabel, *buttongrid;

    buttons = malloc(sizeof(GtkWidget*) * size * size);
    numbers = malloc(sizeof(GtkWidget*) * size * size);
    flagimages = malloc(sizeof(GtkWidget*) * size * size);
    int *btnids = malloc(sizeof(int) * size * size); /* Stupid */

    /* Window*/
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), TXT_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(window), 100, 100);

    /* Grid layout */
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    /* Title label */
    titlelabel = gtk_label_new(TXT_TITLE);
    gtk_grid_attach(GTK_GRID(grid), titlelabel, 0, 0, 1, 1);

    /* Flag label */
    flaglabel = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid), flaglabel, 0, 1, 1, 1);

    /* Button grid */
    buttongrid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), buttongrid, 0, 2, 1, 1);


    /* Add buttons to grid */
    int btni = 0;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            /* Variables stuff */
            btni = (size * y) + x;

            /* Create button, attach it to the grind,
                and connect click signal to callback */
            btnids[btni] = btni;
            buttons[btni] = gtk_button_new();
            gtk_grid_attach(GTK_GRID(buttongrid), buttons[btni], x, y, 1, 1);
            g_signal_connect(G_OBJECT(buttons[btni]), "button-press-event",
                G_CALLBACK(buttonCallback), btnids + btni);
            gtk_widget_set_size_request(buttons[btni], CELL_SIZE, CELL_SIZE);

            /* Flag images - i hate Gtk  */
            flagimages[btni] = gtk_image_new_from_file(FLAG_PNG_PATH);
            g_object_ref_sink(flagimages[btni]);

            /* Number label */
            numbers[btni] = gtk_label_new("");
            gtk_widget_hide(numbers[btni]);
            gtk_grid_attach(GTK_GRID(buttongrid), numbers[btni], x, y, 1, 1);
        }
    }

    updateButtons(window);

    gtk_widget_show_all(window);
}

int
Gtk3Start(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    GtkApplication *app;
    int status;

    char *argv[2] = { TXT_TITLE, NULL };
    int argc = 1;

    app = gtk_application_new ("org.gtk." TXT_TITLE,
        G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
}

void
Gtk3Destroy() {

}
