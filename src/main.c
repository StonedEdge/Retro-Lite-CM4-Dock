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
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "ssd1351.h"
#include "splash_vid.h"

#define IMAGE_SIZE_BYTES 32768

extern uint8_t buffer[OLED_BUF_SIZE];

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
  // Loop splash video v times
  for (int v = 0; v < 3; v++) {
    splash_vid_single();
  }

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

  while(1) {
    // Get Values
    int s = 0;
    while(s < 5){
    scanf("%1024s", stat_buf);
    
    if (stat_buf[0] == 'A') {
      strncpy(SD_usage, stat_buf + 1, strlen(stat_buf));
      sleep_ms(1);
      s++;
    }
    else if (stat_buf[0] == 'B') {
      strncpy(CPU_temp, stat_buf + 1, strlen(stat_buf));
      sleep_ms(1);
      s++;
    }
    else if (stat_buf[0] == 'C') {
      strncpy(CLK_speed, stat_buf + 1, strlen(stat_buf));
      sleep_ms(1);
      s++;
    }
    else if (stat_buf[0] == 'D') {
      strncpy(RAM_usage, stat_buf + 1, strlen(stat_buf));
      sleep_ms(1);
      s++;
    }
    else if (stat_buf[0] == 'E') {
      strncpy(IP_addr, stat_buf + 1, strlen(stat_buf));
      sleep_ms(1);
      // i++;
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
    char start_str[] = "TART";
    int start_str_index = 0;
    int data;

    while (start_str_index < strlen(start_str)) {
        data = getchar();
        if (data == start_str[start_str_index]) {
            start_str_index++;
        } else {
            return false;
        }
    }
    return true;
}

bool receive_end_string() {
    char end_str[] = "ND";
    int end_str_index = 0;
    int data;

    while (end_str_index < strlen(end_str)) {
        data = getchar();
        if (data == end_str[end_str_index]) {
            end_str_index++;
        } else {
            return false;
        }
    }
    return true;
}

void serial_thread(void) {
    // Set up the variables
    char serial_input = '\0';
    int i = 0;
  
      // Check if there's any serial input available
      if (stdin && !feof(stdin)) {
          // Try to read a character from the serial input
          serial_input = getchar();

          // Variables for game end message
          char end_str[] = "END";
          int end_str_index = 0;

          // Check if the character is 'X'
          if (serial_input == 'X') {
              multicore_reset_core1();
              return;
          }

          if (serial_input == 'S') {
              if (receive_start_string) {
                SSD1351_write_image(); 
                SSD1351_update();
                // multicore_reset_core1();
                return;
            }
          }

          else if (serial_input == 'E') {
              if (receive_end_string()) {
                // multicore_launch_core1(display_stats);
                SSD1351_fill(COLOR_WHITE);
                SSD1351_update();
                // multicore_reset_core1();
                return;
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
  spi_init(SPI_PORT, 10000000);
  SSD1351_init();  

  // Set up the video thread
  // multicore_launch_core1(play_splash_vid);

  // Set up the serial thread
  // serial_thread();
  
  while(1) {
    serial_thread();
    // SSD1351_write_image(); 
    // SSD1351_update();
  }




return 0;
}
