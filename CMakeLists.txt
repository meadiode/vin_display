cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(fpanel_remote C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

add_executable(fpanel_remote
        main.c
        fsdata.c
        http_server.c
        websocket.c
        )

target_compile_definitions(fpanel_remote PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
        PPP_SUPPORT=1
        PING_USE_SOCKETS=1
        )
target_include_directories(fpanel_remote PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpserver
        )
target_link_libraries(fpanel_remote
        pico_cyw43_arch_lwip_sys_freertos
        pico_stdlib
        pico_lwip_iperf
        pico_lwip_ppp
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(fpanel_remote)