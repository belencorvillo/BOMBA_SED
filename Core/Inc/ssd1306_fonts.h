#ifndef __SSD1306_FONTS_H__
#define __SSD1306_FONTS_H__

#include "ssd1306.h"

// --- ZONA DE ACTIVACIÓN DE FUENTES ---
// Descomentamos (definimos) las que vamos a usar en el juego
#define SSD1306_INCLUDE_FONT_7x10   // Para textos pequeños
#define SSD1306_INCLUDE_FONT_11x18  // Para títulos
#define SSD1306_INCLUDE_FONT_16x26  // Para el BOOM! grande
// -------------------------------------

#ifdef SSD1306_INCLUDE_FONT_6x8
extern const SSD1306_Font_t Font_6x8;
#endif

#ifdef SSD1306_INCLUDE_FONT_7x10
extern const SSD1306_Font_t Font_7x10;
#endif

#ifdef SSD1306_INCLUDE_FONT_11x18
extern const SSD1306_Font_t Font_11x18;
#endif

#ifdef SSD1306_INCLUDE_FONT_16x26
extern const SSD1306_Font_t Font_16x26;
#endif

#ifdef SSD1306_INCLUDE_FONT_16x24
extern const SSD1306_Font_t Font_16x24;
#endif

#ifdef SSD1306_INCLUDE_FONT_16x15
/** Generated Roboto Thin 15
 * @copyright Google https://github.com/googlefonts/roboto
 * @license This font is licensed under the Apache License, Version 2.0.
*/
extern const SSD1306_Font_t Font_16x15;
#endif

#endif // __SSD1306_FONTS_H__
