
#include "irq.h"


/****************************************************************
 * Interrupts
 ****************************************************************/

// Disable hardware interrupts
void
irq_disable(void)
{
}

// Enable hardware interrupts
void
irq_enable(void)
{
}

// Disable hardware interrupts in not already disabled
irqstatus_t
irq_save(void)
{
    return 0;
}

// Restore hardware interrupts to state from flag returned by irq_save()
void
irq_restore(irqstatus_t flag)
{
}

// Atomically enable hardware interrupts and sleep processor until next irq
void
irq_wait(void)
{
    // // XXX - sleep to prevent excessive cpu usage in simulator
    // usleep(1);

    // irq_poll();
}

// Check if an interrupt is active (used only on architectures that do
// not have hardware interrupts)
void
irq_poll(void)
{
    // uint32_t now = timer_read_time();
    // if (!timer_is_before(now, next_wake_time))
    //     do_timer_dispatch();
}
