#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/reboot.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("User");
MODULE_DESCRIPTION("A simple reboot module");

static int reboot_delay = 10;
module_param(reboot_delay, int, 0);
MODULE_PARM_DESC(reboot_delay, "Delay in seconds before reboot (must be > 0)");

static struct timer_list my_timer;
static int countdown;

static void my_timer_function(struct timer_list *t)
{
    printk(KERN_INFO "Reboot module: Timer expired, attempting to reboot...\n");
    
    emergency_restart();
    
    printk(KERN_INFO "emergency_restart() didn't complete, trying kernel_restart()...\n");
    kernel_restart(NULL);
}

static int __init reboot_module_init(void)
{
    if (reboot_delay <= 0 || reboot_delay > 600) {
        printk(KERN_ERR "Invalid reboot_delay value. Must be greater than 0 and less than or equal to 600.\n");
        return -EINVAL;
    }

    printk(KERN_INFO "Reboot module loaded. Rebooting in %d seconds...\n", reboot_delay);
    countdown = reboot_delay;

    timer_setup(&my_timer, my_timer_function, 0);
    my_timer.expires = jiffies + (countdown * HZ);
    add_timer(&my_timer);

    return 0;
}

static void __exit reboot_module_exit(void)
{
    if (timer_pending(&my_timer)) {
        del_timer_sync(&my_timer);
    }
    printk(KERN_INFO "Reboot module unloaded.\n");
}

module_init(reboot_module_init);
module_exit(reboot_module_exit);