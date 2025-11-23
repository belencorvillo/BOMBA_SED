/*
 * lcd_i2c.h
 * Driver para LCD 1602 con módulo I2C (PCF8574)
 */

#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "main.h"

// Funciones de usuario
void LCD_Init(void);
void LCD_Clear(void);
void LCD_PutChar(char c);
void LCD_Print(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);

#endif /* INC_LCD_I2C_H_ */
