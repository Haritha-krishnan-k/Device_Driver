// vsensor_char.c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "vsensor"
#define MAJOR_NUM 240
#define IOCTL_CALIBRATE _IO(MAJOR_NUM, 0)

static int vsensor_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "vsensor: device opened\n");
    return 0;
}

static int vsensor_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "vsensor: device closed\n");
    return 0;
}

static ssize_t vsensor_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    int sensor_value = 42; // example sensor value
    if (len < sizeof(int))
        return -EINVAL;

    if (copy_to_user(buf, &sensor_value, sizeof(int)))
        return -EFAULT;

    printk(KERN_INFO "vsensor: sensor value read: %d\n", sensor_value);
    return sizeof(int);
}

static long vsensor_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch(cmd) {
        case IOCTL_CALIBRATE:
            printk(KERN_INFO "vsensor: calibration performed\n");
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = vsensor_open,
    .release = vsensor_release,
    .read = vsensor_read,
    .unlocked_ioctl = vsensor_ioctl,
};

static int __init vsensor_init(void) {
    register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    printk(KERN_INFO "vsensor: character device registered\n");
    return 0;
}

static void __exit vsensor_exit(void) {
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk(KERN_INFO "vsensor: character device unregistered\n");
}

module_init(vsensor_init);
module_exit(vsensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Virtual Sensor Character Device");
