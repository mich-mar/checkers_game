//
// Created by michal on 15.05.24.
//

#ifndef GAMEHANDLER_H
#define GAMEHANDLER_H
#include "checkersBoard.h"
#include <sstream>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>

/*
 * Enum representing the current state of the game.
 */
enum gameState {
    BLACK_TURN, // Black's turn
    WHITE_TURN, // White's turn
    DRAW, // Game is a draw
    B_WIN, // Black wins
    W_WIN // White wins
};

/*
 * Enum representing the possible orientations between two positions.
 */
enum orientation {
    UP_R, // Up-Right
    UP_L, // Up-Left
    DOWN_R, // Down-Right
    DOWN_L, // Down-Left
    WRONG // Invalid orientation
};

/*
 * Class handling the game logic, inheriting from checkersBoard.
 */
class gameHandler : public checkersBoard {
protected:
    gameState curentGameState;
    std::vector<pos> currentMoves;
    std::vector<pos> tempCurrentMoves;

    std::vector<pos> getPlayerFields();

    std::vector<pos> getEmptyFields();

    std::vector<std::vector<pos>> generateMovesList();

private:
    bool debug = false;
    int maxKingMoves = 10;
    int whiteKingMoves;
    int blackKingMoves;
    int roundsCtr;

    static std::vector<int> readInput(std::string &input);

    static orientation calculateOrientation(pos pos_1, pos pos_2);

    field getPieceBetween(pos pos_1, pos pos_2);

    static pos getFieldPosBetween(pos pos_1, pos pos_2);

    static float getDistanceBetween(pos tempPos_1, pos tempPos_2);

    void promotion();

    bool canTake(pos piecePos, std::vector<pos> &fieldsToJump);

    bool goodOrientation(pos piecePos1, pos piecePos2, piece pieceKind);

    bool didAllTakes();

    void getTakingOptions(gameHandler game, gameState current_player, std::vector<pos> previous_positions,
                                   std::vector<std::vector<pos> > &legal_takes, bool has_took);

    void handleAnyMoves();

protected:
    std::vector<pos> notationToPos(const std::vector<int> &moves);

    static pos notationToPos(int field);

    std::string posToNotation (pos position);

    void isGameFinished();

    void randomMoves();

    bool isLeagalMoves();

    void virtual play();

    gameHandler();

    virtual ~gameHandler() = default;

    gameHandler &operator=(const gameHandler &other) = default;

    void clearCurrentMoves();

    void setCurrentMoves(std::vector<pos> newCurrentMoves);

public:
    void readOponentsMoves(char *ruchy);

    char * getPlayersMoves();

    void handleNextMoves();

    void askNextMove();
};


#endif //GAMEHANDLER_H
