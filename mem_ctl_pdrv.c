// platform driver
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

#define BUF_SIZE 128

// method 1: use id_table to match device name
#if 1
static struct platform_device_id mem_ctl_id_table[] = {
	{ .name = "mem_ctl_pdev" },
	{ }
};

MODULE_DEVICE_TABLE(platform, mem_ctl_id_table);
#endif

struct mem_ctl_dev {
    struct cdev dev;
    char vbuf[BUF_SIZE];
    // char *private_data; // private data
    int private_data;
    struct resource *res; // resource
    struct device *device; // device
};
static dev_t cur_devno; // device number
static struct class *mem_ctl_class;
static struct mem_ctl_dev mem_ctl_dev;

static int mem_ctl_open(struct inode *inode, struct file *filp)
{
    filp->private_data = container_of(inode->i_cdev, struct mem_ctl_dev, dev);
    return 0;
}

static int mem_ctl_release(struct inode *inode, struct file *filp)
{
    return 0;
}

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

static struct file_operations mem_ctl_fops = {
    .owner = THIS_MODULE,
    .open = mem_ctl_open,
    .release = mem_ctl_release,
    .write = mem_ctl_write,
    .read = mem_ctl_read,
};

// invoke when match device name
// to init device
static int mem_ctl_probe(struct platform_device *pdev)
{
    int ret;

    printk(KERN_INFO "mem_ctl_probe means the match.\n");
    // get platform resource
    mem_ctl_dev.res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    // get platform private data
    mem_ctl_dev.private_data = *(int*)dev_get_platdata(&pdev->dev);
    // print resource and private data
    printk("mem_ctl_probe: res->start = %lld, \
                            res->end = %lld, \
                            private_data = %0x\n", \
                            mem_ctl_dev.res->start, \
                            mem_ctl_dev.res->end, \
                            mem_ctl_dev.private_data);

    // get device number
    ret = alloc_chrdev_region(&cur_devno, 0, 1, "mem_ctl");
    if (ret < 0) {
        goto err_alloc_chrdev;
    }
    // cdev init
    cdev_init(&mem_ctl_dev.dev, &mem_ctl_fops);
    // cdev add
    ret = cdev_add(&mem_ctl_dev.dev, cur_devno, 1);
    if (ret < 0) {
        goto err_cdev_add;
    }

    // class create
    // class already exist in mem_ctl_init function
    // mem_ctl_class = class_create(THIS_MODULE, "mem_ctl_class");
    // if (IS_ERR(mem_ctl_class)) {
    //     ret = PTR_ERR(mem_ctl_class);
    //     goto err_class_create;
    // }
    // device create
    mem_ctl_dev.device = device_create(mem_ctl_class, NULL, cur_devno, NULL, "mem_ctl_%d", MINOR(cur_devno));
    if (IS_ERR(mem_ctl_dev.device)) {
        ret = PTR_ERR(mem_ctl_dev.device);
        goto err_device_create;
    }

    // save as drvdata
    platform_set_drvdata(pdev, &mem_ctl_dev);
    return 0;

err_device_create:
    class_destroy(mem_ctl_class);
// err_class_create:
    cdev_del(&mem_ctl_dev.dev);
err_cdev_add:
    unregister_chrdev_region(cur_devno, 1);
err_alloc_chrdev:
    printk(KERN_INFO "mem_ctl_probe err\n");
    return ret;
}

static int mem_ctl_remove(struct platform_device *pdev)
{
    // struct mem_ctl_dev *dev = platform_get_drvdata(pdev);

    printk(KERN_INFO "mem_ctl_remove\n");
    // cdev del
    // cdev_del(&dev->dev);
    cdev_del(&mem_ctl_dev.dev);
    device_destroy(mem_ctl_class, cur_devno);
    // class_destroy(mem_ctl_class); // don't nesessary, but for safety
    unregister_chrdev_region(cur_devno, 1);
    return 0;
}

static struct platform_driver mem_ctl_pdrv = {
    // if match device name, call probe
    .probe = mem_ctl_probe,
    .remove = mem_ctl_remove,
    .driver = {
        .name = "mem_ctl_pdev",
    },
    .id_table = mem_ctl_id_table,
};

// register driver
static int __init mem_ctl_drv_init(void)
{
    printk(KERN_INFO "mem_ctl_drv_init\n");
    mem_ctl_class = class_create(THIS_MODULE, "mem_ctl_class");
    return platform_driver_register(&mem_ctl_pdrv);
}

static void __exit mem_ctl_drv_exit(void)
{
    printk(KERN_INFO "mem_ctl_drv_exit\n");
    platform_driver_unregister(&mem_ctl_pdrv);
    class_destroy(mem_ctl_class);
}

module_init(mem_ctl_drv_init);
module_exit(mem_ctl_drv_exit);

MODULE_LICENSE("GPL");
