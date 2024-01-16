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

    qt5.cpp: Qt5 frontend

*/

#include <common/frontconf.h>
#include "qt5.hpp"

extern "C" {
#include <common/game.h>
}

#include <QtWidgets>
#include <QMainWindow>

#include <iostream>
#include <functional>

class Cell : public QPushButton {
    Q_OBJECT

public:
    Cell(int lx, int ly);

signals:
    void updateSignal();

private:
    int x, y;

    void mouseReleaseEvent(QMouseEvent *e);
};

class Minesweeper : public QWidget {
    Q_OBJECT

public:
    explicit Minesweeper(QWidget *parent, const int *lboard, int lsize);
    ~Minesweeper();

public slots:
    void update();

private:
    const int *board = nullptr;
    int size = 0;

    QLabel *titlelabel;
    QLabel *flagslabel;

    QIcon flagicon;

    Cell **buttons;
    QLabel **numbers;
};

#include "qt5.moc"  // Cursed

Cell::Cell(int lx, int ly) {
    x = lx; y = ly;
}

void Cell::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton)
        gameClearCell(x, y);
    else if (e->button() == Qt::RightButton)
        gameFlagCell(x, y);

    emit updateSignal();
}

Minesweeper::Minesweeper(QWidget *parent, const int *lboard, int lsize) : QWidget(parent) {
    board = lboard;
    size = lsize;

    // Create labels
    titlelabel = new QLabel(tr(TXT_TITLE));
    flagslabel = new QLabel(tr(""));

    // Create button grid
    QGridLayout *buttonGridLayout = new QGridLayout;
    buttonGridLayout->setHorizontalSpacing(CELL_MARGIN);
    buttonGridLayout->setVerticalSpacing(CELL_MARGIN);
    buttons = new Cell*[size*size];
    numbers = new QLabel*[size*size];

    QPixmap flagpixmap(FLAG_PNG_PATH);
    flagicon = QIcon(flagpixmap);

    // Add buttons to grid
    int btni = 0;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Variables stuff
            btni = (size * y) + x;

            buttons[btni] = new Cell(x, y);
            buttons[btni]->setFixedSize(QSize(CELL_SIZE, CELL_SIZE));
            numbers[btni] = new QLabel();

            numbers[btni]->hide();

            buttonGridLayout->addWidget(buttons[btni], y, x);
            buttonGridLayout->addWidget(numbers[btni], y, x);
            buttonGridLayout->setAlignment(numbers[btni], Qt::AlignCenter);

            connect(buttons[btni], &Cell::updateSignal, this, &Minesweeper::update);
        }
    }

    // Create main layout and add labels
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(titlelabel, 0, 0);
    mainLayout->addWidget(flagslabel, 1, 0);
    mainLayout->setAlignment(flagslabel, Qt::AlignRight);
    mainLayout->addLayout(buttonGridLayout, 2, 0);

    setLayout(mainLayout);
    setWindowTitle(tr(TXT_TITLE));

    update();
}

void Minesweeper::update() {
    flagslabel->setText(QString(std::to_string(gameGetFlagsLeft()).c_str()));

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int btni = (y * size) + x;

            if (CHECK_CLEAR(BOARDXY(x, y))) {
                buttons[btni]->hide();

                int n = gameGetSurroundingMines(x, y);
                if (n) {
                    numbers[btni]->setText(QString(std::to_string(n).c_str()));

                    switch (n) {
                        case 1: numbers[btni]->setStyleSheet("color: blue"); break;
                        case 2: numbers[btni]->setStyleSheet("color: green"); break;
                        case 3: numbers[btni]->setStyleSheet("color: red"); break;
                        case 4: numbers[btni]->setStyleSheet("color: darkcyan"); break;
                        case 5: numbers[btni]->setStyleSheet("color: darkred"); break;
                        case 6: numbers[btni]->setStyleSheet("color: cyan"); break;
                        case 7: numbers[btni]->setStyleSheet("color: black"); break;
                        case 8: numbers[btni]->setStyleSheet("color: grey"); break;
                    }

                    numbers[btni]->show();
                }
            }
            else if (CHECK_FLAG(BOARDXY(x, y))) {
                // Set flag
                buttons[btni]->setIcon(flagicon);
            }
            else {
                // Clear flag if applicable
                buttons[btni]->setIcon(QIcon());
            }
        }
    }

    // Check state
    switch (gameGetState()) {
        case STATE_LOST: {
            QMessageBox m;
            m.setText(TXT_LOST);
            m.exec();
            close();
        } break;
        case STATE_WON: {
            QMessageBox m;
            m.setText(TXT_WON);
            m.exec();
            close();
        } break;
    }
}

Minesweeper::~Minesweeper() {
    delete titlelabel;
    delete flagslabel;
    for (int i = 0; i < size*size; i++)
        delete buttons[i];
    delete buttons;
}


int
Qt5Start(const int *lboard, int size) {
    const char *argv[2] = { TXT_TITLE, NULL };
    int argc = 1;

    QApplication app(argc, (char**)argv);

    Minesweeper m(nullptr, lboard, size);
    m.show();

    return app.exec();
}

void
Qt5Destroy() {

}
