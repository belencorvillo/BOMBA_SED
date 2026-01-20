#include "sounds.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;

void Sound_Init(void) {
    // Apagar Buzzer (PA6)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    // El Altavoz (PC6) empieza apagado por el PWM
}

//FUNCIONES BUZZER


// Función interna para el buzzer
static void Buzzer_Tone(uint16_t duration_ms) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);   // ON
    HAL_Delay(duration_ms);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); // OFF
}

void Sound_Buzzer_Beep(void) {
    Buzzer_Tone(50); // Bip corto de 50ms
}

void Sound_Buzzer_DoubleBeep(void) {
    // Simula "Doble Velocidad" haciendo dos bips rápidos en el mismo tick
    Buzzer_Tone(30);
    HAL_Delay(50);
    Buzzer_Tone(30);
}

void Sound_Buzzer_Arming(void) {
    // Señal al iniciar el juego
    Buzzer_Tone(80);
    HAL_Delay(50);
    Buzzer_Tone(80);
    HAL_Delay(50);
    Buzzer_Tone(200);
}


//FUNCIONES ALTAVOZ

// Función interna para tocar nota en el altavoz
void Speaker_Tone(uint16_t period, uint16_t duration_ms) {
    if (period > 0) {
        __HAL_TIM_SET_AUTORELOAD(&htim3, period);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, period / 2); // 50% Duty
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    }
    HAL_Delay(duration_ms);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void Sound_Speaker_Startup(void) {
    // Melodía inicio
    Speaker_Tone(NOTE_G4, 100);
    HAL_Delay(20);
    Speaker_Tone(NOTE_C5, 100);
    HAL_Delay(20);
    Speaker_Tone(NOTE_E5, 100);
    HAL_Delay(20);
    Speaker_Tone(NOTE_G5, 400);
    HAL_Delay(50);
}

void Sound_Speaker_WinSmall(void) {
    // Pequeña victoria (Cara resuelta)
	Speaker_Tone(523, 80);  // Do (C5)
	HAL_Delay(10);          // Pequeña pausa para separar notas
	Speaker_Tone(659, 80);  // Mi (E5)
	HAL_Delay(10);
	Speaker_Tone(784, 80);  // Sol (G5)
	HAL_Delay(10);
	Speaker_Tone(1047, 200);// Do (C6) - Final largo
}

void Sound_Speaker_WinTotal(void) {
    // Gran Victoria (Cubo resuelto)
    Speaker_Tone(NOTE_C5, 150);
    Speaker_Tone(NOTE_E5, 150);
    Speaker_Tone(NOTE_G5, 150);
    Speaker_Tone(NOTE_C6, 600);
}

void Sound_Speaker_Siren(void) {
    // Sirena emergencia
    // Subida
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    for (int i = 2000; i > 1000; i -= 50) {
        __HAL_TIM_SET_AUTORELOAD(&htim3, i);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, i / 2);
        HAL_Delay(2);
    }
    // Bajada
    for (int i = 1000; i < 2000; i += 50) {
        __HAL_TIM_SET_AUTORELOAD(&htim3, i);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, i / 2);
        HAL_Delay(2);
    }
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void Sound_Speaker_Explosion(void) {
    // SONIDO EXPLOSIÓN
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    // Bajamos desde 1000 (Agudo) hasta 10000 (Muy grave/Click)
    for (int i = 1000; i < 15000; i += 50) {
        __HAL_TIM_SET_AUTORELOAD(&htim3, i);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, i / 2);
        HAL_Delay(3); // Duración de la caída
    }

    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    HAL_Delay(500); // Silencio dramático post-explosión
}


void Sound_Play_Tone(uint16_t frequency, uint16_t duration_ms) {
    if (frequency > 0) {
        // Calculamos periodo para 1MHz (Prescaler 83)
        uint32_t period = 1000000 / frequency;

        __HAL_TIM_SET_AUTORELOAD(&htim3, period);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, period / 2);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    }
    HAL_Delay(duration_ms);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}
