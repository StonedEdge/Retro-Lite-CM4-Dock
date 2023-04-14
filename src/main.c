#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "ssd1351.h"
#include "splash_vid.h"

#define IMAGE_SIZE_BYTES 32768

extern uint8_t buffer[OLED_BUF_SIZE];

volatile  bool thread_running = false;

void play_splash_vid(void) {
  int t = 0;
    while(t < 20){
      SSD1351_write_command(SSD1351_CMD_WRITERAM);
      SSD1351_write_data_buffer(frame[0], OLED_BUF_SIZE);
      sleep_ms(500);
      for (int f = 1; f < FRAME_COUNT; f++) {
      SSD1351_write_command(SSD1351_CMD_WRITERAM);
      SSD1351_write_data_buffer(frame[f], OLED_BUF_SIZE);
      sleep_ms(10);
      }
      SSD1351_write_command(SSD1351_CMD_WRITERAM);
      SSD1351_write_data_buffer(frame[0], OLED_BUF_SIZE);
      sleep_ms(500);

      t++;
    }
    SSD1351_fill(COLOR_WHITE);
    SSD1351_set_cursor(20, 64);
    SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, "NO CONNECTION");
    SSD1351_update();
  }

void splash_vid_single(void) {
  SSD1351_write_command(SSD1351_CMD_WRITERAM);
  SSD1351_write_data_buffer(frame[0], OLED_BUF_SIZE);
  sleep_ms(500);
  for (int f = 1; f < FRAME_COUNT; f++) {
  SSD1351_write_command(SSD1351_CMD_WRITERAM);
  SSD1351_write_data_buffer(frame[f], OLED_BUF_SIZE);
  sleep_ms(10);
  }
  SSD1351_write_command(SSD1351_CMD_WRITERAM);
  SSD1351_write_data_buffer(frame[0], OLED_BUF_SIZE);
  sleep_ms(500);
  }

void display_image(void) {
  SSD1351_write_command(SSD1351_CMD_WRITERAM);
  spi_write_blocking(SPI_PORT, buffer, OLED_BUF_SIZE);
  SSD1351_update();
}

void display_stats(void) {
  // Stats to read from Pi and show on OLED
  char SD_usage[1024];      // A
  char CPU_temp[1024];      // B
  char CLK_speed[1024];     // C
  char RAM_usage[1024];     // D
  char IP_addr[1024];       // E

  char prev_SD_usage[1024];      // A
  char prev_CPU_temp[1024];      // B
  char prev_CLK_speed[1024];     // C
  char prev_RAM_usage[1024];     // D
  char prev_IP_addr[1024];       // E

  // Buffer to hold data read over USB from Pi
  char stat_buf[1024];

  // Title colour
  int r = 255;
  int g = 255;
  int b = 255;

  // Value colour
  int r1 = 180;
  int g1 = 180;
  int b1 = 180;

  while(thread_running) {
    // Get Values
    
    int s = 0;
    while(s < 6){
      scanf("%16s", stat_buf);
      if (stat_buf[0] == 'S') {
        break;
      }
      else if (stat_buf[0] == 'A') {
        strncpy(SD_usage, stat_buf + 1, strlen(stat_buf));
        s++;
      }
      else if (stat_buf[0] == 'B') {
        strncpy(CPU_temp, stat_buf + 1, strlen(stat_buf));
        s++;
      }
      else if (stat_buf[0] == 'C') {
        strncpy(CLK_speed, stat_buf + 1, strlen(stat_buf));
        s++;
      }
      else if (stat_buf[0] == 'D') {
        strncpy(RAM_usage, stat_buf + 1, strlen(stat_buf));
        s++;
      }
      else if (stat_buf[0] == 'E') {
        strncpy(IP_addr, stat_buf + 1, strlen(stat_buf));
        s++;
      }
    }

    // Clear Values
    SSD1351_set_cursor(5, 17);
    SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, prev_SD_usage);
    SSD1351_set_cursor(5, 42);
    SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, prev_CPU_temp);
    SSD1351_set_cursor(5, 67);
    SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, prev_CLK_speed);
    SSD1351_set_cursor(5, 92);
    SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, prev_RAM_usage);
    SSD1351_set_cursor(5, 117);
    SSD1351_printf(SSD1351_get_rgb(0, 0, 0), small_font, prev_IP_addr);

    strncpy(prev_SD_usage, SD_usage, strlen(SD_usage));
    strncpy(prev_CPU_temp, CPU_temp, strlen(CPU_temp));
    strncpy(prev_CLK_speed, CLK_speed, strlen(CLK_speed));
    strncpy(prev_RAM_usage, RAM_usage, strlen(RAM_usage));
    strncpy(prev_IP_addr, IP_addr, strlen(IP_addr));

    // Display Values
    SSD1351_set_cursor(5, 5);
    SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "SD Usage");
    SSD1351_set_cursor(5, 17);
    SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, SD_usage);
    SSD1351_set_cursor(5, 30);
    SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "SoC Temp");
    SSD1351_set_cursor(5, 42);
    SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, CPU_temp);
    SSD1351_set_cursor(5, 55);
    SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "Clock Speed");
    SSD1351_set_cursor(5, 67);
    SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, CLK_speed);
    SSD1351_set_cursor(5, 80);
    SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "RAM Usage");
    SSD1351_set_cursor(5, 92);
    SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, RAM_usage);
    SSD1351_set_cursor(5, 105);
    SSD1351_printf(SSD1351_get_rgb(r, g, b), small_font, "IP Address");
    SSD1351_set_cursor(5, 117);
    SSD1351_printf(SSD1351_get_rgb(r1, g1, b1), small_font, IP_addr);

    SSD1351_update();
    sleep_ms(5000);
  }  
}

bool receive_start_string() {
  char start_str[] = "START\n";
  char input_str[strlen(start_str)+1];
  fgets(input_str, sizeof(input_str), stdin);
  return !strcmp(input_str, start_str);
}

bool receive_end_string() {
  char end_str[] = "END\n";
  char input_str[strlen(end_str)+1];
  fgets(input_str, sizeof(input_str), stdin);
  return !strcmp(input_str, end_str);
}

bool receive_stop_string() {
  char stop_str[] = "X\n";
  char input_str[strlen(stop_str)+1];
  fgets(input_str, sizeof(input_str), stdin);
  return !strcmp(input_str, stop_str);
}

void serial_thread(void) {
  // Check if there's any serial input available
  if (stdin && !feof(stdin)) {
    while(1){

      // Check if the string is 'X'
      if (receive_stop_string()) {
          multicore_reset_core1();
          sleep_ms(10);
          thread_running = true;
          multicore_launch_core1(display_stats);
      }

      // Check if the string is 'START'
      if (receive_start_string()) {
          thread_running = false;
          multicore_reset_core1();
          SSD1351_clear_8();
          SSD1351_write_image(); 
          SSD1351_update();
      }
      
      // Check if the string is 'END'
      if (receive_end_string()) {
          SSD1351_clear_8();
          SSD1351_update();
          thread_running = true;
          multicore_launch_core1(display_stats);
      }
    }
  }    
}

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
  spi_init(SPI_PORT, 15000000);
  SSD1351_init();  

  // Set up the video thread
  multicore_launch_core1(play_splash_vid);
  
  while(1) {
    serial_thread();
  }

return 0;
}
