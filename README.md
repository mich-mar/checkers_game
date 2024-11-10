# Checkers Game - README

## Overview

This project is a Checkers game that can be played either through a **network interface (NET)** or a **Graphical User
Interface (GUI)**. The game features AI decision-making powered by the **Minimax algorithm** for optimal moves. Players
can compete either locally (with a GUI) or over a network. The game logic also includes basic checkers rules, move
validation, and alternating turns.

## Project Structure

- **main.cpp**: The main entry point of the program, which decides whether to start the game over the network or with a
  GUI based on user input.
- **checkersBoard.cpp / checkersBoard.h**: Defines and implements the checkers board, handling the game state, pieces,
  and their movement.
- **gameHandler.cpp / gameHandler.h**: Manages game flow, including player turns and validating moves.
- **gameAlgorithm.cpp / gameAlgorithm.h**: Implements the game’s logic, including move generation, AI decision-making,
  and evaluating the board state using the **Minimax algorithm**.
- **checkers_broker.c**: A simple broker (server) that facilitates communication between the client and the opponent. It
  was authored by **Dr. Witold Paluszyński**, your course instructor. It handles establishing connections, receiving
  moves, and sending moves between players.

## Key Features

- **Network Interface**: The game can be played over a network, where one player is the client, and the other is the
  server. Players take turns sending and receiving moves via TCP sockets.
- **Graphical User Interface (GUI)**: The game can also be played locally using a GUI interface, providing a more
  interactive experience for users who prefer a visual interface.
- **Minimax Algorithm**: The AI opponent uses the **Minimax algorithm** for decision-making. It analyzes the game board
  to generate the best possible moves, simulating all possible future scenarios and evaluating each move based on the
  game state.
- **Game Logic**: The game includes basic checkers rules, including move generation, validation, and alternating turns.
  AI decision-making (if applicable) is handled in the `gameAlgorithm` class using the **Minimax algorithm** to
  calculate the best move for the AI player.
- **Error Handling**: The game features basic error handling for network communication, ensuring the game runs smoothly
  even when there are connection issues or invalid input.

## Compilation Instructions

### Step 1: Compile the Client

1. Open the terminal and navigate to the root directory of the project.
2. Compile the client using the following command, which includes the `-std=c++17` flag for compatibility with newer C++
   features:

   ```bash
   g++ -std=c++17 main.cpp checkersBoard.cpp gameHandler.cpp gameAlgorithm.cpp -o client
   ```

### Step 2: Compile the Broker (for Network Play)

1. In the root directory, compile the broker with the following command:

   ```bash
   gcc -o broker -g -Wall -std=c17 -pedantic checkers_broker.c
   ```

2. The broker facilitates the communication between the client and the server in the network mode. **Note**: The broker
   was authored by **Dr. Witold Paluszyński**, our course instructor.

## Running the Game

### 1. Connecting through Network (NET Mode)

To play the game over a network, follow these steps:

1. Compile both the **client** and the **broker** as described in the previous sections.
2. Start the broker by running the following command, which sets up the server:

   ```bash
   ./broker client client
   ```

3. On the client-side, run the client in **NET mode** by providing the necessary arguments, including the side (BLACK or
   WHITE), server IP address, and port:

   ```bash
   ./client NET WHITE 20 <server_ip> <port>
   ```

    - `NET`: Specifies that the game should run over the network.
    - `WHITE`: Choose the player color (can be either WHITE or BLACK).
    - `20`: The depth of the **Minimax algorithm** for evaluating moves. A higher value increases the accuracy of AI
      moves but requires more processing power.
    - `<server_ip>`: The IP address of the server where the broker is running.
    - `<port>`: The port number used for the connection.

### 2. Connecting through Graphical User Interface (GUI Mode)

To play the game using a graphical interface:

1. First, make sure you have compiled the **client** as described in Step 1.
2. Run the client in **GUI mode** with the following command:

   ```bash
   ./client GUI WHITE 8
   ```

    - `GUI`: Specifies that the game should run with a graphical user interface.
    - `WHITE`: Choose the player color (can be either WHITE or BLACK).
    - `8`: The depth of the **Minimax algorithm** for evaluating moves in the GUI mode. As with `20`, a higher value
      provides a more accurate AI but requires more computation time.

## Minimax Algorithm

The **Minimax algorithm** is implemented in the `gameAlgorithm.cpp` file to enable the AI to make optimal moves. This
algorithm evaluates the game tree by simulating all possible moves and selecting the one that maximizes the AI's chances
of winning while minimizing the opponent's chances. It operates recursively, considering future board states and
assigning scores based on the potential outcomes.

### Key Features of the Minimax Algorithm:

- **Decision Making**: The algorithm evaluates all possible moves by generating future board states and comparing them
  using a scoring function.
- **Optimal Play**: By exploring all potential outcomes, the algorithm ensures the AI always makes the best possible
  move in any given situation.
- **Depth Limitation**: The depth of the Minimax algorithm is adjustable, with higher depths providing better move
  accuracy but requiring more processing power. In this project, the depth values can be set using the arguments, with
  higher numbers resulting in more in-depth analysis by the AI.
