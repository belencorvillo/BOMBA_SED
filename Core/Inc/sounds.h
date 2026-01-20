#ifndef INC_SOUNDS_H_
#define INC_SOUNDS_H_

#include "main.h"

// --- NOTAS MUSICALES (Para el Altavoz PWM) ---
// Periodo en microsegundos (1MHz reloj)
#define NOTE_C4  3822
#define NOTE_D4  3405
#define NOTE_E4  3033
#define NOTE_F4  2863
#define NOTE_G4  2551
#define NOTE_A4  2272
#define NOTE_B4  2024
#define NOTE_C5  1911
#define NOTE_D5  1702
#define NOTE_E5  1516
#define NOTE_F5  1431
#define NOTE_G5  1275
#define NOTE_A5  1136
#define NOTE_B5  1012
#define NOTE_C6  955

// --- FUNCIONES ---

void Sound_Init(void);
void Sound_Play_Tone(uint16_t frequency, uint16_t duration_ms);

// BUZZER (PA6) - Sonidos Tácticos
void Sound_Buzzer_Beep(void);         // Bip normal (1s)
void Sound_Buzzer_DoubleBeep(void);   // Bip rápido (Pánico)
void Sound_Buzzer_Arming(void);       // El sonido de "tata taaa" del botón azul

// ALTAVOZ (PC6 - PWM) - Sonidos Dramáticos
void Sound_Speaker_Startup(void);     // Fanfarria de inicio
void Sound_Speaker_Siren(void);       // Sirena de pánico
void Sound_Speaker_Explosion(void);   // BOOM
void Sound_Speaker_WinSmall(void);    // Cara resuelta
void Sound_Speaker_WinTotal(void);    // Juego ganado
void Speaker_Tone(uint16_t period, uint16_t duration_ms);

#endif /* INC_SOUNDS_H_ */
