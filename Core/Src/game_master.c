#include "game_master.h"
#include "lcd_i2c.h"
#include "sounds.h"
#include <stdio.h>

//INCLUDES PARA LA PANTALLA OLED
#include "ssd1306.h"
#include "ssd1306_fonts.h"


extern TIM_HandleTypeDef htim2;

GameContext bomb;

//         VARIABLES PARA INSTRUCCIONES EN LA PANTALLA
uint32_t instructions_end_time = 0;
uint8_t current_instruction_face = 255; // Texto de instrucciones que mostramos

//        VARIABLES PARA LA ANIMACIÓN DE TIC TAC...
static uint32_t last_anim_time = 0;
static uint8_t anim_frame = 0;

// Variable para detectar cambio de estado (Entry Action)
static BombState last_known_state = STATE_IDLE;

const char* FaceNames[] = {
		"CAJA FUERTE",   // ID 0 (FACE_SAFE)
		"DEFENSA AEREA",    // ID 1 (FACE_AIRDEF)
		" SIMON DICE",    // ID 2 (FACE_SIMON)
		"CODIGO MORSE",  // ID 3 (FACE_MORSE)
		"   PILOTO"  // ID 4 (FACE_GYRO)
};

const char* GameInstructions[] = {
		"\n   Gira los discos\n     cifrados",       // Instrucciones Caja Fuerte (0)
		"\n   Intercepta los\n   cazas enemigos", // Instrucciones Def. Aerea (1)
		"\n  Repite la secuencia\n    de colores", // Instrucciones Simon (2)
		"\n   Decodifica la\n  palabra de 4 letras", // Instrucciones Morse (3)
		"\n     Evita que la\n   nave se estrelle"   // Instrucciones Gyro (4)
};

//      FUNCIÓN CONTROL DE LA OLED:

static void Refresh_OLED_Countdown(void) {

    // CASO 1: Estamos mostrando instrucciones (los primeros 15s)
    if (HAL_GetTick() < instructions_end_time) {
        // No hacemos nada, la pantalla es estática y ya se dibujó al activar la cara.
        return;
    }

    // CASO 2: Pasaron los 15s -> Animación TIC TAC
    if (HAL_GetTick() - last_anim_time > 500) {
        last_anim_time = HAL_GetTick();
        anim_frame = !anim_frame; // Alternar 0/1

        ssd1306_Fill(Black);
        if (anim_frame) {
            ssd1306_SetCursor(40, 67); // Centrado
            ssd1306_WriteString("TIC TAC...", Font_16x26, White);
        }
        ssd1306_UpdateScreen();
    }
}

//    FUNCIÓN PARA REINICIAR VARIABLES
static void Reset_Game_Variables(void) {
    bomb.timeRemaining = 300; // Reset Tiempo
    bomb.gamesLeft = TOTAL_FACES; // Reset Juegos
    bomb.mistakes = 0;

    // Reset estado de caras
    for(int i=0; i < TOTAL_FACES; i++) {
        bomb.faceSolved[i] = 0;
        bomb.faceState[i] = 0;
    }
}

//      FUNCIÓN INICIALIZACIÓN:


void Game_Init(void) {
    bomb.currentState = STATE_IDLE;
    last_known_state = STATE_IDLE;

    bomb.timeRemaining = 300; // TIEMPO INICIAL
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

    // OLED INICIAL
        ssd1306_Init();
        ssd1306_Fill(Black);
        ssd1306_SetCursor(32, 70); //Centramos pantalla
        ssd1306_WriteString("SISTEMA ARMADO!!", Font_11x18, White);
        ssd1306_UpdateScreen();

    Sound_Init();
}

//Función para activar caras con el botón de inicio de cada cara
void Game_ActivateFace(uint8_t face_id) {
    // Solo activamos si estaba en estado 0 (Pendiente)
    if (bomb.faceSolved[face_id] == 0 && bomb.faceState[face_id] == 0) {

        bomb.faceState[face_id] = 1; // 1 = ACTIVO (Jugando)

        //Disparamos temporizador de la oled:
        //Las instrucciones se muestran 15 segundos

        instructions_end_time = HAL_GetTick() + 15000;
        current_instruction_face = face_id;

        //Pintamos las instrucciones de la cara en la OLED
        ssd1306_Fill(Black);
        //Título cara
		ssd1306_SetCursor(20, 20);
		ssd1306_WriteString((char*)FaceNames[face_id], Font_16x26, White);
		//Línea separadora
		for(int i=10; i<230; i++) ssd1306_DrawPixel(i, 50, White); // Línea en Y=50
		//Instrucciones juego
		ssd1306_SetCursor(10, 60);
		ssd1306_WriteString((char*)GameInstructions[face_id], Font_11x18, White);
		ssd1306_UpdateScreen();

    }
}

//       MÁQUINA DE ESTADOS

void Game_Update(void) {

	// Variable local para saber si acabamos de entrar en un estado nuevo
	uint8_t is_new_state = (bomb.currentState != last_known_state);
	last_known_state = bomb.currentState; // Actualizamos para el siguiente ciclo

	switch (bomb.currentState) {

	//ESTADO: BOMBA TODAVÍA NO ARMADA
	        case STATE_IDLE:

	        	//LE DAMOS AL BOTÓN DE INICIO (ARMAR BOMBA)

	            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {

	            	Sound_Buzzer_Arming();
	            	Sound_Speaker_Startup();

	            	bomb.currentState = STATE_COUNTDOWN;
	                HAL_TIM_Base_Start_IT(&htim2);
	                LCD_Clear();
	                LCD_SetCursor(0, 0);
	                LCD_Print(" DETONATION IN: ");

	                // Pequeña espera para que no detecte el botón pulsado dos veces
	                HAL_Delay(500);
	            }
	            break;

	  // ESTADO: CUENTA ATRÁS (Juego Activo)
	        case STATE_COUNTDOWN:

	        	// Si sueltan la seta (PA0 == 0) a mitad de partida
				if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {

					HAL_TIM_Base_Stop_IT(&htim2); // Parar timer hardware

					LCD_Clear();
					LCD_SetCursor(0, 0); LCD_Print("PARTIDA");
					LCD_SetCursor(1, 0); LCD_Print("CANCELADA");
					HAL_Delay(1000);

					Reset_Game_Variables(); // Limpiar datos
					bomb.currentState = STATE_IDLE;
					break; // Salimos del switch
				}



	        	//gestión del tick por segundo:
	        	if (bomb.tick_flag == 1) {
	        	        bomb.tick_flag = 0;

						// Actualizar LCD
						char time_str[16];
						uint8_t m = bomb.timeRemaining / 60;
						uint8_t s = bomb.timeRemaining % 60;
						sprintf(time_str, "TIEMPO: %02d:%02d", m, s);
						LCD_SetCursor(1, 0);
						LCD_Print(time_str);

						// Chequear Fin de Tiempo
						if (bomb.timeRemaining == 0) {
							bomb.currentState = STATE_EXPLOSION;
							break; // Salimos del switch para que en la próxima vuelta entre en EXPLOSION
						}
						// Sonidos según urgencia
						if (bomb.timeRemaining <= 15) {
							Sound_Buzzer_DoubleBeep();
							Sound_Speaker_Siren();
						} else if (bomb.timeRemaining <= 30) {
							Sound_Buzzer_DoubleBeep();
						} else {
							Sound_Buzzer_Beep();
						}
					}

	        	// Refrescar OLED (Tic Tac o Instrucciones)
					Refresh_OLED_Countdown();

				// Escaneo de botones de activación de minijuego (PE7 - PE11)

					if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7) == GPIO_PIN_RESET) Game_ActivateFace(FACE_SIMON);//FACE_SIMON
					else if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_8) == GPIO_PIN_RESET) Game_ActivateFace(FACE_MORSE);//FACE_MORSE
					else if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9) == GPIO_PIN_RESET) Game_ActivateFace(FACE_SAFE);
					else if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10) == GPIO_PIN_RESET) Game_ActivateFace(FACE_AIRDEF);
					else if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_11) == GPIO_PIN_RESET) Game_ActivateFace(FACE_GYRO);

	                   break;
//ESTADO: EXPLOSIÓN (GAME OVER)
		   case STATE_EXPLOSION:
			   //  solo se ejecuta si es la primera vez que entramos (para no entrar en un bucle)
			   if (is_new_state) {
				   LCD_Clear();
				   LCD_SetCursor(0, 2); LCD_Print("!!! BOOM !!!");
				   LCD_SetCursor(1, 2); LCD_Print("GAME OVER");

				   ssd1306_Fill(Black);
				   ssd1306_SetCursor(80, 67);
				   ssd1306_WriteString("BOOM!", Font_16x26, White);
				   ssd1306_UpdateScreen();

				   Sound_Speaker_Explosion();
			    }

			   // TRANSICIÓN A IDLE
			   // Si desenclavan la seta (PA0 == 0)
			   if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
				   Reset_Game_Variables();
				   bomb.currentState = STATE_IDLE;
			   }
			   break;

		   case STATE_DEFUSED:
			   if (is_new_state) {
				   LCD_Clear();
				   LCD_SetCursor(0, 0); LCD_Print(" BOMBA DESACTIVADA");
				   LCD_SetCursor(1, 0); LCD_Print("  BUEN TRABAJO!!");

				   ssd1306_Fill(Black);
				   ssd1306_SetCursor(0, 20);
				   ssd1306_WriteString("VICTORIA!", Font_16x26, White);
				   ssd1306_UpdateScreen();

				   Sound_Speaker_WinTotal();
			   }

			   // TRANSICIÓN A IDLE
			   // Si desenclavan la seta (PA0 == 0)
			   if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
				   Reset_Game_Variables();
				   bomb.currentState = STATE_IDLE;
			   }
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

	//    CASO: VICTORIA TOTAL
	if (bomb.gamesLeft == 0) {
	    bomb.currentState = STATE_DEFUSED;
	    return;
	}


	//     CASO: VICTORIA DE UNA CARA


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

			// Si al restar queda 0, ganamos todo
			if (bomb.gamesLeft == 0) {
				bomb.currentState = STATE_DEFUSED;
				return;
			}

			// CÓDIGO DE LUCES
			// LED_SetFaceColor(face_id, COLOR_GREEN);

			// Sonido de cara resulta
			Sound_Speaker_WinSmall();

			//Mensaje de éxito en la OLED
			instructions_end_time = 0;
			// Mostrar OK en OLED 2 segundos
			ssd1306_Fill(Black);
			ssd1306_SetCursor(10, 20);
			ssd1306_WriteString("MODULO OK!", Font_11x18, White);
			ssd1306_UpdateScreen();
			HAL_Delay(1500);
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
        Sound_Buzzer_Beep();
    }
}

