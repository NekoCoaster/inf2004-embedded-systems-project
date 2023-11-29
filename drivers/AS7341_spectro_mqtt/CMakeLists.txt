# Get the name of the current folder (your project folder)
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Create the executable with the folder's name
add_executable(
    ${PROJECT_NAME} # Use the folder name as the target name
    ${PROJECT_NAME}.c
    mqtt_Rebuilt.c      #Provides MQTT functionality
    AS7341_Rebuilt.c    #The Sensor Library
    i2c_tools.c         #Custom Made I2C Tools for use with the AS7341
    ws2812b_Rebuilt.c   #The LED Library (reserved for future use)
    cycle_delay.S       #Custom Made Delay Function used by the LED Library
    
)
target_include_directories( ${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_link_libraries(
    ${PROJECT_NAME} 
    pico_stdlib              # for core functionality
    hardware_gpio
    hardware_i2c
    hardware_irq
    hardware_clocks
    pico_lwip_mqtt
    #pico_cyw43_arch_lwip_threadsafe_background #Background Interrupt Mode (Choose one)
    pico_cyw43_arch_lwip_poll #Polling Mode (Choose one) 
)

# enable usb output, enable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
