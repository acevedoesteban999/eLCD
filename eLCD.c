#include "eLCD.h"


unsigned char ELCD_SLAVE_ADDR = ELCD_DEFAULT_SLAVE_ADDR;

TaskHandle_t task_elcd_handle = NULL;

elcd_handler ELCD_BUFFER[MAX_ELCD_BUFFER];
size_t elcd_counter=0;

elcd_handler ELCD_BUFFER_copy[MAX_ELCD_BUFFER];
size_t elcd_counter_cpy=0;

void elcd_send_cmd(char cmd) {
    char data_u, data_l;
    uint8_t data_t[4];
    
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);
    
    data_t[0] = data_u | 0x0C;  // en=1, rs=0
    data_t[1] = data_u | 0x08;  // en=0, rs=0
    data_t[2] = data_l | 0x0C;  // en=1, rs=0
    data_t[3] = data_l | 0x08;  // en=0, rs=0

    ei2c_write(ELCD_SLAVE_ADDR, data_t, 4);
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

    ei2c_write( ELCD_SLAVE_ADDR, data_t, 4);
}

void elcd_create_symbol(uint8_t location, uint8_t charmap[8]) {
    location &= 0x7;                        // Solo se pueden almacenar hasta 8 caracteres (0-7)
    elcd_send_cmd(0x40 | (location << 3));  // Comando para cargar a CGRAM
    for (int i = 0; i < 8; i++) {
        elcd_send_data(charmap[i]);         // Escribir cada fila del símbolo
    }
}

esp_err_t elcd_init()
{
    esp_err_t err = ei2c_master_init();
    if(err != ESP_OK)
        return err;

    // Inicialización de 4 bits
    usleep(50000);          // Espera por más de 40ms
    elcd_send_cmd(0x30);
    usleep(4500);           // Espera por más de 4.1ms
    elcd_send_cmd(0x30);
    usleep(200);            // Espera por más de 100us
    elcd_send_cmd(0x30);
    usleep(200);
    elcd_send_cmd(0x20);    // Modo de 4 bits
    usleep(200);

    // Inicialización de la pantalla
    elcd_send_cmd(0x28);    // Configuración de función
    usleep(1000);
    elcd_send_cmd(0x08);    // Apagar la pantalla
    usleep(1000);
    elcd_send_cmd(0x01);    // Limpiar la pantalla
    usleep(2000);           // Espera por más de 1.53ms
    elcd_send_cmd(0x06);    // Configuración de modo de entrada
    usleep(1000);
    elcd_send_cmd(0x0C);    // Encender la pantalla
    usleep(2000);
    
    return ESP_OK;
}

void elcd_set_slave(unsigned char slave_addr){
    ELCD_SLAVE_ADDR = slave_addr;
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
    ei2c_write( ELCD_SLAVE_ADDR, buffer, index);
}

void elcd_print_string_center(int y,char * str) {
    elcd_clear_row(y);
    size_t len = strlen(str);
    int x = (MAX_COL - len)/2;
    elcd_print_string_at(x,y,str);
}

void elcd_print_string_center_c(int y,char * str,int c) {
    elcd_clear_row(y);
    size_t len = strlen(str);
    int x = (MAX_COL - len - c)/2 ;
    elcd_print_string_at(x,y,str);
}

void elcd_draw_symbol(uint8_t x,uint8_t y, uint8_t location) {
    elcd_goto_xy(x, y);
    elcd_send_data(location);
}

bool elcd_is_task_running(TaskHandle_t task_handle) {
    return (task_handle != NULL && eTaskGetState(task_handle) == eRunning);
}

void elcd_add_to_buffer(elcd_handler handler) {
    if (elcd_counter < MAX_ELCD_BUFFER) {
        ELCD_BUFFER[elcd_counter] = handler;
        strcpy(ELCD_BUFFER[elcd_counter].str_buff, (handler.str_ptr != NULL ? handler.str_ptr : handler.str_buff));
        elcd_counter++;
    } else {
        elcd_force_trigger();
        elcd_add_to_buffer(handler);
    }
}

void elcd_clear_at(uint8_t x , uint8_t y , uint8_t len){
    char clear[len + 1];
    for(unsigned i =0 ; i< len;i++)
        clear[i] = ' ';
    clear[len] = '\0';
    elcd_print_string_at(x,y,clear);
}

void _elcd__task_trigger(void* arg) {
    for (size_t i = 0; i < elcd_counter_cpy; i++) {
        switch (ELCD_BUFFER_copy[i].type) {
            case PRINT_STRING_AT:
                elcd_print_string_at(ELCD_BUFFER_copy[i].x, ELCD_BUFFER_copy[i].y, ELCD_BUFFER_copy[i].str_buff);
                break;
            case PRINT_STRING_CENTER:
                elcd_print_string_center(ELCD_BUFFER_copy[i].y, ELCD_BUFFER_copy[i].str_buff);
                break;
            case PRINT_STRING_CENTER_C:
                elcd_print_string_center_c(ELCD_BUFFER_copy[i].y, ELCD_BUFFER_copy[i].str_buff,ELCD_BUFFER_copy[i].location);
                break;
            case DRAW_SYMBOL:
                elcd_draw_symbol(ELCD_BUFFER_copy[i].x, ELCD_BUFFER_copy[i].y, ELCD_BUFFER_copy[i].location);
                break;
            case CLEAR_ROW:
                elcd_clear_row(ELCD_BUFFER_copy[i].y);
                break;
            case CLEAR_AT:
                elcd_clear_at(ELCD_BUFFER_copy[i].x, ELCD_BUFFER_copy[i].y, ELCD_BUFFER_copy[i].location);
                break;
            default:
                break;
        }
    }
    vTaskDelete(NULL);
}

void elcd_check_trigger(){
    if (elcd_counter >= MAX_ELCD_BUFFER)
        return elcd_force_trigger();
}

void elcd_force_trigger() {
    if (elcd_counter != 0) {
        if (elcd_is_task_running(task_elcd_handle)) {
            while (elcd_is_task_running(task_elcd_handle)) {
                vTaskDelay(1);
            }
        }
        for (size_t i = 0; i < elcd_counter; i++) {
            ELCD_BUFFER_copy[i] = ELCD_BUFFER[i];
            strcpy(ELCD_BUFFER_copy[i].str_buff, ELCD_BUFFER[i].str_buff);
        }
        elcd_counter_cpy = elcd_counter;
        elcd_counter = 0;
        xTaskCreatePinnedToCore(_elcd__task_trigger, "xtask_lines", 5000, NULL, 20, &task_elcd_handle, LCD_CORE);
    }
}