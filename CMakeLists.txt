cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(main
    main.c
)

### https://forums.raspberrypi.com/viewtopic.php?t=301902
# pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
# target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
# pico_set_boot_stage2(main slower_boot2)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)
# target_sources(test PRIVATE test.c)

pico_add_extra_outputs(main)

target_link_libraries(main 
    pico_stdlib
    pico_multicore
    hardware_pio
    )

    