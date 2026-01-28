#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>

#define DEVICE_NAME "vblock"
#define VBLK_MAJOR 240
#define NSECTORS 1024
#define SECTOR_SIZE 512

struct vblock_dev {
    int size;
    u8 *data;
    struct request_queue *queue;
    struct gendisk *gd;
};

static struct vblock_dev vblock;

static void vblock_request(struct request_queue *q)
{
    struct request *req;

    while ((req = blk_fetch_request(q)) != NULL) {
        struct bio_vec bv;
        struct req_iterator iter;
        sector_t sector = blk_rq_pos(req);

        rq_for_each_segment(bv, req, iter) {
            unsigned long offset = sector * SECTOR_SIZE;
            char *buffer = kmap(bv.bv_page) + bv.bv_offset;

            if (offset + bv.bv_len > vblock.size) {
                kunmap(bv.bv_page);
                __blk_end_request_all(req, -EIO);
                break;
            }

            if (rq_data_dir(req) == WRITE)
                memcpy(vblock.data + offset, buffer, bv.bv_len);
            else
                memcpy(buffer, vblock.data + offset, bv.bv_len);

            kunmap(bv.bv_page);
            sector += bv.bv_len / SECTOR_SIZE;
        }

        __blk_end_request_all(req, 0);
    }
}

static int vblock_create(struct vblock_dev *dev)
{
    dev->size = NSECTORS * SECTOR_SIZE;

    dev->data = vmalloc(dev->size);
    if (!dev->data)
        return -ENOMEM;

    memset(dev->data, 0, dev->size);

    if (register_blkdev(VBLK_MAJOR, DEVICE_NAME)) {
        vfree(dev->data);
        return -EBUSY;
    }

    dev->queue = blk_init_queue(vblock_request, NULL);
    if (!dev->queue) {
        unregister_blkdev(VBLK_MAJOR, DEVICE_NAME);
        vfree(dev->data);
        return -ENOMEM;
    }

    dev->gd = alloc_disk(1);
    if (!dev->gd) {
        blk_cleanup_queue(dev->queue);
        unregister_blkdev(VBLK_MAJOR, DEVICE_NAME);
        vfree(dev->data);
        return -ENOMEM;
    }

    dev->gd->major = VBLK_MAJOR;
    dev->gd->first_minor = 0;
    dev->gd->fops = &(struct block_device_operations){
        .owner = THIS_MODULE,
    };
    dev->gd->queue = dev->queue;
    dev->gd->private_data = dev;

    snprintf(dev->gd->disk_name, 32, DEVICE_NAME);
    set_capacity(dev->gd, NSECTORS);

    add_disk(dev->gd);
    return 0;
}

static void vblock_destroy(struct vblock_dev *dev)
{
    if (dev->gd) {
        del_gendisk(dev->gd);
        put_disk(dev->gd);
    }

    if (dev->queue)
        blk_cleanup_queue(dev->queue);

    unregister_blkdev(VBLK_MAJOR, DEVICE_NAME);

    if (dev->data)
        vfree(dev->data);
}

static int __init vblock_init(void)
{
    int ret = vblock_create(&vblock);
    if (ret)
        return ret;

    printk(KERN_INFO "vblock: virtual block device loaded\n");
    return 0;
}

static void __exit vblock_exit(void)
{
    vblock_destroy(&vblock);
    printk(KERN_INFO "vblock: unloaded\n");
}

module_init(vblock_init);
module_exit(vblock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("In-memory Virtual Block Device");
