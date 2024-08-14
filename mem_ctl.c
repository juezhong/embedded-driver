#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEV_NAME "mem_ctl"
#define DEV_CNT 2
#define BUF_SIZE 128

static dev_t dev_no; // device number(major minor)
static struct class *class;
// user define data struct of mem_ctl, 
// container cdev structure, data area.
struct mem_ctl_dev {
    struct cdev dev;
    struct device *device;
    char vbuf[BUF_SIZE];
};
// device 1
static struct mem_ctl_dev mem_ctl_dev_1;
// device 2
static struct mem_ctl_dev mem_ctl_dev_2;

/*
because virtual device not associated with hardware,
so open and release function return 0.
*/
static int mem_ctl_open(struct inode *inode, struct file *filp)
{
    // printk("mem_ctl open.\n");
    // via container_of get mem_ctl_dev address
    // inode's member i_cdev saved cdev structure address.
    filp->private_data = container_of(inode->i_cdev, struct mem_ctl_dev, dev);
    return 0;
}

static int mem_ctl_release(struct inode *inode, struct file *filp)
{
    // printk("mem_ctl release.\n");
    return 0;
}

// focus read and write func.
ssize_t mem_ctl_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    // record current file read&write position
    unsigned long p = *ppos;
    int ret;
    int tmp = count;
    // get file's private data
    struct mem_ctl_dev *dev = filp->private_data;
    char *vbuf = dev->vbuf;
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
    // get file's private data
    struct mem_ctl_dev *dev = filp->private_data;
    char *vbuf = dev->vbuf;
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
    // allocate 2 device minor
    ret = alloc_chrdev_region(&dev_no, 0, DEV_CNT, DEV_NAME);
    if (ret)
        goto alloc_err;

    // device 1
    // 2. association struct cdev&file_operations
    cdev_init(&mem_ctl_dev_1.dev, &mem_ctl_fops);
    // 3. add cdev
    ret = cdev_add(&mem_ctl_dev_1.dev, dev_no + 0, 1);
    if (ret)
        goto add_err_1;

    // device 2
    // 2. association struct cdev&file_operations
    cdev_init(&mem_ctl_dev_2.dev, &mem_ctl_fops);
    // 3. add cdev
    ret = cdev_add(&mem_ctl_dev_2.dev, dev_no + 1, 1);
    if (ret)
        goto add_err_2;

    // 4. create class
    class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(class)) {
        ret = PTR_ERR(class);
        goto class_err;
    }
    // 5. create device
    mem_ctl_dev_1.device = device_create(class, NULL, dev_no + 0, NULL, "%s_%d", DEV_NAME, 0);
    mem_ctl_dev_2.device = device_create(class, NULL, dev_no + 1, NULL, "%s_%d", DEV_NAME, 1);
    if (IS_ERR(mem_ctl_dev_1.device) || IS_ERR(mem_ctl_dev_2.device)) {
        ret = PTR_ERR(mem_ctl_dev_1.device);
        goto dev_err;
    }
    return 0;

dev_err:
    device_destroy(class, dev_no + 1);
    device_destroy(class, dev_no + 0);
class_err:
    class_destroy(class);
    printk("fail to create class.\n");
    cdev_del(&mem_ctl_dev_2.dev);
add_err_2:
    printk("fail to add mem_ctl_dev_2.\n");
    cdev_del(&mem_ctl_dev_1.dev);
add_err_1:
    printk("fail to add mem_ctl_dev_1.\n");
    unregister_chrdev_region(dev_no, DEV_CNT);
alloc_err:
    printk("fail to alloc devno.\n");
    return ret;
}

static void __exit mem_ctl_exit(void)
{
    printk("mem_ctl_exit.\n");
    cdev_del(&mem_ctl_dev_1.dev);
    cdev_del(&mem_ctl_dev_2.dev);
    unregister_chrdev_region(dev_no, DEV_CNT);
    device_destroy(class, dev_no + 1);
    device_destroy(class, dev_no + 0);
    class_destroy(class);
}

module_init(mem_ctl_init);
module_exit(mem_ctl_exit);

MODULE_LICENSE("GPL");