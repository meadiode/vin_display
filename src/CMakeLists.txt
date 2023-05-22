cmake_minimum_required(VERSION 3.12)

# pioc-sdc: branch master; sha: f396d05f8252d4670d4ea05c8b7ac938ef0cd381
# FreeRTOS-Kernel: branch smp; sha: 0f9e6e5b521009f585018b47f7e8aa573f010206


include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(mdl2416c_x10 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

add_executable(mdl2416c_x10
        main.c
        fsdata.c
        http_server.c
        websocket.c
        dhcp_server.c
        display.c
        timesync.c
        mjson.c
        mdl2416c.c
        hdsp2112.c
        hdsp2000.c
        buzzer.c
        )

pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/mdl2416c.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/hdsp2112.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/hdsp2000.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/buzzer.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(mdl2416c_x10 PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
        PPP_SUPPORT=1
        PING_USE_SOCKETS=1
        )

target_include_directories(mdl2416c_x10 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpserver
        )

target_link_libraries(mdl2416c_x10
        pico_cyw43_arch_lwip_sys_freertos
        pico_stdlib
        hardware_pio
        hardware_rtc
        pico_lwip_iperf
        pico_lwip_ppp
        pico_rand
        hardware_adc
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(mdl2416c_x10)