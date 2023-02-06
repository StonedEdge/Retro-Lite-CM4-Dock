cmake_minimum_required(VERSION  3.12)

include(pico_sdk_import.cmake)

project(pico-spi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(
    main
    main.c
    ssd1351.c
)

target_link_libraries(
    main
    pico_stdlib
    hardware_gpio
    hardware_spi
    hardware_dma
    pico_binary_info
)

pico_add_extra_outputs(main)