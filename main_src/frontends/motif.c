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

    motif.c: Motif frontend

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>

#include <common/frontconf.h>

static const int *board = NULL;
static int size = 0;

static void
buttonCallback(Widget w, void *client_data, void *call_data) {

}


int
motifStart(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    char *argv[] = { TXT_TITLE, 0 };
    int argc = 1;

    XtAppContext app;
    Widget root, title, form, rowcol, buttons[size * size];
    Arg al[20];
    int ac = 0;

    XtSetLanguageProc(NULL, NULL, NULL);

    /* Create window */
    //root = XtVaAppInitialize(&app, TXT_TITLE, NULL, 0, &argc, argv, NULL, NULL);
    root = XtInitialize(argv[0], "", NULL, 0, &argc, argv);

    /* Window size */
    ac = 0;
    XtSetArg(al[ac], XmNwidth, 200); ac++;
    XtSetArg(al[ac], XmNheight, 200); ac++;
    XtSetValues(root, al, ac);

    /* Create title label */
    ac = 0;
    XtSetArg(al[ac], XmNlabelString,
        XmStringCreate(TXT_TITLE, XmSTRING_DEFAULT_CHARSET)); ac++;
    title = XtCreateManagedWidget("title", xmLabelWidgetClass, root, al, ac);

    /*rowcol = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, root,
        XmNnumColumns,  size,
        XmNpacking,     XmPACK_COLUMN,
        NULL);*/

    /* Create form widget */
    ac = 0;
    form = XtCreateManagedWidget("form", xmFormWidgetClass,
        root, al, ac);


    

    char butname[64];
    int btni = 0;
    XmString btntxt;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int cX = W_MARGIN + (x * (CELL_SIZE + CELL_MARGIN));
            int cY = HEADER_HEIGHT + (y * (CELL_SIZE + CELL_MARGIN));
            btni = (y * size) + x;
            snprintf(butname, 64, "button%d", btni);
            //btntxt = XmStringCreateLocalized(butname);
            /*buttons[buti] = XtVaCreateManagedWidget(butname,
                xmPushButtonWidgetClass, rowcol, 
                XmNlabelString, btntxt, NULL);*/
            //printf("Created %s\n", butname);
            /* Create button with pos and size */
            ac = 0;
            XtSetArg(al[ac],XmNlabelString, XmStringCreate("-",
                        XmSTRING_DEFAULT_CHARSET)); ac++;
            XtSetArg(al[ac], XmNleftAttachment,   XmATTACH_POSITION); ac++;
            XtSetArg(al[ac], XmNleftPosition,     cX); ac++;
            XtSetArg(al[ac], XmNrightAttachment,  XmATTACH_POSITION); ac++;
            XtSetArg(al[ac], XmNrightPosition,    cX + CELL_SIZE); ac++;
            XtSetArg(al[ac], XmNtopAttachment,    XmATTACH_POSITION); ac++;
            XtSetArg(al[ac], XmNtopPosition,      cY); ac++;
            XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
            XtSetArg(al[ac], XmNbottomPosition,   cY + CELL_SIZE); ac++;
            buttons[btni] = XtCreateManagedWidget("label", 
                xmPushButtonWidgetClass, form, al, ac);
            XtAddCallback(buttons[btni], XmNactivateCallback,
                buttonCallback, (void*)(long)btni);
        }
    }

    //XtManageChild(rowcol);
    XtRealizeWidget(root);
    //XtAppMainLoop(app);
    XtMainLoop();
}

void
motifDestroy() {
    
}

