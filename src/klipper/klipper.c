
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <hardware/irq.h>

#include "klipper.h"
#include "board/timer_irq.h"

#include "autoconf.h" // CONFIG_*
#include "basecmd.h" // stats_update
#include "io.h" // readb
#include "irq.h" // irq_save
#include "misc.h" // timer_from_us
#include "pgm.h" // READP
#include "command.h" // shutdown
#include "sched.h" // sched_check_periodic
#include "stepper.h" // stepper_event


#include "mdl2416c.h"
#include "usb_serial.h"


#define KLIPPER_TASK_PRIORITY 5

#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0

DECL_CONSTANT("CLOCK_FREQ", CONFIG_CLOCK_FREQ);
DECL_CONSTANT_STR("MCU", "RP2040");


#define TIMER_IDLE_REPEAT_TICKS timer_from_us(500)
#define TIMER_REPEAT_TICKS timer_from_us(100)
#define TIMER_MIN_TRY_TICKS timer_from_us(2)
#define TIMER_DEFER_REPEAT_TICKS timer_from_us(5)

static uint32_t timer_repeat_until;
static TaskHandle_t klipper_task_handle = NULL;
static SemaphoreHandle_t klipper_timersem = NULL;

extern void ctr_run_initfuncs(void);
extern void sched_run_tasks(uint32_t *start);


static inline void timer_set(uint32_t next)
{
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    timer_hw->alarm[ALARM_NUM] = next;
}


static void klipper_task(void *params)
{
    uint8_t buf[1024], txchar;
    uint32_t rxlen, i;

    klipper_timersem = xSemaphoreCreateBinary();
    uint32_t next, start = timer_read_time();

    ctr_run_initfuncs();

    // sendf("starting");

    for (;;)
    {
        if (xSemaphoreTake(klipper_timersem, 0) == pdTRUE)
        {
            next = timer_dispatch_many();
            timer_set(next);
        }

        rxlen = usb_serial_rx(buf, sizeof(buf));
 
        for (i = 0; i < rxlen; i++)
        {
            serial_rx_byte(buf[i]);
        }

        sched_run_tasks(&start);
    }

}

void klipper_init(void)
{
    gpio_init(26);
    gpio_set_dir(26, GPIO_OUT);

    xTaskCreate(klipper_task,
                "klipper",
                1024 * 4 ,
                NULL,
                KLIPPER_TASK_PRIORITY,
                &klipper_task_handle);
}


/****************************************************************
 * Timer
 ****************************************************************/

uint32_t timer_read_time(void)
{
    return timer_hw->timerawl;
}


void timer_kick(void)
{
    timer_set(timer_read_time() + 50);
}


static void alarm_irq(void)
{
    BaseType_t hi_prio_task_woken = pdFALSE;

    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    xSemaphoreGiveFromISR(klipper_timersem, &hi_prio_task_woken);

    portYIELD_FROM_ISR(hi_prio_task_woken);
}


void timer_init(void)
{    
    irq_disable();

    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
    irq_set_enabled(ALARM_IRQ, true);

    timer_hw->timelw = 0;
    timer_hw->timehw = 0;

    timer_kick();
    irq_enable();
}
DECL_INIT(timer_init);


uint32_t timer_from_us(uint32_t us)
{
    return us * (CONFIG_CLOCK_FREQ / 1000000);
}


uint8_t timer_is_before(uint32_t time1, uint32_t time2)
{
    return time2 > time1;
}


uint32_t timer_dispatch_many(void)
{
    uint32_t tru = timer_repeat_until;
    
    for (;;)
    {
        // Run the next software timer
        uint32_t next = sched_timer_dispatch();

        uint32_t now = timer_read_time();
        int32_t diff = next - now;
        if (diff > (int32_t)TIMER_MIN_TRY_TICKS)
        {
            // Schedule next timer normally.
            return next;            
        }

        if (unlikely(timer_is_before(tru, now)))
        {
            // Check if there are too many repeat timers
            if (diff < (int32_t)(-timer_from_us(1000)))
            {
                try_shutdown("Rescheduled timer in the past");
            }
            if (sched_tasks_busy())
            {
                timer_repeat_until = now + TIMER_REPEAT_TICKS;
                return now + TIMER_DEFER_REPEAT_TICKS;
            }
            timer_repeat_until = tru = now + TIMER_IDLE_REPEAT_TICKS;
        }

        // Next timer in the past or near future - wait for it to be ready
        irq_enable();
        while (unlikely(diff > 0))
        {
            diff = next - timer_read_time();
        }
        irq_disable();
    }
}


void timer_task(void)
{
    uint32_t now = timer_read_time();
    irq_disable();
    if (timer_is_before(timer_repeat_until, now))
    {
        timer_repeat_until = now;
    }
    irq_enable();
}
DECL_TASK(timer_task);

/****************************************************************
 * Interrupts
 ****************************************************************/

void irq_disable(void)
{
    taskENTER_CRITICAL();    
}


void irq_enable(void)
{
    taskEXIT_CRITICAL();
}


irqstatus_t irq_save(void)
{
    return 0;
}


void irq_restore(irqstatus_t flag)
{

}


void irq_wait(void)
{
    vTaskDelay(1);
}


void irq_poll(void)
{

}


void serial_enable_tx_irq(void)
{
    uint8_t data;

    while (serial_get_tx_byte(&data) == 0)
    {
        usb_serial_tx(&data, 1);
    }
}


void command_display_print(uint32_t *args)
{
    uint8_t text_len = args[0];
    uint8_t *text = command_decode_ptr(args[1]);

    irq_disable();
    mdl2416c_print_buf(text, text_len);
    irq_enable();
}
DECL_COMMAND(command_display_print, "display_print text=%*s");
