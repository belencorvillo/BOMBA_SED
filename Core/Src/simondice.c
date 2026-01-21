#include "simondice.h"
#include "game_master.h"
#include <stdlib.h>

static uint16_t LED_PINS[6] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5};
static uint8_t secuencia[8];
static uint8_t nivelActual = 1;
static uint8_t pasoUsuario = 0;
static uint8_t juegoActivo = 0;

volatile int8_t botonPresionado = -1;
volatile uint32_t ultimoTiempoRebote = 0;

void parpadearLED(int indice, int duracion) {
	if(indice < 0 || indice > 5) return;
    HAL_GPIO_WritePin(GPIOD, LED_PINS[indice], GPIO_PIN_SET);
    HAL_Delay(duracion);
    HAL_GPIO_WritePin(GPIOD, LED_PINS[indice], GPIO_PIN_RESET);
    HAL_Delay(200);
}

void animacionPerder() {

    for(int i=0; i<6; i++) HAL_GPIO_WritePin(GPIOD, LED_PINS[i], GPIO_PIN_SET);
    HAL_Delay(2000);
    for(int i=0; i<6; i++) HAL_GPIO_WritePin(GPIOD, LED_PINS[i], GPIO_PIN_RESET);
    HAL_Delay(1000);

}

void animacionGanar() {
    for(int j=0; j<3; j++) {
        for(int i=0; i<6; i++) {
            HAL_GPIO_WritePin(GPIOD, LED_PINS[i], GPIO_PIN_SET);
            HAL_Delay(100);
            HAL_GPIO_WritePin(GPIOD, LED_PINS[i], GPIO_PIN_RESET);
        }
    }
    HAL_Delay(1000);
}

void generarSecuencia() {
    srand(HAL_GetTick());
    for(int i=0; i<8; i++) secuencia[i] = rand() % 6;
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

    // Configurar LEDs (PD0 a PD5)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Configurar Botones (PD6 a PD11)
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Detectar BAJADA (de 1 a 0)
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // Mantener en 1 si no se pulsa
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // 3. ACTIVAR INTERRUPCIONES
    // Pines 6, 7, 8, 9 van por EXTI9_5
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    // Pines 10, 11 van por EXTI15_10
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

// Gestión de intrrupciones
void SimonDice_Boton_Handler(uint16_t GPIO_Pin)
{
	if (bomb.faceState[FACE_SIMON] == 0) return;
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
        else if(GPIO_Pin == GPIO_PIN_9) {
            botonPresionado = 3;
            identificada = 1;
        }
        else if(GPIO_Pin == GPIO_PIN_10) {
            botonPresionado = 4;
            identificada = 1;
        }
        else if(GPIO_Pin == GPIO_PIN_11) {
            botonPresionado = 5;
            identificada = 1;
        }



        if (identificada == 1) {
             ultimoTiempoRebote = HAL_GetTick();
        }
    }
}

//JUEGO
void SimonDice_Loop(void) {

	// Si la cara NO está activa (0), no hacemos nada y salimos.
	if (bomb.faceState[FACE_SIMON] == 0) {
	    juegoActivo = 0;
	    return;
	}

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
        else if(btn == 3) pin_fisico = GPIO_PIN_9;
        else if(btn == 4) pin_fisico = GPIO_PIN_10;
        else if(btn == 5) pin_fisico = GPIO_PIN_11;
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
                if (nivelActual > 6) {
                    animacionGanar();
                    Game_RegisterWin(FACE_SIMON);
                    juegoActivo = 0;
                } else {
                    mostrarSecuencia();
                }
            }
        } else {
        	Game_RegisterMistake();
            animacionPerder();
            juegoActivo = 0;
        }
    }
}


