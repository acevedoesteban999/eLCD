#pragma once

#include "esp_log.h"
#include <unistd.h>
#include "string.h"
#include "eI2C.h"

#define I2C_PORT I2C_NUM_0              
#define I2C_PORT I2C_NUM_0                 
#define I2C_FREQ_HZ 400000              
#define SLAVE_ADDRESS_LCD 0x27        
#define MAX_ROW 4
#define MAX_COL 20 
#define LCD_CORE 1
#define LCD_CLEAR_COMMAND 0x01
#define MAX_DRAW_BUFFER 25

extern const char*TAG_LCD;
extern const char*TAG_I2C;

enum DRAW_TYPE{
    PRINT_STRING_AT,
    PRINT_STRING_CENTER,
    DRAW_SYMBOL,
    CLEAR_ROW,
    CLEAR_AT,
};

/*
enum DRAW_TYPE type;
char *str_ptr ;
char str_buff[20] ;
uint8_t x;
uint8_t y;
uint8_t location;
*/
typedef struct {
    enum DRAW_TYPE type;
    char *str_ptr ;
    char str_buff[20] ;
    uint8_t x;
    uint8_t y;
    uint8_t location;
} draw_handler;


// Funciones del LCD
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
esp_err_t lcd_init(void);
void lcd_set_pins(int SCL, int SDA);
void lcd_clear_all();
void lcd_clear_row(uint8_t y);
void lcd_goto_xy(uint8_t x, uint8_t y);
void lcd_print_string_at(uint8_t x, uint8_t y, char *str);
void lcd_create_char(uint8_t location, uint8_t charmap[]);
void lcd_print_string_center(int y, char *str);
void lcd_draw_symbol(uint8_t x, uint8_t y, uint8_t location);
void lcd_add_draw_to_buffer(draw_handler draw);
void lcd_trigger_draw();
uint8_t decimal_to_bcd(uint8_t decimal);
void lcd_init_custom_symbols(void);
uint8_t bcd_to_decimal(uint8_t bcd);