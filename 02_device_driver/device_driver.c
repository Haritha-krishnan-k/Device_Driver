// vblock.c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>

#define DEVICE_NAME "vblock"
#define VBLK_MAJOR 240
#define NSECTORS 1024  // 1024 sectors
#define SECTOR_SIZE 512

static struct vblock_dev {
    int size;
    u8 *data;
    struct request_queue *queue;
    struct gendisk *gd;
} vblock;

static void vblock_request(struct request_queue *q) {
    struct request *req;
    while ((req = blk_fetch_request(q)) != NULL) {
        struct bio_vec bv;
        struct req_iterator iter;
        sector_t sector = blk_rq_pos(req);
        unsigned int sectors = blk_rq_sectors(req);
        rq_for_each_segment(bv, req, iter) {
            char *buffer = kmap(bv.bv_page) + bv.bv_offset;
            if (rq_data_dir(req) == WRITE)
                memcpy(vblock.data + sector*SECTOR_SIZE, buffer, sectors*SECTOR_SIZE);
            else
                memcpy(buffer, vblock.data + sector*SECTOR_SIZE, sectors*SECTOR_SIZE);
            kunmap(bv.bv_page);
        }
        __blk_end_request_all(req, 0);
    }
}

static int __init vblock_init(void) {
    vblock.size = NSECTORS * SECTOR_SIZE;
    vblock.data = vmalloc(vblock.size);
    memset(vblock.data, 0, vblock.size);

    vblock.queue = blk_init_queue(vblock_request, NULL);
    vblock.gd = alloc_disk(1);
    vblock.gd->major = VBLK_MAJOR;
    vblock.gd->first_minor = 0;
    vblock.gd->fops = NULL;
    vblock.gd->queue = vblock.queue;
    strcpy(vblock.gd->disk_name, DEVICE_NAME);
    set_capacity(vblock.gd, NSECTORS);
    add_disk(vblock.gd);

    printk(KERN_INFO "vblock: block device registered\n");
    return 0;
}

static void __exit vblock_exit(void) {
    del_gendisk(vblock.gd);
    put_disk(vblock.gd);
    blk_cleanup_queue(vblock.queue);
    vfree(vblock.data);
    printk(KERN_INFO "vblock: block device unregistered\n");
}

module_init(vblock_init);
module_exit(vblock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("In-memory Virtual Block Device");
