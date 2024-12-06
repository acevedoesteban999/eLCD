#include "eLCD.h"

const char*TAG_LCD = "LCD";
const char*TAG_I2C = "I2C";
char BufferRtcI2C[25] = "";
char BufferLCD[3][20] = {"","",""};
TaskHandle_t task_lcd_handle = NULL;

void lcd_send_cmd(char cmd)
{
    esp_err_t err;
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
    esp_err_t err;
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

void i2c_scan(){
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    
    int i;
    esp_err_t err;

    ESP_LOGI(TAG_I2C, "Starting I2C scan...");

    for (i = 0x03; i < 0x78; i++) {
        // Enviar una petición de escritura (que no envía datos) para verificar si el dispositivo responde
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK) {
            ESP_LOGI(TAG_I2C, "I2C => 0x%02x", i);
        } else if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG_I2C, "I2C bus is busy");
        }
    }

    ESP_LOGI(TAG_I2C, "I2C scan completed.");

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

esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,  // Cambié SCL a SDA
        .scl_io_num = I2C_SCL_PIN,  // Cambié SDA a SCL
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,  // Usar la frecuencia definida
    };

    i2c_param_config(I2C_PORT, &conf);

    return i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

void lcd_init(void)
{
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

void lcd_draw_symbol(uint8_t x,uint8_t y, uint8_t location) {
    lcd_goto_xy(x, y);
    lcd_send_data(location);
}

uint8_t decimal_to_bcd(uint8_t decimal) {
    return ((decimal / 10) << 4) | (decimal % 10);
}


uint8_t bcd_to_decimal(uint8_t bcd) {
     return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

void set_time() {
    uint8_t data[7];

    // Establecer la hora: segundos, minutos, horas, día de la semana, día del mes, mes, año
    data[0] = decimal_to_bcd(0);    // segundos 
    data[1] = decimal_to_bcd(0);    // minutos 
    data[2] = decimal_to_bcd(21);   // horas (12 en formato 24h)
    data[3] = decimal_to_bcd(2);    // día de la semana (1 para domingo)
    data[4] = decimal_to_bcd(21);   // día del mes (1)
    data[5] = decimal_to_bcd(10);   // mes 
    data[6] = decimal_to_bcd(24);   // año (20 + val)

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS_RTC << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Apuntar al registro de segundos
    i2c_master_write(cmd, data, sizeof(data), true); // Escribir los datos
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
}


rtc_data rtc_read(){
    uint8_t data[7];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Comenzar una transmisión I2C
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS_RTC << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);  // Apuntar al registro de segundos
    i2c_master_start(cmd);  // Repetir el comienzo para leer
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS_RTC << 1) | I2C_MASTER_READ, true);

    // Leer los 7 bytes que contienen la hora, minuto, segundo, día, fecha, mes y año
    i2c_master_read(cmd, data, 6, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, data + 6, I2C_MASTER_NACK);  // Leer el último byte
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    // Convertir los valores de BCD a decimal
    rtc_data _rtc_data;
    _rtc_data.seconds = bcd_to_decimal(data[0] & 0x7F);
    _rtc_data.minutes = bcd_to_decimal(data[1]);
    _rtc_data.hours = bcd_to_decimal(data[2] & 0x3F);  // Para formato 24 horas
    _rtc_data.day_of_week = bcd_to_decimal(data[3]);
    _rtc_data.day_of_month = bcd_to_decimal(data[4]);
    _rtc_data.month = bcd_to_decimal(data[5] & 0x1F);
    _rtc_data.year = bcd_to_decimal(data[6]); 

    return _rtc_data;
}


void start_delay_count(_delays * sd){
    sd->start_time = esp_timer_get_time();
}

void show_delay(char*LOG,_delays * sd){
    sd->end_time = esp_timer_get_time(); 
    uint64_t elapsed_time = sd->end_time - sd->start_time;
    if(elapsed_time > sd->min)
        ESP_LOGW(LOG, "%llu us", elapsed_time);
}

void task_lcd_print_all_lines_task(void * arg){
    
    lcd_print_string_at(0,0,BufferLCD[0]); 
    lcd_print_string_at(0,1,BufferLCD[1]); 
    lcd_print_string_at(0,2,BufferLCD[2]);
    vTaskDelete(NULL);
}

void lcd_print_all_lines(float frecuency,float average_threshold,float drift){
    if(task_lcd_handle != NULL)
        while (eTaskGetState(task_lcd_handle) == eRunning)
            vTaskDelay(pdMS_TO_TICKS(1));
    
    sprintf(BufferLCD[0], "F: %.3f~%.3f", frecuency/2,frecuency);                                   //frecuency
    sprintf(BufferLCD[1], "P:      %.0f", average_threshold);                                       //average_threshold       
    sprintf(BufferLCD[2], "D:%s%.4f~%s%.4f",drift>=0?" ":"", drift/2,drift>=0?" ":"", drift);       //drift


    xTaskCreatePinnedToCore(task_lcd_print_all_lines_task, "task_LCD", 5000, NULL, 20, &task_lcd_handle,1);
    
}