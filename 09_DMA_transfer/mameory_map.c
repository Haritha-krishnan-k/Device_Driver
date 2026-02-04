#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
//how many bytes sent at at time
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
    //vma - virtual memory address
    //this struct contain vm_start , vm_end , vm_flags,vm_page_prot
    //vm_start - start address
    //vm_end - end address
    //vm_flags - permission and behaviour
    //vm_page_proto - cache / page protection
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > DMA_BUF_SIZE)
        return -EINVAL;

    vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;
 //mmap - map the physical DMA page to user
 //physical memory to userspace

 //vma- memory region being mapped
 //vm->vm_start - starting address
 //dma_phys >> PAGE_SHIFT - converts DMA add into page number
 //size - how many bytes to map
 //vma->vm_page_prot - read/write permission

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
    // register a character driver
    //alloc_chrdev_region()
    //cdev_init()
    //cdev_add()
    //class_create()
    //device_create()
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&daq_cdev, &daq_fops);
    ret = cdev_add(&daq_cdev, dev_num, 1);
    if (ret)
        goto err_unregister;

    daq_class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(daq_class, NULL, dev_num, NULL, DEVICE_NAME "0");
    //alloc DMA buffer
    //kernel-virtual add
    //physical DMA add

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


// dev_num - dev_t - 
// - major number and minor number 
// - 32 or 64 bit 
// - which driver and which device instnace
// daq_cdev - cdev 
// - character device object  
// struct cdev {
// struct kobject kobj; // sysfs integration
// constr struct file_operations *ops; // open/read/write/mmap etc
// dev_t dev; // major and minor number
// unsigned int count; // how many minors
// };
// cdev_fops - file_operations
// dma_phys - dma_addr_t 
// - DMA physical address 
// - bus address the device used to read/write RAM
// it just holds the hardware-visible DMA address-32 bit or 64 bit

// DEVICE_NAME "daq_driver" - already done #define
// DMA_BUF_SIZE - "1024*1024*4"- already done #define
// what does the daq_class have 
// what does dma_virt have

// struct vm_area_struct {
// unsigned long vm_start; //virtual start add
// unsigned long vm_end; // virtual end add
// unsigned long vm_flags; // permission & behaviour
// pgprot_t vm_page_prot; // cache/page protection
// struct mm_struct *vm_mm; // which process owns it 

