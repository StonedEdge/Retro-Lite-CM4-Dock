#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "fonts.h"

#define CS    1
#define SCK   2
#define MOSI  3
#define RST   4
#define DC    5
#define BUTTON 6

#define SPI_PORT spi0

#define OLED_WIDTH 128
#define OLED_HEIGHT 128
#define OLED_BUF_SIZE (OLED_WIDTH * OLED_HEIGHT * 2)

#define COLUMNS 128
#define ROWS 128

#define DRAM_SIZE_8 32768
#define DRAM_SIZE_16 16384

typedef union DisplayRAM{
  uint8_t byte[DRAM_SIZE_8];
  uint16_t halfw[DRAM_SIZE_16];
} DRAM;

// SSD1351 Commands
#define SSD1351_CMD_SETCOLUMN       0x15
#define SSD1351_CMD_SETROW          0x75
#define SSD1351_CMD_WRITERAM        0x5C
#define SSD1351_CMD_READRAM         0x5D
#define SSD1351_CMD_SETREMAP        0xA0
#define SSD1351_CMD_STARTLINE       0xA1
#define SSD1351_CMD_DISPLAYOFFSET   0xA2
#define SSD1351_CMD_DISPLAYALLOFF   0xA4
#define SSD1351_CMD_DISPLAYALLON    0xA5
#define SSD1351_CMD_NORMALDISPLAY   0xA6
#define SSD1351_CMD_INVERTDISPLAY   0xA7
#define SSD1351_CMD_FUNCTIONSELECT  0xAB
#define SSD1351_CMD_DISPLAYOFF      0xAE
#define SSD1351_CMD_DISPLAYON       0xAF
#define SSD1351_CMD_PRECHARGE       0xB1
#define SSD1351_CMD_DISPLAYENHANCE  0xB2
#define SSD1351_CMD_CLOCKDIV        0xB3
#define SSD1351_CMD_SETVSL          0xB4
#define SSD1351_CMD_SETGPIO         0xB5
#define SSD1351_CMD_PRECHARGE2      0xB6
#define SSD1351_CMD_SETGRAY         0xB8
#define SSD1351_CMD_USELUT          0xB9
#define SSD1351_CMD_PRECHARGELEVEL  0xBB
#define SSD1351_CMD_VCOMH           0xBE
#define SSD1351_CMD_CONTRASTABC     0xC1
#define SSD1351_CMD_CONTRASTMASTER  0xC7
#define SSD1351_CMD_MUXRATIO        0xCA
#define SSD1351_CMD_COMMANDLOCK     0xFD
#define SSD1351_CMD_HORIZSCROLL     0x96
#define SSD1351_CMD_STOPSCROLL      0x9E
#define SSD1351_CMD_STARTSCROLL     0x9F

// Some color definitions
#define COLOR_RED 0x00F8
#define COLOR_BLUE 0x1F00
#define COLOR_GREEN 0xE007
#define COLOR_YELLOW 0xFF07
#define COLOR_PURPLE 0x1FF8
#define COLOR_AQUA 0xE0FF
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF

void SSD1351_SendByte(uint8_t data);

void SSD1351_SendBuffer(uint8_t *buffer, uint16_t len);

void SSD1351_write_command(uint8_t cmd);

void SSD1351_write_data(uint8_t data);

void SSD1351_write_data_buffer(uint8_t *data, uint16_t len);

void SSD1351_update(void);

void text_update(void);

void SSD1351_init(void);

void SSD1351_clear(void);

void SSD1351_fill(uint16_t color);

uint16_t SSD1351_get_rgb(uint8_t r, uint8_t g, uint8_t b);

void SSD1351_write_pixel(int16_t x, int16_t y, uint16_t color);

static void SSD1351_write_char(uint16_t color, font_t font, char c);

static void SSD1351_write_string(uint16_t color, font_t font, char *line);

static void SSD1351_write_int(uint16_t color, font_t font, int8_t n);

void SSD1351_printf(uint16_t color, font_t font, const char *format, ...);

void SSD1351_set_cursor(uint8_t x, uint8_t y);

void SSD1351_write_image(void);
