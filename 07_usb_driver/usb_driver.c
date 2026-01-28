#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/slab.h>

#define DRIVER_NAME "usb_temp_sensor"

/* Fake USB IDs (replace with real ones if available) */
#define USB_TEMP_VENDOR_ID  0x1234
#define USB_TEMP_PRODUCT_ID 0x5678

/* USB device structure */
struct usb_temp_dev {
    struct usb_device *udev;
    struct usb_interface *interface;
    unsigned char bulk_in_ep;
    unsigned char bulk_out_ep;
};

/* ------------------------------------------------ */
/* USB device ID table                              */
/* ------------------------------------------------ */
static const struct usb_device_id temp_table[] = {
    { USB_DEVICE(USB_TEMP_VENDOR_ID, USB_TEMP_PRODUCT_ID) },
    {}
};
MODULE_DEVICE_TABLE(usb, temp_table);

/* ------------------------------------------------ */
/* Read temperature from device                     */
/* ------------------------------------------------ */
static int read_temperature(struct usb_temp_dev *dev)
{
    int retval;
    unsigned char data[2];
    int actual_length;
    int temperature;

    retval = usb_bulk_msg(dev->udev,
                          usb_rcvbulkpipe(dev->udev, dev->bulk_in_ep),
                          data,
                          sizeof(data),
                          &actual_length,
                          1000);

    if (retval) {
        pr_err("[%s] Failed to read temperature\n", DRIVER_NAME);
        return retval;
    }

    temperature = data[0]; /* Simple 1-byte temperature */
    pr_info("[%s] Current Temperature: %d C\n",
            DRIVER_NAME, temperature);

    return 0;
}

/* ------------------------------------------------ */
/* Probe: called when device is plugged in           */
/* ------------------------------------------------ */
static int temp_probe(struct usb_interface *interface,
                      const struct usb_device_id *id)
{
    struct usb_temp_dev *dev;
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    int i;

    pr_info("[%s] USB Temperature Sensor connected\n",
            DRIVER_NAME);

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->udev = usb_get_dev(interface_to_usbdev(interface));
    dev->interface = interface;

    iface_desc = interface->cur_altsetting;

    /* Find bulk endpoints */
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        endpoint = &iface_desc->endpoint[i].desc;

        if (usb_endpoint_is_bulk_in(endpoint)) {
            dev->bulk_in_ep = endpoint->bEndpointAddress;
        }
        if (usb_endpoint_is_bulk_out(endpoint)) {
            dev->bulk_out_ep = endpoint->bEndpointAddress;
        }
    }

    usb_set_intfdata(interface, dev);

    /* Read temperature once device is connected */
    read_temperature(dev);

    return 0;
}

/* ------------------------------------------------ */
/* Disconnect: called when device is unplugged       */
/* ------------------------------------------------ */
static void temp_disconnect(struct usb_interface *interface)
{
    struct usb_temp_dev *dev;

    dev = usb_get_intfdata(interface);
    usb_set_intfdata(interface, NULL);

    if (dev) {
        usb_put_dev(dev->udev);
        kfree(dev);
    }

    pr_info("[%s] USB Temperature Sensor disconnected\n",
            DRIVER_NAME);
}

/* ------------------------------------------------ */
/* USB driver structure                             */
/* ------------------------------------------------ */
static struct usb_driver temp_usb_driver = {
    .name       = DRIVER_NAME,
    .probe      = temp_probe,
    .disconnect = temp_disconnect,
    .id_table   = temp_table,
};

/* ------------------------------------------------ */
/* Module init / exit                               */
/* ------------------------------------------------ */
static int __init temp_init(void)
{
    pr_info("[%s] USB Temperature Driver loaded\n",
            DRIVER_NAME);
    return usb_register(&temp_usb_driver);
}

static void __exit temp_exit(void)
{
    usb_deregister(&temp_usb_driver);
    pr_info("[%s] USB Temperature Driver unloaded\n",
            DRIVER_NAME);
}

module_init(temp_init);
module_exit(temp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Dev");
MODULE_DESCRIPTION("USB Temperature Sensor Driver");
MODULE_VERSION("1.0");
