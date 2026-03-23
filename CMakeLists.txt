cmake_minimum_required(VERSION 3.13)

### HEADERS BLINK ########

# # CREATE THE TARGET FIRST
# add_executable(pico_app
#     main.c
# )

# # Link required libraries
# target_link_libraries(pico_app
#     pico_stdlib
# )

### HEADERS OLED ########

# ADD your driver file here
add_executable(pico_app
    main.c
    ssd1306.c
)

# ADD hardware_i2c
target_link_libraries(pico_app
    pico_stdlib
    hardware_i2c
)

# (optional but recommended)
target_include_directories(pico_app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

### MAIN BODY ############

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_app C CXX ASM)

pico_sdk_init()

pico_enable_stdio_usb(pico_app 1)
pico_enable_stdio_uart(pico_app 0)

pico_add_extra_outputs(pico_app)
