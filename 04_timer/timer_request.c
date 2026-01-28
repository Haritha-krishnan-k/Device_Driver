#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#define DRIVER_NAME "timer_driver"
#define TIMER_IRQ 0   

static int irq_counter = 0;


static irqreturn_t timer_isr(int irq, void *dev_id)
{
    irq_counter++;
    printk(KERN_INFO "[%s] Timer interrupt #%d\n", DRIVER_NAME, irq_counter);
    return IRQ_HANDLED;
}

static int __init timer_driver_init(void)
{
    int ret;

    printk(KERN_INFO "[%s] Initializing timer driver\n", DRIVER_NAME);

    ret = request_irq(TIMER_IRQ, timer_isr, IRQF_SHARED, DRIVER_NAME, (void *)timer_driver_init);
    if (ret) {
        printk(KERN_ERR "[%s] Failed to register IRQ %d\n", DRIVER_NAME, TIMER_IRQ);
        return ret;
    }

    printk(KERN_INFO "[%s] IRQ %d registered successfully\n", DRIVER_NAME, TIMER_IRQ);
    return 0;
}

static void __exit timer_driver_exit(void)
{
    free_irq(TIMER_IRQ, (void *)timer_driver_init);
    printk(KERN_INFO "[%s] Timer driver removed. Total interrupts: %d\n", DRIVER_NAME, irq_counter);
}

module_init(timer_driver_init);
module_exit(timer_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Dev");
MODULE_DESCRIPTION("Embedded Linux Timer Interrupt Driver");