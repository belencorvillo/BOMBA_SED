/*
 * morse.c
 *
 * Created on: Jan 24, 2025
 * Author: Jorge
 */

#include "morse.h"
#include "game_master.h"
#include <string.h>
#include <stdlib.h>

// ==========================================
// CONFIGURACIÓN Y DICCIONARIO
// ==========================================

const char* DICCIONARIO[] = { "SOS", "BOMBA", "HOLA", "TUNEL", "CLAVE" };
#define NUM_PALABRAS 5

// Tiempos (ms)
#define TIEMPO_PUNTO_MAX  300   // < 300ms = Punto
#define TIEMPO_SILENCIO   1500  // > 1500ms = Fin de letra

// PINES DEL LED RGB (Puerto E)
#define PIN_R     GPIO_PIN_0 // PE0
#define PIN_G     GPIO_PIN_1 // PE1
#define PIN_B     GPIO_PIN_6 // PE6
#define PUERTO_LEDS GPIOE

// PIN DEL BOTÓN (Puerto B)
// Usamos PB14 para evitar conflicto con el sensor MEMS en PE3
#define PIN_BTN   GPIO_PIN_14 // PB14
#define PUERTO_BTN GPIOB     // ¡Cuidado, puerto distinto!

// ESTADOS DEL MÓDULO (Para no usar delays)
typedef enum {
    MORSE_IDLE,         // Esperando input
    MORSE_INPUT,        // Usuario pulsando
    MORSE_ANIM_WIN,     // Animación de acierto (Verde parpadeando)
    MORSE_ANIM_FAIL,    // Animación de fallo (Rojo fijo)
    MORSE_SOLVED        // Módulo resuelto (Verde fijo)
} MorseState;

static MorseState estado_actual = MORSE_IDLE;

// Variables de Juego
static char palabra_objetivo[10];
static uint8_t indice_letra_actual = 0;
static char buffer_morse[6];
static uint8_t indice_buffer = 0;

// Variables de Tiempo (Cronómetros)
static uint32_t tiempo_inicio_pulsacion = 0;
static uint32_t tiempo_ultimo_evento = 0;
static uint32_t timer_animacion = 0;
static uint8_t  contador_parpadeos = 0;

// Estado previo del botón
static uint8_t btn_prev = 1; // 1 = suelto (Pull-Up)

extern GameContext bomb;

// ==========================================
// AUXILIARES
// ==========================================

// Función para controlar el RGB fácil
void SetRGB(uint8_t r, uint8_t g, uint8_t b) {
    HAL_GPIO_WritePin(PUERTO_LEDS, PIN_R, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PUERTO_LEDS, PIN_G, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PUERTO_LEDS, PIN_B, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

char MorseToChar(char* input) {
    if (strcmp(input, ".-") == 0) return 'A';
    if (strcmp(input, "-...") == 0) return 'B';
    if (strcmp(input, "-.-.") == 0) return 'C';
    if (strcmp(input, "-..") == 0) return 'D';
    if (strcmp(input, ".") == 0) return 'E';
    if (strcmp(input, "..-.") == 0) return 'F';
    if (strcmp(input, "--.") == 0) return 'G';
    if (strcmp(input, "....") == 0) return 'H';
    if (strcmp(input, "..") == 0) return 'I';
    if (strcmp(input, ".---") == 0) return 'J';
    if (strcmp(input, "-.-") == 0) return 'K';
    if (strcmp(input, ".-..") == 0) return 'L';
    if (strcmp(input, "--") == 0) return 'M';
    if (strcmp(input, "-.") == 0) return 'N';
    if (strcmp(input, "---") == 0) return 'O';
    if (strcmp(input, ".--.") == 0) return 'P';
    if (strcmp(input, "--.-") == 0) return 'Q';
    if (strcmp(input, ".-.") == 0) return 'R';
    if (strcmp(input, "...") == 0) return 'S';
    if (strcmp(input, "-") == 0) return 'T';
    if (strcmp(input, "..-") == 0) return 'U';
    if (strcmp(input, "...-") == 0) return 'V';
    if (strcmp(input, ".--") == 0) return 'W';
    if (strcmp(input, "-..-") == 0) return 'X';
    if (strcmp(input, "-.--") == 0) return 'Y';
    if (strcmp(input, "--..") == 0) return 'Z';
    return '?';
}

// ==========================================
// LÓGICA PRINCIPAL
// ==========================================

void Morse_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // 1. ACTIVAR RELOJES (IMPORTANTE: Puerto E y B)
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

    // 2. CONFIGURAR BOTÓN (En Puerto B - PB14)
        GPIO_InitStruct.Pin = PIN_BTN;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(PUERTO_BTN, &GPIO_InitStruct); // Usa GPIOB

    // 3. CONFIGURAR RGB (En Puerto E - PE0, PE1, PE6)
        GPIO_InitStruct.Pin = PIN_R | PIN_G | PIN_B;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(PUERTO_LEDS, &GPIO_InitStruct); // Usa GPIOE

    SetRGB(0,0,0);

    // Elegir palabra
    srand(HAL_GetTick());
    strcpy(palabra_objetivo, DICCIONARIO[rand() % NUM_PALABRAS]);

    estado_actual = MORSE_IDLE;
    indice_letra_actual = 0;
    indice_buffer = 0;
    memset(buffer_morse, 0, 6);
}

void Morse_Loop(void) {
    // Si la cara no está activa, salir
    if (bomb.faceState[FACE_MORSE] == 0) {
        SetRGB(0,0,0);
        return;
    }

    uint32_t ahora = HAL_GetTick();

    // ---------------------------------------------------------
    // MÁQUINA DE ESTADOS (Para evitar delays)
    // ---------------------------------------------------------

    switch (estado_actual) {

    case MORSE_SOLVED:
        SetRGB(0, 1, 0); // Verde Fijo (Victoria)
        return;

    case MORSE_ANIM_FAIL:
        // Mostrar ROJO durante 1 segundo
        SetRGB(1, 0, 0);
        if (ahora - timer_animacion > 1000) {
            SetRGB(0, 0, 0);
            estado_actual = MORSE_IDLE; // Volver a jugar
            // Reiniciar progreso
            indice_letra_actual = 0;
            indice_buffer = 0;
            memset(buffer_morse, 0, 6);
        }
        return; // Bloqueamos input mientras mostramos el error

    case MORSE_ANIM_WIN:
        // Parpadeo VERDE 3 veces (cada 100ms cambia)
        if (ahora - timer_animacion > 100) {
            timer_animacion = ahora;
            contador_parpadeos++;

            // Alternar Verde / Apagado
            if (contador_parpadeos % 2 != 0) SetRGB(0, 1, 0);
            else SetRGB(0, 0, 0);

            if (contador_parpadeos >= 6) { // 3 encendidos y 3 apagados
                estado_actual = MORSE_IDLE;
                // Pasar a siguiente letra
                indice_letra_actual++;
                indice_buffer = 0;
                memset(buffer_morse, 0, 6);
                tiempo_ultimo_evento = ahora; // Resetear timeout

                // ¿Ganamos el juego entero?
                if (indice_letra_actual >= strlen(palabra_objetivo)) {
                     estado_actual = MORSE_SOLVED;
                     Game_RegisterWin(FACE_MORSE);
                }
            }
        }
        return; // Bloqueamos input durante animación

    case MORSE_IDLE:
    case MORSE_INPUT:
        // Aquí leemos el botón normalmente
        break;
    }

    // ---------------------------------------------------------
    // LÓGICA DE LECTURA (Solo si estamos en IDLE o INPUT)
    // ---------------------------------------------------------

    uint8_t btn_now = HAL_GPIO_ReadPin(PUERTO_LEDS, PIN_BTN);

    // 1. AL PULSAR (Bajada)
    if (btn_prev == 1 && btn_now == 0) {
        tiempo_inicio_pulsacion = ahora;
        tiempo_ultimo_evento = ahora;
        SetRGB(0, 0, 1); // AZUL ENCENDIDO (Feedback táctil)
        estado_actual = MORSE_INPUT;
    }

    // 2. AL SOLTAR (Subida)
    else if (btn_prev == 0 && btn_now == 1) {
        SetRGB(0, 0, 0); // AZUL APAGADO

        uint32_t duracion = ahora - tiempo_inicio_pulsacion;
        tiempo_ultimo_evento = ahora;

        if (indice_buffer < 5) {
            if (duracion < TIEMPO_PUNTO_MAX) buffer_morse[indice_buffer++] = '.';
            else buffer_morse[indice_buffer++] = '-';
        }
        estado_actual = MORSE_IDLE;
    }

    // 3. SILENCIO LARGO (Procesar letra)
    // Si lleva suelto más de X tiempo y tenemos algo en el buffer
    else if (btn_now == 1 && (ahora - tiempo_ultimo_evento > TIEMPO_SILENCIO) && indice_buffer > 0) {

        char letra = MorseToChar(buffer_morse);

        if (letra == palabra_objetivo[indice_letra_actual]) {
            // ACIERTO -> Iniciar animación WIN
            estado_actual = MORSE_ANIM_WIN;
            timer_animacion = ahora;
            contador_parpadeos = 0;
            SetRGB(0, 0, 0);
        } else {
            // FALLO -> Iniciar animación FAIL
            estado_actual = MORSE_ANIM_FAIL;
            timer_animacion = ahora;
            Game_RegisterMistake();
        }
    }

    btn_prev = btn_now;
}
