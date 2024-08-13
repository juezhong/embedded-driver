#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>

static dev_t dev_no; // device number(major minor)
static struct cdev mem_ctl_dev; // cdev struct

#define DEV_NAME "mem_ctl"
#define DEV_CNT 1
#define BUF_SIZE 128

static char vbuf[BUF_SIZE];

/*
because virtual device not associated with hardware,
so open and release function return 0.
*/
static int mem_ctl_open(struct inode *inode, struct file *filp)
{
    printk("mem_ctl open.\n");
    return 0;
}

static int mem_ctl_release(struct inode *inode, struct file *filp)
{
    printk("mem_ctl release.\n");
    return 0;
}

// focus read and write func.
ssize_t mem_ctl_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    // record current file read&write position
    unsigned long p = *ppos;
    int ret;
    int tmp = count;
    // over buf size
    if (p > BUF_SIZE)
        return 0;
    // write data count over buf, then write residue size of buf.
    if (tmp > BUF_SIZE - p)
        tmp = BUF_SIZE - p;

    // copy from user buf to kernel vbuf, then move the offset.
    ret = copy_from_user(vbuf, buf, tmp);
    *ppos += tmp;
    return tmp;
}

ssize_t mem_ctl_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    // same as write
    unsigned long p = *ppos;
    int ret;
    int tmp = count;

    if (p > BUF_SIZE)
        return 0;
    if (tmp > BUF_SIZE - p)
        tmp = BUF_SIZE - p;
    
    ret = copy_to_user(buf, vbuf + p, tmp);
    *ppos += tmp;
    return tmp;
}

// loff_t (*llseek) (struct file *, loff_t, int);

// operations
static struct file_operations mem_ctl_fops = {
    .owner      = THIS_MODULE,
    .open       = mem_ctl_open,
    .release    = mem_ctl_release,
    .write      = mem_ctl_write,
    .read       = mem_ctl_read,
};

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