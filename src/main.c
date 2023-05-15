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
#define INACTIVE_TIMEOUT 60 * 1000000
#define button_pin 6
#define debounce_time_ms 100

// The worst case for an uncomporessed IPV6 address would be 8 4-digit segments plus 7 colon separators + null, or 40 chars.
// I've reserved 64 characters just in case, although that would never fit in the tiny display.

#define STATS_BUFFER_SIZE 64

extern uint8_t buffer[OLED_BUF_SIZE];

uint8_t boxart_buffer[IMAGE_SIZE_BYTES] = { 0x00 };
uint8_t combined_buffer[IMAGE_SIZE_BYTES] = { 0x00 };
uint8_t consol_buffer[IMAGE_SIZE_BYTES] = { 0x00 };

void EnableDisplay(bool enable);
bool GetLine(char* line, int lineSize);

typedef enum { SPLASH_SCREEN, GAME_ACTIVE, DISPLAY_STATS } DISPLAY_MODE;
volatile DISPLAY_MODE displayMode = SPLASH_SCREEN;
volatile bool displayEnabled = false;   // The display is actually on after initialization, but set to false for EnableDisplay.
uint32_t lastActivityTime;              // Last activity in us

                                        /**
* Display the various images in rotation while in "GAME_ACTIVE" mode.
*/

void ButtonLoop(void) {
    extern volatile DISPLAY_MODE displayMode;
    extern uint32_t lastActivityTime;

    EnableDisplay(true);

    int current_display = 0; // start with the first image
    uint32_t last_press_time = 0;

    while (displayMode == GAME_ACTIVE) {
        if (!gpio_get(button_pin)) {
            uint32_t currentTime = time_us_32();
            lastActivityTime = currentTime;

            if (currentTime - last_press_time >= debounce_time_ms * 1000) {
                last_press_time = currentTime;

                // Button is pressed, cycle through the displays

                if (++current_display > 2) {
                    current_display = 0; // cycle back to the first display
                }

                EnableDisplay(true);

                switch (current_display) {
                    case 0:  // Display the combined image"
                        SSD1351_clear_8();
                        SSD1351_display_image(combined_buffer);
                        break;
                    case 1:  // Display the boxart image
                        SSD1351_clear_8();
                        SSD1351_display_image(boxart_buffer);
                        break;
                    case 2:  // Display the consol image
                        SSD1351_clear_8();
                        SSD1351_display_image(consol_buffer);
                        break;
                }
            }
        }

        sleep_ms(10); // wait a short time before checking again

        if (time_us_32() - lastActivityTime >= INACTIVE_TIMEOUT) {
            EnableDisplay(false);
        }
    }
}

static char SD_usage[STATS_BUFFER_SIZE];    // A
static char CPU_temp[STATS_BUFFER_SIZE];    // B
static char CLK_speed[STATS_BUFFER_SIZE];   // C
static char RAM_usage[STATS_BUFFER_SIZE];   // D
static char IP_addr[STATS_BUFFER_SIZE];     // E

// The previous values are used so that we'll have something to init the display with on the first call when entering "X" mode.
// TODO: This claim is a bit funky - do we really need these?

#if 1
char prev_SD_usage[STATS_BUFFER_SIZE];      // A
char prev_CPU_temp[STATS_BUFFER_SIZE];      // B
char prev_CLK_speed[STATS_BUFFER_SIZE];     // C
char prev_RAM_usage[STATS_BUFFER_SIZE];     // D
char prev_IP_addr[STATS_BUFFER_SIZE];       // E
#endif

/**
* Display statistics.
*
* The values are collected by the main thread, and we display them here once.
* 
* @caution This Code assumes that the messages are always the same length.  If so, no problem.  If not the
* messages could either overlap, or leave text from the previous iteration.
*/

void DisplayStats(void) {
#if 1
    // TODO: Do we need the "prev" values??
#endif

    extern char SD_usage[STATS_BUFFER_SIZE], CPU_temp[STATS_BUFFER_SIZE], CLK_speed[STATS_BUFFER_SIZE], RAM_usage[STATS_BUFFER_SIZE], IP_addr[STATS_BUFFER_SIZE];
#if 1
    extern char prev_SD_usage[STATS_BUFFER_SIZE], prev_CPU_tempSTATS_BUFFER_SIZE, prev_CLK_speed[STATS_BUFFER_SIZE], prev_RAM_usage[STATS_BUFFER_SIZE], prev_IP_addr[STATS_BUFFER_SIZE];
#endif
    const int r = 255;                  // Title colour
    const int g = 255;
    const int b = 255;

    const int r1 = 180;                 // Value colour
    const int g1 = 180;
    const int b1 = 180;

#if 1
    // Clear Values
    // NOTE WELL:  The SSD1351_write_char() function only sets non-blank pixel, hence this code.
    // How about changing that routine to write all pixels?

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
#endif

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
* Enable or disable the display.
* 
* On enable we also reset lastActivityTime, which is used for turning the display off after inactivity.
* 
* @param enable         [in] true to enable the display.
*/

void EnableDisplay(bool enable)
{
    extern volatile bool displayEnabled;
    extern uint32_t lastActivityTime;

    if (displayEnabled != enable) {
        displayEnabled = enable;

        if (enable) {
            SSD1351_write_command(SSD1351_CMD_FUNCTIONSELECT);  // Enable Vdd voltage regulator
            SSD1351_write_data(0x01);
            SSD1351_write_command(SSD1351_CMD_DISPLAYON);
        }
        else {
            // NOTE WELL:  You cannot write the display memory while the voltage regulator is off.

            SSD1351_write_command(SSD1351_CMD_DISPLAYOFF);
            SSD1351_write_command(SSD1351_CMD_FUNCTIONSELECT);  // Disable Vdd voltage regulator to save power
            SSD1351_write_data(0x00);
        }
    }

    if (enable) {  // Reset the last activity time elsewhere we can determine inactivity.
        lastActivityTime = time_us_32();
    }
}

/**
* Switch from current display mode to DISPLAY_STATS mode.
* 
* Called when we get the first "X" command and after an END command.
*/

void EnableDisplayStats()
{
    displayMode = DISPLAY_STATS;

    multicore_reset_core1();
    SSD1351_clear_8();
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

/**
* Play the splash screen for up to 20 seconds.
*/

void PlaySplashVid(void) 
{
    EnableDisplay(true);

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

/**
* Play the splash screen video once.  (Not used.)
*/

void PlaySplashVidSingle(void) {
    EnableDisplay(true);

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
    extern volatile DISPLAY_MODE displayMode;
    extern volatile bool displayEnabled;
    extern uint32_t lastActivityTime;
    extern char SD_usage[STATS_BUFFER_SIZE], CPU_temp[STATS_BUFFER_SIZE], CLK_speed[STATS_BUFFER_SIZE], RAM_usage[STATS_BUFFER_SIZE], IP_addr[STATS_BUFFER_SIZE];

    // Check if there's any serial input available
    if (stdin) {
        char line[STATS_BUFFER_SIZE];

        while (GetLine(line, sizeof line)) {
            if (strcmp(line, "X") == 0) {
                if (displayMode == SPLASH_SCREEN) {  // We've just started, show statistics now.
                    EnableDisplayStats();
                    EnableDisplay(true);
                    DisplayStats();
                }
            }
            else if (strcmp(line, "start") == 0) {
                multicore_reset_core1();
                SSD1351_clear_8();
                SSD1351_get_image(combined_buffer);
                SSD1351_get_image(boxart_buffer);
                SSD1351_get_image(consol_buffer);
                SSD1351_display_image(combined_buffer);
                displayMode = GAME_ACTIVE;
                multicore_launch_core1(ButtonLoop);  // ButtonLoop manages lastActivityTime
            }
            else if (strcmp(line, "END") == 0) {
                EnableDisplayStats();
                EnableDisplay(true);
                DisplayStats();
            }
            else {
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

                    // This should allow the stats to be displayed after an END command, and then turn the screen off.
                    // The only way to turn the screen back on is then to start another game.

                    if (displayMode == DISPLAY_STATS) {
                        if (displayEnabled) {
                            uint32_t currentTime = time_us_32();
                            if (currentTime - lastActivityTime >= INACTIVE_TIMEOUT) {
                                EnableDisplay(false);
                            }
                        }

                        if (displayEnabled) {
                            DisplayStats();
                        }
                    }  // Not in stats mode, just save the data.
                }
            }  // Else nonsense input
        }  // GetLine() failed - probably host went away.

        clearerr(stdin);
    }
}


int main(void)
{
    extern volatile DISPLAY_MODE displayMode;

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

    displayMode = SPLASH_SCREEN;
    multicore_launch_core1(PlaySplashVid);  // PlaySplashVid calls EnableDisplay.

    while (1) {
        SerialThread();
    }

    return 0;
}
