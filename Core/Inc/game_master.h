#ifndef INC_GAME_MASTER_H_
#define INC_GAME_MASTER_H_

#include "main.h" // Para tener acceso a HAL

// Definición de la Máquina de Estados (FSM)
typedef enum {
    STATE_IDLE,         // Esperando botón de inicio
    STATE_COUNTDOWN,    // Juego en marcha, reloj corriendo
    STATE_EXPLOSION,    // Tiempo agotado -> Game Over (Rojo)
    STATE_DEFUSED       // Todo resuelto -> Win (Verde)
} BombState;

//        IDENTIFICADORES DE LAS CARAS
// Así tus compañeros saben qué número poner
#define FACE_SAFE      0  // Cara Caja Fuerte
#define FACE_AIRDEF     1  // Cara Defensa Aérea
#define FACE_SIMON     2  // Cara Simón
#define FACE_MORSE     3  // Cara Morse
#define FACE_GYRO      4  // Cara Giroscopio
#define TOTAL_FACES    5

// Estructura global del juego
typedef struct {
    BombState currentState;
    uint32_t timeRemaining; // En segundos (ej. 240 para 4 mins)
    uint8_t gamesLeft;      // Contador para el display de 1 dígito (ej. 5)
    uint8_t mistakes;       // Número de fallos cometidos
    uint8_t tick_flag;
    uint8_t faceSolved[TOTAL_FACES]; // 0 = pendiente, 1 = resuelta
    uint8_t faceState[TOTAL_FACES]; //0 = inactiva, 1 = activa
} GameContext;

// Funciones de gestión interna
void Game_Init(void);
void Game_Update(void);
void Game_TimerTick(void); // Se llamará cada segundo desde la interrupción
void Game_ReduceTime(uint8_t seconds); // Penalización

//Funciones para los minijuegos:
void Game_RegisterWin(uint8_t face_id);     // Llamar cuando se completa una cara
void Game_RegisterMistake(void); // Llamar cuando se falla (quita tiempo)

//Función para activar caras:
void Game_ActivateFace(uint8_t face_id);
#endif /* INC_GAME_MASTER_H_ */
