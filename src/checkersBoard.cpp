//
// Created by michal on 15.05.24.
//

#include "checkersBoard.h"

/**
 * Function sets the choosed color and font
 */
void setColor(int color, bool bold = false) {
    if (bold) {
        std::cout << "\033[1;" << color << "m";
    } else {
        std::cout << "\033[" << color << "m";
    }
}

/**
 * Function sets the default color
 */
void resetColor() {
    std::cout << "\033[0m";
}


/**
 * Returns the character representation of a piece on the board.
 *
 * @param temp_field The field containing the piece.
 * @return The character representing the piece.
 */
char checkersBoard::returnPieceChar(field tempField) {
    if (tempField.pieceColor == WHITE) {
        setColor(31, true);
    }

    if (tempField.pieceColor == BLACK) {
        setColor(34, true);
    }

    switch (tempField.pieceKind) {
        case CHECKER:
            if (tempField.pieceColor == WHITE)
                return 'w';
            else if (tempField.pieceColor == BLACK)
                return 'b';

        case KING:
            if (tempField.pieceColor == WHITE)
                return 'W';
            else if (tempField.pieceColor == BLACK)
                return 'B';

        case EMPTY:
            return ' ';
    }

    return ' ';
}

/**
 * Constructor for the checkersBoard class. Initializes the board with pieces in their starting positions.
 */
checkersBoard::checkersBoard() {
    // Initialize the board with empty fields
    board.resize(8, std::vector<field>(8, {EMPTY, NONE}));

    // Place black checkers on the board
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if ((x + y) % 2 == 1 && y < 3) {
                board[x][y] = {CHECKER, BLACK};
            }
        }
    }

    // Place white checkers on the board
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if ((x + y) % 2 == 1 && y > 4) {
                board[x][y] = {CHECKER, WHITE};
            }
        }
    }
}

/**
 * Prints the current state of the board to the console.
 */
void checkersBoard::printBoard() {
    int ctr = 1;

    for (int y = 0; y < 8; y++) {
        std::cout << " -------------------------------------------------" << std::endl;
        std::cout << " |";

        for (int x = 0; x < 8; x++) {
            if ((x + y) % 2 == 1) {
                if (ctr < 10)
                    std::cout << " " << ctr;
                else
                    std::cout << ctr;

                ctr++;
            } else
                std::cout << "  ";

            resetColor();
            std::cout << " " << returnPieceChar(board[x][y]);
            resetColor();
            std::cout << " |";
        }

        std::cout << std::endl;
    }
    std::cout << " -------------------------------------------------" << std::endl;
}
