#include "eLCD.h"

const char*TAG_LCD = "LCD";
const char*TAG_I2C = "I2C";
TaskHandle_t task_lcd_handle = NULL;

void lcd_send_cmd(char cmd)
{
    char data_u, data_l;
    uint8_t data_t[4];
    
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);
    
    data_t[0] = data_u | 0x0C;  // en=1, rs=0
    data_t[1] = data_u | 0x08;  // en=0, rs=0
    data_t[2] = data_l | 0x0C;  // en=1, rs=0
    data_t[3] = data_l | 0x08;  // en=0, rs=0

    i2c_master_write_to_device(I2C_PORT, SLAVE_ADDRESS_LCD, data_t, 4, 1000);
}

void lcd_send_data(char data)
{
    char data_u, data_l;
    uint8_t data_t[4];

    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);
    
    data_t[0] = data_u | 0x0D;  // en=1, rs=1
    data_t[1] = data_u | 0x09;  // en=0, rs=1
    data_t[2] = data_l | 0x0D;  // en=1, rs=1
    data_t[3] = data_l | 0x09;  // en=0, rs=1

    i2c_master_write_to_device(I2C_PORT, SLAVE_ADDRESS_LCD, data_t, 4, 1000);

}

void lcd_create_char(uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // Solo se pueden almacenar hasta 8 caracteres (0-7)
    lcd_send_cmd(0x40 | (location << 3)); // Comando para cargar a CGRAM
    for (int i = 0; i < 8; i++) {
        lcd_send_data(charmap[i]); // Escribir cada fila del símbolo
    }
}

void lcd_init_custom_symbols(void) {
    uint8_t warning_symbol[8] = {
        0b00100, 
        0b00100, 
        0b00100, 
        0b00100, 
        0b00100, 
        0b00100,
        0b00000, 
        0b00100  
    };
    uint8_t alarm_symbol[8] = {
        0b10001, 
        0b10001, 
        0b01010, 
        0b00100, 
        0b01010, 
        0b10001,
        0b10001, 
        0b00000  
    };

    uint8_t clear_symbol[8] = {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000
    };

    

    lcd_create_char(0, warning_symbol);     // 0 - Warning
    lcd_create_char(1, alarm_symbol);       //  1 - Danger
    lcd_create_char(2, clear_symbol);       //  2 - Clear
}

esp_err_t lcd_init(void)
{
    esp_err_t err = i2c_master_init(I2C_SDA_PIN,I2C_SCL_PIN,I2C_PORT);
    if(err != ESP_OK)
        return err;
    // Inicialización de 4 bits
    usleep(50000);  // Espera por más de 40ms
    lcd_send_cmd(0x30);
    usleep(4500);   // Espera por más de 4.1ms
    lcd_send_cmd(0x30);
    usleep(200);    // Espera por más de 100us
    lcd_send_cmd(0x30);
    usleep(200);
    lcd_send_cmd(0x20);  // Modo de 4 bits
    usleep(200);

    // Inicialización de la pantalla
    lcd_send_cmd(0x28); // Configuración de función
    usleep(1000);
    lcd_send_cmd(0x08); // Apagar la pantalla
    usleep(1000);
    lcd_send_cmd(0x01); // Limpiar la pantalla
    usleep(2000); // Espera por más de 1.53ms
    lcd_send_cmd(0x06); // Configuración de modo de entrada
    usleep(1000);
    lcd_send_cmd(0x0C); // Encender la pantalla
    usleep(2000);
    lcd_init_custom_symbols();
    return ESP_OK;
}

void lcd_goto_xy(uint8_t x, uint8_t y) {
    uint8_t address;
    switch (y) {
        case 0: address = 0x00 + x; break; // Fila 1
        case 1: address = 0x40 + x; break; // Fila 2
        case 2: address = 0x14 + x; break; // Fila 3
        case 3: address = 0x54 + x; break; // Fila 4
        default: return; // Fila no válida
    }
    lcd_send_cmd(0x80 | address); // Comando para mover el cursor
}

void lcd_print_string_at(uint8_t x, uint8_t y, char * str) {
    lcd_goto_xy(x, y); // Mueve el cursor a la posición deseada

    uint8_t buffer[100]; // Ajusta el tamaño del buffer según sea necesario
    int index = 0;

    // Convertir cada carácter de la cadena en las secuencias de datos necesarias
    for(unsigned i = 0; i < strlen(str); i++){
        char data_u = (str[i] & 0xf0);
        char data_l = ((str[i] << 4) & 0xf0);

        // Añadir las secuencias de 4 bytes por cada carácter
        buffer[index++] = data_u | 0x0D;  // en=1, rs=1
        buffer[index++] = data_u | 0x09;  // en=0, rs=1
        buffer[index++] = data_l | 0x0D;  // en=1, rs=1
        buffer[index++] = data_l | 0x09;  // en=0, rs=1
    }


    i2c_master_write_to_device(I2C_PORT, SLAVE_ADDRESS_LCD, buffer, index, 1000);
}

void lcd_clear() {
    lcd_send_cmd(LCD_CLEAR_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_print_string_center(int y,char * str) {
    size_t len = strlen(str);
    int x = (MAX_COL - len)/2;
    lcd_print_string_at(x,y,str);
}

void lcd_draw_symbol(uint8_t x,uint8_t y, uint8_t location) {
    lcd_goto_xy(x, y);
    lcd_send_data(location);
}

void task_lcd_print_all_lines_task(void * arg){
    char (*buffer)[20] = (char (*)[20])arg;

    lcd_print_string_at(0,0,buffer[0]); 
    lcd_print_string_at(0,1,buffer[1]); 
    lcd_print_string_at(0,2,buffer[2]);
    vTaskDelete(NULL);
}


void lcd_print_lines_xtask(char*buffer,size_t size){
    if(task_lcd_handle != NULL)
        while (eTaskGetState(task_lcd_handle) == eRunning)
            vTaskDelay(pdMS_TO_TICKS(1));
    
    xTaskCreatePinnedToCore(task_lcd_print_all_lines_task, "xtask_lines", 5000, buffer, 20, &task_lcd_handle,LCD_CORE);
    
}