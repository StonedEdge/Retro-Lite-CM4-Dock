/* ssd1351.c -- Retro-Lite CM4 Docking Station -- Display code -- bdt -- 2023-01-01
* Copyright (c) 2023 Benjamin D. Todd.
*
* This is a library to handle the SSD1351 OLED display.
* 
* License
* =======
* Creative Commons Attribution Share Alike 4.0 International*
*
* REVISION HISTORY
* ================
* 2023-06-01 wmm  Modified ssd1351 output routines to accept a color value containing both the foreground and background
*                 colors.  The default background/* ssd1351.c -- Retro-Lite CM4 Docking Station -- Display code -- bdt -- 2023-01-01
* Copyright (c) 2023 Benjamin D. Todd.
*
* This is a library to handle the SSD1351 OLED display.
* 
* License
* =======
* Creative Commons Attribution Share Alike 4.0 International*
*
* REVISION HISTORY
* ================
* 2023-06-01 wmm  Modified ssd1351 output routines to accept a color value containing both the foreground and background
*                 colors.  The default background color is black.
* 
* 2023-06-09 wmm  Added SSD1351_display_text_buffer.
*/

#include "ssd1351.h"

/* Buffer to hold the Display RAM Data.
* 
* You must call SSD1351_update() to transfer the buffer to the real display memory.
*/

static DRAM displayRAM;

#define DRAM_16 displayRAM.halfw
#define DRAM_8 displayRAM.byte

uint8_t buffer[OLED_BUF_SIZE] = { 0x00 };

SSD_CURSOR SSD1351_cursor;

void SSD1351_SendByte(uint8_t data)
{
    spi_write_blocking(SPI_PORT, &data, 1);
}

void SSD1351_SendBuffer(const uint8_t* buffer, uint16_t len)
{
    spi_write_blocking(SPI_PORT, buffer, len);
}

void SSD1351_write_command(uint8_t cmd)
{
    gpio_put(DC, 0);
    gpio_put(CS, 0);
    SSD1351_SendByte(cmd);
    gpio_put(CS, 1);
}

void SSD1351_write_data(uint8_t data)
{
    gpio_put(DC, 1);
    gpio_put(CS, 0);
    SSD1351_SendByte(data);
    gpio_put(CS, 1);
}

void SSD1351_write_data_buffer(const uint8_t* data, uint16_t len)
{
    gpio_put(DC, 1);
    gpio_put(CS, 0);
    SSD1351_SendBuffer(data, len);
    gpio_put(CS, 1);
}

void SSD1351_update(void)
{
    SSD1351_write_command(SSD1351_CMD_SETCOLUMN);
    SSD1351_write_data(0x00);
    SSD1351_write_data(0x7F);
    SSD1351_write_command(SSD1351_CMD_SETROW);
    SSD1351_write_data(0x00);
    SSD1351_write_data(0x7F);
    SSD1351_write_command(SSD1351_CMD_WRITERAM);
    SSD1351_write_data_buffer(DRAM_8, DRAM_SIZE_8);
}

void SSD1351_init(void)
{
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

void SSD1351_clear(void)
{
    memset(DRAM_8, 0, DRAM_SIZE_8);
}

void SSD1351_clear_8(void)
{
    memset(DRAM_8, 0, DRAM_SIZE_8);
}

/**
* Display a block of text, with scrolling.
*
* This function is used to display game meta data such as the author, title, etc.
*
* @param text                   [in] Text buffer.
*
* @param textLen                [in] Number of bytes in "text".
*
* @param color                  [in] Color (same as for SSD1351_write_char)
*
* @param font                   [in] Font.
*/

void SSD1351_display_text_buffer(const char* text, int textLen, uint32_t color, font_t font)
{
    extern SSD_CURSOR SSD1351_cursor;
    bool sawNewline = false;

    uint8_t fh = font.height + 2;         // The write_char() routine has 2 pixels of space between lines.

    while (textLen-- > 0) {
        char c = *text++;

        if (sawNewline) {
            // We have newline from end of previous line.  Now we can advance the cursor.
            // By delaying this operation so we don't end up with empty space after the last
            // display line.

            SSD1351_display_text_buffer_advance_cursor_y(fh, color);
            sawNewline = false;
        }

        if (c == '\n') {                    // Don't handle until we absolutely must.
            sawNewline = true;
        }
        else if (SSD1351_cursor.x == 0 && SSD1351_cursor.y + fh >= OLED_HEIGHT) {
            // We want to start a new line, but it's not going to fit vertically.
            SSD1351_display_text_buffer_advance_cursor_y(fh, color);
        }
        else if (SSD1351_cursor.x >= OLED_WIDTH) {
            // Line won't fit horizontally, so move to next line and scroll if required.
            SSD1351_display_text_buffer_advance_cursor_y(fh, color);
        }

        // Finally, output the next character.

        if (c != '\n') {                // Not newline.
            if (c < ' ' || c >= 0x7F) { // Display control chars, DEL, and extended ASCII as space.
                c = ' ';
            }
            SSD1351_write_char(color, font, c);
            SSD1351_update();
        }
    }
}

/**
* Advance the text cursor in the vertical direction, scrolliing the text if required.
*
* @param fh                             [in] Font height (amount to scroll).
*
* @param color                          [in] fg/bg color (see SSD1351_write_char).
*/

void SSD1351_display_text_buffer_advance_cursor_y(uint8_t fh, uint32_t color)
{
    if (SSD1351_cursor.y + fh < OLED_HEIGHT) {
        // Advancing the cursor won't go off the screen, so simply set the new cursor position.
        SSD1351_set_cursor(0, SSD1351_cursor.y + fh);
    }
    else {
        // Advancing will go off the screen, so we have to move the current display contents.
        // 
        // Suppose we have this situation on a display that only allows 3 lines:
        // ----------
        //      Line1, cursor offset width * 0 * fh.
        //      Line2, cursor offset width * 1 * fh.
        //      Line3, cursor offset width * 2 * fh.
        // ---- Line4: won't fit 

        // Now move the middle lines (Line2 and Line3 above) up on the screen.  Multiply by 2 below
        // accounts for byte to halfword size difference in cursor vs dram access.
        //
        // Number of items to move is the same as the number of items from the start up to Line3, or in
        // this example, two lines worth of data.

        memcpy(DRAM_8 /* top line*/, DRAM_8 + OLED_WIDTH * fh * 2 /* 2nd line */, OLED_WIDTH * SSD1351_cursor.y * 2);

        // Back to beginning of the line that WOULD fit (Line3 in this example).

        SSD1351_set_cursor(0, SSD1351_cursor.y - fh);

        // Now we have:
        // ----------
        //      Line2
        //      Line3
        //      Line4 goes here where Line3 used to be.
        // ----------
        // 
        // Blank out the space previously occupied by Line3.

        uint16_t bgColor = color >> 16;

        int nPixels = fh * OLED_WIDTH;  // Pixels per line of text.
        int i = SSD1351_cursor.y * OLED_WIDTH;  // Offset to display row where we want to start blanking.  Cursor x is 0.
        while (nPixels-- > 0) {
            DRAM_16[i++] = bgColor;
        }
        SSD1351_update();

        sleep_ms(200);  // How long to delay?
    }
}

void SSD1351_fill(uint16_t color)
{
    // TODO: IS THIS OK?  Need to swap bytes?
    for (int i = 0; i < DRAM_SIZE_16; i++) {
        DRAM_16[i] = color;
    }
}

/*
 * @brief Converts from RGB to a single 16bit value
 * 
 * @param r: starting x coordinate
 * 
 * @para g: starting y coordinate
 * 
 * @param b: width of the rectangle
 * 
 * @reval 16bit value with the rgb color for display
 */

uint16_t SSD1351_get_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t rgb_color = 0;
    rgb_color |= ((r / 8) << 8);
    rgb_color |= ((g / 4) >> 3);
    rgb_color |= (((g / 4) % 0x07) << 13);
    rgb_color |= ((b / 8) << 3);
    return rgb_color;
}

/**
 * @brief Writes a pixel data to the screen RAM buffer
 * 
 * @param color: Unsigned int16 containing color code
 * 
 * @param x: Pixel's horizontal position
 * 
 * @param y: Pixel's vertical position
 * 
 * @retval None
 */

void SSD1351_write_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (x > 127 || y > 127 || x < 0 || y < 0) {
        return;
    }
    int a = x + (y * 128);
    DRAM_16[a] = color;
}

/**
* Write a single character.
* 
* This previously wrote only "non-empty" pixels.  However, that would leave
* artifacts from previous output on the screen.  fTherefore, a screen clear was
* always required.
* 
* This version sets all pixels, but the background will be black.
* 
* If you really need a different background color you can add that as a new parameter.
* 
* Note that the maximum font width is 16 pixels.
*
* @param color              [in] A 32-bit fg/bg color.
*
* @param font               [in] A font_t structure.
*
* @param c                  [in] Character to output.
*/

#if 1  // NEW CODE
static void SSD1351_write_char(uint32_t color, font_t font, char c)
{
    if ((COLUMNS <= SSD1351_cursor.x + font.width) || (ROWS <= SSD1351_cursor.y + font.height)) {
        return;
    }
    if (c == '\n') {
        SSD1351_cursor.x = 127;
    }
    else {
        uint16_t fgColor = color & 0xFFFF;
        uint16_t bgColor = (color >> 16);

        for (int i = 0; i < font.height; i++) {
            uint16_t fd = font.data[(c - 32) * font.height + i];
            uint16_t pixMask = 0x8000;

            for (int j = 0; j < font.width; j++) {
                uint16_t pixcolor = (fd & pixMask) != 0 ? fgColor : bgColor;
                pixMask >>= 1;
                SSD1351_write_pixel(SSD1351_cursor.x + j, SSD1351_cursor.y + i, pixcolor);

            }
        }
    }

    SSD1351_cursor.x += font.width;
    if ((SSD1351_cursor.x + font.width >= OLED_WIDTH - 1) && (SSD1351_cursor.y + font.height <= OLED_WIDTH - 1)) {
        SSD1351_cursor.y = SSD1351_cursor.y + font.height + 2;
        SSD1351_cursor.x = 0;
    }
    return;
}
#else // OLD CODE
static void SSD1351_write_char(uint16_t color, font_t font, char c) {
    uint16_t fd;
    if ((COLUMNS <= SSD1351_cursor.x + font.width) || (ROWS <= SSD1351_cursor.y + font.height)) {
        return;
    }
    if (c == '\n') {
        SSD1351_cursor.x = 127;
    }
    else {
        for (int i = 0; i < font.height; i++) {
            fd = font.data[(c - 32) * font.height + i];
            for (int j = 0; j < font.width; j++) {
                if ((fd << j) & 0x8000) {
                    SSD1351_write_pixel(SSD1351_cursor.x + j, SSD1351_cursor.y + i, color);
                }
            }
        }
    }
    SSD1351_cursor.x += font.width;
    if ((SSD1351_cursor.x + font.width >= 127) & (SSD1351_cursor.y + font.height <= 127)) {
        SSD1351_cursor.y = SSD1351_cursor.y + font.height + 2;
        SSD1351_cursor.x = 0;
    }
    return;
}
#endif

/**
 * Write a string.
 * 
 * @param color             [in] A 32-bit fg/bg color.
 * 
 * @param font              [in] A font_t structure.
 * 
 * @param line              [in] The string to write.    
*/

static void SSD1351_write_string(uint32_t color, font_t font, char* line)
{
    if (line != NULL) {
        while (*line != 0) {
            SSD1351_write_char(color, font, *line++);
        }
    }
}

/**
 * Write an integer.
 * 
 * @param color             [in] A 32-bit fg/bg color.
 * 
 * @param font              [in] A font_t structure.
 * 
 * @param n                 [in] 8-bit integer to display.    
*/

static void SSD1351_write_int(uint32_t color, font_t font, int8_t n)
{
    char number[5];
    sprintf(number, "%i", n);
    SSD1351_write_string(color, font, number);
}

/**
 * Write a formatted string.
 * 
 * @param color             [in] A 32-bit fg/bg color.
 * 
 * @param font              [in] A font_t structure.
 * 
 * @param format            [in] Format string.
 * 
 * @param ...               [in] Format arguments.  Limited type support: See code below.    
*/

void SSD1351_printf(uint32_t color, font_t font, const char* format, ...)
{
    if (format != NULL) {
        va_list valist;
        va_start(valist, format);
        while (*format != 0) {
            if (*format != '%') {
                SSD1351_write_char(color, font, *format);
                format++;
            }
            else {
                format++;
                switch (*format) {
                    case 's':
                        SSD1351_write_string(color, font, va_arg(valist, char*));
                        break;
                    case 'c':
                        SSD1351_write_char(color, font, va_arg(valist, int)); //?
                        break;
                    case 'i':
                        SSD1351_write_int(color, font, (int8_t)va_arg(valist, int));
                        break;
                    default:
                        break;
                }
                format++;
            }
        }
    }
}

void SSD1351_set_cursor(uint8_t x, uint8_t y)
{
    SSD1351_cursor.x = x;
    SSD1351_cursor.y = y;
}

// void SSD1351_write_image(void){
//   int i = 0;
//   while (i < DRAM_SIZE_8) {
//     int data = getchar_timeout_us(100000);
//     if (data != PICO_ERROR_TIMEOUT) {
//         DRAM_8[i++] = (uint8_t)data;
//     }
//   }
// }


void SSD1351_get_image(uint8_t buf[OLED_BUF_SIZE])
{
    SSD1351_clear();
    uint32_t i = 0;

    while (i < DRAM_SIZE_8) {
        int data = getchar_timeout_us(0);
        if (data != PICO_ERROR_TIMEOUT) {
            buf[i++] = data;
        }
    }
}

/**
* Receive the metadata text blob from the RPi host.
* 
* There are two parts:  The main metadata and the description.  These are sent
* to the Pico separately.
* 
* @param buffer             [in, out] Area to store the data in.
* 
* @param bufferMax          [in] Maximum number of characters to store.
* 
* @param bufferSize         [in] Actual number of characters received.
*/


void SSD1351_get_metadata(char* buffer, int bufferMax, int* bufferSize)
{
    int buffSize = 0;
    int c = 0;
    int i = 0;

    // The length is sent as a comma-terminated digit string before the actual data.

    while (c != ',') {
        c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT && c != ',') {
            buffSize = buffSize * 10 + (c - '0');
        }
    };

    while(i < buffSize) {
        c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (i < bufferMax) {
                *buffer++ = c;
                i++;
            }
            /*
            * Else if too many characters are being sent, we are probably screwed, but
            * at least try not to implode.
            */
        }
    }

    *bufferSize = buffSize;         // Save actual content size.
}

void SSD1351_display_image(uint8_t buf[OLED_BUF_SIZE])
{
    SSD1351_write_command(SSD1351_CMD_WRITERAM);
    // spi_write_blocking(SPI_PORT, buf, OLED_BUF_SIZE);
    memcpy(DRAM_8, buf, DRAM_SIZE_8);
    SSD1351_update();
}

#define FLASH_OFFSET (1024 * 1536)

void updateFlashData(uint8_t* flashData) // Update calibration data and flags
{
    uint Interrupt = save_and_disable_interrupts();
    flash_range_erase((FLASH_OFFSET), ((OLED_BUF_SIZE + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE);
    flash_range_program((FLASH_OFFSET), (uint8_t*)flashData, OLED_BUF_SIZE);
    restore_interrupts(Interrupt);
} color is black.
* 
* 2023-06-09 wmm  Added SSD1351_display_text_buffer.
*/

#include "ssd1351.h"

/* Buffer to hold the Display RAM Data.
* 
* You must call SSD1351_update() to transfer the buffer to the real display memory.
*/

static DRAM displayRAM;

#define DRAM_16 displayRAM.halfw
#define DRAM_8 displayRAM.byte

uint8_t buffer[OLED_BUF_SIZE] = { 0x00 };

SSD_CURSOR SSD1351_cursor;

void SSD1351_SendByte(uint8_t data)
{
    spi_write_blocking(SPI_PORT, &data, 1);
}

void SSD1351_SendBuffer(const uint8_t* buffer, uint16_t len)
{
    spi_write_blocking(SPI_PORT, buffer, len);
}

void SSD1351_write_command(uint8_t cmd)
{
    gpio_put(DC, 0);
    gpio_put(CS, 0);
    SSD1351_SendByte(cmd);
    gpio_put(CS, 1);
}

void SSD1351_write_data(uint8_t data)
{
    gpio_put(DC, 1);
    gpio_put(CS, 0);
    SSD1351_SendByte(data);
    gpio_put(CS, 1);
}

void SSD1351_write_data_buffer(const uint8_t* data, uint16_t len)
{
    gpio_put(DC, 1);
    gpio_put(CS, 0);
    SSD1351_SendBuffer(data, len);
    gpio_put(CS, 1);
}

void SSD1351_update(void)
{
    SSD1351_write_command(SSD1351_CMD_SETCOLUMN);
    SSD1351_write_data(0x00);
    SSD1351_write_data(0x7F);
    SSD1351_write_command(SSD1351_CMD_SETROW);
    SSD1351_write_data(0x00);
    SSD1351_write_data(0x7F);
    SSD1351_write_command(SSD1351_CMD_WRITERAM);
    SSD1351_write_data_buffer(DRAM_8, DRAM_SIZE_8);
}

void SSD1351_init(void)
{
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

void SSD1351_clear(void)
{
    memset(DRAM_8, 0, DRAM_SIZE_8);
}

void SSD1351_clear_8(void)
{
    memset(DRAM_8, 0, DRAM_SIZE_8);
}

/**
* Display a block of text, with scrolling.
*
* This function is used to display game meta data such as the author, title, etc.
*
* @param text                   [in] Text buffer.
*
* @param textLen                [in] Number of bytes in "text".
*
* @param color                  [in] Color (same as for SSD1351_write_char)
*
* @param font                   [in] Font.
*/

void SSD1351_display_text_buffer(const char* text, int textLen, uint32_t color, font_t font)
{
    extern SSD_CURSOR SSD1351_cursor;
    bool sawNewline = false;

    uint8_t fh = font.height + 2;         // The write_char() routine has 2 pixels of space between lines.

    while (textLen-- > 0) {
        char c = *text++;

        if (sawNewline) {
            // We have newline from end of previous line.  Now we can advance the cursor.
            // By delaying this operation we don't end up with empty space after the last
            // display line.

            SSD1351_display_text_buffer_advance_cursor_y(fh, color);
            sawNewline = false;
        }

        if (c == '\n') {                    // Don't handle until we absolutely must.
            sawNewline = true;
        }
        else if (SSD1351_cursor.x == 0 && SSD1351_cursor.y + fh >= OLED_HEIGHT) {
            // We want to start a new line, but it's not going to fit vertically.
            SSD1351_display_text_buffer_advance_cursor_y(fh, color);
        }
        else if (SSD1351_cursor.x >= OLED_WIDTH) {
            // Line won't fit horizontally, so move to next line and scroll if required.
            SSD1351_display_text_buffer_advance_cursor_y(fh, color);
        }

        // Finally, output the next character.

        if (c != '\n') {                // Not newline.
            if (c < ' ') {              // Display other control characters as a space.
                c = ' ';
            }
            SSD1351_write_char(color, font, c);
            SSD1351_update();
        }
    }
}

/**
* Advance the text cursor in the vertical direction, scrolliing the text if required.
*
* @param fh                             [in] Font height (amount to scroll).
*
* @param color                          [in] fg/bg color (see SSD1351_write_char).
*/

void SSD1351_display_text_buffer_advance_cursor_y(uint8_t fh, uint32_t color)
{
    if (SSD1351_cursor.y + fh < OLED_HEIGHT) {
        // Advancing the cursor won't go off the screen, so simply set the new cursor position.
        SSD1351_set_cursor(0, SSD1351_cursor.y + fh);
    }
    else {
        // Advancing will go off the screen, so we have to move the current display contents.
        // 
        // Suppose we have this situation on a display that only allows 3 lines:
        // ----------
        //      Line1, cursor offset width * 0 * fh.
        //      Line2, cursor offset width * 1 * fh.
        //      Line3, cursor offset width * 2 * fh.
        // ---- Line4: won't fit 

        // Now move the middle lines (Line2 and Line3 above) up on the screen.  Multiply by 2 below
        // accounts for byte to halfword size difference in cursor vs dram access.
        //
        // Number of items to move is the same as the number of items from the start up to Line3, or in
        // this example, two lines worth of data.

        memcpy(DRAM_8 /* top line*/, DRAM_8 + OLED_WIDTH * fh * 2 /* 2nd line */, OLED_WIDTH * SSD1351_cursor.y * 2);

        // Back to beginning of the line that WOULD fit (Line3 in this example).

        SSD1351_set_cursor(0, SSD1351_cursor.y - fh);

        // Now we have:
        // ----------
        //      Line2
        //      Line3
        //      Line4 goes here where Line3 used to be.
        // ----------
        // 
        // Blank out the space previously occupied by Line3.

        uint16_t bgColor = color >> 16;

        int nPixels = fh * OLED_WIDTH;  // Pixels per line of text.
        int i = SSD1351_cursor.y * OLED_WIDTH;  // Offset to display row where we want to start blanking.  Cursor x is 0.
        while (nPixels-- > 0) {
            DRAM_16[i++] = bgColor;
        }
        SSD1351_update();

        sleep_ms(200);  // How long to delay?
    }
}

void SSD1351_fill(uint16_t color)
{
    // TODO: IS THIS OK?  Need to swap bytes?
    for (int i = 0; i < DRAM_SIZE_16; i++) {
        DRAM_16[i] = color;
    }
}

/*
 * @brief Converts from RGB to a single 16bit value
 * 
 * @param r: starting x coordinate
 * 
 * @para g: starting y coordinate
 * 
 * @param b: width of the rectangle
 * 
 * @reval 16bit value with the rgb color for display
 */

uint16_t SSD1351_get_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t rgb_color = 0;
    rgb_color |= ((r / 8) << 8);
    rgb_color |= ((g / 4) >> 3);
    rgb_color |= (((g / 4) % 0x07) << 13);
    rgb_color |= ((b / 8) << 3);
    return rgb_color;
}

/**
 * @brief Writes a pixel data to the screen RAM buffer
 * 
 * @param color: Unsigned int16 containing color code
 * 
 * @param x: Pixel's horizontal position
 * 
 * @param y: Pixel's vertical position
 * 
 * @retval None
 */

void SSD1351_write_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (x > 127 || y > 127 || x < 0 || y < 0) {
        return;
    }
    int a = (x - 127) + (y * 128);
    DRAM_16[a] = color;
}

/**
* Write a single character.
* 
* This previously wrote only "non-empty" pixels.  However, that would leave
* artifacts from previous output on the screen.  fTherefore, a screen clear was
* always required.
* 
* This version sets all pixels, but the background will be black.
* 
* If you really need a different background color you can add that as a new parameter.
* 
* Note that the maximum font width is 16 pixels.
*
* @param color              [in] A 32-bit fg/bg color.
*
* @param font               [in] A font_t structure.
*
* @param c                  [in] Character to output.
*/

#if 1  // NEW CODE
static void SSD1351_write_char(uint32_t color, font_t font, char c)
{
    if ((COLUMNS <= SSD1351_cursor.x + font.width) || (ROWS <= SSD1351_cursor.y + font.height)) {
        return;
    }
    if (c == '\n') {
        SSD1351_cursor.x = 127;
    }
    else {
        uint16_t fgColor = color & 0xFFFF;
        uint16_t bgColor = (color >> 16);

        for (int i = 0; i < font.height; i++) {
            uint16_t fd = font.data[(c - 32) * font.height + i];
            uint16_t pixMask = 0x8000;

            for (int j = 0; j < font.width; j++) {
                uint16_t pixcolor = (fd & pixMask) != 0 ? fgColor : bgColor;
                pixMask >>= 1;
                SSD1351_write_pixel(SSD1351_cursor.x + j, SSD1351_cursor.y + i, pixcolor);

            }
        }
    }

    SSD1351_cursor.x += font.width;
    if ((SSD1351_cursor.x + font.width >= OLED_WIDTH - 1) && (SSD1351_cursor.y + font.height <= OLED_WIDTH - 1)) {
        SSD1351_cursor.y = SSD1351_cursor.y + font.height + 2;
        SSD1351_cursor.x = 0;
    }
    return;
}
#else // OLD CODE
static void SSD1351_write_char(uint16_t color, font_t font, char c) {
    uint16_t fd;
    if ((COLUMNS <= SSD1351_cursor.x + font.width) || (ROWS <= SSD1351_cursor.y + font.height)) {
        return;
    }
    if (c == '\n') {
        SSD1351_cursor.x = 127;
    }
    else {
        for (int i = 0; i < font.height; i++) {
            fd = font.data[(c - 32) * font.height + i];
            for (int j = 0; j < font.width; j++) {
                if ((fd << j) & 0x8000) {
                    SSD1351_write_pixel(SSD1351_cursor.x + j, SSD1351_cursor.y + i, color);
                }
            }
        }
    }
    SSD1351_cursor.x += font.width;
    if ((SSD1351_cursor.x + font.width >= 127) & (SSD1351_cursor.y + font.height <= 127)) {
        SSD1351_cursor.y = SSD1351_cursor.y + font.height + 2;
        SSD1351_cursor.x = 0;
    }
    return;
}
#endif

/**
 * Write a string.
 * 
 * @param color             [in] A 32-bit fg/bg color.
 * 
 * @param font              [in] A font_t structure.
 * 
 * @param line              [in] The string to write.    
*/

static void SSD1351_write_string(uint32_t color, font_t font, char* line)
{
    if (line != NULL) {
        while (*line != 0) {
            SSD1351_write_char(color, font, *line++);
        }
    }
}

/**
 * Write an integer.
 * 
 * @param color             [in] A 32-bit fg/bg color.
 * 
 * @param font              [in] A font_t structure.
 * 
 * @param n                 [in] 8-bit integer to display.    
*/

static void SSD1351_write_int(uint32_t color, font_t font, int8_t n)
{
    char number[5];
    sprintf(number, "%i", n);
    SSD1351_write_string(color, font, number);
}

/**
 * Write a formatted string.
 * 
 * @param color             [in] A 32-bit fg/bg color.
 * 
 * @param font              [in] A font_t structure.
 * 
 * @param format            [in] Format string.
 * 
 * @param ...               [in] Format arguments.  Limited type support: See code below.    
*/

void SSD1351_printf(uint32_t color, font_t font, const char* format, ...)
{
    if (format != NULL) {
        va_list valist;
        va_start(valist, format);
        while (*format != 0) {
            if (*format != '%') {
                SSD1351_write_char(color, font, *format);
                format++;
            }
            else {
                format++;
                switch (*format) {
                    case 's':
                        SSD1351_write_string(color, font, va_arg(valist, char*));
                        break;
                    case 'c':
                        SSD1351_write_char(color, font, va_arg(valist, int)); //?
                        break;
                    case 'i':
                        SSD1351_write_int(color, font, (int8_t)va_arg(valist, int));
                        break;
                    default:
                        break;
                }
                format++;
            }
        }
    }
}

void SSD1351_set_cursor(uint8_t x, uint8_t y)
{
    SSD1351_cursor.x = x;
    SSD1351_cursor.y = y;
}

// void SSD1351_write_image(void){
//   int i = 0;
//   while (i < DRAM_SIZE_8) {
//     int data = getchar_timeout_us(100000);
//     if (data != PICO_ERROR_TIMEOUT) {
//         DRAM_8[i++] = (uint8_t)data;
//     }
//   }
// }


void SSD1351_get_image(uint8_t buf[OLED_BUF_SIZE])
{
    SSD1351_clear();
    uint32_t i = 0;

    while (i < DRAM_SIZE_8) {
        int data = getchar_timeout_us(0);
        if (data != PICO_ERROR_TIMEOUT) {
            buf[i++] = data;
        }
    }
}

/**
* Receive the metadata text blob from the RPi host.
* 
* @param buffer             [in, out] Area to store the data in.
* 
* @param bufferMax          [in] Maximum number of characters to store.
* 
* @param bufferSize         [in] Actual number of characters received.
*/


void SSD1351_get_metadata(char* buffer, int bufferMax, int* bufferSize)
{
    int buffSize = 0;
    int c;

    // The length is sent as a comma-terminated digit string before the actual data.

    while (true) {
        c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            continue;
        }
        if (c == ',') {
            break;
        }
        buffSize = buffSize * 10 + (c - '0');
    };

    for (int i = 0;  i < buffSize; ) {
        c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (i < bufferMax) {
                *buffer++ = c;
                i++;
            }
            /*
            * Else if too many characters are being sent, we are probably screwed, but
            * at least try not to implode.
            */
        }
    }
    
    if (c == PICO_ERROR_TIMEOUT) {  // We are screwed!
        buffSize = 0;
    }

    *bufferSize = buffSize;         // Save actual content size.
}

void SSD1351_display_image(uint8_t buf[OLED_BUF_SIZE])
{
    SSD1351_write_command(SSD1351_CMD_WRITERAM);
    // spi_write_blocking(SPI_PORT, buf, OLED_BUF_SIZE);
    memcpy(DRAM_8, buf, DRAM_SIZE_8);
    SSD1351_update();
}

#define FLASH_OFFSET (1024 * 1536)

void updateFlashData(uint8_t* flashData) // Update calibration data and flags
{
    uint Interrupt = save_and_disable_interrupts();
    flash_range_erase((FLASH_OFFSET), ((OLED_BUF_SIZE + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE);
    flash_range_program((FLASH_OFFSET), (uint8_t*)flashData, OLED_BUF_SIZE);
    restore_interrupts(Interrupt);
}
