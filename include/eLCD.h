#pragma once

#include "esp_log.h"
#include <unistd.h>
#include "string.h"
#include "eI2C.h"
               
#define I2C_FREQ_HZ 400000          
#define MAX_ROW 4
#define MAX_COL 20 
#define LCD_CORE 1
#define LCD_CLEAR_COMMAND 0x01
#define MAX_ELCD_BUFFER 25
#define ELCD_DEFAULT_SLAVE_ADDR 0x27

extern const char*TAG_LCD;
extern const char*TAG_I2C;

enum DRAW_TYPE{
    PRINT_STRING_AT,
    PRINT_STRING_CENTER,
    PRINT_STRING_CENTER_C,
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
} elcd_handler;

         
extern unsigned char ELCD_SLAVE_ADDR;

// Funciones del LCD
void elcd_send_cmd(char cmd);
void elcd_send_data(char data);
void elcd_create_symbol(uint8_t location, uint8_t charmap[8]);
esp_err_t elcd_init();
void elcd_set_slave(unsigned char slave_addr);
void elcd_clear_all();
void elcd_clear_row(uint8_t y);
void elcd_goto_xy(uint8_t x, uint8_t y);
void elcd_print_string_at(uint8_t x, uint8_t y, char *str);
void elcd_print_string_center(int y, char *str);
void elcd_print_string_center_c(int y, char *str, int c);
void elcd_draw_symbol(uint8_t x, uint8_t y, uint8_t location);
void elcd_add_to_buffer(elcd_handler draw);
void elcd_check_trigger();
void elcd_force_trigger();
uint8_t decimal_to_bcd(uint8_t decimal);
uint8_t bcd_to_decimal(uint8_t bcd);