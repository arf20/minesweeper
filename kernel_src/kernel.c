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

    kernel.c: kernel entry point

*/

#include "vgaterm.h"
#include "textdefs.h"

void
kmain() {
    vga_init();
    vga_write_string(TXT_HELLO, -1); 
    vga_write_string(TXT_MENU, -1);

    for (int i = 0; i < 22; i++)
        vga_write_string("asdf\n", -1);

}
