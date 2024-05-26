
#include <pico/stdlib.h>
#include <hardware/timer.h>
#include <hardware/adc.h>
#include <hardware/rtc.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

#include "display.h"
#include "buzzer.h"
#include "knob.h"
#include "buttons.h"
#include "usb_serial.h"
#include "if_uart.h"
#include "command.h"

#define SW_MAJOR_VERSION 0
#define SW_MINOR_VERSION 501

#define MAIN_TASK_PRIORITY  1

static TaskHandle_t main_task_handle = NULL;


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


void main_task(__unused void *params) {

    printf("\n");    
    printf("SW version: %u.%u\n", SW_MAJOR_VERSION, SW_MINOR_VERSION);

    for (;;)
    {
        vTaskDelay(50000);
        printout_sys_stats();
    }
}


int main( void )
{
    stdio_init_all();

    adc_init();

    rtc_init();

    display_driver_init();

    buzzer_init();

    knob_init();

    buttons_init();

    command_init();

    display_init();

    usb_serial_init();

    if_uart_init();

    printf("Starting FreeRTOS SMP on both cores\n");
    
    xTaskCreate(main_task,
                "main",
                1024 * 4,
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
