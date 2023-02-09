#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "ssd1351.h"

int main(void){

  // GPIO Set-Up
  spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  gpio_init(CS);
  gpio_set_dir(CS, GPIO_OUT);
  gpio_put(CS, 1);
  gpio_set_function (SCK, GPIO_FUNC_SPI);
  gpio_set_function (MOSI, GPIO_FUNC_SPI);
  gpio_pull_up(SCK);
  gpio_pull_up(MOSI);
  gpio_init(DC);
  gpio_set_dir(DC,GPIO_OUT);
  gpio_init(RST);
  gpio_set_dir(RST,GPIO_OUT);

  // SPI Initialisation
  stdio_init_all(); 
  spi_init(SPI_PORT, 10000000);
  SSD1351_init();

  // Initialise display
  SSD1351_clear();
  SSD1351_update();
  SSD1351_fill(COLOR_BLACK);
  SSD1351_update();

  // Title colour
  int r = 255;
  int g = 255;
  int b = 255;

  // Value colour
  int r1 = 180;
  int g1 = 180;
  int b1 = 180;
  
  // Buffer to hold data read over USB from Pi
  char buffer[1024];

  // Stats to read from Pi and show on OLED
  char SD_usage[1024];      // A
  char CPU_temp[1024];      // B
  char CLK_speed[1024];     // C
  char RAM_usage[1024];     // D
  char IP_addr[1024];       // E

while (1) {
  // 1) Get new values
  int i = 0;
  while (i < 5) {
    scanf("%1024s", buffer);

    if (buffer[0] == 'A') {
      strncpy(SD_usage, buffer + 1, strlen(buffer));
      sleep_ms(1);
      i++;
    } else if (buffer[0] == 'B') {
      strncpy(CPU_temp, buffer + 1, strlen(buffer));
      sleep_ms(1);
      i++;
    } else if (buffer[0] == 'C') {
      strncpy(CLK_speed, buffer + 1, strlen(buffer));
      sleep_ms(1);
      i++;
    } else if (buffer[0] == 'D') {
      strncpy(RAM_usage, buffer + 1, strlen(buffer));
      sleep_ms(1);
      i++;
    } else if (buffer[0] == 'E') {
      strncpy(IP_addr, buffer + 1, strlen(buffer));
      sleep_ms(1);
    }
  }

  // 2) Clear values on OLED
  SSD1351_set_cursor(5, 15);
  SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, SD_usage);
  SSD1351_set_cursor(5, 40);
  SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, CPU_temp);
  SSD1351_set_cursor(5, 65);
  SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, CLK_speed);
  SSD1351_set_cursor(5, 90);
  SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, RAM_usage);
  SSD1351_set_cursor(5, 115);
  SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, IP_addr);

  // 3) Immediately update values on OLED
  SSD1351_set_cursor(5, 5);
  SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "SD Usage");
  SSD1351_set_cursor(5, 15);
  SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, SD_usage);//, " %");
  SSD1351_set_cursor(5, 30);
  SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "SoC Temp");
  SSD1351_set_cursor(5, 40);
  SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, CPU_temp, "C");//, " C");
  SSD1351_set_cursor(5, 55);
  SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "Clock Speed");
  SSD1351_set_cursor(5, 65);
  SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, CLK_speed);//, " MHZ");
  SSD1351_set_cursor(5, 80);
  SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "RAM Usage");
  SSD1351_set_cursor(5, 90);
  SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, RAM_usage);//, "%");
  SSD1351_set_cursor(5, 105);
  SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "IP Address");
  SSD1351_set_cursor(5, 115);
  SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, IP_addr);

  SSD1351_update();

  sleep_ms(5000);
  
  return 0;
}
