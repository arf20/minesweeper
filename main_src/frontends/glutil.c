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

    gl33.c: OpenGL 3.3/GLFW/GLEW frontend

*/

#include "glutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

GLint
shader_new(const char *vsp, const char *gsp, const char *fsp) {
    /* 1. read vertex/fragment source */
    char *vsc, *gsc, *fsc;
    FILE *vsf, *gsf, *fsf;
    size_t vss, gss, fss;
    GLint vs, gs, fs, pid;
    int success;
    char infoLog[1024];


    if (vsp) {
        /* Read vertex shader source */
        vsf = fopen(vsp, "r");
        fseek(vsf, 0L, SEEK_END);
        vss = ftell(vsf);
        vsc = malloc(vss + 1);
        fseek(vsf, 0L, SEEK_SET);
        fread(vsc, 1, vss, vsf);
        vsc[vss] = 0;

        /* Compile vertex shader */
        vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, (const char* const*)&vsc, NULL);
        glCompileShader(vs);
        
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vs, 1024, NULL, infoLog);
            printf("Error compiling vertex shader: %s\n", infoLog);
            return 0;
        }
    }

    if (gsp) {
        /* Read geometry shader source */
        gsf = fopen(gsp, "r");
        fseek(gsf, 0L, SEEK_END);
        gss = ftell(gsf);
        gsc = malloc(gss + 1);
        fseek(gsf, 0L, SEEK_SET);
        fread(gsc, 1, gss, gsf);
        gsc[gss] = 0;

        /* Compile geometry shader */
        gs = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gs, 1, (const char* const*)&gsc, NULL);
        glCompileShader(gs);
        
        glGetShaderiv(gs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(gs, 1024, NULL, infoLog);
            printf("Error compiling geometry shader: %s\n", infoLog);
            return 0;
        }
    }

    /* Read geometry shader source */
    fsf = fopen(fsp, "r");
    fseek(fsf, 0L, SEEK_END);
    fss = ftell(fsf);
    fsc = malloc(fss + 1);
    fseek(fsf, 0L, SEEK_SET);
    fread(fsc, 1, fss, fsf);
    fsc[fss] = 0;

    /* Compile fragment shader */
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const char* const*)&fsc, NULL);
    glCompileShader(fs);
    
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 1024, NULL, infoLog);
        printf("Error compiling fragment shader: %s\n", infoLog);
        return 0;
    }
    
    /* Shader Program */
    pid = glCreateProgram();
    if (vsp)
        glAttachShader(pid, vs);
    if (gsp)
        glAttachShader(pid, gs);
    glAttachShader(pid, fs);
    glLinkProgram(pid);

    glGetProgramiv(pid, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(pid, 1024, NULL, infoLog);
        printf("Error linking program: %s\n", infoLog);
        return 0;
    }
    
    if (vsp) {
        glDeleteShader(vs);
        free(vsc);
        fclose(vsf);
    }
    
    if (gsp) {
        glDeleteShader(gs);
        free(gsc);
        fclose(gsf);
    }
    
    glDeleteShader(fs);
    free(fsc);
    fclose(fsf);

    return pid;
}

void
shader_set_int(GLint pid, const char *name, int v) {
    glUniform1i(glGetUniformLocation(pid, name), v);
}

void
shader_set_float(GLint pid, const char *name, float v) {
    glUniform1f(glGetUniformLocation(pid, name), v);
}

void
shader_set_float2(GLint pid, const char *name, float v0, float v1) {
    glUniform2f(glGetUniformLocation(pid, name), v0, v1);
}

void
shader_set_float3(GLint pid, const char *name, float v0, float v1, float v2) {
    glUniform3f(glGetUniformLocation(pid, name), v0, v1, v2); 
}

void
shader_set_uintv(GLint pid, const char *name, int size, const unsigned int *v) {
    glUniform1uiv(glGetUniformLocation(pid, name), size, v);
}

void
shader_set_uint(GLint pid, const char *name, unsigned int v) {
    glUniform1ui(glGetUniformLocation(pid, name), v);
}

