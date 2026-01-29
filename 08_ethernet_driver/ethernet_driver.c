#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>


static int eth_open(struct net_device *dev)
{
    netif_start_queue(dev);
    pr_info("%s opened\n", dev->name);
    return 0;
}

static int eth_stop(struct net_device *dev)
{
    netif_stop_queue(dev);
    pr_info("%s stopped\n", dev->name);
    return 0;
}

//receives the packet from the kernel networking stack
//frees it 
//update staticstics (tx_bytes and rx_bytes)
//flag on success

//eg:
//dummy user-space gives u a letter and then u pass it here 
//u do nothing u throw it and update

static netdev_tx_t eth_xmit(struct sk_buff *skb,
                            struct net_device *dev)
{
    /* Free packet (no real hardware) */
    dev_kfree_skb(skb); //send packet to the hardware
    //since nothing we just free it 

    /* Update stats */
    //no of packets transfered and bytes
    dev->stats.tx_packets++; 
    dev->stats.tx_bytes += skb->len;

    return NETDEV_TX_OK;
}


static const struct net_device_ops eth_netdev_ops = {
    .ndo_open       = eth_open,
    .ndo_stop       = eth_stop,
    .ndo_start_xmit = eth_xmit,
};

//this is to setup default values 
//does basic operation
//falg-ether net flag
//kernel networking stack

static void eth_setup(struct net_device *dev)
{
    ether_setup(dev);

    dev->netdev_ops = &eth_netdev_ops;
    dev->flags |= IFF_NOARP;
    eth_hw_addr_random(dev);
}

//net_device - name , flags , MAC add , stats , callbacks and private data
static struct net_device *eth_dev;

static int __init eth_init(void)
{
    int ret;
    
    eth_dev = alloc_netdev(0, "eth_sim%d",
                           NET_NAME_UNKNOWN, eth_setup); //after allocating a struct net_device with alloc_netdev()
    if (!eth_dev)
        return -ENOMEM;

    ret = register_netdev(eth_dev);
    if (ret) {
        free_netdev(eth_dev);
        return ret;
    }

    pr_info("Ethernet driver loaded\n");
    return 0;
}

static void __exit eth_exit(void)
{
    unregister_netdev(eth_dev);
    free_netdev(eth_dev);
    pr_info("Ethernet driver unloaded\n");
}

module_init(eth_init);
module_exit(eth_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Basic Ethernet Driver Skeleton");
