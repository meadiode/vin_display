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


set(KLIPPER_FW_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/klipper)
include_directories(${KLIPPER_FW_SRC_DIR})

set(KLIPPER_FW_SRC_FILES
        klipper/sched.c
        klipper/basecmd.c
        klipper/command.c
        klipper/buttons.c
        klipper/serial_irq.c
        klipper/klipper.c
        klipper/alloc.c
        klipper/crc16_ccitt.c
        klipper/gpio.c
)

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
        knob.c
        usb_descriptors.c
        usb_serial.c

        ${KLIPPER_FW_SRC_FILES}

        ${CMAKE_CURRENT_BINARY_DIR}/compile_time_request.c
        )

pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/mdl2416c.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/hdsp2112.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/hdsp2000.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/buzzer.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(mdl2416c_x10 ${CMAKE_CURRENT_LIST_DIR}/knob.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

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
        cmsis_core
        hardware_pio
        hardware_rtc
        pico_lwip_iperf
        pico_lwip_ppp
        pico_rand
        hardware_adc
        tinyusb_device
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(mdl2416c_x10)


set(KLIPPER_FW_OBJ_FILES "")
set(KLIPPER_FW_CTR_FILES "")
set(KLIPPER_FW_CTR_TXT_FILES "")

foreach(SOURCE_FILE ${KLIPPER_FW_SRC_FILES})
        get_filename_component(FILE_NAME ${SOURCE_FILE} NAME)
        get_filename_component(FILE_DIR ${SOURCE_FILE} DIRECTORY)
        
        set(OBJ_FILE ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mdl2416c_x10.dir/${FILE_DIR}/${FILE_NAME}.obj)
        set(CTR_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.ctr)
        set(CTR_TXT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.ctr.txt)

        add_custom_command(
                OUTPUT ${CTR_FILE}
                COMMAND arm-none-eabi-objcopy -j '.compile_time_request' -O binary ${OBJ_FILE} ${CTR_FILE}
                DEPENDS ${OBJ_FILE}
        )

        add_custom_command(
                OUTPUT ${CTR_TXT_FILE}
                COMMAND cat ${CTR_FILE} | tr -s '\\0' '\\n' > ${CTR_TXT_FILE}
                DEPENDS ${CTR_FILE}
        )

        list(APPEND KLIPPER_FW_CTR_TXT_FILES ${CTR_TXT_FILE})
endforeach()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/compile_time_request.txt
    COMMAND cat ${KLIPPER_FW_CTR_TXT_FILES} > ${CMAKE_CURRENT_BINARY_DIR}/compile_time_request.txt
    DEPENDS ${KLIPPER_FW_CTR_TXT_FILES}
)
# -t "${CMAKE_C_COMPILER};${ASM};${L}"


add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/compile_time_request.c
    COMMAND python3 ${CMAKE_SOURCE_DIR}/klipper/scripts/buildcommands.py -d ${CMAKE_CURRENT_BINARY_DIR}/klipper.dict ${CMAKE_CURRENT_BINARY_DIR}/compile_time_request.txt ${CMAKE_CURRENT_BINARY_DIR}/compile_time_request.c
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/compile_time_request.txt
)