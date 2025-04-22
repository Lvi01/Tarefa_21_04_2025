cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(final C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(final final.c inc/ssd1306.c)

pico_set_program_name(final "final")
pico_set_program_version(final "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(final 1)
pico_enable_stdio_usb(final 1)

pico_generate_pio_header(final ${CMAKE_CURRENT_LIST_DIR}/final.pio)

# Add the standard library to the build
target_link_libraries(final
        pico_stdlib
        hardware_i2c 
        hardware_adc
        hardware_pwm
        hardware_pio
        hardware_gpio)

# Add the standard include files to the build
target_include_directories(final PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Add any user requested libraries
target_link_libraries(final)

pico_add_extra_outputs(final)