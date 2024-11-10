#include <iostream>
#include "checkersBoard.h"
#include "gameAlgorithm.h"
#include "gameHandler.h"

// include z cielnta
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// ===================== odpalanie gry przez interfejs NET =============================
// g++ -std=c++17 main.cpp checkersBoard.cpp gameHandler.cpp gameAlgorithm.cpp -o client
// gcc -o broker -g -Wall -std=c17 -pedantic checkers_broker.c
// ./broker client client

// ===================== odpalanie gry przez interfejs GUI =============================
// g++ -std=c++17 main.cpp checkersBoard.cpp gameHandler.cpp gameAlgorithm.cpp -o gra
// ./gra GUI WHITE 20

// define i errno z clienta
#define BUFSPACE 1024
#define BLACK 1
#define WHITE 0
int errno;

int clientNet(int argc, char *argv[]) {
    gameAlgorithm gra(argc, argv);
    char buf[BUFSPACE];
    int player, serv_sock, n, nr_ruchu;
    struct sockaddr_in serv_addr;
    struct hostent *serv_hostent;

    /* argv[2] okresla ktora strona ma grac program */
    player = -1;
    if (strcasecmp(argv[2], "BLACK") == 0) player = BLACK;
    if (strcasecmp(argv[2], "WHITE") == 0) player = WHITE;
    if (player < 0) {
        fprintf(stderr, "%s: niepoprawne okreslenie gracza w argv[2]: %s\n",
                argv[0], argv[2]);
        exit(-1);
    }

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock < 0) {
        perror("socket");
        exit(errno);
    }
    serv_hostent = gethostbyname(argv[5]);
    if (serv_hostent == 0) {
        fprintf(stderr, "%s: nieznany adres IP %s\n", argv[0], argv[5]);
        exit(-1);
    }
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr, serv_hostent->h_addr, serv_hostent->h_length);
    serv_addr.sin_port = htons(atoi(argv[6]));

    printf("Laczymy sie z serwerem ...\n");
    if (connect(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(-1);
    }

    printf("Polaczenie nawiazane, zaczynamy gre ...\n");

    nr_ruchu = 0; /* czarny robi ruchy parzyste */

    if (player == WHITE)
        nr_ruchu = 0; /* bialy nieparzyste */

    while (1) {
        int index_ruchu = nr_ruchu * 2 + player - 1;

        if ((nr_ruchu > 0) || (player == BLACK)) {
            // ================= ZMIANA - przekazywanie ruchu ==============
            gra.getBestMove();
            std::string moves = gra.getPlayersMoves();
            gra.handleNextMoves();

            char *tempMoves = new char[moves.size() + 1];
            std::strcpy(tempMoves, moves.c_str());
            // ================= ZMIANA - przekazywanie ruchu ==============

            // printf("Wysylam do serwera moj ruch: %s\n", tempMoves); - usunięcie komentarzy

            if (write(serv_sock, tempMoves, strlen(tempMoves)) < 0) {
                // === ZMIANA ===
                perror("write");
                exit(errno);
            }
        }
        // printf("Czekam na ruch przeciwnika ...\n"); - usunięcie komentarzy
        n = read(serv_sock, buf, sizeof buf);
        if (n < 0) {
            perror("read");
            exit(errno);
        }
        if (n == 0) {
            /* pusty komunikat = zamkniete polaczenie */
            // printf("Broker zamknal polaczenie, hmmmm...\n"); - usunięcie komentarzy
            exit(0);
        }
        buf[n] = 0;
        // printf("Otrzymalem ruch przeciwnika: %s", buf); - usunięcie komentarzy

        // ================= ZMIANA - odbieranie ruchu ==============
        if (buf[n - 1] != '\n') {
            gra.readOponentsMoves(buf);
            gra.handleNextMoves();
            printf("\n");
        }
        // ================= ZMIANA - odbieranie ruchu ==============

        ++nr_ruchu;
    }
}

int main(int argc, char *argv[]) {
    interface variant;

    // arg 1 - interference
    std::string arg1 = argv[1];
    if (arg1 == "NET") {
        variant = NET;
    } else if (arg1 == "GUI") {
        variant = GUI;
    } else {
        return 0;
    }

    if (variant == NET) {
        clientNet(argc, argv);
    } else {
        gameAlgorithm gra(argc, argv);
        gra.play();
    }

    return 0;
}
