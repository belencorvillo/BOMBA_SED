#include <fonts.h>
#include <ili9341.h>
#include "game_master.h"
#include "lcd_i2c.h"
#include "sounds.h"
#include "safe.h"
#include "airdef.h"
#include <stdio.h>

//INCLUDES PARA LA PANTALLA OLED


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
		"  PILOTO"  // ID 4 (FACE_GYRO)
};

const char* GameInstructions[] = {
		" Gira los discos cifrados",       // Instrucciones Caja Fuerte (0)
		"      Intercepta los cazas               enemigos", // Instrucciones Def. Aerea (1)
		"    Repite la secuencia de                colores", // Instrucciones Simon (2)
		"   Decodifica la  palabra de             4 letras", // Instrucciones Morse (3)
		"      Evita que la nave se               estrelle"   // Instrucciones Gyro (4)
};

//      FUNCIÓN CONTROL DE LA OLED:

static void Refresh_TFT_Countdown(void) {

    // CASO 1: Estamos mostrando instrucciones (los primeros 15s)
    if (HAL_GetTick() < instructions_end_time) {
        // No hacemos nada, la pantalla es estática y ya se dibujó al activar la cara.
        return;
    }

    //Añadimos esto para que no tenga conflicto con la imagen del giroscopio
    if (bomb.faceState[FACE_GYRO] == 1) {
            return;
        }

    if (bomb.faceState[FACE_MORSE] == 1) {
            //  Usamos anim_frame para pintar solo una vez y no parpadear
            static uint8_t morse_pintado = 0;
            if (morse_pintado == 0) {
                 ILI9341_FillScreen(ILI9341_BLACK);
                 DrawCenteredText(60, "CODIGO MORSE", Font_16x26, ILI9341_CYAN, ILI9341_BLACK);
                 DrawCenteredText(100, "ESCRIBE:", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
                 DrawCenteredText(130, "S O S", Font_16x26, ILI9341_YELLOW, ILI9341_BLACK);
                 DrawCenteredText(170, "... --- ...", Font_16x26, ILI9341_WHITE, ILI9341_BLACK);
                 morse_pintado = 1;
            }
            return; // Salimos para que no pinte Tic Tac
        }

    // CASO 2: Pasaron los 15s -> Animación TIC TAC
    if (HAL_GetTick() - last_anim_time > 500) {
        last_anim_time = HAL_GetTick();
        anim_frame = !anim_frame; // Alternar 0/1


        if (anim_frame) {
        	DrawCenteredText(100, "TIC TAC...", Font_16x26, ILI9341_WHITE, ILI9341_BLACK);
        } else {
        	ILI9341_FillScreen(ILI9341_BLACK);
        }

    }
}

//    FUNCIÓN PARA REINICIAR VARIABLES
static void Reset_Game_Variables(void) {
    bomb.timeRemaining = 180; // Reset Tiempo
    bomb.gamesLeft = TOTAL_FACES; // Reset Juegos
    bomb.mistakes = 0;

    // Reset estado de caras
    for(int i=0; i < TOTAL_FACES; i++) {
        bomb.faceSolved[i] = 0;
        bomb.faceState[i] = 0;
    }

    //Para apagar los leds de las caras que lo necesitan:
   AirDef_Reset();
   Safe_Reset();
   Morse_Reset();
}

//      FUNCIÓN INICIALIZACIÓN:


void Game_Init(void) {
    bomb.currentState = STATE_IDLE;
    last_known_state = STATE_IDLE;

    bomb.timeRemaining = 180; // TIEMPO INICIAL
    bomb.gamesLeft = TOTAL_FACES; //5
    Reset_Game_Variables();

    //Inicializar pantalla LCD I2C
    LCD_Init();
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("PROYECTO MICROS");
    LCD_SetCursor(1, 0);
    LCD_Print(" BOMBA");

    //Inicializar pantalla TFT ILI9341
    ILI9341_Init();
	ILI9341_FillScreen(ILI9341_BLACK);
	ILI9341_DrawLine(10, 10, 310, 10, ILI9341_RED);
	ILI9341_DrawLine(10, 230, 310, 230, ILI9341_RED);
	DrawCenteredText(100, "SISTEMA ARMADO!!", Font_16x26, ILI9341_RED, ILI9341_BLACK);

    Sound_Init();
}

//Función para activar caras con el botón de inicio de cada cara
void Game_ActivateFace(uint8_t face_id) {
    // Solo activamos si estaba en estado 0 (Pendiente)
    if (bomb.faceSolved[face_id] == 0 && bomb.faceState[face_id] == 0) {

        bomb.faceState[face_id] = 1; // 1 = ACTIVO (Jugando)

        // TIEMPOS DE INSTRUCCIONES PERSONALIZADOS
		if (face_id == FACE_MORSE || face_id == FACE_GYRO) {
			// Morse y Gyro: Solo 5 segundos de instrucciones
			instructions_end_time = HAL_GetTick() + 5000;
		} else {
			// Resto de juegos: 15 segundos estándar
			instructions_end_time = HAL_GetTick() + 15000;
		}
        current_instruction_face = face_id;

        //Pintamos las instrucciones de la cara en la TFT
        ILI9341_FillScreen(ILI9341_BLACK);
        //Título cara
        ILI9341_FillRectangle(0, 0, ILI9341_WIDTH, 40, ILI9341_BLUE);
		DrawCenteredText(10, FaceNames[face_id], Font_16x26, ILI9341_WHITE, ILI9341_BLUE);
		//Línea separadora
		ILI9341_DrawLine(0, 45, 320, 45, ILI9341_WHITE);
		//Instrucciones juego
		DrawCenteredText(90, "INSTRUCCIONES:", Font_11x18, ILI9341_YELLOW, ILI9341_BLACK);
		DrawCenteredText(120, GameInstructions[face_id], Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
		// Barra de tiempo visual (Decorativa)
		ILI9341_FillRectangle(20, 200, 280, 10, ILI9341_DARKGREY);

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

	                // Limpiamos la TFT para empezar la cuenta
					ILI9341_FillScreen(ILI9341_BLACK);

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

					ILI9341_FillScreen(ILI9341_BLUE);
					DrawCenteredText(110, "CANCELADO", Font_16x26, ILI9341_WHITE, ILI9341_BLUE);


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

	        	// Refrescar TFT (Tic Tac o Instrucciones)
					Refresh_TFT_Countdown();

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

				   // Pantalla ROJA completa
				   ILI9341_FillScreen(ILI9341_RED);
				   DrawCenteredText(100, "BOOM!", Font_16x26, ILI9341_WHITE, ILI9341_RED);
				   DrawCenteredText(140, "GAME OVER", Font_11x18, ILI9341_WHITE, ILI9341_RED);

				   Sound_Speaker_Explosion();
			    }

			   // TRANSICIÓN A IDLE
			   // Si desenclavan la seta (PA0 == 0)
			   if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
				   Reset_Game_Variables();
				   bomb.currentState = STATE_IDLE;
				   ILI9341_FillScreen(ILI9341_BLACK); // Limpiar rojo
			   }
			   break;

		   case STATE_DEFUSED:
			   if (is_new_state) {
				   LCD_Clear();
				   LCD_SetCursor(0, 0); LCD_Print(" BOMBA DESACTIVADA");
				   LCD_SetCursor(1, 0); LCD_Print("  BUEN TRABAJO!!");

				   // Pantalla VERDE completa
				   ILI9341_FillScreen(ILI9341_GREEN);
				   DrawCenteredText(90, "VICTORIA!", Font_16x26, ILI9341_BLACK, ILI9341_GREEN);
				   DrawCenteredText(130, "BOMBA DESACTIVADA", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);

				   Sound_Speaker_WinTotal();
			   }

			   // TRANSICIÓN A IDLE
			   // Si desenclavan la seta (PA0 == 0)
			   if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
				   Reset_Game_Variables();
				   bomb.currentState = STATE_IDLE;
				   ILI9341_FillScreen(ILI9341_BLACK); // Limpiar verde
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


	//  Verificación de seguridad:
	    // Si no estamos jugando o la cara ya está resuelta, nos vamos.
	    if (bomb.currentState != STATE_COUNTDOWN || bomb.faceSolved[face_id] == 1) {
	        return;
	    }

	    //  Marcar cara como resuelta
	    bomb.faceSolved[face_id] = 1;
	    bomb.faceState[face_id] = 0; // Apagar minijuego
	    bomb.gamesLeft--;            // Restar contador global

	    // ¿Es la última cara o quedan más?

	    if (bomb.gamesLeft == 0) {
	        // CASO A: ERA LA ÚLTIMA -> VICTORIA TOTAL
	        //Vamos al estado de bomba desactivada: STATE_DEFUSED
	        bomb.currentState = STATE_DEFUSED;
	        return;
	    } else {
	        // CASO B: AÚN QUEDAN JUEGOS -> VICTORIA PARCIAL
	        // reproducimos el sonido de "Cara Resuelta"
	        Sound_Speaker_WinSmall();

	        // Mensaje de acierto parcial en pantalla
	        instructions_end_time = HAL_GetTick() + 2000;
	        ILI9341_FillScreen(ILI9341_BLACK);
	        ILI9341_FillRectangle(0, 80, 320, 80, ILI9341_GREEN);
	        DrawCenteredText(110, "MODULO OK!", Font_16x26, ILI9341_BLACK, ILI9341_GREEN);
	        HAL_Delay(1500);
	        ILI9341_FillScreen(ILI9341_BLACK);
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

        // Feedback sonoro y visual de error
        ILI9341_FillRectangle(0, 0, 320, 20, ILI9341_RED);
        Sound_Play_Tone(150, 300);
		ILI9341_FillRectangle(0, 0, 320, 20, ILI9341_BLACK);
    }
}

