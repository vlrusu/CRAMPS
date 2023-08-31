cmake_minimum_required(VERSION 3.13)

set(NAME CRAMPS)
set(PICO_SDK_PATH "/home/mu2e/pico/pico-sdk")
include(pico_sdk_import.cmake)

project(CRAMPS C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    src/main.c
    src/MCP23S17.c
    src/MI2C.c
    src/AMBads1110.c
    src/utils.c
)

target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_adc hardware_pio hardware_spi)

pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_uart(CRAMPS 0)
pico_enable_stdio_usb(CRAMPS 1)


install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)
