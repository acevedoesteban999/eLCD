# eLCD

The eLCD module is an implementation for controlling an LCD display using the ESP32. 

## Features

- Support for custom characters.
- Functions for clearing and writing to the display.
- Buffering for efficient drawing.

## Dependencies

This module depends on the following components:
- [eI2C](https://github.com/acevedoesteban999/eI2C)

## Main Functions

- `elcd_send_cmd(char cmd)`: Sends a command to the LCD.
- `elcd_send_data(char data)`: Sends data to the LCD.
- `elcd_create_symbol(uint8_t location, uint8_t charmap[])`: Creates a custom character.
- `elcd_init()`: Initializes the LCD.
- `elcd_set_pins(int SCL, int SDA)`: Sets the I2C pins.
- `elcd_clear_all()`: Clears the entire display.
- `elcd_clear_row(uint8_t y)`: Clears a specific row.
- `elcd_goto_xy(uint8_t x, uint8_t y)`: Moves the cursor to a specific position.
- `elcd_print_string_at(uint8_t x, uint8_t y, char * str)`: Prints a string at a specific position.
- `elcd_print_string_center(int y, char * str)`: Prints a string centered on a specific row.
- `elcd_draw_symbol(uint8_t x, uint8_t y, uint8_t location)`: Draws a custom symbol.
- `elcd_add_to_buffer(elcd_handler draw)`: Adds a draw command to the buffer.
- `elcd_force_trigger()`: Triggers the drawing of buffered commands.

## Example Usage

```c
#include "eLCD.h"

void app_main() {
    // Initialize the LCD
    elcd_init();

    // Print a string at position (0, 0)
    elcd_print_string_at(0, 0, "Hello, World!");

    // Draw a custom symbol at position (5, 1)
    elcd_draw_symbol(5, 1, 0);
}
```
