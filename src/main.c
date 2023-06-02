
/* Main.c -- Retro-Lite CM4 Docking Station -- Main code -- bdt -- 2023-01-01
* Copyright (c) 2023 Benjamin D. Todd.
*
* This is the main code that runs on a Raspberry Pi Pico.  The host is a Raspberry Pi, and
* the host logic is largely implemented in python.
* 
* License
* =======
* Creative Commons Attribution Share Alike 4.0 International*
*
* Revision History
* ================
* 2023-05-08 wmm  Restructure the serial input thread and main code so that input is handled in only a single thread.
* 
* 2023-05-10 wmm  Change python host code to generate .bin images use Big Endian encoding.  This way, reading the
*                 16-bit rgb 565 values will appear in the Pico in the proper order when reading the image stream
*                 as bytes.
* 
* 2023-05-15 wmm  Implement "screen blackout" after one minute of inactivity. If you are playing a game
*                 you can exit blackout mode by pressing the utility button.  Otherwise you must start a new game.
*                 
*                 (If you are in statistics display mode when blackout occurs, there is no way to exit blackout mode
*                 since this display loop does not interact with the utility button.)
* 
* 2023-05-16 wmm  Remove "previous statistics data" (comment out for now), and to cure the problem of pixel artifacts,
*                 modify SSD1351_write_char() to write all pixels in the font, not just non-black pixels.
* 
*            wmm  When waking up from screen blackout, display the current image instead of skipping to the next image.
*
* 2023-05-19 wmm  Added watchdog timer handling at the end of main().  Terminating main() will now reboot the processor
*                 intead of going into an infinite breakpoint loop.
*
* 2023-05-27 wmm  Fixed SSD1351_write_char -- It was setting all pixels, not just "non-empty" pixels.
* 
* 2023-05-31 wmm  Added "sudo" to python3 command in runcommand-onend.sh.
* 
* 2023-06-01 wmm  Restructured main() so it does initialization only once.  Apparently either the GPIO re-init or the
*                 various library inits are not possible and can result in the display of garbage and other funny
*                 behviour afer running our shutdown logic and then trying to restart.
* 
* 2023-06-01 wmm  Modified ssd1351 output routines to accept a color value containing both the foreground and background
*                 colors.  The default background color is black.
* 
*                 Always show splash screen at start of loop instead of splash screen on first pass, then stats display.
*
* Power Behavior
* ==============
+ TODO: Review these comments - do they all still apply?  2023-05-27.
* Let's assume the charger is always plugged in to the dock at the beginning but no console is connected to start.
*
* (Scenario 1) In the above state, no power is delivered on VBUS. The Pico is not powered and the battery is not charging. This is because the charger only is able to deliver VBUS power if it detects a sink controller. If the console is not plugged in, no power is delivered to the Pico. The Pico is powered by a step-down DC-DC converter which steps down VBUS voltage. No console, no power to the Pico, no power to the OLED.
*
* (Scenario 2) The user plugs in the console in a turned off state. Power is now delivered on VBUS and the code immediately runs as the Pico receives power. 
* The splashscreen will loop for approximately 30 seconds, and then display "NO CONNECTION".  If no connection is established after about one minute,
* the OLED shuts off.
*
* (Scenario 3) The console is in the dock now. The user turns on power to the console from an off state by holding the power button for 3 seconds.
* It takes about 15-20 seconds until EmulationStation process runs from a cold boot. The console will detect the process has begun and then switch over to stats mode.
*
* (Scenario 4) The user has enjoyed playing whatever game he wants to play and now decides its time for the console to be switched off. There's two ways he/she can do this. 
* The first way is by navigating within the RetroPie GUI and clicking "Power down". This will leave the regulator active but the software will be off. The second way is by holding 
* the power button for 3 seconds, after which a software shutdown will begin and then power to the regulator inside the console will be killed.
*
* (Scenario 5) The user decides to pull power directly from the back of the dock. In this case, the Pico does not cleanly shut down because power was immediately pulled. The console no longer charges and the OLED is now off, Pico is powered off, until the user plugs the USB-C charger back in to the dock.
*/

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
#include "hardware/watchdog.h"
#include "ssd1351.h"
#include "splash_vid.h"

#define BUTTON_PIN 6
#define DEBOUNCE_TIME_MS 100
#define IMAGE_SIZE_BYTES 32768
#define INACTIVE_TIMEOUT 60 * 1000000

// The worst case for an uncompressed IPV6 address would be 8 4-digit segments plus 7 colon separators + null, or 40 chars.
// I've reserved 64 characters just in case, although that would never fit in the tiny display.

#define STATS_BUFFER_SIZE 64

extern uint8_t buffer[OLED_BUF_SIZE];

uint8_t boxart_buffer[IMAGE_SIZE_BYTES] = { 0x00 };
uint8_t combined_buffer[IMAGE_SIZE_BYTES] = { 0x00 };
uint8_t consol_buffer[IMAGE_SIZE_BYTES] = { 0x00 };

void EnableDisplay(bool enable);
bool GetLine(char* line, int lineSize);

typedef enum { SPLASH_SCREEN, GAME_ACTIVE, DISPLAY_STATS, DISPLAY_BLANKED } DISPLAY_MODE;
volatile DISPLAY_MODE displayMode = SPLASH_SCREEN;
volatile bool displayEnabled = false;   // The display is actually on after initialization, but set to false for EnableDisplay.
uint32_t lastActivityTime;              // Last activity in us

int shutdownReason = 0; // 0=No, 1=Power off, 2=Game Engine stopped.

                                        /**
* Display the various images in rotation while in "GAME_ACTIVE" mode.
*/

void ButtonLoop(void)
 {
    extern volatile bool displayEnabled;
    extern volatile DISPLAY_MODE displayMode;
    extern uint32_t lastActivityTime;

    EnableDisplay(true);

    int current_display = 0; // start with the first image
    uint32_t last_press_time = 0;

    while (displayMode == GAME_ACTIVE) {
        if (!gpio_get(BUTTON_PIN)) {
            uint32_t currentTime = time_us_32();
            lastActivityTime = currentTime;

            if (currentTime - last_press_time >= DEBOUNCE_TIME_MS * 1000) {
                last_press_time = currentTime;

                // Button is pressed, cycle through the displays

                if (!displayEnabled) {      // If we are waking up, use the current item number.
                    EnableDisplay(true);
                }
                else if (++current_display > 2) {  // Rotate through the items.
                    current_display = 0;
                }

                switch (current_display) {
                    case 0:  // Display the combined image"
                        SSD1351_clear_8();
                        SSD1351_display_image(combined_buffer);
                        break;
                    case 1:  // Display the boxart image
                        SSD1351_clear_8();
                        SSD1351_display_image(boxart_buffer);
                        break;
                    case 2:  // Display the console image
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

#if 0  // 2023-05-16.  Disable.
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
    extern char SD_usage[STATS_BUFFER_SIZE], CPU_temp[STATS_BUFFER_SIZE], CLK_speed[STATS_BUFFER_SIZE], RAM_usage[STATS_BUFFER_SIZE], IP_addr[STATS_BUFFER_SIZE];
#if 0  // 2023-05-16.  Disable.
    extern char prev_SD_usage[STATS_BUFFER_SIZE], prev_CPU_tempSTATS_BUFFER_SIZE, prev_CLK_speed[STATS_BUFFER_SIZE], prev_RAM_usage[STATS_BUFFER_SIZE], prev_IP_addr[STATS_BUFFER_SIZE];
#endif
    const int r = 255;                  // Title colour
    const int g = 255;
    const int b = 255;

    const int r1 = 180;                 // Value colour
    const int g1 = 180;
    const int b1 = 180;

#if 0  // 2023-05-16.  Disable.
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

#if 1 // DISABLE THIS CODE UNTIL CURRENT FUNKY FONT ISSUE IS SOLVED.  2023-05-28
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
#endif
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
    SSD1351_set_cursor(1, 64);

    uint8_t wait_time = 30;
    while (wait_time != 0) {
        SSD1351_printf(COLORFGBG(SSD1351_get_rgb(0, 0, 0), COLOR_WHITE), small_font, "Wait for Pi: %i ", wait_time);
        SSD1351_update();
        sleep_ms(1000);
        wait_time--;
    }

    // This thread is normally killed by the main control process upon receipt of an "X" command.  If that
    // still hasn't arrived, blank the display.

    EnableDisplay(false);
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
    extern int shutdownReason;

    // Check if there's any serial input available
    char line[STATS_BUFFER_SIZE];

    while (shutdownReason == 0 && GetLine(line, sizeof line)) {
        if (strcmp(line, "X") == 0) {
            if (displayMode == SPLASH_SCREEN) {  // We've just started, show statistics now.
                EnableDisplayStats();
                EnableDisplay(true);
                DisplayStats();
            }
            else if (displayMode == DISPLAY_BLANKED) {  // Same as above for now.  CAN PROBABLY REMOVE THIS MODE
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
        else if (strncmp(line, "SD", 2) == 0) {  // Shutdown.
            shutdownReason = line[2] - '0';     // Next character is the shutdown reason code.
        }
        else {
            if (line[0] == 'A') {
                line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                strcpy(SD_usage, line+1);
            }
            else if (line[0] == 'B') {
                line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                strcpy(CPU_temp, line+1);
            }
            else if (line[0] == 'C') {
                line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                strcpy(CLK_speed, line+1);
            }
            else if (line[0] == 'D') {
                line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                strcpy(RAM_usage, line+1);
            }
            else if (line[0] == 'E') {
                line[STATS_BUFFER_SIZE - 1] = '\0'; // Truncate if too long
                strcpy(IP_addr, line+1);

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
}

int main(void)
{
    extern volatile DISPLAY_MODE displayMode;
    bool done = false;
#if 0  // Pre 2023-06-02
    bool firstGame = true;
#endif
    extern int shutdownReason;

    // GPIO Set-Up

    // TODO: Will re-running these init calls work ok?

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
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // SPI Initialisation

    stdio_init_all();
    spi_init(SPI_PORT, 15000000);
    SSD1351_init();

    if (!stdin) {  // We are screwed!
        SSD1351_clear_8();
        SSD1351_printf(SSD1351_get_rgb(255, 0, 0), small_font, "No Stdin!");
        SSD1351_update();
        sleep_ms(5000); // Display for a bit before we get to the watchdog timer code.
        done = true;
    }

    while (!done) {
        clearerr(stdin);

        // Set up the video thread

#if 1  // 2023-06-02  -- Always show splash screen.
        displayMode = SPLASH_SCREEN;
        multicore_launch_core1(PlaySplashVid);  // PlaySplashVid calls EnableDisplay.
#else
        if (firstGame) {
            displayMode = SPLASH_SCREEN;
            multicore_launch_core1(PlaySplashVid);  // PlaySplashVid calls EnableDisplay.
            firstGame = false;
        }
        else {
            displayMode = DISPLAY_BLANKED;  // Don't do anything until we get an 'X' command.
        }
#endif

        SerialThread();

        if (shutdownReason != 0) {
            multicore_reset_core1();
            SSD1351_clear_8();
            SSD1351_update();

            if (shutdownReason == 1) {
                // The Pico gets power from the VBUS, so there is always power unless you unplug the
                // docking station from the all.
                // 
                // We could go into dormant mode here to save power, but to exit dormant mode we would
                // need a pin to trigger wakeup, which we do not have.
                // 
                // Therefore, treat the power down event just like the emulator exit even.

                shutdownReason = 0;
            }
            else {
                shutdownReason = 0;             // Just restart the loop
            }
        }  // Otherwise we probably got here because of an I/O error.  Hopefully re-init will fix it.
    }

    /*
    * Returning will lead you to exit(), and this can have one of two behaviors:
    * 
    * (1) If PICO_ENTER_USB_BOOT_ON_EXIT was set duriing build, the PICO will reboot.  This build option
    * is off by default.
    * 
    * (2) If not set, you will be in an endless _breakpoint() loop.  This is pretty useless unless you are
    * running the debugger since you will then need to power down and power up to continue.
    * 
    * Therefore, below we turn on the watchdog timer and enter an infinite loop.  After a moment the
    * watchdog timer will detect the loop and reset the processor.
    * 
    * PRESENTLY THERE IS NO WAY TO GET HERE UNLESS STDIN IS NOT DEFINED.  Shutdown1 waits above for power off, and Shutdown2 tries to restart
    * the loop abov.  However, leave this code for reference.
    */

    watchdog_enable(1, 1);
    while (1);

    return 0;  // Never reached
}
