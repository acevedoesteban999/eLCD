#pragma once
#include "driver/i2c.h"
#include "esp_log.h"
#include <unistd.h>
#include "string.h"
#include "esp_timer.h"

#define I2C_PORT I2C_NUM_0               // Puerto I2C
#define I2C_SCL_PIN 22                   // Pin GPIO para SCL
#define I2C_SDA_PIN 21                   // Pin GPIO para SDA
#define I2C_FREQ_HZ 400000               // Frecuencia del bus I2C
#define SLAVE_ADDRESS_LCD 0x27           // Dirección I2C del adaptador LCD (PCF8574)
#define SLAVE_ADDRESS_RTC 0x68 


extern const char*TAG_LCD;
extern const char*TAG_I2C;
extern char BufferLCD[3][20];
extern char BufferRtcI2C[25];
typedef struct{
   uint64_t start_time; 
   uint64_t end_time; 
   uint64_t min; 

} _delays;

typedef struct{
   uint8_t seconds;
   uint8_t minutes;
   uint8_t hours;
   uint8_t day_of_week;
   uint8_t day_of_month;
   uint8_t month;
   uint8_t year;
}rtc_data;

// Funciones del LCD
esp_err_t i2c_master_init(void);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_init(void);
void lcd_goto_xy(uint8_t x, uint8_t y);
void lcd_print_string_at(uint8_t x, uint8_t y, char *str);
void lcd_create_char(uint8_t location, uint8_t charmap[]);
void lcd_draw_symbol(uint8_t x,uint8_t y, uint8_t location);
uint8_t decimal_to_bcd(uint8_t decimal);
void lcd_init_custom_symbols(void);
void i2c_scan();
rtc_data rtc_read();
void start_delay_count(_delays *d);
void show_delay(char *LOG, _delays *d);
void lcd_print_all_lines(float frecuency,float average_threshold,float drift);
uint8_t bcd_to_decimal(uint8_t bcd);
void set_time();
