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


// CONFIGURACIÓN Y DICCIONARIO


const char* DICCIONARIO[] = { "SOS", "BOMBA", "HOLA", "TUNEL", "CLAVE" };
#define NUM_PALABRAS 5

// PINES DEL LED RGB (Puerto E)
#define PIN_R     GPIO_PIN_13 // PD13
#define PIN_G     GPIO_PIN_14 // PD14
#define PIN_B     GPIO_PIN_15 // PD15
#define PUERTO_LEDS GPIOD

// PIN DEL BOTÓN (Puerto B14)

#define PIN_BTN   GPIO_PIN_14
#define PUERTO_BTN GPIOB
uint8_t debug_estado_boton = 5;
// ESTADOS DEL MÓDULO
typedef enum {
    MORSE_IDLE,         // Esperando input
    MORSE_INPUT,        // Usuario pulsando
    MORSE_ANIM_WIN,     // Animación de acierto (Verde y azul)
    MORSE_ANIM_FAIL,    // Animación de fallo (Rojo parpadeando)
    MORSE_SOLVED        // Módulo resuelto (Verde fijo)
} MorseState;

static MorseState estado_actual = MORSE_IDLE;

// Variables de Juego
static char palabra_objetivo[10];
static uint8_t indice_letra_actual = 0;
char buffer_morse[6];
char debug_letra = ' ';     // Para ver qué letra ha traducido
uint8_t indice_buffer = 0;

// Variables de Tiempo (Cronómetros)
static uint32_t tiempo_inicio_pulsacion = 0;
static uint32_t tiempo_ultimo_evento = 0;
static uint32_t timer_animacion = 0;
static uint8_t  contador_parpadeos = 0;

// Estado previo del botón
static uint8_t btn_prev = 1; // 1 = suelto (Pull-Up)

extern GameContext bomb;


// AUXILIARES


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

        //HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
        SetRGB(0,0,0);

    // Elegir palabra
    srand(HAL_GetTick());
    strcpy(palabra_objetivo, DICCIONARIO[0]);//rand() % NUM_PALABRAS

    estado_actual = MORSE_IDLE;
    indice_letra_actual = 0;
    indice_buffer = 0;
    memset(buffer_morse, 0, 6);
}

// ==========================================
// Tiempos Ajustados y Loop de juego
// ==========================================
#define TIEMPO_PUNTO_MAX  400   // Hasta 400ms es un punto (más margen)
#define TIEMPO_SILENCIO   1200  // Esperar 1.2s para dar por terminada la letra

void Morse_Loop(void) {
    if (bomb.faceState[FACE_MORSE] == 0) {
        SetRGB(0,0,0);
        return;
    }

    uint32_t ahora = HAL_GetTick();
    uint8_t btn_now = HAL_GPIO_ReadPin(PUERTO_BTN, PIN_BTN);

    // ---------------------------------------------------------
    // MÁQUINA DE ESTADOS
    // ---------------------------------------------------------
    switch (estado_actual) {

    case MORSE_SOLVED:
        SetRGB(0, 1, 0); // Verde Fijo
        return;

    case MORSE_ANIM_FAIL:
        if (ahora - timer_animacion > 100) { // Parpadeo Rojo Rápido
            timer_animacion = ahora;
            contador_parpadeos++;
            SetRGB((contador_parpadeos % 2), 0, 0);
            if (contador_parpadeos >= 10) { // 1 segundo
                SetRGB(0, 0, 0);
                estado_actual = MORSE_IDLE;
                // Reset parcial (solo la letra actual)
                indice_buffer = 0;
                memset(buffer_morse, 0, 6);
            }
        }
        return;

    case MORSE_ANIM_WIN: // Acierto de LETRA (No palabra entera aún)
        if (ahora - timer_animacion > 100) {
            timer_animacion = ahora;
            contador_parpadeos++;
            // Parpadeo Verde/Azul celebración
            if (contador_parpadeos % 2 == 0) SetRGB(0, 1, 0); else SetRGB(0, 0, 1);

            if (contador_parpadeos >= 6) {
                estado_actual = MORSE_IDLE;
                indice_letra_actual++;
                indice_buffer = 0;
                memset(buffer_morse, 0, 6);
                tiempo_ultimo_evento = ahora; // Reiniciar cuenta silencio

                // Comprobar si hemos acabado la PALABRA
                if (indice_letra_actual >= strlen(palabra_objetivo)) {
                     estado_actual = MORSE_SOLVED;
                     Game_RegisterWin(FACE_MORSE);
                }
            }
        }
        return;

    case MORSE_IDLE:
    case MORSE_INPUT:
        // Lógica de lectura abajo
        break;
    }

    // ---------------------------------------------------------
    // LÓGICA DE PULSACIÓN
    // ---------------------------------------------------------

    // 1. FLANCO DE BAJADA (Pulsar)
    if (btn_prev == 1 && btn_now == 0) {
        tiempo_inicio_pulsacion = ahora;
        SetRGB(1, 1, 1); // BLANCO al pulsar (Feedback visual de contacto)
        estado_actual = MORSE_INPUT;
    }

    // 2. FLANCO DE SUBIDA (Soltar)
    else if (btn_prev == 0 && btn_now == 1) {
        uint32_t duracion = ahora - tiempo_inicio_pulsacion;

        // Filtro Anti-Rebote (ignoramos clics de < 50ms)
        if (duracion > 50) {
            tiempo_ultimo_evento = ahora; // Reseteamos el temporizador de silencio

            if (indice_buffer < 5) {
                if (duracion < TIEMPO_PUNTO_MAX) {
                    buffer_morse[indice_buffer++] = '.';
                    SetRGB(0, 1, 0); // Flash VERDE momentáneo (Punto)
                } else {
                    buffer_morse[indice_buffer++] = '-';
                    SetRGB(0, 0, 1); // Flash AZUL momentáneo (Raya)
                }
                buffer_morse[indice_buffer] = '\0';
            }
        } else {
            SetRGB(0,0,0); // Si fue ruido, apagamos
        }
        estado_actual = MORSE_IDLE;
    }

    // 3. TIEMPO DE SILENCIO (Procesar Letra)
    // Solo si estamos en IDLE (botón suelto) y hay algo en el buffer
    else if (estado_actual == MORSE_IDLE && indice_buffer > 0) {

        // Apagar LED de feedback después de un rato
        if (ahora - tiempo_ultimo_evento > 200) SetRGB(0,0,0);

        // Si ha pasado el tiempo de silencio, PROCESAR
        if (ahora - tiempo_ultimo_evento > TIEMPO_SILENCIO) {

            char letra_detectada = MorseToChar(buffer_morse);
            char letra_esperada = palabra_objetivo[indice_letra_actual];

            if (letra_detectada == letra_esperada) {
                // ¡Letra Correcta!
                estado_actual = MORSE_ANIM_WIN;
                timer_animacion = ahora;
                contador_parpadeos = 0;
            } else {
                // ¡Letra Incorrecta! -> Error
                estado_actual = MORSE_ANIM_FAIL;
                timer_animacion = ahora;
                contador_parpadeos = 0;
                Game_RegisterMistake(); // Penalización


            }
        }
    }

    btn_prev = btn_now;
}

void Morse_Reset(void) {

    SetRGB(0, 0, 0);
    estado_actual = MORSE_IDLE;
    indice_letra_actual = 0;
    indice_buffer = 0;
    memset(buffer_morse, 0, 6);

}
