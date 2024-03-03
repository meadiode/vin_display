cmake_minimum_required(VERSION 3.12)

# pico-sdk: branch master; sha: f396d05f8252d4670d4ea05c8b7ac938ef0cd381
# FreeRTOS-Kernel: branch smp; sha: 0f9e6e5b521009f585018b47f7e8aa573f010206


include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(vin_display C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()


add_definitions(
        -DBOARD_MDL2416C_X10=1
        -DBOARD_HDSP2112_X6=2
)

set(VIN_DSIP_SOURCES
        main.c
        display.c
        buttons.c
        command.c
        mjson.c
        usb_descriptors.c
        usb_serial.c
        if_uart.c
        buzzer.c
        knob.c
)

set (VIN_DSIP_LIBS
        pico_stdlib
        cmsis_core
        hardware_dma
        hardware_pio
        hardware_rtc
        pico_rand
        hardware_adc
        hardware_pwm
        tinyusb_device
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
)


# MDL2416C_X10

add_executable(mdl2416c_x10 mdl2416c.c ${VIN_DSIP_SOURCES})

pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/mdl2416c.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/buzzer.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/knob.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_include_directories(mdl2416c_x10 PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(mdl2416c_x10 PRIVATE VIN_DISP_BOARD=BOARD_MDL2416C_X10)

target_link_libraries(mdl2416c_x10 ${VIN_DSIP_LIBS})
pico_add_extra_outputs(mdl2416c_x10)


# HDSP2112_X6

add_executable(hdsp2112_x6 hdsp2112.c ${VIN_DSIP_SOURCES})

pico_generate_pio_header(hdsp2112_x6 ${CMAKE_CURRENT_LIST_DIR}/hdsp2112.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(hdsp2112_x6 ${CMAKE_CURRENT_LIST_DIR}/buzzer.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(hdsp2112_x6 ${CMAKE_CURRENT_LIST_DIR}/knob.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_include_directories(hdsp2112_x6 PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(hdsp2112_x6 PRIVATE VIN_DISP_BOARD=BOARD_HDSP2112_X6)

target_link_libraries(hdsp2112_x6 ${VIN_DSIP_LIBS})
pico_add_extra_outputs(hdsp2112_x6)
