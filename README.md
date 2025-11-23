INSTRUCCIONES DE INSTALACIÓN:

1. DESCARGAR
2. IMPORTAR: En STM32CubeIDE, id a File > Import > Existing Projects into Workspace y seleccionad la carpeta.
Si faltan archivos de configuración, haced doble clic en el archivo .ioc para que se regenere todo.
3. CONFIGURACIÓN CRÍTICA DEL RELOJ:
Abrid el archivo .ioc.
Id a la pestaña Clock Configuration.
En la caja central HCLK (MHz), escribid 84 y dadle a Enter.

MAPA DE PINES RESERVADOS:
PB10 / PB11: Pantalla LCD (I2C2).
PA6: Zumbador (PWM).
PA0: Botón de Inicio (Botón Azul placa).


FUNCIONAMIENTO DEL CODIGO:
Cread vuestros propios archivos .c y .h (ej: minijuego_simon.c, minijuego_cables.c).
Programad vuestra lógica ahí y llamadla desde el main.

Para decir si el minijuego ha sido ganado o fallado, usad las funciones que he preparado. 
Primero, poned esto arriba de vuestro archivo:
#include "game_master.h"
Cuando ganeis vuestro minijuego llamad a esta funcion:
Game_RegisterWin(FACE_ID);

IDs disponibles:
FACE_SAFE (Caja fuerte / Potenciómetro)
FACE_WIRES (Cables)
FACE_SIMON (Simón Dice)
FACE_MORSE (Morse)
FACE_GYRO (Giroscopio / Acelerómetro)

Ejemplo: Game_RegisterWin(FACE_SIMON); -> La bomba pitará, marcará esa cara como OK y restará 1 al contador de módulos pendientes.

Si el jugador falla llamad a esta función:
Game_RegisterMistake();
Esto resta automáticamente 10 segundos de vida y hace sonar la alarma de error.
