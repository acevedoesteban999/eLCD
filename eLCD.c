#include "eLCD.h"

const char*TAG_LCD = "LCD";
const char*TAG_I2C = "I2C";
TaskHandle_t task_draw_handle = NULL;

draw_handler DRAW_BUFFER[MAX_DRAW_BUFFER];
size_t draw_counter=0;

draw_handler DRAW_BUFFER_copy[MAX_DRAW_BUFFER];
size_t draw_counter_cpy=0;

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

void lcd_clear_all(){
    char clear[MAX_COL + 1];
    for(unsigned i =0 ; i< MAX_COL;i++)
        clear[i] = ' ';
    clear[MAX_COL] = '\0';
    
    for(unsigned i =0 ; i< MAX_ROW;i++)
        lcd_print_string_at(0,i,clear);
}

void lcd_clear_row(uint8_t y){
    char clear[MAX_COL + 1];
    for(unsigned i =0 ; i< MAX_COL;i++)
        clear[i] = ' ';
    clear[MAX_COL] = '\0';
    
    lcd_print_string_at(0,y,clear);
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

// void lcd_clear() {
//     lcd_send_cmd(LCD_CLEAR_COMMAND);
//     vTaskDelay(pdMS_TO_TICKS(2));
// }

void lcd_print_string_center(int y,char * str) {
    size_t len = strlen(str);
    int x = (MAX_COL - len)/2;
    lcd_print_string_at(x,y,str);
}

void lcd_draw_symbol(uint8_t x,uint8_t y, uint8_t location) {
    lcd_goto_xy(x, y);
    lcd_send_data(location);
}

void lcd_add_draw_to_buffer(draw_handler draw){
    if(draw_counter < MAX_DRAW_BUFFER){
        DRAW_BUFFER[draw_counter] = draw;
        strcpy(DRAW_BUFFER[draw_counter].str_buff, (draw.str_ptr != NULL ? draw.str_ptr : draw.str_buff) );
        draw_counter++;
    }else{
        lcd_trigger_draw();
        lcd_add_draw_to_buffer(draw);
    }
}


void _task_trigger_draw(void*arg){
    for(size_t i =0;i<draw_counter_cpy;i++){
        switch (DRAW_BUFFER_copy[i].type)
        {
        case  PRINT_STRING_AT:
            lcd_print_string_at(DRAW_BUFFER_copy[i].x,DRAW_BUFFER_copy[i].y,DRAW_BUFFER_copy[i].str_buff);
            break;
        
        case  PRINT_STRING_CENTER:
            lcd_print_string_center(DRAW_BUFFER_copy[i].y,DRAW_BUFFER_copy[i].str_buff);
            break;
        
        case  DRAW_SYMBOL:
            lcd_draw_symbol(DRAW_BUFFER_copy[i].x,DRAW_BUFFER_copy[i].y,DRAW_BUFFER_copy[i].location);
            break;
    
        default:
            break;
        }
    }
    vTaskDelete(NULL);
}

void lcd_trigger_draw(){
    if(draw_counter != 0){
        if(task_draw_handle != NULL)
            while (eTaskGetState(task_draw_handle) == eRunning)
                vTaskDelay(1);
        
        for(size_t i =0;i<draw_counter;i++){
            DRAW_BUFFER_copy[i] = DRAW_BUFFER[i];
            strcpy(DRAW_BUFFER_copy[i].str_buff, DRAW_BUFFER[i].str_buff);
        }
        draw_counter_cpy = draw_counter;
        draw_counter = 0;
        xTaskCreatePinnedToCore(_task_trigger_draw, "xtask_lines", 5000, NULL, 20, &task_draw_handle,LCD_CORE);
    }
    
}