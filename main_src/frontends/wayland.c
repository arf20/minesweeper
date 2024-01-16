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

    wayland.c: Wayland frontend

*/

#include "wayland.h"

#include <wayland-client.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const int *board = NULL;
static int size = 0;

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_shell *shell = NULL;
struct wl_shell_surface *shell_surface;

static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{
    printf("Got a registry event for %s id %d\n", interface, id);
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry, 
				      id, 
				      &wl_compositor_interface, 
				      1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, 1);
    }
}

static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id) {
    printf("Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

int
WaylandStart(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    display = wl_display_connect(NULL);
    if (display == NULL) {
        printf("Cannot connect to display\n");
        exit(1);
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (compositor == NULL) {
	    printf("Can't find compositor\n");
	    exit(1);
    } else {
	    printf("Found compositor\n");
    }

    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
	    printf("Can't create surface\n");
	    exit(1);
    } else {
	    printf("Created surface\n");
    }

    if (shell == NULL) {
	    printf("Haven't got a Wayland shell\n");
	    exit(1);
    }
    shell_surface = wl_shell_get_shell_surface(shell, surface);
    if (shell_surface == NULL) {
	    printf("Can't create shell surface\n");
	    exit(1);
    } else {
	    printf("Created shell surface\n");
    }
    wl_shell_surface_set_toplevel(shell_surface);
}

void
WaylandDestroy() {
    wl_display_disconnect(display);
}
