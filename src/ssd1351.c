#include "ssd1351.h"

uint8_t buffer[OLED_BUF_SIZE] = {0x00};

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
  SSD1351_write_command(SSD1351_CMD_WRITERAM);
  SSD1351_write_data_buffer(buffer, OLED_BUF_SIZE);
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

void setup_gpio(void) {
  gpio_init(CS);
  gpio_set_dir(CS, GPIO_OUT);
  gpio_put(CS, 1);
  gpio_set_function(SCK, GPIO_FUNC_SPI);
  gpio_set_function(MOSI, GPIO_FUNC_SPI);
  gpio_pull_up(SCK);
  gpio_pull_up(MOSI);
  gpio_init(DC);
  gpio_set_dir(DC, GPIO_OUT);
  gpio_init(RST);
  gpio_set_dir(RST, GPIO_OUT);
}

void setup_spi(void) {
  spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  stdio_init_all(); 
  spi_init(SPI_PORT, 10000000);
}

void setup_dma(const uint dma_tx) {
  dma_channel_config c = dma_channel_get_default_config(dma_tx);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(SPI_PORT, true));
}

void transfer_data_with_dma(const uint dma_tx, const uint8_t *frame) {
  SSD1351_write_command(SSD1351_CMD_WRITERAM);
  gpio_put(DC, 1);

  dma_channel_configure(
      dma_tx,
      &c,
      &spi_get_hw(SPI_PORT)->dr, // write address
      frame,                    // read address
      OLED_BUF_SIZE,             // element count (each element is of size transfer_data_size)
      true                       // start
  );
