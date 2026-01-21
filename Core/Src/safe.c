/*
 * safe.c
 *
 * Created on: Jan 24, 2025
 * Author: Jorge
 */

#include "safe.h"
#include "game_master.h"
#include "stdlib.h"// Para la función abs()

// ==========================================
// CONFIGURACIÓN DE LA COMBINACIÓN SECRETA
// ==========================================
// El ADC lee de 0 a 4095.
// Elegimos 3 números entre ese rango para la clave.
const uint32_t COMBINACION[3] = { 2500, 1200, 500 };

// ==========================================
// DEFINICIÓN DE PINES
// ==========================================

// CANALES DEL ADC (Puerto A)
// Potenciómetro 1 -> PA1 (ADC Channel 1)
// Potenciómetro 2 -> PA2 (ADC Channel 2)
// Potenciómetro 3 -> PA3 (ADC Channel 3)

// LEDS INDICADORES (Puerto E)
// LED 1 -> PE2
// LED 2 -> PE4 (Saltamos el 3 que suele ser conflictivo)
// LED 3 -> PE5
static uint16_t LED_PINS[3] = {GPIO_PIN_2, GPIO_PIN_4, GPIO_PIN_5};

// Variable para manejar el ADC
extern ADC_HandleTypeDef hadc1;

extern GameContext bomb;

// ==========================================
// FUNCIONES AUXILIARES
// ==========================================

// Función para leer un canal analógico específico "al vuelo"
uint32_t Read_ADC_Channel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        // Error Handler (si quieres)
    }

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10); // Esperar a que termine
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return val;
}

// ==========================================
// FUNCIONES PRINCIPALES
// ==========================================
// Función pública para apagar los LEDs de la Caja Fuerte

void Safe_Reset(void) {
    // Recorremos los 3 LEDs definidos en el array y los apagamos
    for(int i=0; i<3; i++) {
        HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_RESET);
    }
}

void Safe_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    // 1. ACTIVAR RELOJES
    __HAL_RCC_GPIOA_CLK_ENABLE(); // Para Potenciómetros
    __HAL_RCC_GPIOE_CLK_ENABLE(); // Para LEDs
    __HAL_RCC_ADC1_CLK_ENABLE();  // Para el conversor ADC

    // 2. CONFIGURAR PINES LEDS (Salida)
    for(int i=0; i<3; i++) {
        GPIO_InitStruct.Pin = LED_PINS[i];
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_RESET);
    }

    // 3. CONFIGURAR PINES POTENCIÓMETROS (Analógicos)
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 4. CONFIGURAR EL MÓDULO ADC (El cerebro analógico) LO HACE EL MAIN
}

void Safe_Loop(void) {

    // --- FILTRO: Solo jugar si la cara está activa ---
	if (bomb.faceState[FACE_SAFE] == 0) {
    	// Apagar leds por si acaso
    	for(int i=0; i<3; i++) HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_RESET);
    	return;
    }

    // Si ya ganamos, LEDs fijos
    if (bomb.faceSolved[FACE_SAFE] == 1) {
        for(int i=0; i<3; i++) HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_SET);
        return;
    }

    // --- LECTURA Y LÓGICA ---
    uint32_t margenes[3] = { 250, 150, 50 };
    uint32_t canales[3] = {ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3};
    uint8_t aciertos = 0;

    for (int i = 0; i < 3; i++) {
    	// Leemos el valor (0 - 4095)
    	  	    uint32_t lectura = Read_ADC_Channel(canales[i]);

    	 // Calculamos la distancia absoluta al objetivo
    	        int distancia = abs((int)lectura - (int)COMBINACION[i]);

        // Feedback Visual (El juego del "Frio o Caliente")
    	        // --- LÓGICA DEL CONTADOR GEIGER ---

                // ZONA 0: ¡EXACTO! (Dentro del margen de tolerancia, ej: 150)
                if (distancia < margenes[i]) {
                    HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_SET); // Fijo (ON)
                    aciertos++;
                }
                // ZONA 1: MUY CALIENTE (Pánico) - Parpadeo Frenético (cada 50ms)
                else if (distancia < 400) {
                    if ((HAL_GetTick() / 50) % 2 == 0) {
                        HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_SET);
                    } else {
                        HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_RESET);
                    }
                }
                // ZONA 2: CALIENTE (Cerca) - Parpadeo Rápido (cada 150ms)
                else if (distancia < 1000) {
                    if ((HAL_GetTick() / 150) % 2 == 0) {
                        HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_SET);
                    } else {
                        HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_RESET);
                    }
                }
                // ZONA 3: TEMPLADO (Acercándose) - Parpadeo Lento (cada 400ms)
                else if (distancia < 2500) {
                    if ((HAL_GetTick() / 400) % 2 == 0) {
                        HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_SET);
                    } else {
                        HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_RESET);
                    }
                }
                // ZONA 4: FRÍO (Lejos) - Apagado
                else {
                    HAL_GPIO_WritePin(GPIOE, LED_PINS[i], GPIO_PIN_RESET);
                }
            }

    // --- COMPROBAR VICTORIA ---
    if (aciertos == 3) {
        Game_RegisterWin(FACE_SAFE);
    }
}
