# SED-MICROCONTROLADORES
## Instalación
1. Navega hasta la carpeta de tu workspace del stm32cubeide y colocate en ella con el terminal, suele ser algo así: 
```bash
cd C:\Users\TuUsuario\STM32CubeIDE\workspace_1.x
```
2. Copia la URL del repositorio desde la página principal del github:

     (botón verde "Code" -> HTTPS -> copiar)
3. Ejecuta:
```bash
git clone https://github.com/usuario/nombre-del-repo.git
```
4. Abre STM32CubeIDE -> File > Import -> General > Existing Projects into Workspace -> Next -> Select root directory -> Browse -> Carpeta que acabas de clonar en el Paso 1.

⚠️ IMPORTANTE: Asegúrate de que la casilla "Copy projects into workspace" esté DESMARCADA

Debería aparecer el proyecto detectado en la lista "Projects" -> Finish

##  Mapa de Conexiones (Pinout)

Hardware: **STM32F407G-DISC1**

###  | **Etiqueta** | **Pin**  | **Configuración** | **Estado Lógico** |

| **Pantalla LCD** | `PB10` `PB11` | I2C2_SCL, I2C2_SDA | - |

| **Zumbador(PWM)** | `PA6` | GPIO_Output | - |

| **BotónInicio** | `PA0` | GPIO_Input | `1` = ON |

| **LEDsSimonDice** | `PD0` hasta `PD5` | GPIO_Output | `1` = ON |

| **BotonesSimonDice** | `PD6` hasta `PD11` | GPIO_Input | `1` = ON |

| **LEDsRojosDefensaAérea** | `PB0` `PB1` `PB2` `PB4` `PB5` `PB7` | GPIO_Output | `1` = ON |

| **LEDsVerdesDefensaAérea** | `PB12` `PB13` `PE12` hasta `PE15` | GPIO_Output | `1` = ON |

| **InterruptoresDefensaAérea** | `PC0` hasta `PC5` | GPIO_Input | `1` = ON |

| **PulsadorMorse** | `PB14` | GPIO_Input | `1` = ON |

| **LEDsMorse** | `PE0` `PE1` `PE6` | GPIO_Output | `1` = ON |

| **LEDsCajaFuerte** | `PE2` `PE4` `PE5` | GPIO_Output | `1` = ON |

| **PotenciómetrosCajaFuerte** | `PA1` `PA2` `PA3` | GPIO_Analog | `0` to `4095` |

| **BotonesActivaciónCaras** | `PE7` hasta `PE11` | GPIO_Input | `1` = ON |

| **Altavoz** |  `PC6` | GPIO_Output / TIM3 | `1` = ON |

| **Pantalla TFT** |  CS => `PA8`, RESET=> `PC8`, DC/RS => `PC9`, MOSI=> `PA7`, SCK => `PA5`, VCC => 3.3V, GND => GND, LED => 3.3V |

| **Acelerómetro** | `PB6` `PB9` | I2C2_SCL, I2C2_SDA | - |





## Funcionamiento del Código
Cread vuestros propios archivos .c y .h (ej: minijuego_simon.c, minijuego_cables.c). Programad vuestra lógica ahí y llamadla desde el main.

Para decir si el minijuego ha sido ganado o fallado, usad las funciones que he preparado. Primero, poned esto arriba de vuestro archivo: #include "game_master.h" Cuando ganeis vuestro minijuego llamad a esta funcion: Game_RegisterWin(FACE_ID);

IDs disponibles: FACE_SAFE (Caja fuerte / Potenciómetro) FACE_AIRDEF (Interruptores) FACE_SIMON (Simón Dice) FACE_MORSE (Morse) FACE_GYRO (Giroscopio / Acelerómetro)

Ejemplo: Game_RegisterWin(FACE_SIMON); -> La bomba pitará, marcará esa cara como OK y restará 1 al contador de módulos pendientes.

Si el jugador falla llamad a esta función: Game_RegisterMistake(); Esto resta automáticamente 10 segundos de vida y hace sonar la alarma de error.
