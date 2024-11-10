/* -*- mode: C; comment-column: 40; c-indent-comments-syntactically-p: t -*- */

/****************************************************************************/
/*                                                                          */
/* Copyright (c) 2024 Witold Paluszynski                                    */
/*                                                                          */
/* I grant everyone the right to copy this program in whole or part, and    */
/* to use it for any purpose, provided the source is properly acknowledged. */
/*                                                                          */
/* Udzielam kazdemu prawa do kopiowania tego programu w calosci lub czesci, */
/* i wykorzystania go w dowolnym celu, pod warunkiem zacytowania zrodla.    */
/*                                                                          */
/****************************************************************************/


/*
 * Program posredniczy w grze dwoch programow w warcaby angielskie.
 * Nazwy programow podane sa w argumentach wywolania: czarny bialy.
 * Jesli argument wywolania podany jest jako - to przyjmujemy
 *  polaczenie z programem wywolanym niezaleznie (moze to byc telnet).
 *
 * Compile on Solaris:
 *   cc -o broker -g -Xc checkers_broker.c -lsocket -lnsl
 * Compile on Linux:
 *   gcc -o broker -g -Wall -std=c17 -pedantic checkers_broker.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PROGRAM_VERSION 202406071136L
#define BUFSPACE 1024

//symbole dla graczy
#define WHITE 0
#define BLACK 1
#define OPPONENT(player) (1-player)
char *playername[]={"BIALY", "CZARNY"};
char *winnername[]={"WW", "BW", "DRAW"}; /* kod wyniku na stdout */

//symbole stanu pol warcabnicy, specjalne liczby, nie do pomylenia
#define BLACK_M 51
#define WHITE_M 52
#define BLACK_K 53
#define WHITE_K 54
#define EMPTY_F 55
#define INVALID 56
#define OUTSIDE 57
#define IS_PIECE(piece) ((piece>=BLACK_M)&&(piece<=WHITE_K))
#define IS_MAN(piece)   ((piece>=BLACK_M)&&(piece<=WHITE_M))
#define IS_KING(piece)  ((piece>=BLACK_K)&&(piece<=WHITE_K))
#define OWNER(piece)    ((piece-50)%2)
char *symbols[]={"!!!", " B ", " W ", "(B)", "(W)", "___", "   ", "***", "???"};
/* konwencje reprezentacji warcabnicy:
   warcabnica jest tablica 10x10 gdzie skrajne rzedy i kolumny nie graja
   wspolrzedne liczymy od lewego gornego rogu, pierwsza pionowa, druga pozioma
   obsadzone pola warcabnicy oznaczamy wartosciami 51..54 a nieobsadzone 55..57
   natomiast wartosci 1,2,3,...32 i ujemne uzywamy do oznaczania samych pol
*/
int hor[34]={-1,2,4,6,8,1,3,5,7,2,4,6,8,1,3,5,7,2,4,6,8,1,3,5,7,2,4,6,8,1,3,5,7,-1};
int ver[34]={-1,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,-1};
int board[10][10] =   { { 57, 57, 57, 57, 57, 57, 57, 57, 57, 57 },
			{ 57, 56, 51, 56, 51, 56, 51, 56, 51, 57 },
			{ 57, 51, 56, 51, 56, 51, 56, 51, 56, 57 },
			{ 57, 56, 51, 56, 51, 56, 51, 56, 51, 57 },
			{ 57, 55, 56, 55, 56, 55, 56, 55, 56, 57 },
			{ 57, 56, 55, 56, 55, 56, 55, 56, 55, 57 },
			{ 57, 52, 56, 52, 56, 52, 56, 52, 56, 57 },
			{ 57, 56, 52, 56, 52, 56, 52, 56, 52, 57 },
			{ 57, 52, 56, 52, 56, 52, 56, 52, 56, 57 },
			{ 57, 57, 57, 57, 57, 57, 57, 57, 57, 57 } };
/* to sa numery pol oficjalnie stosowane w warcabach */
/* uwaga: wartosci -1 i -2 sa twardo zakodowane w programie */
int fields[10][10] =  { { -2, -2, -2, -2, -2, -2, -2, -2, -2, -2 },
			{ -2, -1,  1, -1,  2, -1,  3, -1,  4, -2 },
			{ -2,  5, -1,  6, -1,  7, -1,  8, -1, -2 },
			{ -2, -1,  9, -1, 10, -1, 11, -1, 12, -2 },
			{ -2, 13, -1, 14, -1, 15, -1, 16, -1, -2 },
			{ -2, -1, 17, -1, 18, -1, 19, -1, 20, -2 },
			{ -2, 21, -1, 22, -1, 23, -1, 24, -1, -2 },
			{ -2, -1, 25, -1, 26, -1, 27, -1, 28, -2 },
			{ -2, 29, -1, 30, -1, 31, -1, 32, -1, -2 },
			{ -2, -2, -2, -2, -2, -2, -2, -2, -2, -2 } };

/* kody wynikowe ruchu, w wiekszosci wykorzystywane w funkcji make-move:
   ujemne - bledy programistyczne, sytuacje niedozwolone lub niemozliwe
   0 - poprawny ruch (zwykly lub bicie)
   100 - poprawny ruch + promocja piona
   1 - proba nielegalnego ruchu - pole from niepoprawne
   2 - proba nielegalnego ruchu - pole to niepoprawne
   3 - proba nielegalnego ruchu - brak figury gracza w polu from
   4 - proba nielegalnego ruchu - zajete pole to
   5 - proba nielegalnego ruchu zwyklego - blad geometrii ruchu
   6 - proba nielegalnego ruchu bicia - blad geometrii ruchu
   7 - proba nielegalnego ruchu bicia - brak figury przeciwnika pomiedzy from,to
   8 - proba nielegalnego ruchu zwyklego/bicia - pionem do tylu
   ---- warunki 1-8 sprawdz w funkcji make_move; pozostale w prog.glownym
   9 - proba nielegalnego ruchu zwyklego - gracz ma bicie
   10 - proba nielegalnego bicia - po promocji piona bicie wstecz
   11 - proba nielegalnego bicia - w koncowym polu mozliwe dalsze bicie
   12..19 - kody zarezerwowane na inne przypadki niedopuszczalnych ruchow

   kody wynikowe programu (status), plus sygnalizacja wyniku gry:
   ujemne - bledy programistyczne, sytuacje niedozwolone lub niemozliwe
          - interpretujemy to jako gre nierozstrzygnieta, jak remis
   0 - zwykly wynik gry, na stdout wyswietl. BW(black wins) albo WW(white wins)
       (zbicie wszystkich figur przeciwnika, lub przeciwnik nie ma legaln.ruchu)
   1-19 - przegrana przez probe wykonania nielegalnego ruchu
   20 - przegrana przez niewykonanie ruchu w zadanym czasie
   21 - przegrana przez probe ruchu poza kolejnoscia
   22 - przegrana przez zamkniecie polaczenia z brokerem
   40 - remis - 20 ruchow damkami bez zbicia
   41 - remis - przekroczony czas gry
*/
#define DRAW_LIMIT 20
struct timeval timeout = { 60, 0 };	/* domyslnie 10 sekund na odpowiedz */


int errno;


/* wyswietl stan planszy w standardowym ukladzie (czarne na gorze) */
void show_board_state();


/* sprawdz czy ruch/bicie gracza PLAYER z pola FROM do TO jest zgodny z zasadami
   gdy nie, to zwroc kod odpowiedniego naruszenia
   gdy tak, to wykonaj ruch i zwroc kod 0 lub 100 gdy zakoncz. promocja piona */
int make_move(int player, int from, int to, int CAPTURE);


/* sprawdz czy figura w polu FIELD ma mozliwosc bicia */
int can_capture(int field);


/* sprawdz czy figura w polu FIELD ma mozliwosc zwyklego ruchu */
int can_move(int field);


/* sprawdz czy gracz PLAYER nie zostal z 0 figurami */
int check_lostgame(int player);


int main(int argc, char *argv[])
{
  char buf[BUFSPACE], player_bl[BUFSPACE], player_wh[BUFSPACE];
  int black_serv_sock, black_cli_sock, white_serv_sock, white_cli_sock,
    sock, sock2, maxsock;
  int black_port_num, white_port_num;
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t addr_len;
  //struct hostent *serv_hostent;
  fd_set readsocks, testsocks;
  int i, j, pid1, pid2, msg_len, player, res, move_num, draw_counter;
  int move_from, move_to, move_to2, nchars, offset;

  fprintf(stderr,
	  "Program brokera rozgrywki w warcaby angielskie, wersja %ld.\n",
	  PROGRAM_VERSION);

  /* parsujemy argumenty - dwoch przeciwnikow do gry */
  if (argc!=3) {
    fprintf(stderr, "%s: blad wywolania - musza byc dwa argumenty (%d).\n",
	    argv[0], argc);
    exit(-1);
  }

  strcpy(player_bl, argv[1]);
  strcpy(player_wh, argv[2]);
  fprintf(stderr, "Player_Bl=\"%s\", Player_Wh=\"%s\"\n", player_bl, player_wh);

  /* sanity check - sprawdzamy spojnosc inicjalizacji danych */
  for (i=0; i<10; ++i) {
    for (j=0; j<10; ++j) {
      if (fields[i][j] == -2) {	/* tylko na obrzezu */
	if ((i!=0)&&(i!=9)&&(j!=0)&&(j!=9))
	  fprintf(stderr,
		  "Nieprawidlowa wartosc pola -2 nie na obrzezu [i,j]=%d,%d\n",
		  i, j);
      } else if (fields[i][j] == -1) {	/* tylko w bialych polach */
	if ((i%2)!=(j%2)) {
	  fprintf(stderr,
		  "Nieprawidlowa wartosc pola -1 nie na nieparzystym bialym polu [i,j]=%d,%d\n",
		  i, j);
	}
      } else {	/* zwykle pole robocze, sprawdz jego standardowy numer */
	if (ver[fields[i][j]] != i)
	  fprintf(stderr,
		  "Nie zgadza sie wspolrzedna pionowa i=%d, hor[%d]=%d !!\n",
		  i, fields[i][j], ver[fields[i][j]]);
	if (hor[fields[i][j]] != j)
	  fprintf(stderr,
		  "Nie zgadza sie wspolrzedna pozioma j=%d, hor[%d]=%d !!\n",
		  j, fields[i][j], hor[fields[i][j]]);
      }
    }
  }

  for (i=0; i<10; ++i) {
    //fprintf(stderr, "%03d", i);
    for (j=0; j<10; ++j) {
      sprintf(buf, "%2d", fields[i][j]);
      fprintf(stderr, "%3s", fields[i][j]==-2?"**":fields[i][j]==-1?"  ":buf);
      
    }
    fprintf(stderr, "\n");
  }

  /* standardowa deklaracja gniazdek oczekiwania na polaczenie */
  black_serv_sock = socket(AF_INET, SOCK_STREAM, 0);
  white_serv_sock = socket(AF_INET, SOCK_STREAM, 0);
  if ((black_serv_sock < 0)||(white_serv_sock < 0)) {
    perror("socket black/white");
    exit(-1);
  }

  /* przygotowanie struktury adresowej czarnych */
  bzero((char*)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(0);
  addr_len = sizeof(serv_addr);
  /* rejestracja gniazdka z adresem serwera */
  if (bind(black_serv_sock, (struct sockaddr *) &serv_addr, addr_len) < 0) {
    perror("bind1");
    exit(-1);
  }
  /* zczytujemy z powrotem adres z wypelnionym numerem portu */
  if (getsockname(black_serv_sock, (struct sockaddr *) &serv_addr, &addr_len)) {
    perror("getsockname black");
    exit(-1);
  }
  black_port_num = ntohs(serv_addr.sin_port);
  fprintf(stderr, "Numer portu czarnych: %d\n", black_port_num);
  /* bedziemy czekali na tylko jedno polaczenie */
  if (listen(black_serv_sock, 1) < 0) {
    perror("listen black");
    exit(-1);
  }
  /* tak samo dla bialych */
  bzero((char*)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(0);
  addr_len = sizeof(serv_addr);
  if (bind(white_serv_sock, (struct sockaddr *) &serv_addr, addr_len) < 0) {
    perror("bind2");
    exit(-1);
  }
  if (getsockname(white_serv_sock, (struct sockaddr *) &serv_addr, &addr_len)) {
    perror("getsockname white");
    exit(-1);
  }
  white_port_num = ntohs(serv_addr.sin_port);
  fprintf(stderr, "Numer portu bialych: %d\n", white_port_num);
  /* tu tez tylko jedno polaczenie */
  if (listen(white_serv_sock, 1) < 0) {
    perror("listen white");
    exit(-1);
  }

  /* tu nastapi uruchomienie programow grajacych jako podprocesow */
  if (strcmp(player_bl, "-") != 0) {
    if ((pid1=fork())==0) {
      close(black_serv_sock);
      close(white_serv_sock);
      sprintf(buf, "%d", black_port_num);
      execl(player_bl,player_bl,"NET","BLACK","8","1291","localhost",buf,NULL);
      perror("execl1");
      exit(-1);
    }
    fprintf(stderr, "Process czarnych uruchomiony pid=%d.\n", pid1);
  }
  if (strcmp(player_wh, "-") != 0) {
    if ((pid2=fork())==0) {
      close(black_serv_sock);
      close(white_serv_sock);
      sprintf(buf, "%d", white_port_num);
      execl(player_wh,player_wh,"NET","WHITE","8","15031","localhost",buf,NULL);
      perror("execl2");
      exit(-1);
    }
    fprintf(stderr, "Process bialych uruchomiony pid=%d.\n", pid2);
  }
	    
  fprintf(stderr, "Czekamy na polaczenie graczy.\n");
  int twoplayers = 0; /* liczba klientow */
  FD_ZERO(&readsocks);
  FD_SET(black_serv_sock, &readsocks);
  FD_SET(white_serv_sock, &readsocks);
  maxsock = MAX(black_serv_sock, white_serv_sock) + 1;
  while (twoplayers<2) {
    testsocks = readsocks;
    if ((res=select(maxsock, &testsocks, (fd_set *) 0, (fd_set *) 0,
		    (struct timeval *) 0)) < 0) { /* tu czekamy do skutku */
      perror("select serv_sock");
      exit(-1);
    }
    if (FD_ISSET(black_serv_sock, &testsocks)) {
      addr_len = sizeof(cli_addr);
      black_cli_sock = accept(black_serv_sock, (struct sockaddr *) &cli_addr, &addr_len);
      if (black_cli_sock == -1) {
	perror("accept black");
	exit(-1);
      }
      fprintf(stderr, "Czarny sie polaczyl.\n");
      FD_CLR(black_serv_sock, &readsocks);
      ++twoplayers;
    }
    if (FD_ISSET(white_serv_sock, &testsocks)) {
      addr_len = sizeof(cli_addr);
      white_cli_sock = accept(white_serv_sock, (struct sockaddr *) &cli_addr, &addr_len);
      if (white_cli_sock == -1) {
	perror("accept white");
	exit(-1);
      }
      fprintf(stderr, "Bialy sie polaczyl.\n");
      FD_CLR(white_serv_sock, &readsocks);
      ++twoplayers;
    }
  }
  
  fprintf(stderr, "Zaczynamy rozgrywke, pierwszy rusza czarny.\n");
  show_board_state();
  FD_ZERO(&readsocks);
  FD_SET(black_cli_sock, &readsocks);
  FD_SET(white_cli_sock, &readsocks);
  maxsock = MAX(black_cli_sock, white_cli_sock) + 1;
  move_num = 0;
  draw_counter = 0;
  while(1) {

    ++move_num;
    ++draw_counter;
    if (draw_counter > DRAW_LIMIT) {
      fprintf(stderr,"Przekroczony limit ruchow damkami bez postepu - remis.\n");
      printf("DRAW\n");
      exit(40);	/* przekroczony limit ruchu damkami bez postepu - remis */
    }

    /* to powinien byc ruch gracza move_num%2, czyli pierwszy ruch czarnego */
    player = move_num%2;

    testsocks = readsocks;
    struct timeval timeout2 = timeout;
    if ((res=select(maxsock, &testsocks, (fd_set *) 0, (fd_set *) 0,
		    (struct timeval *) &timeout2)) < 0) {
      perror("select cli_sock");
      exit(-1);
    }
    if (res>1) {
      fprintf(stderr, "Funkcja select zwrocila wartosc: %d (>1) - co robic??\n",
	      res);
    }
    if (res==0) { /* funkcja select wrocila przez timeout */
      fprintf(stderr, "Gracz %s nie wykonal ruchu na czas, przegral, koniec.\n",
	      playername[player]);
      printf("%s\n", winnername[OPPONENT(player)]);
      exit(20); /* przegrana przez przekroczony czas ruchu */
    }

    if (FD_ISSET(black_cli_sock, &testsocks))
      { sock = black_cli_sock;
	sock2 = white_cli_sock; }
    else
    if (FD_ISSET(white_cli_sock, &testsocks))
      { sock = white_cli_sock;
        sock2 = black_cli_sock; }
    else {
      fprintf(stderr, "Funkcja select wrocila, ale to nie zaden z klientow.\n");
      exit(-1);
    }

    /* ok, jeden z graczy przyslal ruch = sprawdz czy poprawny gracz */
    if (((sock==black_cli_sock)&&(player==WHITE)) ||
	((sock==white_cli_sock)&&(player==BLACK))) {
      /* czyli ten drugi gracz ruszyl sie poza kolejnoscia */
      fprintf(stderr, "%s wykonal ruch poza kolejnoscia - przegrywa.\n",
	      playername[OPPONENT(player)]);
      printf("%s\n", winnername[player]);
      exit(21); /* przegrana przez probe ruchu poza kolejnoscia */
    }

    /* poprawny gracz przyslal ruch - odczytaj */
    if ((msg_len=read(sock, buf, sizeof buf)) < 0) {
      perror("read move");
      exit(-1);
    }
    /* specjalna sytuacja, pusty komunikat = zamkniete polaczenie */
    if (msg_len==0) {
      fprintf(stderr, "Gracz %s zamknal polaczenie, przegral, koniec gry.\n",
	      playername[player]);
      printf("%s\n", winnername[OPPONENT(player)]);
      exit(22); /* przegrana przez zamkniecie polaczenia */
    }

    /* gracz przyslal jakis komunikat - dekoduj ruch */
    buf[msg_len] = 0;
    fprintf(stderr, "%6s: %s", playername[player], buf);
    if (buf[msg_len-1] != '\n') fprintf(stderr,"\n");

    /* czy to jest zwykly ruch */
    if (2==sscanf(buf, "%d-%d", &move_from, &move_to)) {
      fprintf(stderr, "        ruch z pola %d na %d\n", move_from, move_to);
      /* gracz probuje zrobic zwykly ruch - sprawdz czy nie ma bicia */
      for (i=1; i<=32; ++i) {
	int piece = board[ver[i]][hor[i]];
	if (IS_PIECE(piece)&&(player==OWNER(piece)))
	  if (1==can_capture(i)) {
	    fprintf(stderr,
		    "Nielegalny zwykly ruch gracza %s, mozliwe bicie w polu %d, koniec.\n",
		    playername[player], i);
	    printf("%s\n", winnername[OPPONENT(player)]);
	    exit(9); /* nielegalny zwykly ruch gdy mozliwe jest bicie */
	  }
      }
      /* gracz nie ma bicia, sprawdz i wykonaj zwykly ruch */
      if (((res=make_move(player, move_from, move_to, 0))%100)!=0) {
	fprintf(stderr, "Nielegalny zwykly ruch kod %d gracza %s, koniec.\n",
		res, playername[player]);
	printf("%s\n", winnername[OPPONENT(player)]);
	exit(res);
      }

      /* kazdy ruch zwyklego piona resetuje licznik ruchow do remisu */
      if (IS_MAN(board[ver[move_to]][hor[move_to]]))
	draw_counter = 0;
    } /* koniec zwyklego ruchu */

    /* jesli nie zwykly ruch to moze bicie */
    else if (2==sscanf(buf, "%dx%d%n", &move_from, &move_to, &nchars)) {
      fprintf(stderr, "        bicie z pola %d na %d\n", move_from, move_to);
      if (((res=make_move(player, move_from, move_to, 1))%100)!=0) {
	fprintf(stderr, "Nielegalne bicie kod %d gracza %s, koniec.\n",
		res, playername[player]);
	printf("%s\n", winnername[OPPONENT(player)]);
	exit(res);
      }
      offset=nchars;

      /* bicie moze byc kontynuowane - sprawdzamy */
      while (1==sscanf(buf+offset, "x%d%n", &move_to2, &nchars)) {
	fprintf(stderr, "        dalsze bicie z pola %d na %d\n",
		move_to, move_to2);
	/* ale nie jest to dopuszczalne po promocji piona */
	if (res==100) { /* kod zakonczenia poprzedniego ruchu */
	  fprintf(stderr,
		  "Nielegalna proba kontynuowania bicia po promocji piona, %s przegrywa.\n",
		  playername[player]);
	  printf("%s\n", winnername[OPPONENT(player)]);
	  exit(10); /* niedozwolona kontynuacja bicia po promocji piona */
	}
	if (((res=make_move(player, move_to, move_to2, 1))%100)!=0) {
	  fprintf(stderr,
		  "Nielegalne kontynuowane bicie kod %d gracza %s, koniec.\n",
		  res, playername[player]);
	  printf("%s\n", winnername[OPPONENT(player)]);
	  exit(res);
	}
	offset+=nchars;
	move_to = move_to2;
      }	/* bicie zakonczone */

      /* wykonanie bicia zakonczone - sprawdz czy nie moglo byc kontynuowane */
      if ((1==can_capture(move_to))&&(res!=100)) { /* gracz wtopil jesli sa...*/
	                                /*dalsze bicia,z wyj.gdy byla promocja*/
	fprintf(stderr,
		"Nielegalne bicie gracza %s, mozliwe dalsze bicie, koniec.\n",
		playername[player]);
	printf("%s\n", winnername[OPPONENT(player)]);
	exit(11); /* brak obowiazkowej kontynuacji bicia */
      }

      /* kazde wykonanie bicia resetuje licznik ruchow do remisu */
      draw_counter = 0;
    } /* koniec interpretacji bicia i calego ruchu */

    show_board_state();
    /* sprawdzamy czy gra sie nie zakonczyla przez zbicie wszystkich ... */
    if (1==check_lostgame(OPPONENT(player))) {
      fprintf(stderr, "Gracz %s ma 0 figur i przegral.\n",
	      playername[OPPONENT(player)]);
      printf("%s\n", winnername[player]);
      exit(0); /* najzwyklejszy koniec gry przez zbicie wszystkich */
    }
    /* ... lub przez brak mozliwosci ruchu przeciwnika */
    int is_stuck=1;
    for (i=1; i<=32; ++i) {
      int piece = board[ver[i]][hor[i]];
      if (IS_PIECE(piece)&&(OPPONENT(player)==OWNER(piece)))
	if ((1==can_move(i))||(1==can_capture(i))) { is_stuck = 0; break; }
    }
    if (1==is_stuck) {
      fprintf(stderr,
	      "Brak mozliwosci ruchu ani bicia gracza %s, koniec.\n",
	      playername[OPPONENT(player)]);
      printf("%s\n", winnername[player]);
      exit(0); /* regulaminowy koniec gry przez brak mozliwosci ruchu */
    }

    /* i to juz wszystko - szybciutko wysylamy ruch drugiemu graczowi */
    if (write(sock2, buf, msg_len) < 0) {
      perror("write move");
      exit(-1);
    }
  } /* while(1) */
} /* main */


/* wyswietl stan planszy w standardowym ukladzie (czarne na gorze) */
void show_board_state()
{
  for (int i=0; i<10; ++i) {
    //fprintf(stderr, "%03d", i);
    for (int j=0; j<10; ++j) {
      fprintf(stderr, "%3s", symbols[board[i][j]-50]);
    }
    fprintf(stderr, "\n");
  }
}


/* sprawdz czy ruch/bicie gracza PLAYER z pola FROM do TO jest zgodny z zasadami
   gdy nie, to zwroc kod odpowiedniego naruszenia
   gdy tak, to wykonaj ruch i zwroc kod 0 lub 100 gdy zakoncz. promocja piona */
int make_move(int player, int from, int to, int CAPTURE)
{
  int result, piece_mid, /* tylko przy biciu */
    piece_from=board[ver[from]][hor[from]],
    field_to=board[ver[to]][hor[to]];

  if ((from<1)||(from>32)) return 1;
  if ((to<1)||(to>32)) return 2;
  if (( ! IS_PIECE(piece_from)) || (OWNER(piece_from) != player)) return 3;
  if (field_to != EMPTY_F) return 4;
  if ((CAPTURE==0)&&((abs(ver[from]-ver[to])!=1)||(abs(hor[from]-hor[to])!=1)))
    return 5;
  if ((CAPTURE==1)&&((abs(ver[from]-ver[to])!=2)||(abs(hor[from]-hor[to])!=2)))
    return 6;
  if (CAPTURE==1) {
    piece_mid=board[(ver[from]+ver[to])/2][(hor[from]+hor[to])/2];
    if ((!IS_PIECE(piece_mid))||(OWNER(piece_mid)!=OPPONENT(player))) return 7;
  }
  if (IS_MAN(piece_from)) {
    if (((player==BLACK)&&((to-from)<3)) ||
	((player==WHITE)&&((from-to)<3))) /* ruch w zlym kierunku */
      return 8;
  }

  /* ok, ten ruch jest legalny, przynajmniej jako czesc dluzszej sekwencji
     jeszcze moze sie okazac nielegalna cala sekwencja, jesli bicie moze
     byc kontynuowane a nie jest
     ale tutaj tego nie sprawdzamy
     tu sprawdzamy i wykonujemy tylko ruch pojedynczy */
  result = 0;
  if (IS_MAN(piece_from) &&
      (((player==BLACK)&&(ver[to]==8)) ||
       ((player==WHITE)&&(ver[to]==1))))
    result += 100; /* promocja piona */

  /* teraz wykonujemy zadany ruch */
  board[ver[to]][hor[to]] = board[ver[from]][hor[from]];
  if (result==100) board[ver[to]][hor[to]] += 2; /* promocja piona do damki */
  board[ver[from]][hor[from]] = EMPTY_F;
  if (CAPTURE==1) {
    board[(ver[from]+ver[to])/2][(hor[from]+hor[to])/2] = EMPTY_F;
  }

  return result;
}


/* sprawdz czy figura w polu FIELD ma mozliwosc bicia */
int can_capture(int field)
{
  int vert=ver[field], horz=hor[field], piece=board[vert][horz], vert2, vert3;

  /* najpierw sprawdz bicia w normalnym kierunku, pionek lub damka */
  if (OWNER(piece)==BLACK) { /* sprawdz bicie czarnego w dol */
    vert2 = vert+1; vert3 = vert+2;
    if ((vert3<=8)&&((horz-2)>=1)) /* w ramach planszy w lewo */
      if ((board[vert3][horz-2]==EMPTY_F) &&
	  IS_PIECE(board[vert2][horz-1]) &&
	  (OWNER(board[vert2][horz-1])==WHITE)) return 1;
    if ((vert3<=8)&&((horz+2)<=8)) /* w ramach planszy w prawo */
      if ((board[vert3][horz+2]==EMPTY_F) &&
	  IS_PIECE(board[vert2][horz+1]) &&
	  (OWNER(board[vert2][horz+1])==WHITE)) return 1;
  }
  if (OWNER(piece)==WHITE) { /* sprawdz bicie bialego w gore */
    vert2 = vert-1; vert3 = vert-2;
    if ((vert3>=1)&&((horz-2)>=1)) /* w ramach planszy w lewo */
      if ((board[vert3][horz-2]==EMPTY_F) &&
	  IS_PIECE(board[vert2][horz-1]) &&
	  (OWNER(board[vert2][horz-1])==BLACK)) return 1;
    if ((vert3>=1)&&((horz+2)<=8)) /* w ramach planszy w prawo */
      if ((board[vert3][horz+2]==EMPTY_F) &&
	  IS_PIECE(board[vert2][horz+1]) &&
	  (OWNER(board[vert2][horz+1])==BLACK)) return 1;
  }
  if (IS_MAN(piece)) return 0; /* dla piona nie ma innych opcji bicia */

  /* teraz wiemy ze to damka i sprawdzamy bicia w kierunku przeciwnym */
  if (OWNER(piece)==WHITE) { /* sprawdz bicie bialego w dol */
    vert2 = vert+1; vert3 = vert+2;
    if ((vert3<=8)&&((horz-2)>=1)) /* w ramach planszy w lewo */
      if ((board[vert3][horz-2]==EMPTY_F) &&
	  IS_PIECE(board[vert2][horz-1]) &&
	  (OWNER(board[vert2][horz-1])==BLACK)) return 1;
    if ((vert3<=8)&&((horz+2)<=8)) /* w ramach planszy w prawo */
      if ((board[vert3][horz+2]==EMPTY_F) &&
	  IS_PIECE(board[vert2][horz+1]) &&
	  (OWNER(board[vert2][horz+1])==BLACK)) return 1;
  }
  if (OWNER(piece)==BLACK) { /* sprawdz bicie czarnego w gore */
    vert2 = vert-1; vert3 = vert-2;
    if ((vert3>=1)&&((horz-2)>=1)) /* w ramach planszy w lewo */
      if ((board[vert3][horz-2]==EMPTY_F) &&
	  IS_PIECE(board[vert2][horz-1]) &&
	  (OWNER(board[vert2][horz-1])==WHITE)) return 1;
    if ((vert3>=1)&&((horz+2)<=8)) /* w ramach planszy w prawo */
      if ((board[vert3][horz+2]==EMPTY_F) &&
	  IS_PIECE(board[vert2][horz+1]) &&
	  (OWNER(board[vert2][horz+1])==WHITE)) return 1;
  }

  return 0; /* domyslnie nie ma */
}


/* sprawdz czy figura w polu FIELD ma mozliwosc zwyklego ruchu */
int can_move(int field)
{
  int vert=ver[field], horz=hor[field], piece=board[vert][horz];

  /* to jest troche prostsze niz sprawdzanie bicia
     nie musze sprawdzac czy pole docelowe lezy w planszy
     korzystamy z tego, ze na obrzezu planszy tez sa pola i mozemy je testowac*/

  /* najpierw sprawdz ruchu w normalnym kierunku, pionek lub damka */
  if (OWNER(piece)==BLACK) { /* sprawdz ruch czarnego w dol */
    if (board[vert+1][horz-1]==EMPTY_F) return 1; /* w ramach planszy w lewo */
    if (board[vert+1][horz+1]==EMPTY_F) return 1; /* w ramach planszy w prawo */
  }
  if (OWNER(piece)==WHITE) { /* sprawdz ruch bialego w gore */
    if (board[vert-1][horz-1]==EMPTY_F) return 1; /* w ramach planszy w lewo */
    if (board[vert-1][horz+1]==EMPTY_F) return 1; /* w ramach planszy w prawo */
  }
  if (IS_MAN(piece)) return 0; /* dla piona nie ma innych opcji ruchu */

  /* teraz wiemy ze to damka i sprawdzamy ruchy w kierunku przeciwnym */
  if (OWNER(piece)==WHITE) { /* sprawdz ruch bialego w dol */
    if (board[vert+1][horz-1]==EMPTY_F) return 1; /* w ramach planszy w lewo */
    if (board[vert+1][horz+1]==EMPTY_F) return 1; /* w ramach planszy w prawo */
  }
  if (OWNER(piece)==BLACK) { /* sprawdz ruch czarnego w gore */
    if (board[vert-1][horz-1]==EMPTY_F) return 1; /* w ramach planszy w lewo */
    if (board[vert-1][horz+1]==EMPTY_F) return 1; /* w ramach planszy w prawo */
  }
  return 0; /* domyslnie nie ma */
}


/* sprawdz czy gracz PLAYER nie zostal z 0 figurami */
int check_lostgame(int player)
{
  int i, count=0, content;
  for (i=1; i<=32; ++i) {
    content=board[ver[i]][hor[i]];
    if (IS_PIECE(content)&&(OWNER(content)==player))
      ++count;
  }
  if (count==0) return 1; else return 0;
}
