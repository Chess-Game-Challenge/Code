#include <esp_log.h>
#include "led_strip.h"
#include <display.h>

extern bool DEBUG;

enum Type {
    BLACK = -1,
    EMPTY,
    WHITE
};

enum Role {
    NONE = -1,
    PAWN,   // Peão
    BISHOP, // Bispo
    KNIGHT, // Cavalo
    ROOK,   // Torre
    QUEEN,  // Rainha
    KING    // Rei
};

typedef struct {
    enum Type type;
    enum Role role;
    uint8_t extra;
} Piece;

#define P_EMPTY     (Piece) {EMPTY, NONE, 0}

#define W_PAWN      (Piece) {WHITE, PAWN, 0}
#define W_BISHOP1   (Piece) {WHITE, BISHOP, 0}
#define W_BISHOP2   (Piece) {WHITE, BISHOP, 1}
#define W_KNIGHT    (Piece) {WHITE, KNIGHT, 0}
#define W_ROOK      (Piece) {WHITE, ROOK, 0}
#define W_QUEEN     (Piece) {WHITE, QUEEN, 0}
#define W_KING      (Piece) {WHITE, KING, 0}

#define B_PAWN      (Piece) {BLACK, PAWN, 0}
#define B_BISHOP1   (Piece) {BLACK, BISHOP, 0}
#define B_BISHOP2   (Piece) {BLACK, BISHOP, 1}
#define B_KNIGHT    (Piece) {BLACK, KNIGHT, 0}
#define B_ROOK      (Piece) {BLACK, ROOK, 0}
#define B_QUEEN     (Piece) {BLACK, QUEEN, 0}
#define B_KING      (Piece) {BLACK, KING, 0}

typedef struct {
    Piece piece;
    uint8_t originalX;
    uint8_t originalY;
} BoardPiece;

#define B_NONE (BoardPiece) {P_EMPTY, 127, 127}

BoardPiece floating = B_NONE;
BoardPiece floating2 = B_NONE;

void initialBoard(Piece board[8][8]) {
    Piece initialBoard[8][8] = {
            {B_ROOK,  B_KNIGHT, B_BISHOP1, B_QUEEN, B_KING,  B_BISHOP2, B_KING,  B_ROOK},
            {B_PAWN,  B_PAWN,   B_PAWN,    B_PAWN,  B_PAWN,  B_PAWN,    B_PAWN,  B_PAWN},
            {P_EMPTY, P_EMPTY,  P_EMPTY,   P_EMPTY, P_EMPTY, P_EMPTY,   P_EMPTY, P_EMPTY},
            {P_EMPTY, P_EMPTY,  P_EMPTY,   P_EMPTY, P_EMPTY, P_EMPTY,   P_EMPTY, P_EMPTY},
            {P_EMPTY, P_EMPTY,  P_EMPTY,   P_EMPTY, P_EMPTY, P_EMPTY,   P_EMPTY, P_EMPTY},
            {P_EMPTY, P_EMPTY,  P_EMPTY,   P_EMPTY, P_EMPTY, P_EMPTY,   P_EMPTY, P_EMPTY},
            {W_PAWN,  W_PAWN,   W_PAWN,    W_PAWN,  W_PAWN,  W_PAWN,    W_PAWN,  W_PAWN},
            {W_ROOK,  W_KNIGHT, W_BISHOP2, W_QUEEN, W_KING,  W_BISHOP1, W_KING,  W_ROOK}
    };

    for (int i = 0; i < sizeof(initialBoard) / sizeof(initialBoard[0]); ++i) {
        for (int j = 0; j < sizeof(initialBoard[j]) / sizeof(initialBoard[i][0]); ++j) {
            board[i][j] = initialBoard[i][j];
        }
    }
}

Piece positions[8][8];

void showMove(BoardPiece piece) {
    if (piece.piece.role == PAWN) {

    }
}

// Set the board to the initial positions
void initBoard() {
    initialBoard(positions);
}

extern int getSensorToLedPosition(int row, int col);

extern led_strip_handle_t led_strip;
extern SemaphoreHandle_t mutex;

void isReadyToPlay(int col, const int values[8]) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    for (int row = 0; row < 8; ++row) {
        switch (values[row]) {
            case 0: {
                if (row == 0 || row == 1 || row == 6 || row == 7) {
                    if (row == 0 || row == 1) {
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

void handleData(int col, const int values[8]) {
    for (int row = 0; row < 8; ++row) {
        Piece *rowValues = positions[row];

        // Check for NULL before dereferencing
        if (rowValues == NULL) {
            ESP_LOGE("Sensor", "Error: piece is NULL for row %d\n", row);
            continue;
        }
        // Additional NULL check
        /*if (rowValues[col] == NULL) {
            ESP_LOGE("Sensor", "Error: No piece at row %d, col %d\n", row, col);
            continue;
        }*/
        Piece piece = rowValues[7 - col];
        xSemaphoreTake(mutex, portMAX_DELAY);
        switch (values[row]) {
            case 0: {
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

                //ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255, 255, 255));
                break;
            }
            // case -1: {
            //     led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 255, 0);
            //     break;
            // }
            // case 1: {
            //     led_strip_set_pixel(led_strip, getSensorToLedPosition(row, col), 0, 0, 255);
            //     break;
            // }
        }
        led_strip_refresh(led_strip);
        xSemaphoreGive(mutex);

        // Handle movement
            if (values[row] == 0) {
                if (piece.type == EMPTY) return;         
            } 
    }
}