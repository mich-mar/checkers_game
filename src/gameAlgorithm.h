//
// Created by michal on 23.05.24.
//

#ifndef GAMEALGORITHM_H
#define GAMEALGORITHM_H
#include "gameHandler.h"


enum interface { NET, GUI };


class gameAlgorithm : public gameHandler{
private:
    interface variant;
    color playerColor;
    int depth;
    std::string ip_address;
    std::string ip_port;
    int roundsCtr;

    int calculateBoard();

    int minMAxAlgo(gameAlgorithm curGame, int depth, int alpha, int beta, bool maximizingPlayer);

    int vulnerablePenalty(int row, int col, field piece);

    int heurystyka();

public:

    void getBestMove();

    gameAlgorithm(interface variant, color playerColor, int depth, std::string ip_address,
                      std::string ip_port);

    gameAlgorithm(int argc, char *argv[]);

    ~gameAlgorithm() = default;

    gameAlgorithm &operator=(const gameAlgorithm &other) = default;

    void play() override;
};


#endif //GAMEALGORITHM_H
