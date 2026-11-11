cmake_minimum_required(VERSION 3.13)

# Requer a variável de ambiente PICO_SDK_PATH apontando para o pico-sdk (2.x)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ps2_usb_rp2040 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(ps2_usb_rp2040
    main.c
    psx.c
    led.c
    usb_descriptors.c
)

target_include_directories(ps2_usb_rp2040 PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(ps2_usb_rp2040
    pico_stdlib
    pico_unique_id
    hardware_spi
    hardware_pio
    hardware_clocks
    tinyusb_device
)

# gera ws2812.pio.h a partir do programa PIO do LED
pico_generate_pio_header(ps2_usb_rp2040 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

pico_add_extra_outputs(ps2_usb_rp2040)
