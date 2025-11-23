#ifndef INC_SOUNDS_H_
#define INC_SOUNDS_H_

#include "main.h"

// Definición de notas musicales (Frecuencias en Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// Funciones
void Sound_Init(void);
void Sound_PlayTone(uint16_t frequency, uint16_t duration_ms);
void Sound_PlayBeep(void);
void Sound_PlayStart(void);
void Sound_PlayError(void);
void Sound_PlayPanic(void);
void Sound_PlayExplosion(void);
void Sound_PlayWin(void);
void Sound_PlayCaraResuelta(void);

#endif /* INC_SOUNDS_H_ */
