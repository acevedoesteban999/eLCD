idf_component_register(
    SRCS
        "eLCD.c"
    
    INCLUDE_DIRS 
        "include"
    
    REQUIRES
        eI2C
        esp_timer
)
