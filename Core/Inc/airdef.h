/*
 * airdef.h
 *
 * Created on: Jan 23, 2025
 * Author: Jorge
 * Descripción: Control de la Cara 4 (Defensa Aérea / Interruptores Top Gun)
 */

#ifndef INC_AIRDEF_H_
#define INC_AIRDEF_H_

#include "main.h"

// Configura los pines y el estado inicial
void AirDef_Init(void);

// Contiene la lógica del juego (se llama constantemente en el while)
void AirDef_Loop(void);

#endif /* INC_AIRDEF_H_ */
