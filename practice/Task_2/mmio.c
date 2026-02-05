#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#define DEVICE_NAME "pcie"
#define VENDOR_ID 0x1234
#define DEVICE_ID 0x5678

static int pci_probe(struct pci_dev *pdev , const struct pci_device_id *id)
{
    struct pcie_dev *dev;
    in ret;

    ret = pci_enable_device(pdev);
    if (ret)
        return ret;
    ret = pci_request_regions(pdev,DRIVER_NAME);
    if (ret) {
        pci_disable_device(pdev);
        return ret;
    }

    dev = kzalloc(sizeof(*dev),GFP_KERNEL);
    if (!dev) {
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        return -ENOMEM;
    } 
    
    dev->bar0 = pci_iomap(pdev, 0, 0);
    if (!dev->bar0) {
        kfree(dev);
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        return -EIO;
    }

    pci_set_drvdata(pdev, dev);

    pr_info("[%s] BAR0 mapped successfully\n", DRIVER_NAME);

    return 0;

}

static void pci_remove(struct pci_dev *pdev)
{
struct pcie_dev *dev = pci_get_drvdata(pdev);

    if (dev) {
        pci_iounmap(pdev, dev->bar0);
        kfree(dev);
    }

    pci_release_regions(pdev);
    pci_disable_device(pdev);

    pr_info("[%s] Device removed\n", DRIVER_NAME);
}

static const struct pci_device_id pci_ids[] ={
    {PCI_DEVICE(VENDOR_ID,DEVICE_ID)},
    {}
};


static const struct pci_driver pci_driver = {
            .name =DRIVER_NAME,
            .id_table= pci_ids,
            .probe=pci_probe,
            .remove=pci_remove,
};


module_pci_driver(pci_driver);
