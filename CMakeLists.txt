idf_component_register(
    SRCS
        "eLCD.c"
    
    INCLUDE_DIRS 
        "include"
    
    REQUIRES
        driver
        esp_timer
)
