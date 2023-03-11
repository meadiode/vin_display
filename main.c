
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <lwip/ip4_addr.h>
#include <hardware/timer.h>
#include <hardware/adc.h>
#include <FreeRTOS.h>
#include <task.h>

#include "http_server.h"
#include "dhcp_server.h"
#include "fpanel.h"

#define SW_MAJOR_VERSION 0
#define SW_MINOR_VERSION 437

#define MAIN_TASK_PRIORITY  1

static TaskHandle_t main_task_handle = NULL;
static dhcp_server_t dhcp_server;


static void printout_sys_stats(void)
{
    static TaskStatus_t tstats[20] = {0};
    uint32_t total_rt;
    uint8_t num, i;
    float prc;

    num = uxTaskGetSystemState(tstats, sizeof(tstats), &total_rt);

    printf("\n\n\n");
    printf("Task name       | Proc time %% | Stack HWM\n");
    printf("-----------------------------------------\n");

    for (i = 0; i < num; i++)
    {
        prc = (float)tstats[i].ulRunTimeCounter / (float)total_rt * 100.0f;

        printf("%-16s|%12.2f%%|%10u\n",
               tstats[i].pcTaskName, prc, tstats[i].usStackHighWaterMark);
    }
    printf("\n");
    
    prc = 1.0f - (float)xPortGetFreeHeapSize() / (float)configTOTAL_HEAP_SIZE;
    prc *= 100.0f;
    printf("Mem usage: %0.2f%%; %u free out of %u\n",
           prc, xPortGetFreeHeapSize(), configTOTAL_HEAP_SIZE);
}


static void wifi_start_ap(void)
{
    ip4_addr_t gw, mask;
    IP4_ADDR(&gw, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);

    cyw43_arch_enable_ap_mode("pico_w", "", CYW43_AUTH_OPEN);
    dhcp_server_init(&dhcp_server, &gw, &mask);
}


static void wifi_start_sta(void)
{
    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi... %s:%s\n", WIFI_SSID, WIFI_PASSWORD);

    for (;;)
    {
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                               CYW43_AUTH_WPA2_AES_PSK, 30000))
        {
            printf("failed to connect.\n");
            vTaskDelay(2000);
        } else {
            printf("Connected\n");
            break;
        }

    }
}


void main_task(__unused void *params) {

    printf("\n");    
    printf("SW version: %u.%u\n", SW_MAJOR_VERSION, SW_MINOR_VERSION);

    if (cyw43_arch_init())
    {
        printf("Failed to initialise\n");
        return;
    }

    // wifi_start_ap();

    wifi_start_sta();

    fpanel_init();

    printf("Starting server at %s on port %u\n",
           ip4addr_ntoa(netif_ip4_addr(netif_list)), 80);
    http_server_init();

    for (;;)
    {
        vTaskDelay(5000);
        printout_sys_stats();
    }

    cyw43_arch_deinit();
}


int main( void )
{
    stdio_init_all();

    adc_init();

    printf("Starting FreeRTOS SMP on both cores\n");
    
    xTaskCreate(main_task,
                "main",
                1024 * 1,
                NULL,
                MAIN_TASK_PRIORITY,
                &main_task_handle);

    vTaskCoreAffinitySet(main_task_handle, 1);

    vTaskStartScheduler();

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
    for (;;)
    {
        __breakpoint();
    }
}


void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
    for (;;)
    {
        __breakpoint();
    }
}


uint32_t stats_get_time(void)
{
    return time_us_32();
}
