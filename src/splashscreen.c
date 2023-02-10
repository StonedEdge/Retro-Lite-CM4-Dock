#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "ssd1351.h"
#include "splash.h"
#include "splash_vid.h"

// extern uint8_t buffer[OLED_BUF_SIZE];

#define sleep_time 16
clock_t start, end;
double cpu_time_used;

int main(void){

  // GPIO Set-Up
  setup_gpio(); 

  // SPI Initialisation
  setup_spi(); 

  // DMA Initialisation 
  setup_dma(dma_tx); 

while(1) {
  for (int i = 0; i < FRAME_COUNT; i++) {
    transfer_data_with_dma();
    start = clock();
    dma_channel_wait_for_finish_blocking(dma_tx);
    end  = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC; 
    // Integer division is used, if more precision is required you can use time_us_64 if mem permits
    sleep_ms( cpu_time_used - sleep_time );
  } 
}
  
return 0;
  
}

