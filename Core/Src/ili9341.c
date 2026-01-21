/* vim: set ai et ts=4 sw=4: */
#include "stm32f4xx_hal.h"
#include "ili9341.h"
#include "stdlib.h"
#include <string.h>

static void ILI9341_Select() {
    HAL_GPIO_WritePin(ILI9341_CS_GPIO_Port, ILI9341_CS_Pin, GPIO_PIN_RESET);
}

void ILI9341_Unselect() {
    HAL_GPIO_WritePin(ILI9341_CS_GPIO_Port, ILI9341_CS_Pin, GPIO_PIN_SET);
}

static void ILI9341_Reset() {
    HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_SET);
}

static void ILI9341_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&ILI9341_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

static void ILI9341_WriteData(uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_SET);

    // split data in small chunks because HAL can't send more then 64K at once
    while(buff_size > 0) {
        uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
        HAL_SPI_Transmit(&ILI9341_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
        buff += chunk_size;
        buff_size -= chunk_size;
    }
}

static void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // column address set
    ILI9341_WriteCommand(0x2A); // CASET
    {
        uint8_t data[] = { (x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF };
        ILI9341_WriteData(data, sizeof(data));
    }

    // row address set
    ILI9341_WriteCommand(0x2B); // RASET
    {
        uint8_t data[] = { (y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF };
        ILI9341_WriteData(data, sizeof(data));
    }

    // write to RAM
    ILI9341_WriteCommand(0x2C); // RAMWR
}

// Función auxiliar privada SOLO para la inicialización (envía 1 byte)
static void Init_WriteData(uint8_t data) {
    HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&ILI9341_SPI_PORT, &data, 1, 10);
}

void ILI9341_Init(void) {
    ILI9341_Select();
    ILI9341_Unselect();

        //  Reset Físico (Despierta a la pantalla a la fuerza)
        HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_RESET); // Baja a 0V
        HAL_Delay(200); // Espera 200ms
        HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_SET);   // Sube a 3.3V
        HAL_Delay(200); // Espera a que arranque

        ILI9341_Select();

    // Reset por Software
    ILI9341_WriteCommand(0x01);
    HAL_Delay(1000);

    // Power Control A
    ILI9341_WriteCommand(0xCB);
    Init_WriteData(0x39);
    Init_WriteData(0x2C);
    Init_WriteData(0x00);
    Init_WriteData(0x34);
    Init_WriteData(0x02);

    // Power Control B
    ILI9341_WriteCommand(0xCF);
    Init_WriteData(0x00);
    Init_WriteData(0xC1);
    Init_WriteData(0x30);

    // Driver timing control A
    ILI9341_WriteCommand(0xE8);
    Init_WriteData(0x85);
    Init_WriteData(0x00);
    Init_WriteData(0x78);

    // Driver timing control B
    ILI9341_WriteCommand(0xEA);
    Init_WriteData(0x00);
    Init_WriteData(0x00);

    // Power on sequence control
    ILI9341_WriteCommand(0xED);
    Init_WriteData(0x64);
    Init_WriteData(0x03);
    Init_WriteData(0x12);
    Init_WriteData(0x81);

    // Pump ratio control
    ILI9341_WriteCommand(0xF7);
    Init_WriteData(0x20);

    // Power Control 1
    ILI9341_WriteCommand(0xC0);
    Init_WriteData(0x23);

    // Power Control 2
    ILI9341_WriteCommand(0xC1);
    Init_WriteData(0x10);

    // VCOM Control 1
    ILI9341_WriteCommand(0xC5);
    Init_WriteData(0x3E);
    Init_WriteData(0x28);

    // VCOM Control 2
    ILI9341_WriteCommand(0xC7);
    Init_WriteData(0x86);

    // Memory Access Control (Rotación)
    ILI9341_WriteCommand(0x36);
    Init_WriteData(ILI9341_ROTATION);

    // Pixel Format Set
    ILI9341_WriteCommand(0x3A);
    Init_WriteData(0x55);

    // Frame Rate Control
    ILI9341_WriteCommand(0xB1);
    Init_WriteData(0x00);
    Init_WriteData(0x18);

    // Display Function Control
    ILI9341_WriteCommand(0xB6);
    Init_WriteData(0x08);
    Init_WriteData(0x82);
    Init_WriteData(0x27);

    // Sleep Out
    ILI9341_WriteCommand(0x11);
    HAL_Delay(120);

    // Display ON
    ILI9341_WriteCommand(0x29);

    ILI9341_Unselect();
}

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT))
        return;

    ILI9341_Select();

    ILI9341_SetAddressWindow(x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ILI9341_WriteData(data, sizeof(data));

    ILI9341_Unselect();
}

static void ILI9341_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;

    ILI9341_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                uint8_t data[] = { color >> 8, color & 0xFF };
                ILI9341_WriteData(data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                ILI9341_WriteData(data, sizeof(data));
            }
        }
    }
}

void ILI9341_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
    ILI9341_Select();

    while(*str) {
        if(x + font.width >= ILI9341_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= ILI9341_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        ILI9341_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    ILI9341_Unselect();
}

void ILI9341_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
    if((x + w - 1) >= ILI9341_WIDTH) w = ILI9341_WIDTH - x;
    if((y + h - 1) >= ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    ILI9341_Select();
    ILI9341_SetAddressWindow(x, y, x+w-1, y+h-1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_SET);
    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            HAL_SPI_Transmit(&ILI9341_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
        }
    }

    ILI9341_Unselect();
}

void ILI9341_FillScreen(uint16_t color) {
    ILI9341_FillRectangle(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ILI9341_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
    if((x + w - 1) >= ILI9341_WIDTH) return;
    if((y + h - 1) >= ILI9341_HEIGHT) return;

    ILI9341_Select();
    ILI9341_SetAddressWindow(x, y, x+w-1, y+h-1);
    ILI9341_WriteData((uint8_t*)data, sizeof(uint16_t)*w*h);
    ILI9341_Unselect();
}

void ILI9341_InvertColors(bool invert) {
    ILI9341_Select();
    ILI9341_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
    ILI9341_Unselect();
}

// Algoritmo de Bresenham para dibujar líneas
void ILI9341_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0);
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;

    while (1) {
        ILI9341_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Función que he añadido para textos centrados:

void DrawCenteredText(uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint16_t width = strlen(str) * font.width;
    uint16_t x = (ILI9341_WIDTH - width) / 2;
    if (x < 0) x = 0; // Protección
    ILI9341_WriteString(x, y, str, font, color, bgcolor);
}
