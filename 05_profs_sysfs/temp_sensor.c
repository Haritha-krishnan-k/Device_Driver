#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>

#define DRIVER_NAME "temp_monitor"
#define PROC_NAME   "temp_monitor"

static int current_temp = 40;     /* Celsius */
static int threshold = 70;        /* Celsius */

static struct proc_dir_entry *proc_entry; //procfs pointer
static struct kobject *temp_kobj; //sysfs kobject


static int read_temperature(void)
{
    current_temp = 30 + (jiffies % 40);  /* 30°C – 69°C */
    return current_temp;
}

static ssize_t proc_read(struct file *file, char __user *buf,
                         size_t count, loff_t *ppos)
{
    char buffer[128];
    int len;

    read_temperature();

    len = snprintf(buffer, sizeof(buffer),
                   "Temperature: %d C\nThreshold: %d C\n",
                   current_temp, threshold);

    return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};


static ssize_t temperature_show(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                char *buf)
{
    read_temperature();
    return sprintf(buf, "%d\n", current_temp);
}

static struct kobj_attribute temperature_attr =
    __ATTR(temperature, 0444, temperature_show, NULL);


static ssize_t threshold_show(struct kobject *kobj,
                              struct kobj_attribute *attr,
                              char *buf)
{
    return sprintf(buf, "%d\n", threshold);
}

static ssize_t threshold_store(struct kobject *kobj,
                               struct kobj_attribute *attr,
                               const char *buf, size_t count)
{
    int value;

    if (kstrtoint(buf, 10, &value))
        return -EINVAL;

    threshold = value;
    pr_info("%s: Threshold set to %d C\n", DRIVER_NAME, threshold);

    return count;
}

static struct kobj_attribute threshold_attr =
    __ATTR(threshold, 0664, threshold_show, threshold_store);


static int __init temp_driver_init(void)
{
    int ret;

    pr_info("%s: Initializing temperature driver\n", DRIVER_NAME);

    /* Procfs */
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_fops);
    if (!proc_entry)
        return -ENOMEM;

    /* Sysfs */
    temp_kobj = kobject_create_and_add(DRIVER_NAME, kernel_kobj);
    if (!temp_kobj)
        return -ENOMEM;

    ret = sysfs_create_file(temp_kobj, &temperature_attr.attr);
    ret |= sysfs_create_file(temp_kobj, &threshold_attr.attr);
    if (ret)
        pr_err("%s: Failed to create sysfs files\n", DRIVER_NAME);

    return 0;
}

static void __exit temp_driver_exit(void)
{
    proc_remove(proc_entry);
    sysfs_remove_file(temp_kobj, &temperature_attr.attr);
    sysfs_remove_file(temp_kobj, &threshold_attr.attr);
    kobject_put(temp_kobj);

    pr_info("%s: Driver unloaded\n", DRIVER_NAME);
}

module_init(temp_driver_init);
module_exit(temp_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Temperature Monitoring Driver with procfs and sysfs");
MODULE_VERSION("1.0");

