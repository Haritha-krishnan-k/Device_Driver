// vhrm.c - Virtual Heart Rate Monitor
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/random.h>
#include <linux/mutex.h>

#define DEVICE_NAME "vhrm"
#define MAJOR_NUM 241

// IOCTL commands
#define IOCTL_SET_SAMPLING_RATE _IOW(MAJOR_NUM, 0, int)

static int hrm_major = MAJOR_NUM;
static int sampling_rate = 1; // default 1Hz
static DEFINE_MUTEX(hrm_mutex); // protect shared data

// Device open
static int hrm_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "vhrm: device opened\n");
    return 0;
}

// Device close
static int hrm_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "vhrm: device closed\n");
    return 0;
}

// Device read
static ssize_t hrm_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    int hr_value;

    // Simulate heart rate: random number 60-100 bpm
    get_random_bytes(&hr_value, sizeof(hr_value));
    hr_value = 60 + (hr_value % 41); // 60-100

    if (len < sizeof(int))
        return -EINVAL;

    if (copy_to_user(buf, &hr_value, sizeof(int)))
        return -EFAULT;

    printk(KERN_INFO "vhrm: heart rate read: %d bpm\n", hr_value);
    msleep(1000 / sampling_rate); // simulate sampling delay

    return sizeof(int);
}

// IOCTL handler
static long hrm_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int rate; // this is passed from the user ioctl(fd,ioctl_Set_sampleing_rate,&rate)
    switch(cmd) {
        case IOCTL_SET_SAMPLING_RATE:
            if (copy_from_user(&rate, (int __user *)arg, sizeof(rate))) //copy data from the user-space
                return -EFAULT;
            if (rate <= 0 || rate > 100)
                return -EINVAL;

            mutex_lock(&hrm_mutex);
            sampling_rate = rate; // only one function overwrites the value at a time
            mutex_unlock(&hrm_mutex);

            printk(KERN_INFO "vhrm: sampling rate set to %d Hz\n", rate);
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

// File operations
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = hrm_open,
    .release = hrm_release,
    .read = hrm_read,
    .unlocked_ioctl = hrm_ioctl,
};

// Module init
static int __init hrm_init(void) {
    int result = register_chrdev(hrm_major, DEVICE_NAME, &fops);
    if (result < 0) {
        printk(KERN_ERR "vhrm: failed to register device\n");
        return result;
    }
    printk(KERN_INFO "vhrm: Virtual Heart Rate Monitor loaded (major %d)\n", hrm_major);
    return 0;
}

// Module exit
static void __exit hrm_exit(void) {
    unregister_chrdev(hrm_major, DEVICE_NAME);
    printk(KERN_INFO "vhrm: Virtual Heart Rate Monitor unloaded\n");
}

module_init(hrm_init);
module_exit(hrm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Virtual Heart Rate Monitor Character Device");
