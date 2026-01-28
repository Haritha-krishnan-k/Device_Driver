#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "daq_driver"
#define DEVICE_NAME "daq"
#define DMA_BUF_SIZE (4 * 1024 * 1024)   /* 4 MB buffer */

static dev_t dev_num;
static struct cdev daq_cdev;
static struct class *daq_class;

static void *dma_virt;
static dma_addr_t dma_phys;


static int daq_open(struct inode *inode, struct file *file)
{
    pr_info("%s: device opened\n", DRIVER_NAME);
    return 0;
}

static int daq_release(struct inode *inode, struct file *file)
{
    pr_info("%s: device closed\n", DRIVER_NAME);
    return 0;
}

/* mmap: map DMA buffer into user space */
static int daq_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > DMA_BUF_SIZE)
        return -EINVAL;

    vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;

    return remap_pfn_range(vma,
                           vma->vm_start,
                           dma_phys >> PAGE_SHIFT,
                           size,
                           vma->vm_page_prot);
}

static struct file_operations daq_fops = {
    .owner   = THIS_MODULE,
    .open    = daq_open,
    .release = daq_release,
    .mmap    = daq_mmap,
};

static int __init daq_init(void)
{
    int ret;

    pr_info("%s: initializing\n", DRIVER_NAME);


    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&daq_cdev, &daq_fops);
    ret = cdev_add(&daq_cdev, dev_num, 1);
    if (ret)
        goto err_unregister;

    daq_class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(daq_class, NULL, dev_num, NULL, DEVICE_NAME "0");


    dma_virt = dma_alloc_coherent(NULL,
                                  DMA_BUF_SIZE,
                                  &dma_phys,
                                  GFP_KERNEL);
    if (!dma_virt) {
        ret = -ENOMEM;
        goto err_device;
    }

    pr_info("%s: DMA buffer allocated\n", DRIVER_NAME);
    pr_info("  virt=%p phys=%pad\n", dma_virt, &dma_phys);

    

    return 0;

err_device:
    device_destroy(daq_class, dev_num);
    class_destroy(daq_class);
    cdev_del(&daq_cdev);

err_unregister:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit daq_exit(void)
{
    dma_free_coherent(NULL,
                      DMA_BUF_SIZE,
                      dma_virt,
                      dma_phys);

    device_destroy(daq_class, dev_num);
    class_destroy(daq_class);
    cdev_del(&daq_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("%s: driver unloaded\n", DRIVER_NAME);
}

module_init(daq_init);
module_exit(daq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("High-Speed Data Acquisition Driver with DMA + mmap");
MODULE_VERSION("1.0");
