/*
 * airdef.c
 *
 * Created on: Jan 23, 2025
 * Author: Jorge
 */

#include "airdef.h"
#include "game_master.h"

// ==========================================
// CONFIGURACIÓN DE LA SECUENCIA "TOP GUN"
// ==========================================
// 1 = Palanca ACTIVADA (Cerrada a tierra), 0 = Palanca DESACTIVADA
// Cambia esto para definir tu clave secreta:
const uint8_t PATRON_OBJETIVO[6] = {1, 0, 0, 1, 0, 1};

// ==========================================
// DEFINICIÓN DE PINES (CONFIGURACIÓN FINAL)
// ==========================================

// 1. LOS 6 INTERRUPTORES (Puerto C: 0 al 5)
static GPIO_TypeDef* SW_PORTS[6] = {GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC};
static uint16_t SW_PINS[6]       = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5};

// 2. LOS LEDS - CANAL ROJO (Puerto B: 0, 1, 2, 4, 5, 7)
static GPIO_TypeDef* LED_R_PORTS[6] = {GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB};
static uint16_t LED_R_PINS[6]       = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_7};

// 3. LOS LEDS - CANAL VERDE (Mezcla Puerto E y B)
static GPIO_TypeDef* LED_G_PORTS[6] = {GPIOE, GPIOE, GPIOE, GPIOE, GPIOB, GPIOB};
static uint16_t LED_G_PINS[6]       = {GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15, GPIO_PIN_12, GPIO_PIN_13};

// Traemos la variable "bomb" desde game_master.c para saber si estamos jugando
extern GameContext bomb;

// ==========================================
// FUNCIONES
// ==========================================

void AirDef_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. ENCENDER RELOJES DE LOS PUERTOS (Ajustar si usas Puerto A, D, E...)
    __HAL_RCC_GPIOC_CLK_ENABLE(); // Para Interruptores
    __HAL_RCC_GPIOB_CLK_ENABLE(); // Para LEDs Rojos y parte de Verdes
    __HAL_RCC_GPIOE_CLK_ENABLE(); // Para LEDs Verdes


    // 2. CONFIGURAR INTERRUPTORES (INPUT PULL-UP)
    // Usamos Pull-Up interna para no soldar resistencias.
    // Al cerrar el interruptor, debe conectar a GND.
    for (int i = 0; i < 6; i++) {
        GPIO_InitStruct.Pin = SW_PINS[i];
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(SW_PORTS[i], &GPIO_InitStruct);
    }

    // 3. CONFIGURAR LEDS (OUTPUT)
    // Inicializamos todos apagados (LOW)
    for (int i = 0; i < 6; i++) {
        // Configurar Rojo
        GPIO_InitStruct.Pin = LED_R_PINS[i];
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(LED_R_PORTS[i], &GPIO_InitStruct);
        HAL_GPIO_WritePin(LED_R_PORTS[i], LED_R_PINS[i], GPIO_PIN_RESET);

        // Configurar Verde
        GPIO_InitStruct.Pin = LED_G_PINS[i];
        HAL_GPIO_Init(LED_G_PORTS[i], &GPIO_InitStruct);
        HAL_GPIO_WritePin(LED_G_PORTS[i], LED_G_PINS[i], GPIO_PIN_RESET);
    }
}

void AirDef_Loop(void) {

    // --- FILTRO DE ESTADO ---
    // Solo ejecutamos lógica si la cara está ACTIVA y NO ha sido resuelta aún.
    // Si bomb.faceState[FACE_AIRDEF] es 0, significa que la bomba no ha activado esta cara.
    if (bomb.faceState[FACE_AIRDEF] == 0) {
        // Aseguramos que los LEDs estén apagados para no dar pistas ni gastar luz
        for(int i=0; i<6; i++) {
            HAL_GPIO_WritePin(LED_R_PORTS[i], LED_R_PINS[i], GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_G_PORTS[i], LED_G_PINS[i], GPIO_PIN_RESET);
        }
        return;
    }

    // Si ya está resuelta, mantenemos todo en VERDE fijo (efecto de "Sistema Validado")
    if (bomb.faceSolved[FACE_AIRDEF] == 1) {
        for(int i=0; i<6; i++) {
             HAL_GPIO_WritePin(LED_G_PORTS[i], LED_G_PINS[i], GPIO_PIN_SET);
             HAL_GPIO_WritePin(LED_R_PORTS[i], LED_R_PINS[i], GPIO_PIN_RESET);
        }
        return;
    }

    // --- LÓGICA DEL JUEGO ---
    uint8_t interruptores_correctos = 0;

    for (int i = 0; i < 6; i++) {
        // Leer estado físico
        // Como usamos PULL-UP:
        //   Interruptor ABIERTO = 1 (3.3V) -> Lógica 0
        //   Interruptor CERRADO a GND = 0 (0V) -> Lógica 1
        // Invertimos el valor '!' para que sea intuitivo (1 = Activado)
        uint8_t estado_actual = (HAL_GPIO_ReadPin(SW_PORTS[i], SW_PINS[i]) == GPIO_PIN_RESET) ? 1 : 0;

        if (estado_actual == PATRON_OBJETIVO[i]) {
            // -- CORRECTO --
            interruptores_correctos++;
            // Enciende VERDE, Apaga ROJO
            HAL_GPIO_WritePin(LED_G_PORTS[i], LED_G_PINS[i], GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_R_PORTS[i], LED_R_PINS[i], GPIO_PIN_RESET);
        } else {
            // -- INCORRECTO --
            // Enciende ROJO, Apaga VERDE
            HAL_GPIO_WritePin(LED_G_PORTS[i], LED_G_PINS[i], GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_R_PORTS[i], LED_R_PINS[i], GPIO_PIN_SET);
        }
    }

    // --- COMPROBAR VICTORIA ---
    if (interruptores_correctos == 6) {
        // ¡Todas las palancas están en su sitio!
        // Notificamos al Game Master
        Game_RegisterWin(FACE_AIRDEF);
    }
}
