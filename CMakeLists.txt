cmake_minimum_required(VERSION  3.12)

include(pico_sdk_import.cmake)

project(pico-image C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(
    main
    main.c
    ssd1351.c
    fonts.c
)

target_link_libraries(
    main
    pico_stdlib
    hardware_gpio
    hardware_spi
    hardware_dma
    pico_binary_info
    pico_multicore
    pico_stdio_usb
)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_add_extra_outputs(main)
