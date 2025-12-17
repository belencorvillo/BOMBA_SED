#include "ssd1306.h"
#include <stdlib.h>
#include <string.h>

// Definimos el puerto SPI (confirma que es hspi1)
extern SPI_HandleTypeDef hspi1;
#define TFT_SPI hspi1

// PINES (Confirma con tu .ioc)
#define TFT_CS_PORT    GPIOA
#define TFT_CS_PIN     GPIO_PIN_1
#define TFT_DC_PORT    GPIOA
#define TFT_DC_PIN     GPIO_PIN_2
#define TFT_RST_PORT   GPIOA
#define TFT_RST_PIN    GPIO_PIN_4

// Comandos ST7796
#define TFT_CASET   0x2A
#define TFT_RASET   0x2B
#define TFT_RAMWR   0x2C
#define TFT_MADCTL  0x36
#define TFT_COLMOD  0x3A
#define TFT_SLPOUT  0x11
#define TFT_DISPON  0x29
#define TFT_SWRESET 0x01

static uint16_t cursorX = 0;
static uint16_t cursorY = 0;

// --- Funciones de Bajo Nivel ---
void TFT_WriteCmd(uint8_t cmd) {
    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&TFT_SPI, &cmd, 1, 10);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
}

void TFT_WriteData(uint8_t data) {
    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&TFT_SPI, &data, 1, 10);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
}

void TFT_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    TFT_WriteCmd(TFT_CASET);
    uint8_t d[] = {x0 >> 8, x0, x1 >> 8, x1};
    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&TFT_SPI, d, 4, 10);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);

    TFT_WriteCmd(TFT_RASET);
    uint8_t d2[] = {y0 >> 8, y0, y1 >> 8, y1};
    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&TFT_SPI, d2, 4, 10);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);

    TFT_WriteCmd(TFT_RAMWR);
}

// --- Inicialización y Gráficos ---

void ssd1306_Init(void) {
    // Reset Físico
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);

    // Inicialización
    TFT_WriteCmd(TFT_SWRESET); HAL_Delay(120);
    TFT_WriteCmd(TFT_SLPOUT);  HAL_Delay(120);
    TFT_WriteCmd(TFT_COLMOD);  TFT_WriteData(0x05); // 16-bit color

    // --- ROTACIÓN HORIZONTAL (LANDSCAPE) ---
    // 0xE8 suele ser Landscape (X cambiada por Y, lectura izq-der)
    TFT_WriteCmd(TFT_MADCTL);  TFT_WriteData(0xE8);

    TFT_WriteCmd(TFT_DISPON);  HAL_Delay(50);

    ssd1306_Fill(Black);
}

void ssd1306_Fill(SSD1306_COLOR color) {
    // Pantalla horizontal: 480 ancho, 320 alto
    TFT_SetWindow(0, 0, 479, 319);
    uint16_t c = (color == Black) ? 0x0000 : 0xFFFF;

    // Buffer para ir rápido (una línea de 480px)
    uint8_t line[960];
    for(int i=0; i<480; i++) {
        line[i*2] = c >> 8;
        line[i*2+1] = c;
    }

    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
    for(int y=0; y<320; y++) {
        HAL_SPI_Transmit(&TFT_SPI, line, 960, 100);
    }
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
}

void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    // Escalado x2 (Zoom)
    uint16_t px = x * 2;
    uint16_t py = y * 2;

    // Protección de bordes (480x320)
    if (px >= 478 || py >= 318) return;

    uint16_t c = (color == Black) ? 0x0000 : 0xFFFF;

    TFT_SetWindow(px, py, px+1, py+1);
    uint8_t d[] = {c>>8, c, c>>8, c, c>>8, c, c>>8, c}; // Pixel 2x2

    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&TFT_SPI, d, 8, 10);
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
}

// --- Texto ---
char ssd1306_WriteChar(char ch, SSD1306_Font_t Font, SSD1306_COLOR color) {

	// añadimos esto para que la pantalla entienda los saltos de línea
	    if (ch == '\n') {
	        cursorX = 0;
	        cursorY += Font.height + 2; // +2 pixeles de separación extra
	        return ch;
	    }


    // Ajustamos límite de ancho lógico (240 porque luego se multiplica x2 = 480)
    if (cursorX + Font.width > 238) {
        cursorX = 0;
        cursorY += Font.height;
    }
    for (int i = 0; i < Font.height; i++) {
        uint16_t b = Font.data[(ch - 32) * Font.height + i];
        for (int j = 0; j < Font.width; j++) {
            if ((b << j) & 0x8000) {
                ssd1306_DrawPixel(cursorX + j, cursorY + i, color);
            }
        }
    }
    cursorX += Font.width;
    return ch;
}

char ssd1306_WriteString(char* str, SSD1306_Font_t Font, SSD1306_COLOR color) {
    while (*str) {
        ssd1306_WriteChar(*str, Font, color);
        str++;
    }
    return *str;
}

void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    cursorX = x;
    cursorY = y;
}

void ssd1306_UpdateScreen(void) {} // No necesaria

// --- Bitmap (Bomba) ---
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    uint16_t byteWidth = (w + 7) / 8;
    for(int j=0; j<h; j++) {
        for(int i=0; i<w; i++) {
            uint8_t byte = bitmap[j * byteWidth + i / 8];
            if(byte & (0x80 >> (i & 7))) {
                ssd1306_DrawPixel(x+i, y+j, color);
            }
        }
    }
}

