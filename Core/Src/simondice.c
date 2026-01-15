#include "simondice.h"
#include <stdlib.h>

static uint16_t LED_PINS[3] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2}; //GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5
static uint8_t secuencia[8];
static uint8_t nivelActual = 1;
static uint8_t pasoUsuario = 0;
static uint8_t juegoActivo = 0;

volatile int8_t botonPresionado = -1;
volatile uint32_t ultimoTiempoRebote = 0;

void parpadearLED(int indice, int duracion) {
	if(indice < 0 || indice > 2) return;
    HAL_GPIO_WritePin(GPIOD, LED_PINS[indice], GPIO_PIN_SET);
    HAL_Delay(duracion);
    HAL_GPIO_WritePin(GPIOD, LED_PINS[indice], GPIO_PIN_RESET);
    HAL_Delay(200);
}

void animacionPerder() {
    for(int i=0; i<3; i++) HAL_GPIO_WritePin(GPIOD, LED_PINS[i], GPIO_PIN_SET);
    HAL_Delay(2000);
    for(int i=0; i<3; i++) HAL_GPIO_WritePin(GPIOD, LED_PINS[i], GPIO_PIN_RESET);
    HAL_Delay(1000);
}

void animacionGanar() {
    for(int j=0; j<3; j++) {
        for(int i=0; i<3; i++) {
            HAL_GPIO_WritePin(GPIOD, LED_PINS[i], GPIO_PIN_SET);
            HAL_Delay(100);
            HAL_GPIO_WritePin(GPIOD, LED_PINS[i], GPIO_PIN_RESET);
        }
    }
    HAL_Delay(1000);
}

void generarSecuencia() {
    srand(HAL_GetTick());
    for(int i=0; i<8; i++) secuencia[i] = rand() % 3;
}

void mostrarSecuencia() {
    HAL_Delay(1000);
    for (int i = 0; i < nivelActual; i++) {
        parpadearLED(secuencia[i], 800);
    }
    botonPresionado = -1; // Limpiar buffer
}

// --- CONFIGURACIÓN DE HARDWARE ---
void SimonDice_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*
    // Configurar LEDs (PD0-PD5)
    HAL_GPIO_WritePin(GPIOD, 0x3F, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = 0x3F; // Pines 0 a 5
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Configurar Botones (PD6-PD11) con Interrupción
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Activar interrupciones en NVIC
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    */

    //PARA PRUEBA CON 3 LEDS
    // 1. Configurar LEDs (PD0, PD1, PD2)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // 2. Configurar Botones (PD6, PD7, PD8)
    // CAMBIOS IMPORTANTES AQUÍ:
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Detectar BAJADA (de 1 a 0)
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // Mantener en 1 si no se pulsa
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Activar interrupciones (PD6, PD7, PD8 están en la línea 9_5)
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

// Gestión de intrrupciones
void SimonDice_Boton_Handler(uint16_t GPIO_Pin)
{
    // Solo entramos si ha pasado el tiempo de seguridad (Debounce)
    if ((HAL_GetTick() - ultimoTiempoRebote) > 200) {

        uint8_t identificada = 0; // Bandera para saber si fue un botón válido

        if(GPIO_Pin == GPIO_PIN_6) {
            botonPresionado = 0;
            identificada = 1;
        }
        else if(GPIO_Pin == GPIO_PIN_7) {
            botonPresionado = 1;
            identificada = 1;
        }
        else if(GPIO_Pin == GPIO_PIN_8) {
            botonPresionado = 2;
            identificada = 1;
        }

        if (identificada == 1) {
             ultimoTiempoRebote = HAL_GetTick();
        }
    }
}

//JUEGO
void SimonDice_Loop(void) {
    if (juegoActivo == 0) {
        generarSecuencia();
        nivelActual = 1;
        pasoUsuario = 0;
        juegoActivo = 1;
        animacionGanar(); // Intro también
        HAL_Delay(1000);
        mostrarSecuencia();
    }

    if (botonPresionado != -1) {
        int btn = botonPresionado;

        uint16_t pin_fisico = 0;
        if(btn == 0) pin_fisico = GPIO_PIN_6;
        else if(btn == 1) pin_fisico = GPIO_PIN_7;
        else if(btn == 2) pin_fisico = GPIO_PIN_8;
        uint32_t tiempoEspera = HAL_GetTick();
        while (HAL_GPIO_ReadPin(GPIOD, pin_fisico) == GPIO_PIN_RESET) {
        	if ((HAL_GetTick() - tiempoEspera) > 3000) break; // Salir si lleva 3s bloqueado
        }

        HAL_Delay(50);

        botonPresionado = -1;
        parpadearLED(btn, 300);

        if (btn == secuencia[pasoUsuario]) {
            pasoUsuario++;
            if (pasoUsuario == nivelActual) {
                nivelActual++;
                pasoUsuario = 0;
                if (nivelActual > 8) {
                    animacionGanar();
                    juegoActivo = 0;
                } else {
                    mostrarSecuencia();
                }
            }
        } else {
            animacionPerder();
            juegoActivo = 0;
        }
    }
}


