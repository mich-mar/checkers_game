//
// Created by michal on 15.05.24.
//

#ifndef CHECKERSBOARD_H
#define CHECKERSBOARD_H
#include <vector>
#include <iostream>

/**
 * Enum representing the different types of pieces in the game.
 */
enum piece {
    CHECKER,
    KING,
    EMPTY
};

enum color {
    WHITE,
    BLACK,
    NONE
};

/**
 * Struct representing a position on the board.
 */
struct pos {
    int x;
    int y;

    bool operator==(const pos& other) const {
        return (x == other.x && y == other.y);
    }
};

/**
 * Struct representing a field on the board.
 */
struct field {
    piece pieceKind;
    color pieceColor;
};

/**
 * Class representing the checkers board and its operations.
 */
class checkersBoard {
protected:
    std::vector<std::vector<field>> board;

private:
    static char returnPieceChar(field tempField);

public:
    checkersBoard();

    ~checkersBoard() = default;

    void printBoard();
};


#endif //CHECKERSBOARD_H
