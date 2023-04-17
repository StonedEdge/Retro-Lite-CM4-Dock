#include "ssd1351.h"

/* Buffer to hold the Display RAM Data */
static DRAM displayRAM;

#define DRAM_16 displayRAM.halfw
#define DRAM_8 displayRAM.byte

uint8_t buffer[OLED_BUF_SIZE] = {0x00};

// Screen cursor for printing
struct cursor{
  uint8_t x;
  uint8_t y;
}SSD1351_cursor;

void SSD1351_SendByte(uint8_t data){
  spi_write_blocking(SPI_PORT, &data, 1);
}

void SSD1351_SendBuffer(uint8_t *buffer, uint16_t len){
  spi_write_blocking(SPI_PORT, buffer, len);
}

void SSD1351_write_command(uint8_t cmd){
  gpio_put(DC, 0);
  gpio_put(CS, 0);
  SSD1351_SendByte(cmd);
  gpio_put(CS, 1);
}

void SSD1351_write_data(uint8_t data){
  gpio_put(DC, 1);
  gpio_put(CS, 0);
  SSD1351_SendByte(data);
  gpio_put(CS, 1);
}

void SSD1351_write_data_buffer(uint8_t *data, uint16_t len){
  gpio_put(DC, 1);
  gpio_put(CS, 0);
  SSD1351_SendBuffer(data, len);
  gpio_put(CS, 1);
}

void SSD1351_update(void){
  SSD1351_write_command(SSD1351_CMD_SETCOLUMN);
  SSD1351_write_data(0x00);
  SSD1351_write_data(0x7F);
  SSD1351_write_command(SSD1351_CMD_SETROW);
  SSD1351_write_data(0x00);
  SSD1351_write_data(0x7F);
  SSD1351_write_command(SSD1351_CMD_WRITERAM);
  SSD1351_write_data_buffer(DRAM_8, DRAM_SIZE_8);
}

void SSD1351_init(void){
  gpio_put(DC, 1);
  gpio_put(RST, 1);
  sleep_ms(10);
  gpio_put(RST, 0);
  sleep_ms(500);
  gpio_put(RST, 1);
  gpio_put(DC, 0);
  sleep_ms(50);
 
  SSD1351_write_command(SSD1351_CMD_COMMANDLOCK);
  SSD1351_write_data(0x12);
  SSD1351_write_command(SSD1351_CMD_COMMANDLOCK);
  SSD1351_write_data(0xB1);

  SSD1351_write_command(SSD1351_CMD_DISPLAYOFF);
  SSD1351_write_command(SSD1351_CMD_CLOCKDIV);
  SSD1351_write_data(0xF1);
  SSD1351_write_command(SSD1351_CMD_MUXRATIO);
  SSD1351_write_data(127);
  sleep_ms(600);

  SSD1351_write_command(SSD1351_CMD_SETREMAP);
  SSD1351_write_data(0x74);

  SSD1351_write_command(SSD1351_CMD_SETCOLUMN);
  SSD1351_write_data(0x00);
  SSD1351_write_data(0x7F);

  SSD1351_write_command(SSD1351_CMD_SETROW);
  SSD1351_write_data(0x00);
  SSD1351_write_data(0x7F);

  SSD1351_write_command(SSD1351_CMD_STARTLINE);
  SSD1351_write_data(0x00);

  SSD1351_write_command(SSD1351_CMD_DISPLAYOFFSET);
  SSD1351_write_data(0x00);

  SSD1351_write_command(SSD1351_CMD_SETGPIO);
  SSD1351_write_data(0x00);

  SSD1351_write_command(SSD1351_CMD_FUNCTIONSELECT);
  SSD1351_write_data(0x01);

  SSD1351_write_command(SSD1351_CMD_PRECHARGE);
  SSD1351_write_data(0x32);

  SSD1351_write_command(SSD1351_CMD_VCOMH);
  SSD1351_write_data(0x05);

  SSD1351_write_command(SSD1351_CMD_NORMALDISPLAY);

  SSD1351_write_command(SSD1351_CMD_CONTRASTABC);
  SSD1351_write_data(0xC8);                          // Color A: Blue
  SSD1351_write_data(0x80);                          // Color B: Green
  SSD1351_write_data(0xC8);                          // Color C: Red

  SSD1351_write_command(SSD1351_CMD_CONTRASTMASTER);
  SSD1351_write_data(0x0F);

  SSD1351_write_command(SSD1351_CMD_SETVSL);
  SSD1351_write_data(0xA0);
  SSD1351_write_data(0xB5);
  SSD1351_write_data(0x55);

  SSD1351_write_command(SSD1351_CMD_PRECHARGE2);
  SSD1351_write_data(0x01);

  SSD1351_write_command(SSD1351_CMD_DISPLAYON);

  sleep_ms(10);
}

void SSD1351_clear(void){
  memset(DRAM_16, 0, DRAM_SIZE_16);
}

void SSD1351_clear_8(void){
  memset(DRAM_8, 0, DRAM_SIZE_8);
}

void SSD1351_fill(uint16_t color){
  for (int i = 0; i < DRAM_SIZE_16; i++){
    DRAM_16[i] = color;
  }
}

/*
 * @brief Converts from RGB to a single 16bit value
 * @param r: starting x coordinate
 * @para g: starting y coordinate
 * @param b: width of the rectangle
 * @reval 16bit value with the rgb color for display
 */
uint16_t SSD1351_get_rgb(uint8_t r, uint8_t g, uint8_t b){
  uint16_t rgb_color = 0;
  rgb_color |= ((r/8) << 8);
  rgb_color |= ((g/4) >> 3);
  rgb_color |= (((g/4) % 0x07) << 13);
  rgb_color |= ((b/8) << 3);
  return rgb_color;
}

/**
 * @brief Writes a pixel data to the screen RAM buffer
 * @param color: Unsigned int16 containing color code
 * @param x: Pixel's horizontal position
 * @param y: Pixel's vertical position
 * @retval None
 */
void SSD1351_write_pixel(int16_t x, int16_t y, uint16_t color){
  if ( x > 127 || y > 127 || x < 0 || y < 0){
    return;
  }
  int a = (x - 127) + (y * 128);
  DRAM_16[a] = color;
}

static void SSD1351_write_char(uint16_t color, font_t font, char c){
  uint16_t fd;
  if ((COLUMNS <= SSD1351_cursor.x + font.width) || (ROWS <= SSD1351_cursor.y + font.height)){
    return;
  }
  if (c == '\n'){
    SSD1351_cursor.x = 127;
  }
  else{
    for (int i = 0; i < font.height; i++){
        fd = font.data[(c - 32) * font.height + i];
        for (int j = 0; j < font.width; j++){
          if ((fd << j) & 0x8000){
            SSD1351_write_pixel(SSD1351_cursor.x + j, SSD1351_cursor.y + i, color);
          }
        }
      }
  }
  SSD1351_cursor.x += font.width;
  if ((SSD1351_cursor.x + font.width >= 127) & (SSD1351_cursor.y + font.height <= 127)){
    SSD1351_cursor.y = SSD1351_cursor.y + font.height + 2;
    SSD1351_cursor.x = 0;
  }
  return;
}

static void SSD1351_write_string(uint16_t color, font_t font, char *line){
  if (line == NULL){
    return;
  }
  while (*line != 0){
    SSD1351_write_char(color, font, *line);
    line++;
  }
}

static void SSD1351_write_int(uint16_t color, font_t font, int8_t n){
  char number[5];
  sprintf(number, "%i", n);
  SSD1351_write_string(color, font, number);
}

/*
 * @brief Prints a formatted string to the display
 * @param color: unsigned integer for the color of the string
 * @param font: structure holding the type of font
 * @param format: formatted string
 */
void SSD1351_printf(uint16_t color, font_t font, const char *format, ...){
  if (format == NULL){
    return;
  }
  va_list valist;
  va_start(valist, format);
  while (*format != 0){
    if (*format != '%'){
      SSD1351_write_char(color, font, *format);
      format++;
    }
    else{
      format++;
      switch(*format){
        case 's':
          SSD1351_write_string(color, font, va_arg(valist, char *));
          break;
        case 'c':
          SSD1351_write_char(color, font, va_arg(valist, int)); //?
          break;
        case 'i':
          SSD1351_write_int(color, font, (int8_t)va_arg(valist, int));
          break;
        default:
          break;
      }
      format++;
    }
  }
}

void SSD1351_set_cursor(uint8_t x, uint8_t y){
  SSD1351_cursor.x = x;
  SSD1351_cursor.y = y;
}

// void SSD1351_write_image(void){
//   int i = 0;
//   while (i < DRAM_SIZE_8) {
//     int data = getchar_timeout_us(100000);
//     if (data != PICO_ERROR_TIMEOUT) {
//         DRAM_8[i++] = (uint8_t)data;
//     }
//   }
// }

void SSD1351_get_image(uint8_t buf[OLED_BUF_SIZE]){
  SSD1351_clear();
  int i = 0;
  while (i < DRAM_SIZE_8) {
    int data = getchar_timeout_us(0);
    if (data != PICO_ERROR_TIMEOUT) {
        buf[i++] = (uint16_t)data;
    }
  }
  // sleep_ms(100);
  // memcpy(DRAM_8, buf, DRAM_SIZE_8);
}

void SSD1351_display_image(uint8_t buf[OLED_BUF_SIZE]) {
  SSD1351_write_command(SSD1351_CMD_WRITERAM);
  // spi_write_blocking(SPI_PORT, buf, OLED_BUF_SIZE);
  memcpy(DRAM_8, buf, DRAM_SIZE_8);
  SSD1351_update();
}
