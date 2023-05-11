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
#define button_pin 6
#define debounce_time 100

// The worst case for an uncomporessed IPV6 address would be 8 4-digit segments plus 7 colon separators + null, or 40 chars.
// I've reserved 64 characters just in case, although that would never fit in the tiny display.

#define STATS_BUFFER_SIZE 64

extern uint8_t buffer[OLED_BUF_SIZE];

uint8_t boxart_buffer[IMAGE_SIZE_BYTES] = { 0x00 };
uint8_t combined_buffer[IMAGE_SIZE_BYTES] = { 0x00 };
uint8_t consol_buffer[IMAGE_SIZE_BYTES] = { 0x00 };

bool GetLine(char* line, int lineSize);

typedef enum { SPLASH_SCREEN, GAME_ACTIVE, DISPLAY_STATS } DISPLAY_MODE;
volatile DISPLAY_MODE display_mode = SPLASH_SCREEN;

/**
* Display the various images in rotation while in "GAME_ACTIVE" mode.
*/

void button_loop(void) {
    extern volatile DISPLAY_MODE display_mode;

    int current_display = 0; // start with the first image
    uint32_t last_press_time = 0;

    while (display_mode == GAME_ACTIVE) {
        if (!gpio_get(button_pin)) {
            uint32_t current_time = time_us_32();
            if (current_time - last_press_time >= debounce_time * 1000) {
                last_press_time = current_time;

                // Button is pressed, cycle through the displays
                current_display++;
                if (current_display > 2) {
                    current_display = 0; // cycle back to the first display
                }
                switch (current_display) {
                    case 0:
                        SSD1351_clear_8();
                        // Display the combined image"
                        SSD1351_display_image(combined_buffer);
                        break;
                    case 1:
                        SSD1351_clear_8();
                        // Display the boxart image
                        SSD1351_display_image(boxart_buffer);
                        break;
                    case 2:
                        SSD1351_clear_8();
                        // Display the consol image
                        SSD1351_display_image(consol_buffer);
                        break;
                }
            }
        }

        sleep_ms(10); // wait a short time before checking again
    }
}

static char SD_usage[STATS_BUFFER_SIZE];    // A
static char CPU_temp[STATS_BUFFER_SIZE];    // B
static char CLK_speed[STATS_BUFFER_SIZE];   // C
static char RAM_usage[STATS_BUFFER_SIZE];   // D
static char IP_addr[STATS_BUFFER_SIZE];     // E

// The previous values are used so that we'll have something to init the display with on the first call when entering "X" mode.
// TODO: This claim is a bit funky - do we really need these?

char prev_SD_usage[STATS_BUFFER_SIZE];      // A
char prev_CPU_temp[STATS_BUFFER_SIZE];      // B
char prev_CLK_speed[STATS_BUFFER_SIZE];     // C
char prev_RAM_usage[STATS_BUFFER_SIZE];     // D
char prev_IP_addr[STATS_BUFFER_SIZE];       // E

/**
* Display statistics.
*
* The values are collected by the main thread, and we display them here once.
* 
* @caution This Code assumes that the messages are always the same length.  If so, no problem.  If not the
* messages could either overlap, or leave text from the previous iteration.
*/

void DisplayStats(void) {
    // TODO: Do we need the "prev" values??

    extern char SD_usage[STATS_BUFFER_SIZE], CPU_temp[STATS_BUFFER_SIZE], CLK_speed[STATS_BUFFER_SIZE], RAM_usage[STATS_BUFFER_SIZE], IP_addr[STATS_BUFFER_SIZE];
    extern char prev_SD_usage[STATS_BUFFER_SIZE], prev_CPU_tempSTATS_BUFFER_SIZE, prev_CLK_speed[STATS_BUFFER_SIZE], prev_RAM_usage[STATS_BUFFER_SIZE], prev_IP_addr[STATS_BUFFER_SIZE];

    const int r = 255;                  // Title colour
    const int g = 255;
    const int b = 255;

    const int r1 = 180;                 // Value colour
    const int g1 = 180;
    const int b1 = 180;

    // Clear Values  TODO: "Clear"??
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
}

/**
* Read a line from stdin.
*
* @param line               [out] Output area.  Will contain null-terminated text without the newline character.
*
* @lineSize                 [in] Size of the the line area in bytes.  Must be > 1 !
*
* @return true if line read, false on EOF or error.
*/

bool GetLine(char* line, int lineSize)
{
    int lineIndex = 0;

    lineSize--;                         // Save space for final null.

    while (true) {
        int c = getchar();              // Pico version returns a uint8 value
        if (c < 0) {                    // STDIO_ERROR (-1) or STDIO_NO_INPUT (-2):  We are done.
            return false;
        }
        if (c == '\n') {                // Done.
            line[lineIndex] = '\0';
            return true;
        }
        else if (lineIndex < lineSize) {
            line[lineIndex++] = c;
        }
    }
}

void play_splash_vid(void) {
    int t = 0;
    while (t < 20) {
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

/**
* Serial Input handler.
* 
* This routine is called from main().  It receives commands from the RPi host via the USB port,
* which we use to switch display modes.
* 
* This code runs on thread 0 (the main thread).  Thread 1 is used for the splash screen display
* and key monitorring.
*/

void SerialThread(void) {
    extern volatile DISPLAY_MODE display_mode;
    extern char SD_usage[STATS_BUFFER_SIZE], CPU_temp[STATS_BUFFER_SIZE], CLK_speed[STATS_BUFFER_SIZE], RAM_usage[STATS_BUFFER_SIZE], IP_addr[STATS_BUFFER_SIZE];

    // Check if there's any serial input available
    if (stdin) {
        char line[STATS_BUFFER_SIZE];

        while (GetLine(line, sizeof line)) {
            if (strcmp(line, "X") == 0) {
                if (display_mode != DISPLAY_STATS) {
                    multicore_reset_core1();
                    SSD1351_clear_8();
                    SSD1351_update();
                    display_mode = DISPLAY_STATS;
                }
            }
            else if (strcmp(line, "start") == 0) {
                multicore_reset_core1();
                SSD1351_clear_8();
                SSD1351_get_image(combined_buffer);
                SSD1351_get_image(boxart_buffer);
                SSD1351_get_image(consol_buffer);
                SSD1351_display_image(combined_buffer);
                display_mode = GAME_ACTIVE;
                multicore_launch_core1(button_loop);
            }
            else if (strcmp(line, "END") == 0) {
                multicore_reset_core1();
                SSD1351_clear_8();
                SSD1351_update();
                display_mode = DISPLAY_STATS;
                DisplayStats();         // Redraw (possibly empty data).
            }
            else if (display_mode == DISPLAY_STATS) {
                if (line[0] == 'A') {
                    line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                    strcpy(SD_usage, line);
                }
                else if (line[0] == 'B') {
                    line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                    strcpy(CPU_temp, line);
                }
                else if (line[0] == 'C') {
                    line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                    strcpy(CLK_speed, line);
                }
                else if (line[0] == 'D') {
                    line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                    strcpy(RAM_usage, line);
                }
                else if (line[0] == 'E') {
                    line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                    strcpy(IP_addr, line);
                    DisplayStats();         // We should have all five items now -- update the display.
                }
            }  // Else nonsense input
        }  // GetLine() failed - probably host went away.

        clearerr(stdin);
    }
}

int main(void)
{
    extern volatile DISPLAY_MODE display_mode;

    // GPIO Set-Up
    spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
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
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_pull_up(button_pin);

    // SPI Initialisation
    stdio_init_all();
    spi_init(SPI_PORT, 15000000);
    SSD1351_init();

    // Set up the video thread
    multicore_launch_core1(play_splash_vid);
    display_mode = SPLASH_SCREEN;

    while (1) {
        SerialThread();
    }

    return 0;
}
