CODING LEVEL 1 — PCI Driver Skeleton
✅ Task 1:

Write a PCI driver that:

• registers with PCI subsystem
• matches vendor/device ID
• implements probe() and remove()

👉 (no DMA yet)

🧩 CODING LEVEL 2 — MMIO Mapping
✅ Task 2:

Inside probe():

• enable PCI device
• request BAR regions
• map BAR0 using pci_iomap()
• store pointer in device struct

🧩 CODING LEVEL 3 — DMA Buffer Allocation
✅ Task 3:

Write code to:

• allocate coherent DMA buffer
• store kernel virtual address
• store DMA bus address
• free buffer on remove

🧩 CODING LEVEL 4 — Interrupt Handling
✅ Task 4:

Write:

• IRQ handler
• request_irq() in probe
• free_irq() in remove
• acknowledge device interrupt

🧩 CODING LEVEL 5 — Character Device
✅ Task 5:

Implement:

• alloc_chrdev_region
• cdev_init
• device_create

Create /dev/pcie_dma

🧩 CODING LEVEL 6 — mmap Support
✅ Task 6:

Implement:

mmap() → remap_pfn_range()


Map DMA buffer to user space.

🧩 CODING LEVEL 7 — Start DMA Transfer
✅ Task 7:

Write function:

start_dma(struct pcie_dma_dev *dev)


That programs hardware registers:

DMA address
DMA length
DMA start bit

🧩 CODING LEVEL 8 — Clean Resource Handling
✅ Task 8:

Ensure proper cleanup order in remove():

IRQ
DMA buffer
MMIO
PCI regions
char device