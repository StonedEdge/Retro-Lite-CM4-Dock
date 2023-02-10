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

  // Old repeated values 
  char old_SD_usage[1024];
  char old_CPU_temp[1024];
  char old_CLK_speed[1024];
  char old_RAM_usage[1024];
  char old_IP_addr[1024];

while (1) {
  // 1) Get new values
  getOLEDvalues(); 

  // 3) Clear values on OLED
  clearOLEDvalues(); 

  // 2) Store new values in old values
  storeNewvalues(); 

  // 4) Immediately update values on OLED
  updateNewvalues(); 
  
  // 5) Update OLED screen 
  SSD1351_update();

  // 6) Wait 5 seconds before loop
  sleep_ms(5000);
  }

  return 0;
}
