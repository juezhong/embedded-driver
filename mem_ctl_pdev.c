// platform device
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>

// platform device resources
static struct resource mem_ctl_pdev_res[] = {
    [0] = {
        .start = 0x1,
        .end = 0x1,
        .flags = IORESOURCE_IRQ,
    },
};

// platform device private data
// static char mem_ctl_pdev_private_data[] = "virtual charcter device";
static int mem_ctl_pdev_private_data = 0xFFFF;

static void	mem_ctl_pdev_release(struct device *dev)
{
    printk(KERN_INFO "mem_ctl_pdev_release %p.\n", dev);
}

static struct platform_device mem_ctl_pdev = {
    .name = "mem_ctl_pdev",
    .id = 0,
    .num_resources = ARRAY_SIZE(mem_ctl_pdev_res),
    .resource = mem_ctl_pdev_res,
    .dev = {
        .release = mem_ctl_pdev_release,
        .platform_data = (void*)&mem_ctl_pdev_private_data,
    }
};

static int __init mem_ctl_pdev_init(void)
{
    int ret;

    printk(KERN_INFO "mem_ctl_pdev_init.\n");

    ret = platform_device_register(&mem_ctl_pdev);
    if (ret) {
        printk(KERN_ERR "mem_ctl_pdev_init: platform_device_register failed.\n");
        return ret;
    }

    return 0;
}

static void __exit mem_ctl_pdev_exit(void)
{
    printk(KERN_INFO "mem_ctl_pdev_exit.\n");

    platform_device_unregister(&mem_ctl_pdev);
}

module_init(mem_ctl_pdev_init);
module_exit(mem_ctl_pdev_exit);

MODULE_LICENSE("GPL");
