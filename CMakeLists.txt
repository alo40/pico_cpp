cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_app C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_app
    src/main.c
    src/dashboard.c
    src/ssd1306.c
    src/vedirect_parser.c
    src/vedirect_uart.c
)

target_link_libraries(pico_app
    pico_stdlib
    hardware_i2c
    hardware_irq
    hardware_uart
)

target_include_directories(pico_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

pico_enable_stdio_usb(pico_app 1)
pico_enable_stdio_uart(pico_app 0)

pico_add_extra_outputs(pico_app)
