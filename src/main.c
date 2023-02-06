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
unsigned int ms_start, ms_stop, ms_elapsed;

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

  // DMA Initialisation
  const uint dma_tx = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(dma_tx);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(SPI_PORT, true));

while(1) {
  for (int i = 0; i < FRAME_COUNT; i++) {
    SSD1351_write_command(SSD1351_CMD_WRITERAM);
    // DMA Data Transfer
    gpio_put(DC, 1);
    
    dma_channel_configure(
        dma_tx, 
        &c,
        &spi_get_hw(SPI_PORT)->dr, // write address
        frame[i],                    // read address
        OLED_BUF_SIZE,             // element count (each element is of size transfer_data_size)
        true                       // start
    );
    //  DMA set up done above in ...
    ms_start = time_us_32() / 1000;
    dma_channel_wait_for_finish_blocking(dma_tx);
    ms_stop = time_us_32() / 1000;
    ms_elapsed = ms_stop - ms_start;
    
    // integer division is used, if more precision is required you can use doubles and time_us_64
    if(sleep_time > ms_elapsed) sleep_ms(sleep_time - ms_elapsed);
      sleep_ms(sleep_time);
  } 
}
  
return 0;
  
}
