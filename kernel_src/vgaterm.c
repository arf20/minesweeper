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

    vgaterm.c: VGA terminal driver

*/

#include "vgaterm.h"

#include "port.h"

/* VGA registers */
#define VGA_CTRL_REGISTER   0x3d4
#define VGA_DATA_REGISTER   0x3d5
#define VGA_OFFSET_LOW      0x0f
#define VGA_OFFSET_HIGH     0x0e

/* VGA buffer */
#define VGA_ADDRESS 0xb8000

unsigned char *buff = (unsigned char*)VGA_ADDRESS;

void memcpy(const char *src, char *dst, int n) {
    for (int i = 0; i < n; i++)
        *(dst + i) = *(src + i);
}

/* ====== Computation operations ====== */
int
vga_offset_row(int off) {
    return off / (2 * TXTMODE_COLS);
}

int
vga_row_col_offset(int col, int row) {
    return 2 * (row * TXTMODE_COLS + col);
}

/* ====== Register operations ====== */
/* credit: https://www.reddit.com/r/osdev/comments/70fcig/blinking_text/ */
void
vga_enable_blink() {
    port_byte_in(0x3da);    /* address mode */
    port_byte_out(0x3c0, 0x30); /* set address 0x30 */
    unsigned char am = port_byte_in(0x3c1); /* read AMCR */
    am |= 0x80; /* blink enable bit */
    port_byte_out(0x4c0, am);
}

void
vga_set_cursor_off(int off) {
    off /= 2;
    port_byte_out(VGA_CTRL_REGISTER, VGA_OFFSET_HIGH);
    port_byte_out(VGA_DATA_REGISTER, (unsigned char)(off >> 8));
    port_byte_out(VGA_CTRL_REGISTER, VGA_OFFSET_LOW);
    port_byte_out(VGA_DATA_REGISTER, (unsigned char)(off & 0xff));
}

int
vga_get_cursor_off() {
    int off;
    port_byte_out(VGA_CTRL_REGISTER, VGA_OFFSET_HIGH);
    off = port_byte_in(VGA_DATA_REGISTER) << 8;
    port_byte_out(VGA_CTRL_REGISTER, VGA_OFFSET_LOW);
    off += port_byte_in(VGA_DATA_REGISTER);
    return off * 2;
}

/* ====== Buffer operations ====== */
void
vga_set_char_c(char c, int off, unsigned char color) {
    buff[off] = c;
    buff[off + 1] = color;
}

void
vga_set_char(char c, int off) {
    vga_set_char_c(c, off, WHITE_ON_BLACK);
}

void
vga_clear() {
    /* Clear buffer */
    int i;
    for (i = 0; i < TXTMODE_COLS * TXTMODE_ROWS; i++)
        vga_set_char(' ', i * 2);
    /* Reset cursor to top left */
    vga_set_cursor_off(0);
}

int
vga_scroll_line(int off) {
    /* Move buffer */
    memcpy(
        (char *)(vga_row_col_offset(0, 1) + VGA_ADDRESS),
        (char *)(vga_row_col_offset(0, 0) + VGA_ADDRESS),
        TXTMODE_COLS * (TXTMODE_ROWS - 1) * 2
    );

    /* Clear bottom line */
    for (int col = 0; col < TXTMODE_COLS; col++)
        vga_set_char(' ', vga_row_col_offset(col, TXTMODE_ROWS - 1));
    
    return off - 2 * TXTMODE_COLS;
}

void
vga_print_char_c(char c, int off, unsigned char color) {
    /* if off < 0, place at cursor, else use given offset */
    if (off < 0)
        off = vga_get_cursor_off();
    
    /* handle control characters */
    if (c == '\n') off = vga_row_col_offset(0, vga_offset_row(off) + 1);
    else if (c == '\b') off -= 2;
    else {
        vga_set_char_c(c, off, color);
        off += 2;
    }

    /* if after printing, cursor is ouside the screen, do scroll */
    if (off >= TXTMODE_ROWS * TXTMODE_COLS * 2)
        off = vga_scroll_line(off);
    if (off < 0) off = 0;

    vga_set_cursor_off(off);
}

void
vga_print_char(char c, int off) {
    vga_print_char_c(c, off, WHITE_ON_BLACK);
}

void
vga_print_string_c(const char *str, int off, unsigned char color) {
    if (off < 0)
        off = vga_get_cursor_off();
    else vga_set_cursor_off(off);
    int i = 0;
    while (str[i] != '\0') {
        vga_print_char_c(str[i], -1, color);
        i++;
    }
}

void
vga_print_string(const char *str, int off) {
    vga_print_string_c(str, off, WHITE_ON_BLACK);
}

void
vga_set_mode(unsigned char mode) {
    /* default 3 */
    
}

void
vga_init() {
    vga_enable_blink();
    vga_clear();
}
