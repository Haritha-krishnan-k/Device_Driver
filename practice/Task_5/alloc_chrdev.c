#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>

#define DRIVER_NAME "pcie_dma"
#define CLASS_NAME  "pcie"

#define PCI_VENDOR_EXAMPLE  0x1234
#define PCI_DEVICE_EXAMPLE  0x5678

#define DMA_BUF_SIZE (4 * 1024 * 1024)

struct pci_dma_dev {
    struct pci_dev *pdev;

    void __iomem *bar0;

    void *dma_virt;
    dma_addr_t dma_handle;
    size_t dma_size;

    int irq;

    dev_t devt;
    struct cdev cdev;
    struct device *device;
};

static struct pci_device_id pci_ids[] = {
    { PCI_DEVICE(PCI_VENDOR_EXAMPLE, PCI_DEVICE_EXAMPLE) },
    { 0 }
};
MODULE_DEVICE_TABLE(pci, pci_ids);

static irqreturn_t pci_dma_irq(int irq, void *dev_id)
{
    struct pci_dma_dev *dev = dev_id;

    pr_info(DRIVER_NAME ": interrupt received\n");

    /* Example: acknowledge device interrupt */
    /* iowrite32(ACK, dev->bar0 + STATUS_REG); */

    return IRQ_HANDLED;
}

static int pcie_open(struct inode *inode, struct file *file)
{
    struct pci_dma_dev *dev =
        container_of(inode->i_cdev, struct pci_dma_dev, cdev);
    file->private_data = dev;
    return 0;
}

static int pcie_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations pcie_fops = {
    .owner = THIS_MODULE,
    .open = pcie_open,
    .release = pcie_release,
};


static int pci_dma_probe(struct pci_dev *pdev,
                         const struct pci_device_id *id)
{
    int ret;
    struct pci_dma_dev *dev;

    pr_info(DRIVER_NAME ": device detected\n");

    /* Enable device */
    ret = pci_enable_device(pdev);
    if (ret)
        return ret;

    pci_set_master(pdev);

    /* Request BARs */
    ret = pci_request_regions(pdev, DRIVER_NAME);
    if (ret)
        goto err_disable;

    /* Allocate device struct */
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev) {
        ret = -ENOMEM;
        goto err_regions;
    }

    dev->pdev = pdev;

    /* MMIO mapping (LEVEL 2) */
    dev->bar0 = pci_iomap(pdev, 0, 0);
    if (!dev->bar0) {
        ret = -ENOMEM;
        goto err_free;
    }

    /* DMA allocation (LEVEL 3) */
    dev->dma_size = DMA_BUF_SIZE;
    dev->dma_virt = dma_alloc_coherent(&pdev->dev,
                                       dev->dma_size,
                                       &dev->dma_handle,
                                       GFP_KERNEL);
    if (!dev->dma_virt) {
        ret = -ENOMEM;
        goto err_iounmap;
    }

    /* IRQ setup (LEVEL 4) */
    dev->irq = pdev->irq;
    ret = request_irq(dev->irq, pci_dma_irq,
                      IRQF_SHARED, DRIVER_NAME, dev);
    if (ret)
        goto err_dma;

    /* Character device (LEVEL 5) */
    ret = alloc_chrdev_region(&dev->devt, 0, 1, DRIVER_NAME);
    if (ret)
        goto err_irq;

    cdev_init(&dev->cdev, &pcie_fops);
    dev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&dev->cdev, dev->devt, 1);
    if (ret)
        goto err_chrdev;

    dev->device = device_create(class_create(THIS_MODULE, CLASS_NAME),
                                NULL, dev->devt, NULL,
                                "pcie_dma");

    pci_set_drvdata(pdev, dev);

    pr_info(DRIVER_NAME ": /dev/pcie_dma created\n");
    return 0;

err_chrdev:
    unregister_chrdev_region(dev->devt, 1);
err_irq:
    free_irq(dev->irq, dev);
err_dma:
    dma_free_coherent(&pdev->dev, dev->dma_size,
                      dev->dma_virt, dev->dma_handle);
err_iounmap:
    pci_iounmap(pdev, dev->bar0);
err_free:
    kfree(dev);
err_regions:
    pci_release_regions(pdev);
err_disable:
    pci_disable_device(pdev);
    return ret;
}

static void pci_dma_remove(struct pci_dev *pdev)
{
    struct pci_dma_dev *dev = pci_get_drvdata(pdev);

    device_destroy(class_find(CLASS_NAME), dev->devt);
    cdev_del(&dev->cdev);
    unregister_chrdev_region(dev->devt, 1);

    free_irq(dev->irq, dev);

    dma_free_coherent(&pdev->dev, dev->dma_size,
                      dev->dma_virt, dev->dma_handle);

    pci_iounmap(pdev, dev->bar0);
    kfree(dev);

    pci_release_regions(pdev);
    pci_disable_device(pdev);

    pr_info(DRIVER_NAME ": device removed\n");
}

static struct pci_driver pci_dma_driver = {
    .name = DRIVER_NAME,
    .id_table = pci_ids,
    .probe = pci_dma_probe,
    .remove = pci_dma_remove,
};

module_pci_driver(pci_dma_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haritha");
MODULE_DESCRIPTION("PCIe DMA driver with char device");









