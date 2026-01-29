// vdisk.c - Virtual Hard Disk block device
#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/vmalloc.h>
#include <linux/hdreg.h>
#include <linux/errno.h>

#define DEVICE_NAME "vdisk"
#define VDISK_MAJOR 240
#define NSECTORS 1024    // number of sectors
#define SECTOR_SIZE 512  // 512 bytes per sector

struct vdisk_dev {
    int size;                  // device size in bytes
    u8 *data;                  // memory to store disk data
    struct request_queue *queue;
    struct gendisk *gd;
};

static struct vdisk_dev vdisk;

// Request handler
static void vdisk_request(struct request_queue *q) {
    struct request *req; // one blocl input / output req 
    while ((req = blk_fetch_request(q)) != NULL) {
        sector_t sector = blk_rq_pos(req); // the sector size
        unsigned int sectors = blk_rq_sectors(req); // the sector 
        struct bio_vec bv; // this is the memory segment 
        struct req_iterator iter; // iteration

        rq_for_each_segment(bv, req, iter) { //ever sector the memory segment , request block all taken with them
            char *buffer = kmap(bv.bv_page) + bv.bv_offset; // maps the physical memory to the virtual memoty and buffer is the pointer to that memory

            if (rq_data_dir(req) == WRITE) { // req_data_dir - tells the direction
                memcpy(vdisk.data + sector*SECTOR_SIZE, buffer, sectors*SECTOR_SIZE); //kernel buffer to virtual disk
            } else {
                memcpy(buffer, vdisk.data + sector*SECTOR_SIZE, sectors*SECTOR_SIZE); // virtual disk to kernel buffer 
            }

            kunmap(bv.bv_page);
        }

        __blk_end_request_all(req, 0);
    }
}

// Module init
static int __init vdisk_init(void) {
    printk(KERN_INFO "vdisk: Initializing virtual disk\n");

    // Allocate memory for disk
    vdisk.size = NSECTORS * SECTOR_SIZE;
    vdisk.data = vmalloc(vdisk.size);
    if (!vdisk.data)
        return -ENOMEM;
    memset(vdisk.data, 0, vdisk.size);

    // Initialize request queue
    vdisk.queue = blk_init_queue(vdisk_request, NULL);
    if (!vdisk.queue) {
        vfree(vdisk.data);
        return -ENOMEM;
    }

    // Initialize gendisk structure
    vdisk.gd = alloc_disk(1);
    if (!vdisk.gd) {
        blk_cleanup_queue(vdisk.queue);
        vfree(vdisk.data);
        return -ENOMEM;
    }

    vdisk.gd->major = VDISK_MAJOR;
    vdisk.gd->first_minor = 0;
    vdisk.gd->fops = NULL; // no special ops
    vdisk.gd->queue = vdisk.queue;
    strcpy(vdisk.gd->disk_name, DEVICE_NAME);
    set_capacity(vdisk.gd, NSECTORS);
    add_disk(vdisk.gd);

    printk(KERN_INFO "vdisk: Virtual disk registered as /dev/%s\n", DEVICE_NAME);
    return 0;
}

// Module exit
static void __exit vdisk_exit(void) {
    del_gendisk(vdisk.gd);
    put_disk(vdisk.gd);
    blk_cleanup_queue(vdisk.queue);
    vfree(vdisk.data);
    printk(KERN_INFO "vdisk: Virtual disk unregistered\n");
}

module_init(vdisk_init);
module_exit(vdisk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Virtual Hard Disk Block Device Driver");
