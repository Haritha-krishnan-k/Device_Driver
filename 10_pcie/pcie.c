#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "pcie_dma"
#define DEVICE_NAME "pcie_dma"
#define DMA_BUF_SIZE (4 * 1024 * 1024) /* 4MB */

/* Fake PCI IDs */
#define PCI_VENDOR_ID_FAKE 0x1234
#define PCI_DEVICE_ID_FAKE 0x5678

/* BAR0 register offsets */
#define REG_DMA_ADDR   0x00
#define REG_DMA_LEN    0x08
#define REG_DMA_START  0x10
#define REG_IRQ_ACK    0x18

struct pcie_dma_dev {
    struct pci_dev *pdev;
    void __iomem *mmio;
    dma_addr_t dma_phys;
    void *dma_virt;
    int irq;
    struct cdev cdev;
    dev_t devt;
};

static struct class *dma_class;
static struct pcie_dma_dev *dma_dev;

/* -------------------------------------------------- */
/* Interrupt Handler                                  */
/* -------------------------------------------------- */
static irqreturn_t dma_irq_handler(int irq, void *dev_id)
{
    struct pcie_dma_dev *dev = dev_id;

    /* Acknowledge device interrupt */
    iowrite32(1, dev->mmio + REG_IRQ_ACK);

    pr_info("[%s] DMA transfer completed\n", DRIVER_NAME);
    return IRQ_HANDLED;
}

/* -------------------------------------------------- */
/* mmap(): map DMA buffer to user space               */
/* -------------------------------------------------- */
static int dma_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > DMA_BUF_SIZE)
        return -EINVAL;

    vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;

    return remap_pfn_range(vma,
                           vma->vm_start,
                           dma_dev->dma_phys >> PAGE_SHIFT,
                           size,
                           vma->vm_page_prot);
}

static const struct file_operations dma_fops = {
    .owner = THIS_MODULE,
    .mmap  = dma_mmap,
};

/* -------------------------------------------------- */
/* PCI Probe                                          */
/* -------------------------------------------------- */
static int dma_probe(struct pci_dev *pdev,
                     const struct pci_device_id *id)
{
    int ret;

    dma_dev = kzalloc(sizeof(*dma_dev), GFP_KERNEL);
    if (!dma_dev)
        return -ENOMEM;

    dma_dev->pdev = pdev;
    pci_set_drvdata(pdev, dma_dev);

    ret = pci_enable_device(pdev);
    if (ret)
        goto err_free;

    ret = pci_request_regions(pdev, DRIVER_NAME);
    if (ret)
        goto err_disable;

    dma_dev->mmio = pci_iomap(pdev, 0, 0);
    if (!dma_dev->mmio)
        goto err_regions;

    /* Allocate DMA buffer */
    dma_dev->dma_virt = dma_alloc_coherent(&pdev->dev,
                                           DMA_BUF_SIZE,
                                           &dma_dev->dma_phys,
                                           GFP_KERNEL);
    if (!dma_dev->dma_virt)
        goto err_iounmap;

    /* Request interrupt */
    dma_dev->irq = pdev->irq;
    ret = request_irq(dma_dev->irq,
                      dma_irq_handler,
                      IRQF_SHARED,
                      DRIVER_NAME,
                      dma_dev);
    if (ret)
        goto err_dma;

    /* Character device */
    alloc_chrdev_region(&dma_dev->devt, 0, 1, DEVICE_NAME);
    cdev_init(&dma_dev->cdev, &dma_fops);
    cdev_add(&dma_dev->cdev, dma_dev->devt, 1);

    dma_class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(dma_class, NULL, dma_dev->devt, NULL, DEVICE_NAME);

    pr_info("[%s] PCIe DMA device initialized\n", DRIVER_NAME);
    return 0;

err_dma:
    dma_free_coherent(&pdev->dev, DMA_BUF_SIZE,
                      dma_dev->dma_virt, dma_dev->dma_phys);
err_iounmap:
    pci_iounmap(pdev, dma_dev->mmio);
err_regions:
    pci_release_regions(pdev);
err_disable:
    pci_disable_device(pdev);
err_free:
    kfree(dma_dev);
    return ret;
}

/* -------------------------------------------------- */
/* PCI Remove                                         */
/* -------------------------------------------------- */
static void dma_remove(struct pci_dev *pdev)
{
    device_destroy(dma_class, dma_dev->devt);
    class_destroy(dma_class);

    cdev_del(&dma_dev->cdev);
    unregister_chrdev_region(dma_dev->devt, 1);

    free_irq(dma_dev->irq, dma_dev);
    dma_free_coherent(&pdev->dev, DMA_BUF_SIZE,
                      dma_dev->dma_virt, dma_dev->dma_phys);

    pci_iounmap(pdev, dma_dev->mmio);
    pci_release_regions(pdev);
    pci_disable_device(pdev);

    kfree(dma_dev);
    pr_info("[%s] Device removed\n", DRIVER_NAME);
}

/* -------------------------------------------------- */
/* PCI ID Table                                       */
/* -------------------------------------------------- */
static const struct pci_device_id dma_ids[] = {
    { PCI_DEVICE(PCI_VENDOR_ID_FAKE, PCI_DEVICE_ID_FAKE) },
    { }
};
MODULE_DEVICE_TABLE(pci, dma_ids);

static struct pci_driver dma_pci_driver = {
    .name     = DRIVER_NAME,
    .id_table = dma_ids,
    .probe    = dma_probe,
    .remove   = dma_remove,
};

module_pci_driver(dma_pci_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Dev");
MODULE_DESCRIPTION("PCIe DMA Driver with mmap + interrupts");
