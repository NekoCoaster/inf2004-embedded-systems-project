# Get the name of the current folder (your project folder)
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Create the executable with the folder's name
add_executable(
    ${PROJECT_NAME} # Use the folder name as the target name
    ${PROJECT_NAME}.c
)

target_include_directories( ${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_link_libraries(
    ${PROJECT_NAME} 
    pico_stdlib              
    #pico_cyw43_arch_lwip_threadsafe_background #Background Interrupt Mode (Choose one)
    pico_cyw43_arch_lwip_poll #Polling Mode (Choose one)
)

# enable usb output, enable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})