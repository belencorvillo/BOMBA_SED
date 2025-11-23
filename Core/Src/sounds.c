#include "sounds.h"

// Importamos el manejador del Timer 3 desde main.c
extern TIM_HandleTypeDef htim3;

void Sound_Init(void) {
    // Nos aseguramos de que el PWM empiece apagado
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

// Función maestra para tocar una nota
void Sound_PlayTone(uint16_t frequency, uint16_t duration_ms) {
    if (frequency == 0) {
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        HAL_Delay(duration_ms);
        return;
    }

    // 1. Calculamos el periodo para esa frecuencia
    // Reloj base = 1 MHz (gracias al Prescaler 84-1)
    uint32_t period = 1000000 / frequency;

    // 2. Configuramos el ARR (Frecuencia)
    __HAL_TIM_SET_AUTORELOAD(&htim3, period - 1);

    // 3. Configuramos el CCR (Volumen/Duty Cycle) al 50%
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, period / 2);

    // 4. Arrancamos el sonido
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    // 5. Esperamos lo que dure la nota
    HAL_Delay(duration_ms);

    // 6. Paramos el sonido
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void Sound_PlayBeep(void) {
    // Un pitido corto y agudo (tipo cuenta atrás de bomba)
    Sound_PlayTone(2000, 50); // 2000Hz durante 50ms
}

void Sound_PlayStart(void) {
    // Melodía de inicio (tipo Zelda o similar simple)
    Sound_PlayTone(NOTE_E4, 100);
    HAL_Delay(50);
    Sound_PlayTone(NOTE_E4, 100);
    HAL_Delay(50);
    Sound_PlayTone(NOTE_C5, 300);
}

void Sound_PlayError(void) {
    // Sonido grave de error
    Sound_PlayTone(150, 300);
}


void Sound_PlayPanic(void) {
    // Doble pitido rápido y agudo
    Sound_PlayTone(2000, 50);  // 2kHz, 50ms
    HAL_Delay(50);             // Silencio corto
    Sound_PlayTone(2000, 50);  // 2kHz, 50ms
}


void Sound_PlayExplosion(void) {
    // Efecto de caída de bomba (Frequency Sweep)
    // Baja de 1000Hz a 50Hz rápidamente
    for (uint16_t i = 1000; i > 50; i -= 10) {
        Sound_PlayTone(i, 5); // Notas muy cortas (5ms)
    }
    // Final grave y largo
    Sound_PlayTone(50, 2000);
}

void Sound_PlayWin(void) {
    // Arpegio de victoria (Do Mayor rápido hacia arriba)
    // C5 -> E5 -> G5 -> C6

    Sound_PlayTone(523, 100); // Do (C5)
    HAL_Delay(20);            // Pequeña pausa para separar notas

    Sound_PlayTone(659, 100); // Mi (E5)
    HAL_Delay(20);

    Sound_PlayTone(784, 100); // Sol (G5)
    HAL_Delay(20);

    // Nota final más larga y aguda
    Sound_PlayTone(1047, 600); // Do agudo (C6)
}
void Sound_PlayCaraResuelta(void) {
    // Sonido de "Checkpoint" o "Éxito Parcial"
    // Dos notas rápidas ascendentes (Sol -> Do agudo)

    Sound_PlayTone(784, 80);  // Sol (G5)
    HAL_Delay(20);            // Breve separación
    Sound_PlayTone(1047, 150); // Do (C6) - Un poco más largo
}
