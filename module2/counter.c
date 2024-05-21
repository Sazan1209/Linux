//
// Created by mikhail on 21.05.24.
//
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/interrupt.h>

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Key press counter");

// Counter

static atomic_t press_count = ATOMIC_INIT(0);

irqreturn_t irq_handler(int irq, void* dev_id) {
  atomic_inc(&press_count);
  return IRQ_NONE;
}

static struct timer_list timer;

// Timer callback

static void reschedule_timer(void) {
  mod_timer(&timer, jiffies + msecs_to_jiffies(60000));
}

void callback(struct timer_list* _) {
  int number = atomic_read(&press_count);
  pr_info("Characters typed: %d\n", number);
  reschedule_timer();
}

// Module funcs

static int __init counter_init(void) {
  timer_setup(&timer, callback, 0);
  reschedule_timer();
  int err = request_irq(1, // PS/2 irq
                        irq_handler,
                        IRQF_SHARED,
                        "Key counter",
                        (void*) irq_handler);
  if (err) {
    del_timer(&timer);
    pr_err("request_irq failed with errcode %d\n.", err);
    return err;
  }
  return 0;
}

static void __exit counter_exit(void) {
  free_irq(1, (void*) irq_handler);
  del_timer(&timer);
}

module_init(counter_init);
module_exit(counter_exit);