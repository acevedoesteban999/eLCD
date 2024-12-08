#pragma once

#include "esp_log.h"
#include <unistd.h>
#include "string.h"
#include "eI2C.h"

#define I2C_PORT I2C_NUM_0              
#define I2C_SCL_PIN 22                  
#define I2C_SDA_PIN 21                  
#define I2C_FREQ_HZ 400000              
#define SLAVE_ADDRESS_LCD 0x27        
#define MAX_ROW 4
#define MAX_COL 20 


extern const char*TAG_LCD;
extern const char*TAG_I2C;



// Funciones del LCD
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
esp_err_t lcd_init(void);
void lcd_goto_xy(uint8_t x, uint8_t y);
void lcd_print_string_at(uint8_t x, uint8_t y, char *str);
void lcd_create_char(uint8_t location, uint8_t charmap[]);
void lcd_print_string_center(int y, char *str);
void lcd_draw_symbol(uint8_t x, uint8_t y, uint8_t location);
uint8_t decimal_to_bcd(uint8_t decimal);
void lcd_init_custom_symbols(void);
void lcd_print_all_lines(float frecuency,float average_threshold,float drift);
uint8_t bcd_to_decimal(uint8_t bcd);