#ifndef BALANCE_CAR_OLED_SSD1306_H
#define BALANCE_CAR_OLED_SSD1306_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define OLED_WIDTH     128U
#define OLED_HEIGHT    64U
#define OLED_PAGES     8U

HAL_StatusTypeDef Oled_Init(void);
HAL_StatusTypeDef Oled_RefreshNextPage(void);
void Oled_Clear(void);
void Oled_WriteString(uint8_t x, uint8_t page, const char *text);
uint8_t Oled_GetAddress7Bit(void);
uint8_t Oled_GetProbeMask(void);
uint8_t Oled_GetFailStep(void);

#endif
