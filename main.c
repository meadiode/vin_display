/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <lwip/ip4_addr.h>
#include <hardware/adc.h>
#include <FreeRTOS.h>
#include <task.h>

#include "http_server.h"
#include "dhcp_server.h"
#include "fpanel.h"

#ifndef PING_ADDR
#define PING_ADDR "192.168.18.7"
#endif
#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif
#define TEST_TASK_PRIORITY              ( tskIDLE_PRIORITY + 1UL )

#define SW_MAJOR_VERSION 0
#define SW_MINOR_VERSION 424

void main_task(__unused void *params) {

    printf("\n");    
    printf("SW version: %u.%u\n", SW_MAJOR_VERSION, SW_MINOR_VERSION);


    if (cyw43_arch_init()) {
        printf("Failed to initialise\n");
        return;
    }

    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi... %s:%s\n", WIFI_SSID, WIFI_PASSWORD);    

    // cyw43_arch_enable_ap_mode("pico_w", "", CYW43_AUTH_OPEN);

    // ip4_addr_t gw, mask;
    // IP4_ADDR(&gw, 192, 168, 4, 1);
    // IP4_ADDR(&mask, 255, 255, 255, 0);

    // // Start the dhcp server
    // dhcp_server_t dhcp_server;
    // dhcp_server_init(&dhcp_server, &gw, &mask);

    for (;;)
    {
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            printf("failed to connect.\n");
            vTaskDelay(2000);
        } else {
            printf("Connected\n");
            break;
        }

    }

    // ip_addr_t ping_addr;
    // ip4_addr_set_u32(&ping_addr, ipaddr_addr(PING_ADDR));
    // ping_init(&ping_addr);

    printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), 80);

    fpanel_init();

    http_server_init();

    for (;;)
    {
        vTaskDelay(100);
    }

    cyw43_arch_deinit();
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "main", configMINIMAL_STACK_SIZE * 10, NULL, TEST_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    /*  we must bind the main task to one core (well at least while the init is called)
     * (note we only do this in NO_SYS mode, because cyw43_arch_freertos
     * takes care of it otherwise)
     */
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

int main( void )
{
    stdio_init_all();

    adc_init();

#if ( portSUPPORT_SMP == 1 ) && ( configNUM_CORES == 2 )
    printf("Starting FreeRTOS SMP on both cores\n");
    vLaunch();
#else
# error "Wrong FreeRTOS configuration: portSUPPORT_SMP != 1; configNUM_CORES != 2"
#endif

    /* Something went wrong */
    for (;;)
    {
        __breakpoint();
    }
    
    return 0;
}


void vApplicationMallocFailedHook(void)
{

    printf("malloc failure!\n");
    // for (;;)
    // {
    //     __breakpoint();
    // }
}


void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
    for (;;)
    {
        __breakpoint();
    }
}
