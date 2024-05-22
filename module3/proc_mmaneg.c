//
// Created by mikhail on 21.05.24.
//

// Module registration
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

// Creation of character device
#include <linux/cdev.h>
#include <linux/fs.h>

// Procfs
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/mm_types.h>
#include <asm/page.h>
#include <asm/io.h>

//------------------------------------------------------------------------------


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Proc mmaneg module");

// Variables

#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "mmaneg"

static struct proc_dir_entry* proc_file;
static char mmaneg_buf[PROCFS_MAX_SIZE + 1];

static void listvma(void) {
  struct task_struct* task = current;
  struct mm_struct* mm = get_task_mm(task);
  struct vm_area_struct* vma;

  mmap_read_lock(mm);
  VMA_ITERATOR(iter, mm, 0);
  pr_info("VMA for process %d:\n", task->pid);
  for_each_vma(iter, vma) {
    pr_info("Start: 0x%lx, End: 0x%lx\n", vma->vm_start, vma->vm_end);
  }
  mmap_read_unlock(mm);
}

static void findpage(unsigned long addr) {
  if (!virt_addr_valid(addr)) {
    pr_warn("Virtual addr 0x%lx is invalid", addr);
    return;
  }
  struct page* page = virt_to_page(addr);
  pr_info("VA: 0x%lx, PA: 0x%llx\n", addr, page_to_phys(page));
  put_page(page);
}

static void writeval(unsigned long addr, unsigned long val) {
  if (put_user(val, (unsigned long*) addr)) {
    pr_warn("Couldn't write to addr 0x%lx)\n", addr);
  } else {
    pr_info("Wrote %lu to addr 0x%lx", val, addr);
  }
}

static ssize_t mmaneg_write(__always_unused struct file* file,
                            const char __user* buf,
                            size_t len,
                            __always_unused loff_t* pos) {
  if (len > PROCFS_MAX_SIZE) {
    len = PROCFS_MAX_SIZE;
  }
  if (copy_from_user(mmaneg_buf, buf, len)) {
    return -EFAULT;
  }
  mmaneg_buf[len] = '\0';
  unsigned long addr = 0;
  if (strncmp(mmaneg_buf, "listvma", 7) == 0) {
    listvma();
  } else if (strncmp(mmaneg_buf, "findpage", 8) == 0) {
    if (sscanf(mmaneg_buf + 9, "%lx", &addr) != 1) {
      goto unrecognized_arguments;
    }
    findpage(addr);
  } else if (strncmp(mmaneg_buf, "writeval", 8) == 0) {
    unsigned long val = 0;
    if (sscanf(mmaneg_buf + 9, "%lx %lu", &addr, &val) != 2) {
      goto unrecognized_arguments;
    }
    writeval(addr, val);
  } else {
    pr_warn("Invalid command: %s\n", mmaneg_buf);
    return -EINVAL;
  }
  return (ssize_t) len;

  unrecognized_arguments:
  pr_warn("Invalid arguments: %s\n", mmaneg_buf);
  return -EINVAL;
}

// Dummy funcs
static int mmaneg_open(__always_unused struct inode* inode,
                       __always_unused struct file* file) {
  return 0;
}

static int mmaneg_release(__always_unused struct inode* inode,
                          __always_unused struct file* file) {
  return 0;
}

static loff_t mmaneg_lseek(__always_unused struct file* file,
                           __always_unused loff_t loff,
                           __always_unused int whence) {
  return 0;
}

static ssize_t mmaneg_read(__always_unused struct file* file,
                           __always_unused char __user* buf,
                           __always_unused size_t len,
                           __always_unused loff_t* pos) {
  return -EACCES;
}

static struct proc_ops mmaneg_fops =
    {
        .proc_open = mmaneg_open,
        .proc_read = mmaneg_read,
        .proc_write = mmaneg_write,
        .proc_lseek = mmaneg_lseek,
        .proc_release = mmaneg_release,
    };

static int __init mmaneg_init(void) {
  proc_file = proc_create(PROCFS_NAME, 222, NULL, &mmaneg_fops);
  if (!proc_file) {
    pr_err("Failed to create proc file (for some reason)");
    return -ENOMEM;
  }
  return 0;
}

static void __exit mmaneg_exit(void) {
  remove_proc_entry(PROCFS_NAME, NULL);
}

module_init(mmaneg_init)
module_exit(mmaneg_exit)