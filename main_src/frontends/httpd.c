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

    httpd.c: HTTP webapp frontend

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <WinSock2.h>
#define close closesocket
#else
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#endif

#include <pthread.h>

#include <common/frontconf.h>
#include "httpd.h"
#include <common/game.h>

#define BUFF_SIZE 65535

static const int* board = NULL;
static int size = 0;


static const char* indexHeaders =
"HTTP/1.1 200 OK\n"
"Server: arfminesweeper httpd\n"
"Content-Type: text/html\n";

static const char* pngHeaders =
"HTTP/1.1 200 OK\n"
"Server: arfminesweeper httpd\n"
"Content-Type: image/png\n";

static const char* err404Headers =
"HTTP/1.1 404 Not Found\n"
"Server: arfminesweeper httpd\n";

static const char* alertContent =
"%s\n<!DOCTYPE html><html><head><script>alert(%s);</script></head>"
"<body></body></html>";

static char* htmlContent = NULL;
static char* pngFlagContent = NULL;
static size_t pngFlagSize = 0;

static char* sendBuffer = NULL;



size_t strlcat(char* restrict dst, const char* restrict src, size_t dstsize) {
    int d_len, s_len, offset, src_index;

    /* obtain initial sizes */
    d_len = strlen(dst);
    s_len = strlen(src);

    /* get the end of dst */
    offset = d_len;

    /* append src */
    src_index = 0;
    while (*(src + src_index) != '\0')
    {
        *(dst + offset) = *(src + src_index);
        offset++;
        src_index++;
        /* don't copy more than dstsize characters
           minus one */
        if (offset == dstsize - 1)
            break;
    }
    /* always cap the string! */
    *(dst + offset) = '\0';

    return d_len + s_len;
}

void
cpynum(const char* restrict src, char* restrict dst, size_t dstsize) {
    const char* restrict ptr = src;
    while (*ptr && (*ptr >= 48 && *ptr <= 57) && (ptr - src < dstsize)) {
        *dst = *ptr;
        ptr++;
        dst++;
    }
    *dst = '\0';
}

int
strlencrlf(const char* str) {
    const char* ptr = str;
    int size = 0;
    while (*ptr != '\0') {
        if (*ptr == '\n') size++;
        size++;
        ptr++;
    }
    return size;
}

static void
convertcrlf(char* buff, size_t size) {
    size_t bufflen = strlen(buff) + 1;
    char* ptr = buff;

    while (*ptr) {
        char* p = strchr(ptr, '\n');
        /* walked past last occurrence of needle or run out of buffer */
        if (p == NULL || bufflen + 1 >= size || p >= buff + size) {
            break;
        }

        /* move part after lf */
        memmove(p + 1, p, bufflen - (p - buff));

        /* insert cr */
        *p = '\r';

        bufflen++;
        ptr = p + 2;
    }
}

static void
convertlfescaped(char* buff, size_t size) {
    size_t bufflen = strlen(buff) + 1;
    char* ptr = buff;

    while (*ptr) {
        char* p = strchr(ptr, '\n');
        /* walked past last occurrence of needle or run out of buffer */
        if (p == NULL || bufflen + 1 >= size || p >= buff + size) {
            break;
        }

        /* move part after lf */
        memmove(p + 1, p, bufflen - (p - buff));

        /* insert cr */
        p[0] = '\\';
        p[1] = 'n';

        bufflen += 2;
        ptr = p + 2;
    }
}

static void
printToSpace(const char* str) {
    while (*str != ' ') {
        putc(*str, stdout);
        str++;
    }
}

static void
generateBoardResponse() {
    char tmpBuff[BUFF_SIZE];
    char tmpBuff2[1024];
    tmpBuff[0] = '\0';

    for (int y = 0; y < size; y++) {
        strlcat(tmpBuff, "<tr>\n", BUFF_SIZE);
        for (int x = 0; x < size; x++) {
            int btni = (size * y) + x;
            /* If clear, count surrounding cells and print n of mines */
            if (CHECK_CLEAR(BOARDXY(x, y))) {
                strlcat(tmpBuff, "<td>\n", BUFF_SIZE);
                int n = gameGetSurroundingMines(x, y);
                const char* color = NULL;
                if (n) {
                    switch (n) {
                    case 1: color = "blue"; break;
                    case 2: color = "green"; break;
                    case 3: color = "red"; break;
                    case 4: color = "darkblue"; break;
                    case 5: color = "darkred"; break;
                    case 6: color = "darkcyan"; break;
                    case 7: color = "black"; break;
                    case 8: color = "darkgrey"; break;
                    }
                    snprintf(tmpBuff2, 1024,
                        "<span style=\"color: %s;\">%d</span>\n",
                        color, n);
                    strlcat(tmpBuff, tmpBuff2, BUFF_SIZE);
                }
                strlcat(tmpBuff2, "</td>\n", BUFF_SIZE);
            }
            /* If not clear, check flag and draw it */
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                snprintf(tmpBuff2, 1024, "<td><button name=\"btn\" class=\"cell\">"
                    "<img id=\"%d\" src=\"/flag.png\"></button></td>\n", btni);
                strlcat(tmpBuff, tmpBuff2, BUFF_SIZE);
            }
            /* Otherwise just a tile */
            else {
                snprintf(tmpBuff2, 1024, "<td><a href=\"?clear=%d\" name=\"btn\">"
                    "<button id=\"%d\" class=\"cell\"></button></a></td>\n",
                    btni, btni);
                strlcat(tmpBuff, tmpBuff2, BUFF_SIZE);
            }
        }
        strlcat(tmpBuff, "</tr>\n", BUFF_SIZE);
    }

    /* Check game state*/
    char alert[1024];
    alert[0] = '\0';
    switch (gameGetState()) {
    case STATE_LOST: {
        strncpy(alert, "alert(\""TXT_LOST"\");", 1024);
        convertlfescaped(alert, 1024);
    } break;
    case STATE_WON: {
        strncpy(alert, "alert(\""TXT_WON"\");", 1024);
        convertlfescaped(alert, 1024);
    } break;
    }

    snprintf(sendBuffer, BUFF_SIZE, htmlContent, indexHeaders,
        gameGetFlagsLeft(), tmpBuff, alert);

    convertcrlf(sendBuffer, BUFF_SIZE);
}

static size_t
generateFlagPNGResponse() {
    snprintf(sendBuffer, BUFF_SIZE, "%s\n", pngHeaders);
    size_t size = strlen(sendBuffer);
    memcpy(sendBuffer + size, pngFlagContent, pngFlagSize);
    return size + pngFlagSize;
}

static void
generate404Response() {
    snprintf(sendBuffer, BUFF_SIZE, "%s", err404Headers);
    convertcrlf(sendBuffer, BUFF_SIZE);
}

static void*
clientThread(void* data) {
    int cfd = *(int*)data;

    char recvBuff[BUFF_SIZE];
    char tmpBuff[256];

    printf("Accepted connection\n");

    /* Recv loop loop */
    while (1) {
        int r = recv(cfd, recvBuff, BUFF_SIZE, 0);
        if (r < 0) {
            printf("Error receiving \n");
            return NULL;
        }
        else if (r == 0) {
            printf("Client exited \n");
            return NULL;
        }

        /*fwrite(recvBuff, 1, r, stdout);*/

        if (strncmp(recvBuff, "GET", 3) == 0) {
            if (strncmp(recvBuff + 4, "/ ", 2) == 0) {
                /* Index */
                generateBoardResponse();
                send(cfd, sendBuffer, strlen(sendBuffer), 0);
                printf("GET / 200 OK\n");
            }
            else if (strncmp(recvBuff + 4, "/flag.png ", 10) == 0) {
                /* Flag PNG file */
                size_t size = generateFlagPNGResponse();
                send(cfd, sendBuffer, size, 0);
                printf("GET /flag.png 200 OK\n");
            }
            else if (strncmp(recvBuff + 4, "/?", 2) == 0) {
                /* Parameter */
                if (strncmp(recvBuff + 6, "clear=", 6) == 0) {
                    cpynum(recvBuff + 12, tmpBuff, 256);
                    int btni = atoi(tmpBuff);
                    gameClearCell(btni % size, btni / size);
                }
                else if (strncmp(recvBuff + 6, "flag=", 5) == 0) {
                    cpynum(recvBuff + 11, tmpBuff, 256);
                    int btni = atoi(tmpBuff);
                    gameFlagCell(btni % size, btni / size);
                }

                generateBoardResponse();
                send(cfd, sendBuffer, strlen(sendBuffer), 0);
                printf("GET ");
                printToSpace(recvBuff + 4);
                printf(" 200 OK\n");
            }
            else {
                /* 404 */
                generate404Response();
                send(cfd, sendBuffer, strlen(sendBuffer), 0);
                printf("GET ");
                printToSpace(recvBuff + 4);
                printf(" 404 Not Found\n");
            }
        }
        else {
            printf("Warning: Only GET supported\n");
            return NULL;
        }

        close(cfd);
        printf("Connection closed\n");
        return NULL;
    }
}

static int
httpdListen(unsigned short port) {
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = 0;

    int fd = -1;
    /* Create socket */
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        return -1;
    }

    int v = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&v, sizeof(int)) < 0) {
        return -1;
    }

    /* Bind socket */
    if (bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        printf("Error binding socket\n");
        return -1;
    }

    /* Listen on socket */
    if (listen(fd, SOMAXCONN) < 0) {
        return -1;
    }

    return fd;
}

static int
loadFile(const char* path, char** buff, size_t* size) {
    FILE* f = fopen(path, "r");
    if (f == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return -1;
    }
    fseek(f, 0L, SEEK_END);
    size_t s = ftell(f);
    fseek(f, 0L, SEEK_SET);
    *buff = malloc(s);
    fread(*buff, 1, s, f);

    if (size) *size = s;

    return 0;
}

int
httpdStart(const int* lboard, int lsize) {
    board = lboard;
    size = lsize;

    /* Load assets */
    loadFile("../assets/msboard.html", &htmlContent, NULL);
    loadFile("../assets/flag.png", &pngFlagContent, &pngFlagSize);

    sendBuffer = malloc(65536);

    int lfd = httpdListen(8080);
    if (lfd < 0)
        return -1;

    printf("Listening on 0.0.0.0:8080\n");

    pthread_t cthread;
    int cfd = -1;

    /* Accept loop */
    while (1) {
        cfd = accept(lfd, NULL, NULL);
        if (cfd < 0) {
            printf("Error accepting client\n");
            continue;
        }

        pthread_create(&cthread, NULL, clientThread, &cfd);
        pthread_detach(cthread);
    }

    return 0;
}

void
httpdDestroy() {

}
