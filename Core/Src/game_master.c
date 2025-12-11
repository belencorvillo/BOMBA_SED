#include "game_master.h"
#include "lcd_i2c.h"
#include "display_7seg.h"
#include "sounds.h"
#include <stdio.h>

extern TIM_HandleTypeDef htim2;

GameContext bomb;

void Game_Init(void) {
    bomb.currentState = STATE_IDLE;
    bomb.timeRemaining = 240; // TIEMPO INICIAL
    bomb.gamesLeft = TOTAL_FACES; //5
    bomb.mistakes = 0;

    // Iniciamos el array de caras
        for(int i=0; i < TOTAL_FACES; i++) {
            bomb.faceSolved[i] = 0;
            bomb.faceState[i] = 0;
        }

    LCD_Init();
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("PROYECTO MICROS");
    LCD_SetCursor(1, 0);
    LCD_Print(" BOMBA");

    Sound_Init();
}

//Función para activar caras con el botón de inicio de cada cara
void Game_ActivateFace(uint8_t face_id) {
    // Solo activamos si estaba en estado 0 (Pendiente)
    if (bomb.faceSolved[face_id] == 0 && bomb.faceState[face_id] == 0) {

        bomb.faceState[face_id] = 1; // 1 = ACTIVO (Jugando)

        //meter efecto de sonido de activación (dramático)

        //meter efecto en pantalla OLED
    }
}

void Game_Update(void) {

    // --- GESTIÓN DEL TIEMPO Y SONIDOS ---
    if (bomb.tick_flag == 1) {
        bomb.tick_flag = 0;

        // Si el tiempo ya es 0, no hacemos nada más aquí (ya explotó)
        if (bomb.currentState == STATE_EXPLOSION) return;

        // Actualizamos el reloj en el LCD
        char time_str[16];
        uint8_t m = bomb.timeRemaining / 60;
        uint8_t s = bomb.timeRemaining % 60;
        sprintf(time_str, "TIEMPO: %02d:%02d", m, s);
        LCD_SetCursor(1, 0);
        LCD_Print(time_str);


        if (bomb.timeRemaining == 0) {

             bomb.currentState = STATE_EXPLOSION;

             LCD_Clear();
             LCD_SetCursor(0, 2);
             LCD_Print("!!! BOOM !!!");
             LCD_SetCursor(1, 2);
             LCD_Print("GAME OVER");

             Sound_PlayExplosion(); // ¡EL SONIDO POTENTE!
        }
        else if (bomb.timeRemaining <= 30) {
            Sound_PlayPanic(); // Pánico
        }
        else {
            Sound_PlayBeep(); // Normal
        }
    }

    switch (bomb.currentState) {
        case STATE_IDLE:
            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
                bomb.currentState = STATE_COUNTDOWN;
                HAL_TIM_Base_Start_IT(&htim2);
                LCD_Clear();
                LCD_SetCursor(0, 0);
                LCD_Print(" DETONATION IN: ");
                Sound_PlayStart();
                // Pequeña espera para que no detecte el botón pulsado dos veces
                HAL_Delay(500);
            }
            break;

        case STATE_COUNTDOWN:

        	// Escaneo de botones de activación de minijuego (PE7 - PE11)

        		if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7) == GPIO_PIN_RESET) Game_ActivateFace(FACE_SIMON);
				else if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_8) == GPIO_PIN_RESET) Game_ActivateFace(FACE_MORSE);
				else if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9) == GPIO_PIN_RESET) Game_ActivateFace(FACE_SAFE);
				else if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10) == GPIO_PIN_RESET) Game_ActivateFace(FACE_AIRDEF);
				else if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_11) == GPIO_PIN_RESET) Game_ActivateFace(FACE_GYRO);

            break;

        case STATE_EXPLOSION:
            //  poner LEDs parpadeando EN ROJO

            break;

        case STATE_DEFUSED:
            // poner LEDs parpadeando en verde y cancion victoria
            break;
    }
}


void Game_TimerTick(void) {
    if (bomb.currentState == STATE_COUNTDOWN) {
        if (bomb.timeRemaining > 0) {
            bomb.timeRemaining--;
            bomb.tick_flag = 1;
        } else {
            // Si llega a 0, también levantamos la bandera para que
            // el Game_Update se entere y haga la explosión.
            bomb.tick_flag = 1;
        }
    }
}

void Game_RegisterWin(uint8_t face_id) {

	//    VICTORIA TOTAL
	if (bomb.gamesLeft == 0) {
	    bomb.currentState = STATE_DEFUSED;

	    LCD_Clear();
	    LCD_SetCursor(0, 0);
	    LCD_Print(" BOMBA DESACTIVADA");
	    LCD_SetCursor(1, 0);
	    LCD_Print("  BUEN TRABAJO!!");
	    Sound_PlayWin();
	}


	//                VICTORIA DE UNA CARA


	// Solo hacemos caso si estamos jugando y esa cara NO estaba ya resuelta
	if (bomb.currentState == STATE_COUNTDOWN) {

		// Verificamos que el ID es válido y que no la habían resuelto ya
		if (face_id < TOTAL_FACES && bomb.faceSolved[face_id] == 0) {

			//  Marcamos cara específica como resuelta
			bomb.faceSolved[face_id] = 1;
			//Desactivamos estado
			bomb.faceState[face_id] = 0;

			//  Restamos uno al contador global
			bomb.gamesLeft--;

			// CÓDIGO DE LUCES
			// LED_SetFaceColor(face_id, COLOR_GREEN);

			// Sonido de cara resulta
			Sound_PlayCaraResuelta();

			//Mensaje de éxito en la OLED
		}
}
}

void Game_RegisterMistake(void) {
    if (bomb.currentState == STATE_COUNTDOWN) {
        bomb.mistakes++;

        // PENALIZACIÓN: Quitar 10 segundos por fallo
        if (bomb.timeRemaining > 10) {
            bomb.timeRemaining -= 10;
        } else {
            bomb.timeRemaining = 0; // Si queda poco, explota ya
        }

        // Feedback sonoro de error
        Sound_PlayError();
    }
}

