#ifndef INC_SIMONDICE_H_
#define INC_SIMONDICE_H_

#include "main.h"

void SimonDice_Init(void);
void SimonDice_Loop(void);

void SimonDice_Boton_Handler(uint16_t GPIO_Pin); //para usar en el callback

#endif /* INC_SIMONDICE_H_ */
