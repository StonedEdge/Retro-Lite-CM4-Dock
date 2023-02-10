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
  setup_gpio(); 

  // SPI Initialisation
  setup_spi(); 

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
