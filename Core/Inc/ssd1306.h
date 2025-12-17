#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "stm32f4xx_hal.h"

// --- CONFIGURACIÓN SPI ---
#define STM32F4
#define SSD1306_USE_SPI
// Definimos el puerto SPI que configuraste en el IOC
extern SPI_HandleTypeDef hspi1;
#define SSD1306_SPI_PORT hspi1

// --- DEFINICIÓN DE PINES (Puerto A) ---
#define SSD1306_CS_Port         GPIOA
#define SSD1306_CS_Pin          GPIO_PIN_1

#define SSD1306_DC_Port         GPIOA
#define SSD1306_DC_Pin          GPIO_PIN_2

#define SSD1306_Reset_Port      GPIOA
#define SSD1306_Reset_Pin       GPIO_PIN_4

// --- TAMAÑO DE PANTALLA ---
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
#define SSD1306_BUFFER_SIZE     (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

// --- DEFINICIONES DE TIPOS (Lo que te daba error) ---
typedef enum {
    Black = 0x00,
    White = 0x01
} SSD1306_COLOR;

typedef enum {
    SSD1306_OK = 0x00,
    SSD1306_ERR = 0x01
} SSD1306_Error_t;

typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Initialized;
    uint8_t DisplayOn;
} SSD1306_t;

typedef struct {
    uint8_t x;
    uint8_t y;
} SSD1306_VERTEX;

typedef struct {
	const uint8_t width;
	const uint8_t height;
	const uint16_t *const data;
} SSD1306_Font_t;

// --- FUNCIONES ---
void ssd1306_Init(void);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_UpdateScreen(void);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
char ssd1306_WriteString(char* str, SSD1306_Font_t Font, SSD1306_COLOR color);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color);

#endif // __SSD1306_H__
