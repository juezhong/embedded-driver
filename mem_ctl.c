#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>

static dev_t dev_no; // device number(major minor)
static struct cdev mem_ctl_dev; // cdev struct
static struct file_operations mem_ctl_fops; // operations

#define DEV_NAME "mem_ctl"
#define DEV_CNT 1

static int __init mem_ctl_init(void)
{
    // 1. dynamic alloc device number
    int ret;
    printk("mem_ctl_init\n");
    ret = alloc_chrdev_region(&dev_no, 0, DEV_CNT, DEV_NAME);
    if (ret)
        goto alloc_err;

    // 2. association struct cdev&file_operations
    cdev_init(&mem_ctl_dev, &mem_ctl_fops);

    // 3. add cdev
    ret = cdev_add(&mem_ctl_dev, dev_no, DEV_CNT);
    if (ret)
        goto add_err;
    return 0;

add_err:
    printk("fail to add cdev.\n");
    unregister_chrdev_region(dev_no, DEV_CNT);
alloc_err:
    printk("fail to alloc devno.\n");
    return ret;
}

static void __exit mem_ctl_exit(void)
{
    printk("mem_ctl_exit.\n");
    cdev_del(&mem_ctl_dev);
    unregister_chrdev_region(dev_no, DEV_CNT);
}

module_init(mem_ctl_init);
module_exit(mem_ctl_exit);

MODULE_LICENSE("GPL");