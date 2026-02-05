#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>

#define DRIVER_NAME "pcie"

#define VENDOR_ID 0x1234
#define DEVICE_ID 0x5678

static int pci_probe(struct pci_dev *pdev, const struct pci_device_id *id) 
{ 
  pr_info("[%s] Device detected!\n", DRIVER_NAME); 
  if(pci_enable_device(pdev)) // this turns ON the hardware
    return -ENODEV; 
  pr_info("[%s] PCI device enabled\n", DRIVER_NAME); 
  return 0; 
}

static void pci_remove(struct pci_dev *pdev) 
{
     pci_disable_device(pdev); 
     pr_info("[%s] Device removed\n", DRIVER_NAME); 
}

static const struct pci_device_id pci_ids[]={
    { PCI_DEVICE(VENDOR_ID, DEVICE_ID) },
    {}

};

MODULE_DEVICE_TABLE(pci, pci_ids);

static struct pci_driver pci_driver = {
    .name     = DRIVER_NAME,
    .id_table = pci_ids,
    .probe    = pci_probe,
    .remove   = pci_remove,
};

module_pci_driver(pci_driver);

// static int __init pci_init(void)
// {
//     pr_info("[%s] Driver loaded\n",DRIVER_NAME);
//     return pci_register_driver(&pci_driver);
// }

// static void __exit pci_exit(void)
// {
//     pci_unregister_driver(&pci_driver);
//     pr_info("[%s] Driver unloaded\n",DRIVER_NAME);
// }

// module_init(pci_init);
// module_exit(pci_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haritha");
MODULE_DESCRIPTION("PCI driver skeleton with init/exit");

//pcie device (network card , GPU)
//pcie have its own memory area (register(BAR0) and RAM(BAR1) )
//how does cpu access them ? this is where memroy mapped I/O called
//