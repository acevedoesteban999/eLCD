#include "eLCD.h"

const char*TAG_LCD = "LCD";
const char*TAG_I2C = "I2C";
int I2C_SCL_PIN = 21;                 
int I2C_SDA_PIN = 22;
TaskHandle_t task_draw_handle = NULL;

draw_handler DRAW_BUFFER[MAX_DRAW_BUFFER];
size_t draw_counter=0;

draw_handler DRAW_BUFFER_copy[MAX_DRAW_BUFFER];
size_t draw_counter_cpy=0;

void elcd_send_cmd(char cmd) {
    char data_u, data_l;
    uint8_t data_t[4];
    
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);
    
    data_t[0] = data_u | 0x0C;  // en=1, rs=0
    data_t[1] = data_u | 0x08;  // en=0, rs=0
    data_t[2] = data_l | 0x0C;  // en=1, rs=0
    data_t[3] = data_l | 0x08;  // en=0, rs=0

    ei2c_write(I2C_PORT, SLAVE_ADDRESS_LCD, data_t, 4);
}

void elcd_send_data(char data) {
    char data_u, data_l;
    uint8_t data_t[4];

    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);
    
    data_t[0] = data_u | 0x0D;  // en=1, rs=1
    data_t[1] = data_u | 0x09;  // en=0, rs=1
    data_t[2] = data_l | 0x0D;  // en=1, rs=1
    data_t[3] = data_l | 0x09;  // en=0, rs=1

    ei2c_write(I2C_PORT, SLAVE_ADDRESS_LCD, data_t, 4);
}

void elcd_create_char(uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // Solo se pueden almacenar hasta 8 caracteres (0-7)
    elcd_send_cmd(0x40 | (location << 3)); // Comando para cargar a CGRAM
    for (int i = 0; i < 8; i++) {
        elcd_send_data(charmap[i]); // Escribir cada fila del símbolo
    }
}

void eelcd_init_custom_symbols(void) {
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

    elcd_create_char(0, warning_symbol);     // 0 - Warning
    elcd_create_char(1, alarm_symbol);       //  1 - Danger
}

esp_err_t elcd_init(void)
{
    esp_err_t err = ei2c_master_init(I2C_SDA_PIN,I2C_SCL_PIN,I2C_PORT);
    if(err != ESP_OK)
        return err;
    // Inicialización de 4 bits
    usleep(50000);  // Espera por más de 40ms
    elcd_send_cmd(0x30);
    usleep(4500);   // Espera por más de 4.1ms
    elcd_send_cmd(0x30);
    usleep(200);    // Espera por más de 100us
    elcd_send_cmd(0x30);
    usleep(200);
    elcd_send_cmd(0x20);  // Modo de 4 bits
    usleep(200);

    // Inicialización de la pantalla
    elcd_send_cmd(0x28); // Configuración de función
    usleep(1000);
    elcd_send_cmd(0x08); // Apagar la pantalla
    usleep(1000);
    elcd_send_cmd(0x01); // Limpiar la pantalla
    usleep(2000); // Espera por más de 1.53ms
    elcd_send_cmd(0x06); // Configuración de modo de entrada
    usleep(1000);
    elcd_send_cmd(0x0C); // Encender la pantalla
    usleep(2000);
    eelcd_init_custom_symbols();
    return ESP_OK;
}

void elcd_set_pins(int SCL,int SDA){
    I2C_SCL_PIN = SCL;
    I2C_SDA_PIN = SDA;
}

void elcd_clear_all(){
    char clear[MAX_COL + 1];
    for(unsigned i =0 ; i< MAX_COL;i++)
        clear[i] = ' ';
    clear[MAX_COL] = '\0';
    
    for(unsigned i =0 ; i< MAX_ROW;i++)
        elcd_print_string_at(0,i,clear);
}

void elcd_clear_row(uint8_t y){
    char clear[MAX_COL + 1];
    for(unsigned i =0 ; i< MAX_COL;i++)
        clear[i] = ' ';
    clear[MAX_COL] = '\0';
    
    elcd_print_string_at(0,y,clear);
}

void elcd_goto_xy(uint8_t x, uint8_t y) {
    if (x >= MAX_COL) x = MAX_COL - 1; // Limitar x al máximo de columnas
    if (y >= MAX_ROW) y = MAX_ROW - 1; // Limitar y al máximo de filas

    uint8_t address;
    switch (y) {
        case 0: address = 0x00 + x; break; // Fila 1
        case 1: address = 0x40 + x; break; // Fila 2
        case 2: address = 0x14 + x; break; // Fila 3
        case 3: address = 0x54 + x; break; // Fila 4
        default: return; // Fila no válida
    }
    elcd_send_cmd(0x80 | address); // Comando para mover el cursor
}

void elcd_print_string_at(uint8_t x, uint8_t y, char * str) {
    elcd_goto_xy(x, y); 

    uint8_t buffer[100]; 
    int index = 0;

    size_t len = strlen(str);

    if (x + len > MAX_COL) {
        len = MAX_COL - x;
    }

    for (unsigned i = 0; i < len; i++) {
        char data_u = (str[i] & 0xf0);
        char data_l = ((str[i] << 4) & 0xf0);

        buffer[index++] = data_u | 0x0D;  // en=1, rs=1
        buffer[index++] = data_u | 0x09;  // en=0, rs=1
        buffer[index++] = data_l | 0x0D;  // en=1, rs=1
        buffer[index++] = data_l | 0x09;  // en=0, rs=1
    }
    ei2c_write(I2C_PORT, SLAVE_ADDRESS_LCD, buffer, index);
}

void elcd_print_string_center(int y,char * str) {
    elcd_clear_row(y);
    size_t len = strlen(str);
    int x = (MAX_COL - len)/2;
    elcd_print_string_at(x,y,str);
}

void elcd_draw_symbol(uint8_t x,uint8_t y, uint8_t location) {
    elcd_goto_xy(x, y);
    elcd_send_data(location);
}

bool elcd_is_task_running(TaskHandle_t task_handle) {
    return (task_handle != NULL && eTaskGetState(task_handle) == eRunning);
}

void elcd_add_draw_to_buffer(draw_handler draw) {
    if (draw_counter < MAX_DRAW_BUFFER) {
        DRAW_BUFFER[draw_counter] = draw;
        strcpy(DRAW_BUFFER[draw_counter].str_buff, (draw.str_ptr != NULL ? draw.str_ptr : draw.str_buff));
        draw_counter++;
    } else {
        elcd_trigger_draw();
        elcd_add_draw_to_buffer(draw);
    }
}

void elcd_clear_at(uint8_t x , uint8_t y , uint8_t len){
    char clear[len + 1];
    for(unsigned i =0 ; i< len;i++)
        clear[i] = ' ';
    clear[len] = '\0';
    elcd_print_string_at(x,y,clear);
}

void _task_trigger_draw(void* arg) {
    for (size_t i = 0; i < draw_counter_cpy; i++) {
        switch (DRAW_BUFFER_copy[i].type) {
            case PRINT_STRING_AT:
                elcd_print_string_at(DRAW_BUFFER_copy[i].x, DRAW_BUFFER_copy[i].y, DRAW_BUFFER_copy[i].str_buff);
                break;
            case PRINT_STRING_CENTER:
                elcd_print_string_center(DRAW_BUFFER_copy[i].y, DRAW_BUFFER_copy[i].str_buff);
                break;
            case DRAW_SYMBOL:
                elcd_draw_symbol(DRAW_BUFFER_copy[i].x, DRAW_BUFFER_copy[i].y, DRAW_BUFFER_copy[i].location);
                break;
            case CLEAR_ROW:
                elcd_clear_row(DRAW_BUFFER_copy[i].y);
                break;
            case CLEAR_AT:
                elcd_clear_at(DRAW_BUFFER_copy[i].x, DRAW_BUFFER_copy[i].y, DRAW_BUFFER_copy[i].location);
                break;
            default:
                break;
        }
    }
    vTaskDelete(NULL);
}

void elcd_trigger_draw() {
    if (draw_counter != 0) {
        if (elcd_is_task_running(task_draw_handle)) {
            while (elcd_is_task_running(task_draw_handle)) {
                vTaskDelay(1);
            }
        }
        for (size_t i = 0; i < draw_counter; i++) {
            DRAW_BUFFER_copy[i] = DRAW_BUFFER[i];
            strcpy(DRAW_BUFFER_copy[i].str_buff, DRAW_BUFFER[i].str_buff);
        }
        draw_counter_cpy = draw_counter;
        draw_counter = 0;
        xTaskCreatePinnedToCore(_task_trigger_draw, "xtask_lines", 5000, NULL, 20, &task_draw_handle, LCD_CORE);
    }
}