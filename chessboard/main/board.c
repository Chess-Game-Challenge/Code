#include <esp_log.h>
#include "led_strip.h"
#include <display.h>
#include <esp_timer.h>

extern bool DEBUG;

#define B_ROOK    1
#define B_KNIGHT  2
#define B_BISHOP1 3
#define B_QUEEN   4
#define B_KING    5
#define B_BISHOP2 6
#define B_PAWN    7
#define W_ROOK    8
#define W_KNIGHT  9
#define W_BISHOP1 10
#define W_QUEEN   11
#define W_KING    12
#define W_BISHOP2 13
#define W_PAWN    14
#define P_EMPTY   0

bool isMoving = false;
bool isAtacking = false;
bool showMoves = false;

typedef struct {
    int type;   // Tipo da peça (ex: B_PAWN, W_KING, etc.)
    int row;    // Posição da peça na matriz (linha)
    int col;    // Posição da peça na matriz (coluna)
} ChessPiece;

ChessPiece piece1 = {P_EMPTY, 0, 0};  // Um cavalo branco na posição (0, 1)
ChessPiece piece2 = {P_EMPTY, 0, 0};    // Um peão preto na posição (6, 1)

// Example board initialization

int board[8][8] = {
    {B_ROOK,   B_KNIGHT, B_BISHOP1, B_QUEEN,  B_KING,   B_BISHOP2, B_KNIGHT,  B_ROOK},
    {B_PAWN,   B_PAWN,    B_PAWN,    B_PAWN,   B_PAWN,   B_PAWN,    B_PAWN,    B_PAWN},
    {P_EMPTY,  P_EMPTY,   P_EMPTY,   P_EMPTY,  P_EMPTY,  P_EMPTY,   P_EMPTY,   P_EMPTY},
    {P_EMPTY,  P_EMPTY,   P_EMPTY,   P_EMPTY,  P_EMPTY,  P_EMPTY,   P_EMPTY,   P_EMPTY},
    {P_EMPTY,  P_EMPTY,   P_EMPTY,   P_EMPTY,  P_EMPTY,  P_EMPTY,   P_EMPTY,   P_EMPTY},
    {P_EMPTY,  P_EMPTY,   P_EMPTY,   P_EMPTY,  P_EMPTY,  P_EMPTY,   P_EMPTY,   P_EMPTY},
    {W_PAWN,   W_PAWN,    W_PAWN,    W_PAWN,   W_PAWN,   W_PAWN,    W_PAWN,    W_PAWN},
    {W_ROOK,   W_KNIGHT, W_BISHOP1, W_QUEEN,  W_KING,   W_BISHOP2, W_KNIGHT,  W_ROOK}
};

const char* getPieceName(int pieceType) {
    switch(pieceType) {
        case B_ROOK: return "B_Rook";
        case B_KNIGHT: return "B_Knight";
        case B_BISHOP1: return "B_Bishop1";
        case B_QUEEN: return "B_Queen";
        case B_KING: return "B_King";
        case B_BISHOP2: return "B_Bishop2";
        case B_PAWN: return "B_Pawn";
        case W_ROOK: return "W_Rook";
        case W_KNIGHT: return "W_Knight";
        case W_BISHOP1: return "W_Bishop1";
        case W_QUEEN: return "W_Queen";
        case W_KING: return "W_King";
        case W_BISHOP2: return "W_Bishop2";
        case W_PAWN: return "W_Pawn";  // Adicionando o W_PAWN
        case P_EMPTY: return "Empty";
        default: return "Unknown"; // Para valores não reconhecidos
    }
}

typedef struct {
    int value;     // Valor armazenado na posição
    int lockedColor;   // 0 NONE 1 YELLOW 2 GREEN
} Cell;
Cell lastMatrix[8][8];

//int lastMatrix[8][8] = {}; // Matriz 8x8 para armazenar os valores de todas as colunas e linhas

extern int getSensorToLedPosition(int row, int col);
extern esp_timer_handle_t gameTimerHandle;
extern led_strip_handle_t led_strip;
extern SemaphoreHandle_t mutex;

void isReadyToPlay(int col, const int values[8]) {
    xSemaphoreTake(mutex, portMAX_DELAY);

    for (int row = 0; row < 8; ++row) {
        if(lastMatrix[row][col].value != values[row]) {
            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 0, 125);
            led_strip_refresh(led_strip);
            void displayText(const char *text); // Supondo que essa função exista para exibir o texto
            char message[64]; // Buffer para armazenar a string formatada
            sprintf(message, "CHANGE\n col: %d\n row: %d", col, row);
            displayText(message);

            vTaskDelay(pdMS_TO_TICKS(1000));
            //displayText("Adicionou peça em col:" + col + "row:" + row);
            lastMatrix[row][col].value = values[row];
        }
        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 96, 208);
        led_strip_refresh(led_strip);
        switch (values[row]) {
            case 0: {
                if (row == 0 || row == 1 || row == 6 || row == 7) {
                    if (row == 6 || row == 7) {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 0);
                    } else {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 255, 255);
                    }
                } else {
                    if (row % 2 == 0) {
                        if (col % 2 == 0) {
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 0);
                        } else {
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 255);
                        }
                    } else {
                        if (col % 2 != 0) {
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 0);
                        } else {
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 255);
                        }
                    }
                }
                //ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255, 255, 255));
                break;
            }
            default: {
                if (row % 2 == 0) {
                    if (col % 2 == 0) {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 0);
                    } else {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 255);
                    }
                } else {
                    if (col % 2 != 0) {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 0);
                    } else {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 255);
                    }
                }
            }
        }
        led_strip_refresh(led_strip);
    }
    led_strip_refresh(led_strip);
    xSemaphoreGive(mutex);
}
void frontWalker(int row, int col, int turn) {
    int i;
    
    if (turn == -1) { // Jogada azul
        i = row + 1;
        if (i < 8 && lastMatrix[i][col].value == 0) {
            lastMatrix[i][col].lockedColor = 2;
        }
        // Captura adversária à esquerda
        if (i < 8 && col - 1 >= 0 && lastMatrix[i][col - 1].value != turn && lastMatrix[i][col - 1].value != 0) {
            lastMatrix[i][col - 1].lockedColor = 2; // Marca a peça adversária azul
        }
        // Captura adversária à direita
        if (i < 8 && col + 1 < 8 && lastMatrix[i][col + 1].value != turn && lastMatrix[i][col + 1].value != 0) {
            lastMatrix[i][col + 1].lockedColor = 2; // Marca a peça adversária azul
        }
        // Movimento duplo se for o primeiro movimento
        if (row == 1 && lastMatrix[i + 1][col].value == 0) {
            lastMatrix[i + 1][col].lockedColor = 2; // Marca o movimento duplo
        }
    } else { // Jogada branca
        i = row - 1;
        if (i >= 0 && lastMatrix[i][col].value == 0) {
            lastMatrix[i][col].lockedColor = 2;
        }
        // Captura adversária à esquerda
        if (i >= 0 && col - 1 >= 0 && lastMatrix[i][col - 1].value != turn && lastMatrix[i][col - 1].value != 0) {
            lastMatrix[i][col - 1].lockedColor = 2; // Marca a peça adversária branca
        }
        // Captura adversária à direita
        if (i >= 0 && col + 1 < 8 && lastMatrix[i][col + 1].value != turn && lastMatrix[i][col + 1].value != 0) {
            lastMatrix[i][col + 1].lockedColor = 2; // Marca a peça adversária branca
        }
        // Movimento duplo se for o primeiro movimento
        if (row == 6 && lastMatrix[i - 1][col].value == 0) {
            lastMatrix[i - 1][col].lockedColor = 2; // Marca o movimento duplo
        }
    }
}

void moveInBothDirections(int row, int col, int turn) {
    int i;

    // Andando para a esquerda
    if (turn == -1) { // Jogada azul
        i = col - 1;
        while (i >= 0 && lastMatrix[row][i].value == 0) {
            lastMatrix[row][i].lockedColor = 2;
            i--;
        }
        if (i >= 0 && lastMatrix[row][i].value == 1) {
            lastMatrix[row][i].lockedColor = 2; // Marca a peça adversária azul
        }
    } else { // Jogada branca
        i = col - 1;
        while (i >= 0 && lastMatrix[row][i].value == 0) {
            lastMatrix[row][i].lockedColor = 2;
            i--;
        }
        if (i >= 0 && lastMatrix[row][i].value == -1) {
            lastMatrix[row][i].lockedColor = 2; // Marca a peça adversária branca
        }
    }
    // Andando para a direita
    if (turn == -1) { // Jogada azul
        i = col + 1;
        while (i < 8 && lastMatrix[row][i].value == 0) {
            lastMatrix[row][i].lockedColor = 2;
            i++;
        }
        if (i < 8 && lastMatrix[row][i].value == 1) {
            lastMatrix[row][i].lockedColor = 2; // Marca a peça adversária azul
        }
    } else { // Jogada branca
        i = col + 1;
        while (i < 8 && lastMatrix[row][i].value == 0) {
            lastMatrix[row][i].lockedColor = 2;
            i++;
        }
        if (i < 8 && lastMatrix[row][i].value == -1) {
            lastMatrix[row][i].lockedColor = 2; // Marca a peça adversária branca
        }
    }
    // Andando para cima
    if (turn == -1) { // Jogada azul
        i = row - 1;
        while (i >= 0 && lastMatrix[i][col].value == 0) {
            lastMatrix[i][col].lockedColor = 2;
            i--;
        }
        if (i >= 0 && lastMatrix[i][col].value == 1) {
            lastMatrix[i][col].lockedColor = 2; // Marca a peça adversária azul
        }
    } else { // Jogada branca
        i = row - 1;
        while (i >= 0 && lastMatrix[i][col].value == 0) {
            lastMatrix[i][col].lockedColor = 2;
            i--;
        }
        if (i >= 0 && lastMatrix[i][col].value == -1) {
            lastMatrix[i][col].lockedColor = 2; // Marca a peça adversária branca
        }
    }
    // Andando para baixo
    if (turn == -1) { // Jogada azul
        i = row + 1;
        while (i < 8 && lastMatrix[i][col].value == 0) {
            lastMatrix[i][col].lockedColor = 2;
            i++;
        }
        if (i < 8 && lastMatrix[i][col].value == 1) {
            lastMatrix[i][col].lockedColor = 2; // Marca a peça adversária azul
        }
    } else { // Jogada branca
        i = row + 1;
        while (i < 8 && lastMatrix[i][col].value == 0) {
            lastMatrix[i][col].lockedColor = 2;
            i++;
        }
        if (i < 8 && lastMatrix[i][col].value == -1) {
            lastMatrix[i][col].lockedColor = 2; // Marca a peça adversária branca
        }
    }
}
void diagonalWalker(int row, int col, int turn) {
    int i, j;

    // Diagonal superior esquerda
    i = row + 1;
    j = col - 1;
    while (i < 8 && j >= 0 && lastMatrix[i][j].value == 0) {
        lastMatrix[i][j].lockedColor = 2;
        i++;
        j--;
    }
    if (i < 8 && j >= 0 && lastMatrix[i][j].value != turn) {
        lastMatrix[i][j].lockedColor = 2; // Marca a peça adversária
    }

    // Diagonal superior direita
    i = row + 1;
    j = col + 1;
    while (i < 8 && j < 8 && lastMatrix[i][j].value == 0) {
        lastMatrix[i][j].lockedColor = 2;
        i++;
        j++;
    }
    if (i < 8 && j < 8 && lastMatrix[i][j].value != turn) {
        lastMatrix[i][j].lockedColor = 2; // Marca a peça adversária
    }

    // Diagonal inferior esquerda
    i = row - 1;
    j = col - 1;
    while (i >= 0 && j >= 0 && lastMatrix[i][j].value == 0) {
        lastMatrix[i][j].lockedColor = 2;
        i--;
        j--;
    }
    if (i >= 0 && j >= 0 && lastMatrix[i][j].value != turn) {
        lastMatrix[i][j].lockedColor = 2; // Marca a peça adversária
    }

    // Diagonal inferior direita
    i = row - 1;
    j = col + 1;
    while (i >= 0 && j < 8 && lastMatrix[i][j].value == 0) {
        lastMatrix[i][j].lockedColor = 2;
        i--;
        j++;
    }
    if (i >= 0 && j < 8 && lastMatrix[i][j].value != turn) {
        lastMatrix[i][j].lockedColor = 2; // Marca a peça adversária
    }
}
void knightWalker(int row, int col, int turn) {
    // Todas as possíveis movimentações do cavalo (par ordenado de deslocamentos)
    int moves[8][2] = {
        {-2, -1}, {-2, 1}, {2, -1}, {2, 1},
        {-1, -2}, {-1, 2}, {1, -2}, {1, 2}
    };

    for (int k = 0; k < 8; k++) {
        int newRow = row + moves[k][0];
        int newCol = col + moves[k][1];

        // Verifica se a nova posição está dentro do tabuleiro
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            if (lastMatrix[newRow][newCol].value == 0) {
                // Marca a posição como válida para movimentação
                lastMatrix[newRow][newCol].lockedColor = 2;
            } else if (lastMatrix[newRow][newCol].value != turn) {
                // Marca a peça adversária como válida para captura
                lastMatrix[newRow][newCol].lockedColor = 2;
            }
        }
    }
}
void kingWalker(int row, int col, int turn) {
    // Todas as 8 direções possíveis para o Rei
    int moves[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, // Diagonais superiores e cima
        {0, -1},          {0, 1},  // Lados esquerdo e direito
        {1, -1}, {1, 0}, {1, 1}    // Diagonais inferiores e baixo
    };

    for (int k = 0; k < 8; k++) {
        int newRow = row + moves[k][0];
        int newCol = col + moves[k][1];

        // Verifica se a nova posição está dentro do tabuleiro
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            if (lastMatrix[newRow][newCol].value == 0) {
                // A casa está vazia: válida para movimentação
                lastMatrix[newRow][newCol].lockedColor = 2;
            } else if (lastMatrix[newRow][newCol].value * turn < 0) {
                // A casa contém uma peça adversária: válida para captura
                lastMatrix[newRow][newCol].lockedColor = 2;
            }
        }
    }
}
void handleData(int col, const int values[8]) {
    for (int row = 0; row < 8; ++row) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        if(lastMatrix[row][col].value != values[row]) {
            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 0, 125);
            led_strip_refresh(led_strip);
            char message[64];
            //Está Movendo?
            if(lastMatrix[row][col].value != 0 && isMoving == false) {
                isMoving = true;
                piece1.type = getPieceName(board[row][col]);
                piece1.row = row;
                piece1.col = col;
                snprintf(message, sizeof(message), "CHANGE P1\n col: %d\n row: %d\n %s", col, row, getPieceName(board[row][col]));
                displayText(message);
                showMoves = true;
                lastMatrix[row][col].lockedColor = 1;

                if (strcmp(getPieceName(board[row][col]), "B_Rook") == 0 || strcmp(getPieceName(board[row][col]), "W_Rook") == 0) {
                    moveInBothDirections(row, col, lastMatrix[row][col].value);
                } else if (strcmp(getPieceName(board[row][col]), "B_Knight") == 0 || strcmp(getPieceName(board[row][col]), "W_Knight") == 0) {
                    knightWalker(row, col, lastMatrix[row][col].value);
                } else if (strcmp(getPieceName(board[row][col]), "B_Bishop1") == 0 || strcmp(getPieceName(board[row][col]), "W_Bishop1") == 0) {
                    diagonalWalker(row, col, lastMatrix[row][col].value);
                } else if (strcmp(getPieceName(board[row][col]), "B_Bishop2") == 0 || strcmp(getPieceName(board[row][col]), "W_Bishop2") == 0) {
                    diagonalWalker(row, col, lastMatrix[row][col].value);
                } else if (strcmp(getPieceName(board[row][col]), "B_Queen") == 0 || strcmp(getPieceName(board[row][col]), "W_Queen") == 0) {
                    moveInBothDirections(row, col, lastMatrix[row][col].value);
                    diagonalWalker(row, col, lastMatrix[row][col].value);
                    kingWalker(row, col, lastMatrix[row][col].value);
                } else if (strcmp(getPieceName(board[row][col]), "B_King") == 0 || strcmp(getPieceName(board[row][col]), "W_King") == 0) {
                    kingWalker(row, col, lastMatrix[row][col].value);
                } else if (strcmp(getPieceName(board[row][col]), "B_Pawn") == 0 || strcmp(getPieceName(board[row][col]), "W_Pawn") == 0) {
                    frontWalker(row, col, lastMatrix[row][col].value);
                }

                //frontWalker(row, col, lastMatrix[row][col].value);
                //moveInBothDirections(row, col, lastMatrix[row][col].value);
                //diagonalWalker(row, col, lastMatrix[row][col].value);
                //knightWalker(row, col, lastMatrix[row][col].value);
                //kingWalker(row, col, lastMatrix[row][col].value);
            }
            else if(lastMatrix[row][col].value == 0 && isMoving == true && isAtacking == false) {
                board[row][col] = board[piece1.row][piece1.col];
                board[piece1.row][piece1.col] = P_EMPTY;
                snprintf(message, sizeof(message), "P1 MOVEU\n cl: %d rw: %d\n PARA \n cl: %d rw: %d", piece1.col, piece1.row, col ,row);
                displayText(message);
                isMoving = false;
                showMoves = false;
                for (int row = 0; row < 8; ++row) {
                    for (int col = 0; col < 8; ++col) {
                        lastMatrix[row][col].lockedColor = 0;
                    }
                }
            }

            //Está atacando?
            else if(lastMatrix[row][col].value != 0 && isMoving == true && isAtacking == false) {
                isAtacking = true;
                piece2.type = getPieceName(board[row][col]);
                piece2.row = row;
                piece2.col = col;
                snprintf(message, sizeof(message), "CHANGE P2\n col: %d\n row: %d\n %s", col, row, getPieceName(board[row][col]));
                displayText(message);
                isMoving = false;
                for (int row = 0; row < 8; ++row) {
                    for (int col = 0; col < 8; ++col) {
                        lastMatrix[row][col].lockedColor = 0;
                    }
                }
                lastMatrix[row][col].lockedColor = 2;
            }
            else if(lastMatrix[row][col].value == 0 && isAtacking == true) {
                snprintf(message, sizeof(message), "ATAQUE\n%s\nCOMEU\n%s", getPieceName(board[piece1.row][piece1.col]), getPieceName(board[piece2.row][piece2.col]));
                displayText(message);
                board[piece2.row][piece2.col] = board[piece1.row][piece1.col];
                board[piece1.row][piece1.col] = P_EMPTY;
                isAtacking = false;
                isMoving = false;
                showMoves = false;
                lastMatrix[piece2.row][piece2.col].lockedColor = 0;
                if (strcmp(piece2.type, "W_King") == 0 || strcmp(piece2.type, "B_King") == 0) {
                    // A peça é um Rei branco (W_King) ou azul (B_King)
                    snprintf(message, sizeof(message), "GAME OVER");
                    displayText(message);
                    for (int r = 0; r < 8; ++r) {
                        for (int c = 0; c < 8; ++c) {
                            lastMatrix[r][c].lockedColor = 4;
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(r, c), 0, 0, 255);
                            led_strip_refresh(led_strip); 
                        }
                    }
                    esp_timer_stop(gameTimerHandle);
                    esp_timer_delete(gameTimerHandle);
                }
            }

            lastMatrix[row][col].value = values[row];         
        }  
        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 96, 208);
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(1));
        if(lastMatrix[row][col].lockedColor == 0) {
            //CODIGO DE VERIFICAÇÃO DE POLARIDADE COMENTADO
            //switch (lastMatrix[row][col].value) {
                //case 0: {
                    if (row % 2 == 0) {
                        if (col % 2 == 0) {
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 0);
                        } else {
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 255);
                        }
                    } else {
                        if (col % 2 != 0) {
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 0);
                        } else {
                            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 255);
                        }
                    }
                //CODIGO DE VERIFICAÇÃO DE POLARIDADE COMENTADO
                    //break;
                    //}
                    /*case -1: {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 255, 0);
                        break;
                    }
                    case 1: {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 255);
                        break;
                    }
                    default: {
                        led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 0);
                        break;
                    }
            }*/
        } else if (lastMatrix[row][col].lockedColor == 1) {
            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 255, 0); //AMARELO
        } else if (lastMatrix[row][col].lockedColor == 2) {
            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 255, 0); //VERDE
        } else if (lastMatrix[row][col].lockedColor == 3) {
            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 255, 0, 125); //ROXO
        } else if (lastMatrix[row][col].lockedColor == 4) {
            led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 255); //AZUL
        }
        led_strip_refresh(led_strip);
        xSemaphoreGive(mutex);
    }
}