//
// Created by michal on 23.05.24.
//

#include "gameAlgorithm.h"

#include <cfloat>
#include <climits>
#include <utility>

/**
 * Constructor for the gameAlgorithm class.
 *
 * @param depth The maximum depth for the minimax algorithm.
 *  @param variant Game variant GUI or NET.
 *  @param playerColor Which player begins.
 *  @param depth Depth of serching in min-max algorithm.
 *  @param ip_address IP adress for playing using NET.
 *  @param ip_port IP port for playing using NET.
 */
gameAlgorithm::gameAlgorithm(interface variant, color playerColor, int depth, std::string ip_address,
                             std::string ip_port) {
    this->variant = variant;
    this->playerColor = playerColor;
    this->depth = depth;
    this->ip_address = std::move(ip_address);
    this->ip_port = std::move(ip_port);
    roundsCtr = 0;
}

gameAlgorithm::gameAlgorithm(int argc, char *argv[]) {
    roundsCtr = 0;

    // arg 1 - interference
    std::string arg1 = argv[1];
    if (arg1 == "NET") {
        variant = NET;
    } else if (arg1 == "GUI") {
        variant = GUI;
    } else {
    }

    // arg 2 - player color
    std::string arg2 = argv[2];
    if (arg2 == "WHITE" || arg2 == "white") {
        playerColor = WHITE;
    } else if (arg2 == "BLACK" || arg2 == "black") {
        playerColor = BLACK;
    } else {
    }

    // arg 3 - min_max depth
    try {
        depth = std::stoi(argv[3]);
    } catch (const std::invalid_argument &e) {
    }
}

/**
 * Struct helpful in getBestMove() function.
 */
struct evalBoards {
    int index;
    int score;
};

/**
 * Finds the best move for the current game state.
 *
 * @param game The current game state.
 */
void gameAlgorithm::getBestMove() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::vector<std::vector<pos> > legalMoves;
    legalMoves = generateMovesList();
    int val = 0;
    int minValue = INT_MAX, maxValue = -INT_MAX;

    if (legalMoves.empty() && curentGameState == WHITE_TURN) {
        curentGameState = B_WIN;
        return;
    }
    if (legalMoves.empty() && curentGameState == BLACK_TURN) {
        curentGameState = W_WIN;
        return;
    }

    bool isMaxPlayer = true;

    if (curentGameState == BLACK_TURN)
        isMaxPlayer = false;
    if (curentGameState == WHITE_TURN)
        isMaxPlayer = true;

    // alpha = -inf (worst case for max player white)
    // white is max player
    // beta = inf (worst case for min player black)
    // black is min player

    std::vector<evalBoards> valuesVector;

    for (int i = 0; i < legalMoves.size(); i++) {
        currentMoves = legalMoves[i];

        val = minMAxAlgo(*this, 0, -INT_MAX, INT_MAX, isMaxPlayer);

        valuesVector.push_back({i, val});

        minValue = std::min(minValue, val);
        maxValue = std::max(maxValue, val);
    }

    std::vector<int> maxIndexVector;
    std::vector<int> minIndexVector;
    for (int i = 0; i < legalMoves.size(); i++) {
        if (valuesVector[i].score == minValue)
            minIndexVector.push_back(i);

        if (valuesVector[i].score == maxValue)
            maxIndexVector.push_back(i);
    }

    // dodać losowanie ruchu dla tych samych wyników
    int randomIndex;
    if (isMaxPlayer == false) {
        randomIndex = std::rand() % minIndexVector.size();
        currentMoves = legalMoves[minIndexVector[randomIndex]];
    }
    if (isMaxPlayer == true) {
        randomIndex = std::rand() % maxIndexVector.size();
        currentMoves = legalMoves[maxIndexVector[randomIndex]];
    }
}

/**
 * Calculates the score of the board for the current game state.
 *
 * @param board The game board.
 * @return The score of the board.
 */
int gameAlgorithm::calculateBoard() {
    isGameFinished();

    if (curentGameState == W_WIN)
        return INT_MAX;

    if (curentGameState == B_WIN)
        return -INT_MAX;

    if (curentGameState == DRAW)
        return 0;

    return heurystyka();
}


/**
 * Calculates the penalty for making move beneficial for oponent.
 *
 * @return The value of penalty.
 */
int gameAlgorithm::vulnerablePenalty(int row, int col, field piece) {
    int penalty = 0;
    int takePenalty = 2;

    if (piece.pieceColor == BLACK) {
        if (row - 1 >= 0 && col - 1 >= 0 && board[col - 1][row - 1].pieceKind == EMPTY)
            if (row + 1 <= 7 && col + 1 <= 7 && board[col + 1][row + 1].pieceColor == WHITE)
                penalty += takePenalty; // dodaje karę dla pionka postawionego przy przeciwniku i z wolnym polem za sobą

        if (row - 1 >= 0 && col + 1 <= 7 && board[col + 1][row - 1].pieceKind == EMPTY)
            if (row + 1 <= 7 && col - 1 >= 0 && board[col - 1][row + 1].pieceColor == WHITE)
                penalty += takePenalty; // dodaje karę dla pionka postawionego przy przeciwniku i z wolnym polem za sobą
    } else if (piece.pieceColor == WHITE) {
        if (row + 1 <= 7 && col - 1 >= 0 && board[col - 1][row + 1].pieceKind == EMPTY)
            if (row - 1 >= 0 && col + 1 <= 7 && board[col + 1][row - 1].pieceColor == BLACK)
                penalty += takePenalty; // dodaje karę dla pionka postawionego przy przeciwniku i z wolnym polem za sobą

        if (row + 1 <= 7 && col + 1 <= 7 && board[col + 1][row + 1].pieceKind == EMPTY)
            if (row - 1 >= 0 && col - 1 >= 0 && board[col - 1][row - 1].pieceColor == BLACK)
                penalty += takePenalty; // dodaje karę dla pionka postawionego przy przeciwniku i z wolnym polem za sobą
    }

    return penalty;
}


/**
 * Calculates the score of the board for the current game state.
 *
 * @return The score of the board.
 */
int gameAlgorithm::heurystyka() {
    const int w1 = 1;
    const int w2 = 2;

    int valueWhiteCheckers = 0;
    int valueBlackCheckers = 0;
    int valueWhiteKings = 0;
    int valueBlackKings = 0;

    // pętla for licząca wartości dla każde figury na planszy w zależnośći od jej położenia
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            field pieceField = board[col][row];

            if (pieceField.pieceKind == CHECKER && pieceField.pieceColor == WHITE) {
                // dodanie punktu za pionka
                valueWhiteCheckers++;

                // dodanie punktu za stanie przy ścianie (bezpieczniej)
                if (col == 0 || col == 7)
                    valueWhiteCheckers += 2;

                // dodanie punktu na stanie na polu do promocji lub przy tym polu
                if (row == 0 || row == 1)
                    valueWhiteCheckers += 2;

                // odjęcie punktów za ustawienie pozwalające na skucie
                valueWhiteKings -= vulnerablePenalty(row, col, pieceField);
            }
            if (pieceField.pieceKind == KING && pieceField.pieceColor == WHITE) {
                // dodanie punktu za króla
                valueWhiteKings++;

                // dodanie punktu za stanie przy ścianie (bezpieczniej)
                if (col == 0 || col == 7)
                    valueWhiteKings += 2;

                // odjęcie punktów za ustawienie pozwalające na skucie
                valueWhiteKings -= vulnerablePenalty(row, col, pieceField);
            }
            if (pieceField.pieceKind == CHECKER && pieceField.pieceColor == BLACK) {
                // dodanie punktu za pionka
                valueBlackCheckers++;

                // dodanie punktu za stanie przy ścianie (bezpieczniej)
                if (col == 0 || col == 7)
                    valueBlackCheckers += 2;

                // dodanie punktu na stanie na polu do promocji lub przy tym polu
                if (row == 7 || row == 6)
                    valueBlackCheckers += 2;

                // odjęcie punktów za ustawienie pozwalające na skucie
                valueWhiteKings -= vulnerablePenalty(row, col, pieceField);
            }
            if (pieceField.pieceKind == KING && pieceField.pieceColor == BLACK) {
                // dodanie punktu za króla
                valueBlackKings++;

                // dodanie punktu za stanie przy ścianie (bezpieczniej)
                if (col == 0 || col == 7)
                    valueBlackKings += 2;


                // odjęcie punktów za ustawienie pozwalające na skucie
                valueWhiteKings -= vulnerablePenalty(row, col, pieceField);
            }
        }
    }

    // suma wartości białych minus wartości czarnych, pomnożone przez odpowiednie współczynniki
    int wartosc = (w1 * (valueWhiteCheckers - valueBlackCheckers) +
                   w2 * (valueWhiteKings - valueBlackKings));

    return wartosc;
}

/**
 * Minimax algorithm implementation with alpha-beta pruning and an additional maximizingPlayer flag.
 *
 * @param curGame The current game state.
 * @param depth The current depth of the search.
 * @param alpha The alpha value for alpha-beta pruning.
 * @param beta The beta value for alpha-beta pruning.
 * @param maximizingPlayer True if the current player is the maximizing player.
 * @return The evaluation score of the board.
 */
int gameAlgorithm::minMAxAlgo(gameAlgorithm curGame, int depth, int alpha, int beta, bool maximizingPlayer) {
    // sprawdzanie dla osiągniętej głębokosci
    if (depth >= this->depth) {
        return curGame.calculateBoard();
    }

    // zwracanie evaluacji planszy dla końca gry
    curGame.isGameFinished();
    if (curGame.curentGameState == W_WIN || curGame.curentGameState == B_WIN || curGame.curentGameState == DRAW)
        return curGame.calculateBoard();

    // generowanie listy ruchów
    std::vector<std::vector<pos> > moves = generateMovesList();

    // sprawdzanie czy były jakieś legalne ruchy, jeśli nie to zwraca evaluacje planszy
    if (moves.empty())
        return curGame.calculateBoard();

    if (maximizingPlayer == true) {
        // ustawienie maxEval na najgorszy przypadek
        int maxEval = -INT_MAX;

        for (int i = 0; i < moves.size(); i++) {
            gameAlgorithm newGame = curGame;

            // generowanie i plansz dla możliwych ruchów
            newGame.setCurrentMoves(moves[i]);

            // wykonywanie ruchów
            newGame.handleNextMoves();

            // wywołanie algorytmu dla każdej kolejnej planszy
            int eval = minMAxAlgo(newGame, depth + 1, alpha, beta, false);

            // alpha cięcie
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);

            // break dla ostrego cięcia
            if (beta < alpha)
                break;
        }

        return maxEval;
    } else {
        int minEval = INT_MAX;

        for (int i = 0; i < moves.size(); i++) {
            gameAlgorithm newGame = curGame;

            // generowanie i plansz dla możliwych ruchów
            newGame.setCurrentMoves(moves[i]);

            // wykonywanie ruchów
            newGame.handleNextMoves();

            // wywołanie algorytmu dla każdej kolejnej planszy
            int eval = minMAxAlgo(newGame, depth + 1, alpha, beta, true);

            // beta cięcie
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);

            // break dla ostrego cięcia
            if (beta < alpha)
                break;
        }
        return minEval;
    }
}

/**
 * Plays the game by alternating turns until the game ends.
 */
void gameAlgorithm::play() {
    std::cout << "== Początkowa plansza ==" << std::endl;
    printBoard();
    std::cout << std::endl;

    while (curentGameState == BLACK_TURN || curentGameState == WHITE_TURN) {
        if (curentGameState == BLACK_TURN && playerColor == BLACK)
            askNextMove();

        else if (curentGameState == WHITE_TURN && playerColor == WHITE)
            askNextMove();

        else if (curentGameState == BLACK_TURN && playerColor == WHITE) {
            std::cout << " === Czarne: min-max ===" << std::endl;
            getBestMove();
        } else if (curentGameState == WHITE_TURN && playerColor == BLACK) {
            std::cout << " === Białe: min-max ===" << std::endl;
            getBestMove();
        }

        handleNextMoves();
        printBoard();
        std::cout << std::endl;
        isGameFinished();

        roundsCtr++;
    }

    std::cout << " ================ Końcowa plansza ================" << std::endl;
    printBoard();

    if (curentGameState == B_WIN)
        std::cout << " > KONIEC GRY - czarne wygrywają " << std::endl << std::endl;
    else if (curentGameState == W_WIN)
        std::cout << " > KONIEC GRY - białe wygrywają " << std::endl << std::endl;
    else
        std::cout << " > KONIEC GRY - remis" << std::endl << std::endl;
}
