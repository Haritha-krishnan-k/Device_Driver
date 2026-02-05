#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>

#define DRIVER_NAME "pci_dma_driver"

#define PCI_VENDOR_EXAMPLE  0x1234
#define PCI_DEVICE_EXAMPLE  0x5678

#define DMA_BUF_SIZE (4 * 1024 * 1024)   /* 4MB */

/* -------------------------------------------------- */
/* Per-device private structure */
/* -------------------------------------------------- */
//one software object for one pci device
struct pci_dma_dev {
    struct pci_dev *pdev;  //which pci device am I controlling

    void __iomem *mmio_base; // points to device registers (BAR0)
    void *dma_virt;           //normal kernel pointer , where data actually lands
    dma_addr_t dma_handle;   //the add the pci device understands 
    			      // address the pci device understands
    size_t dma_size; //one physical address and this has two other address -> dma_virt (cpu view) and dma_handle(device view)
};

/* -------------------------------------------------- */
/* PCI ID table */
/* -------------------------------------------------- */
static struct pci_device_id pci_ids[] = {
    { PCI_DEVICE(PCI_VENDOR_EXAMPLE, PCI_DEVICE_EXAMPLE) },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, pci_ids);

/* -------------------------------------------------- */
/* Probe */
/* -------------------------------------------------- */
static int pci_dma_probe(struct pci_dev *pdev,
                         const struct pci_device_id *id)
{
    int ret;
    struct pci_dma_dev *dev;

    printk(KERN_INFO "PCI device detected\n");

    ret = pci_enable_device(pdev);
    if (ret)
        return ret;

    ret = pci_request_regions(pdev, DRIVER_NAME);
    if (ret)
        goto err_disable;

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev) {
        ret = -ENOMEM;
        goto err_release;
    }

    dev->pdev = pdev;

    dev->mmio_base = pci_iomap(pdev, 0, 0);
    if (!dev->mmio_base) {
        ret = -EIO;
        goto err_free;
    }

    /* ---------------- DMA ALLOCATION ---------------- */
    //creates RAM buffer
    //write data at this DMA address -  this uses MMIO registers
    
    dev->dma_size = DMA_BUF_SIZE;

    dev->dma_virt = dma_alloc_coherent(&pdev->dev,
                                       dev->dma_size,
                                       &dev->dma_handle,
                                       GFP_KERNEL);
    if (!dev->dma_virt) {
        ret = -ENOMEM;
        goto err_iounmap;
    }

    printk(KERN_INFO "DMA buffer allocated\n");
    printk(KERN_INFO "Kernel addr: %p\n", dev->dma_virt);
    printk(KERN_INFO "DMA addr: %pad\n", &dev->dma_handle);

    pci_set_drvdata(pdev, dev);

    return 0;

err_iounmap:
    pci_iounmap(pdev, dev->mmio_base);
err_free:
    kfree(dev);
err_release:
    pci_release_regions(pdev);
err_disable:
    pci_disable_device(pdev);
    return ret;
}

/* -------------------------------------------------- */
/* Remove */
/* -------------------------------------------------- */
static void pci_dma_remove(struct pci_dev *pdev)
{
    struct pci_dma_dev *dev = pci_get_drvdata(pdev);

    printk(KERN_INFO "Removing PCI device\n");

    if (!dev)
        return;

    if (dev->dma_virt)
        dma_free_coherent(&pdev->dev,
                          dev->dma_size,
                          dev->dma_virt,
                          dev->dma_handle);

    if (dev->mmio_base)
        pci_iounmap(pdev, dev->mmio_base);

    kfree(dev);

    pci_release_regions(pdev);
    pci_disable_device(pdev);
}

/* -------------------------------------------------- */
/* PCI driver */
/* -------------------------------------------------- */
static struct pci_driver pci_dma_driver = {
    .name = DRIVER_NAME,
    .id_table = pci_ids,
    .probe = pci_dma_probe,
    .remove = pci_dma_remove,
};

module_pci_driver(pci_dma_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haritha");
MODULE_DESCRIPTION("PCI DMA coherent buffer example");
