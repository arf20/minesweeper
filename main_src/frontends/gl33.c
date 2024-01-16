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

    gl33.c: OpenGL 3.3/GLFW/GLAD frontend

*/

#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "glutil.h"

#include <common/frontconf.h>
#include "gl33.h"
#include <common/game.h>

#define C_WHITE  1.0f, 1.0f, 1.0f
#define C_BLACK  0.0f, 0.0f, 0.0f
#define C_YELLOW 1.0f, 1.0f, 0.0f
#define C_RED    1.0f, 0.0f, 0.0f
#define C_GREEN  0.0f, 1.0f, 0.0f
#define C_BLUE   0.0f, 0.0f, 1.0f
#define C_DBLUE  0.0f, 0.0f, 0.54f
#define C_DRED   0.54f, 0.0f, 0.0f
#define C_DCYAN  0.0f, 0.54f, 0.54f
#define C_DGREY  0.66f, 0.66f, 0.66f

static const int *board = NULL;
static int size = 0;

static int wWidth, wHeight;
static GLFWwindow *g_window = NULL;
static GLint boardShader, dummyvao, boardUBO;
static unsigned int *boardUBOdata = NULL;

static void
render(GLFWwindow *window) {
    glClearColor(C_BLACK, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Execute shader */
    glUseProgram(boardShader);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void
resizeCallback(GLFWwindow *window, int w, int h) {
    
}

static void
keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

}

void
updateBoardUBO() {
    glBindBuffer(GL_UNIFORM_BUFFER, boardUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(int)*size*size, board, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, boardUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, boardUBO);
}

static void
mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    //if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    if (action != GLFW_RELEASE) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    int ix = ((int)x - W_MARGIN) /
        (CELL_SIZE + CELL_MARGIN);
    int iy = ((int)y - HEADER_HEIGHT) /
        (CELL_SIZE + CELL_MARGIN);
    if (ix < 0 || ix >= size || iy < 0 || iy >= size) return;

    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: {
            gameClearCell(ix, iy);
            updateBoardUBO();
        } break;
        case GLFW_MOUSE_BUTTON_RIGHT: {
            gameFlagCell(ix, iy);
            updateBoardUBO();
        }
    }

}

int
GL33Start(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    wWidth = (2 * W_MARGIN) + (size * CELL_SIZE) + ((size - 1) * CELL_MARGIN);
    wHeight = HEADER_HEIGHT + W_MARGIN + (size * CELL_SIZE) +
        ((size - 1) * CELL_MARGIN);

    /* Init glfw context */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    /* Create window */
    if ((g_window = glfwCreateWindow(wWidth, wHeight, TXT_TITLE, NULL, NULL))
        == NULL)
    {
        printf("Error creating window\n");
        return -1;
    }
    glfwMakeContextCurrent(g_window);

    /* Init glad */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Error initializing GLAD\n");
        return -1;
    }

    /* Set callbacks */
    glfwSetFramebufferSizeCallback(g_window, resizeCallback);
    glfwSetKeyCallback(g_window, keyCallback);
    glfwSetMouseButtonCallback(g_window, mouseCallback);
    glfwSwapInterval(0);

    /* enable stuff */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    /* Compile shaders */
    boardShader = shader_new(
        "../assets/msboard.vs",
        NULL,
        "../assets/msboard.fs");
    /* Set shader uniforms */
    glUseProgram(boardShader);
    shader_set_float2(boardShader, "sSize", wWidth, wHeight);
    shader_set_uint(boardShader, "size", size);

    /* Generate and bind dummy VAO, required in most OpenGL implementations */
    glGenVertexArrays(1, &dummyvao);
    glBindVertexArray(dummyvao);

    /* Generate board UBO */
    glGenBuffers(1, &boardUBO);
    //boardUBO_idx = glGetUniformBlockIndex(boardShader, "BoardBlock");
    glBindBuffer(GL_UNIFORM_BUFFER, boardUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(int)*size*size, board, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, boardUBO);

    /* Enter the infinite event-processing loop */
    while (!glfwWindowShouldClose(g_window)) {
        render(g_window);
        glfwSwapBuffers(g_window);
        glfwPollEvents();
    }
}

void
GL33Destroy() {
    glfwDestroyWindow(g_window);
    glfwTerminate();
}

