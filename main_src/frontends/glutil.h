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

*/

#ifndef _GLUTIL_H
#define _GLUTIL_H

#include "glad/include/glad/glad.h"

GLint shader_new(const char *vsp, const char *gsp, const char *fsp);

void shader_set_int(GLint pid, const char *name, int v);
void shader_set_float(GLint pid, const char *name, float v);
void shader_set_float2(GLint pid, const char *name, float v0, float v1);
void shader_set_float3(GLint pid, const char *name, float v0, float v1, float v2);
void shader_set_uintv(GLint pid, const char *name, int size, const unsigned int *v);
void shader_set_uint(GLint pid, const char *name, unsigned int v);


#endif
