#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Fifo-module");

// Queue
#define BUF_SIZE 1024
static char device_buffer[BUF_SIZE];
static int read_pos = 0;
static int write_pos = 0;
static int bytes_in_fifo = 0;
static DEFINE_MUTEX(fifo_mutex);
static DECLARE_WAIT_QUEUE_HEAD(fifo_read_q);
static DECLARE_WAIT_QUEUE_HEAD(fifo_write_q);

static ssize_t fifo_read(__always_unused struct file* file,
                         char __user* buf,
                         size_t len,
                         __always_unused loff_t* off) {
  if (len == 0) {
    return 0;
  }
  mutex_lock(&fifo_mutex);
  while (bytes_in_fifo == 0) {
    mutex_unlock(&fifo_mutex);
    if (wait_event_interruptible(fifo_read_q, bytes_in_fifo > 0)) {
      return -EINTR;
    }
    mutex_lock(&fifo_mutex);
  }
  if (copy_to_user(buf, &buf[read_pos], 1)) {
    mutex_unlock(&fifo_mutex);
    return -EFAULT;
  }
  read_pos = (read_pos + 1) % BUF_SIZE;
  bytes_in_fifo--;
  wake_up(&fifo_write_q);
  mutex_unlock(&fifo_mutex);
  return 1;
}

static ssize_t fifo_write(__always_unused struct file* file,
                          const char* buf,
                          size_t len,
                          __always_unused loff_t* off) {
  if (len == 0) {
    return 0;
  }
  if (mutex_lock_interruptible(&fifo_mutex)) {
    return -EINTR;
  }
  while (bytes_in_fifo == BUF_SIZE) {
    mutex_unlock(&fifo_mutex);
    if (wait_event_interruptible(fifo_write_q, bytes_in_fifo < BUF_SIZE)) {
      return -EINTR;
    }
    if (mutex_lock_interruptible(&fifo_mutex)) {
      return -EINTR;
    }
  }

  if (copy_from_user(&device_buffer[write_pos], buf, 1)) {
    mutex_unlock(&fifo_mutex);
    return -EFAULT;
  }
  write_pos = (write_pos + 1) % BUF_SIZE;
  bytes_in_fifo += 1;
  wake_up(&fifo_read_q);
  mutex_unlock(&fifo_mutex);
  return 1;
}

// Dummy func
static int fifo_open_release(__always_unused struct inode* inode,
                             __always_unused struct file* file) {
  return 0;
}

static struct file_operations fifo_fops =
    {
        .owner      = THIS_MODULE,
        .read       = fifo_read,
        .write      = fifo_write,
        .open       = fifo_open_release,
        .release    = fifo_open_release,
    };

static dev_t dev_num;
static struct cdev fifo_cdev;
static struct class* fifo_class;

static int __init fifo_init(void) {
  int err;
  if ((err = alloc_chrdev_region(&dev_num, 0, 1, "fifo"))) {
    pr_err("Cannot allocate major number\n");
    return err;
  }
  pr_info("Major = %d Minor = %d \n", MAJOR(dev_num), MINOR(dev_num));
  cdev_init(&fifo_cdev, &fifo_fops);
  if ((err = cdev_add(&fifo_cdev, dev_num, 1))) {
    pr_err("Cannot cdev add fifo");
    goto r_class;
  }

  fifo_class = class_create("fifo");
  if (IS_ERR(fifo_class)) {
    err = (int) PTR_ERR(fifo_class);
    pr_err("Cannot create fifo class\n");
    goto r_class;
  }
  struct device
      * device = device_create(fifo_class, NULL, dev_num, NULL, "fifo_proc");
  if (IS_ERR(device)) {
    err = (int) PTR_ERR(device);
    pr_err("Cannot create fifo device\n");
    goto r_device;
  }
  return 0;

  r_device:
  class_destroy(fifo_class);

  r_class:
  unregister_chrdev_region(dev_num, 1);

  return err;
}

static void __exit fifo_exit(void) {
  device_destroy(fifo_class, dev_num);
  class_destroy(fifo_class);
  unregister_chrdev_region(dev_num, 1);
}

module_init(fifo_init)
module_exit(fifo_exit)