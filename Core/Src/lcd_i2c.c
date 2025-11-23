/*
 * lcd_i2c.c
 */

#include "lcd_i2c.h"

// Importamos la variable hi2c1 que está definida en main.c
extern I2C_HandleTypeDef hi2c2;

// Dirección del dispositivo I2C (0x27 es la más común, si falla prueba 0x3F)
// Se desplaza 1 bit a la izquierda porque el bit 0 es R/W


//#define LCD_ADDR (0x27 << 1)
#define LCD_ADDR (0x27 << 1)

// Comandos internos
void lcd_send_cmd(char cmd) {
    char data_u, data_l;
    uint8_t data_t[4];

    // Separamos el comando en dos partes (nibbles)
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);

    // Enviamos parte alta con EN=1 y luego EN=0 (Pulso)
    data_t[0] = data_u | 0x0C;  // en=1, rs=0
    data_t[1] = data_u | 0x08;  // en=0, rs=0

    // Enviamos parte baja con EN=1 y luego EN=0
    data_t[2] = data_l | 0x0C;  // en=1, rs=0
    data_t[3] = data_l | 0x08;  // en=0, rs=0

    HAL_I2C_Master_Transmit(&hi2c2, LCD_ADDR, (uint8_t *)data_t, 4, 100);
}

void lcd_send_data(char data) {
    char data_u, data_l;
    uint8_t data_t[4];

    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);

    // Lo mismo pero con RS=1 (porque es dato, no comando)
    data_t[0] = data_u | 0x0D;  // en=1, rs=1
    data_t[1] = data_u | 0x09;  // en=0, rs=1
    data_t[2] = data_l | 0x0D;  // en=1, rs=1
    data_t[3] = data_l | 0x09;  // en=0, rs=1

    HAL_I2C_Master_Transmit(&hi2c2, LCD_ADDR, (uint8_t *)data_t, 4, 100);
}

// --- Funciones Públicas ---

void LCD_Init(void) {
    // Inicialización según hoja de datos Hitachi HD44780
    HAL_Delay(50);
    lcd_send_cmd(0x30);
    HAL_Delay(5);
    lcd_send_cmd(0x30);
    HAL_Delay(1);
    lcd_send_cmd(0x30);
    HAL_Delay(10);
    lcd_send_cmd(0x20); // Modo 4 bits
    HAL_Delay(10);

    // Configuración básica
    lcd_send_cmd(0x28); // Function set: DL=4bit, N=2 line, F=5x8 dots
    HAL_Delay(1);
    lcd_send_cmd(0x08); // Display off
    HAL_Delay(1);
    lcd_send_cmd(0x01); // Clear display
    HAL_Delay(1);
    lcd_send_cmd(0x06); // Entry mode set
    HAL_Delay(1);
    lcd_send_cmd(0x0C); // Display on, Cursor off
}

void LCD_Clear(void) {
    lcd_send_cmd(0x01); // Comando borrar
    HAL_Delay(2);       // Este comando es lento, necesita espera
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t mask;
    // La fila 0 empieza en 0x80, la fila 1 en 0xC0
    mask = (row == 0) ? 0x80 : 0xC0;
    lcd_send_cmd(mask | col);
}

void LCD_PutChar(char c) {
    lcd_send_data(c);
}

void LCD_Print(char *str) {
    while (*str) {
        lcd_send_data(*str++);
    }
}
