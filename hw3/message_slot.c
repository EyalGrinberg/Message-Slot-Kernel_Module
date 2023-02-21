/* Declare what kind of code we want
 * from the header files. Defining __KERNEL__
 * and MODULE allows us to access kernel-level
 * code not usually available to userspace programs. */
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"


/* an array of minors, each entry is a pointer to the head of a linked list of all the channels that the specific minor uses */
static channel* minors_array[257]; 


static int device_open(struct inode* inode, struct file* file)
{
    int minor = iminor(inode);
    if (minors_array[minor] == NULL) { /* not initialized yet */
        minors_array[minor] = (channel*)kmalloc(sizeof(channel), GFP_KERNEL);
        /* initialize fields in the head channel of LL */
        minors_array[minor] -> id = 0; 
        minors_array[minor] -> minor = minor;
        minors_array[minor] -> buffer = NULL;
        minors_array[minor] -> msg_size = 0;
        minors_array[minor] -> next = NULL;
    }
    file -> private_data = minors_array[minor]; /* save in private_data field a pointer to the first channel node of the minor */
    return 0;
}


static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param)
{
    int minor;
    channel *head_channel, *curr_channel;
    channel *saved_channel = file -> private_data; /* extract saved pointer to channel */
    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0) { /* Error cases */
        return -EINVAL;
    }
    else { /* search the requested channel */
        minor = saved_channel -> minor; /* extract minor number to get the head of the LL */
        /* both head_channel and curr_channel are pointing to the first node */
        head_channel = minors_array[minor];
        curr_channel = head_channel;
        while (curr_channel -> next != NULL) {
            if (curr_channel -> next -> id == ioctl_param) { /* channel found ---> save pointer to the requested channel node in private_data */
                file -> private_data = curr_channel -> next;
                return 0;
            }
            curr_channel = curr_channel -> next;
        }
        /* channel not found ---> create new one */
        curr_channel -> next = (channel*)kmalloc(sizeof(channel), GFP_KERNEL);
        if (curr_channel -> next == NULL) { /* allocation failed */
            return -ENOMEM;
        }
        /* update fields of the created channel */
        curr_channel -> next -> id = ioctl_param;
        curr_channel -> next -> minor = minor;
        curr_channel -> next -> buffer = NULL;
        curr_channel -> next -> msg_size = 0;
        curr_channel -> next -> next = NULL;
        file -> private_data = curr_channel -> next; /* save a pointer to the requested channel node in private_data */
    }
    return 0;
}


/* a process which has already opened the device file attempts to read from it */
static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset)
{
    char *channel_msg;
    int channel_msg_size;
    channel *saved_channel = file -> private_data; /* extract saved pointer to channel */
    channel_msg = saved_channel -> buffer;
    channel_msg_size = saved_channel -> msg_size;
    /* Error cases */
    if (buffer == NULL || saved_channel -> id == 0) { 
        return -EINVAL;
    }
    if (channel_msg_size == 0) {
        return -EWOULDBLOCK;
    }
    if (length < channel_msg_size) {
        return -ENOSPC;
    }
    /* copy_to_user returns number of bytes that could not be copied. On success, this will be zero.*/
    if (copy_to_user(buffer, channel_msg, channel_msg_size) != 0) { 
        return -EIO;
    }
    return channel_msg_size; /* should return number of bytes read */
}


/* a processs which has already opened the device file attempts to write to it */
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    channel *saved_channel = file -> private_data; /* extract saved pointer to channel */
    /* Error cases */
    if (buffer == NULL || saved_channel -> id == 0) { 
        return -EINVAL;
    }
    if (length == 0 || length > 128) {
        return -EMSGSIZE;
    }
    if (saved_channel -> msg_size != 0) { /* need to overwrite existinig message */
        kfree(saved_channel -> buffer);
    }
    saved_channel -> buffer = (char*)kmalloc(length, GFP_KERNEL); /* and allocate length bytes for new message */
    if (saved_channel -> buffer == NULL) {
        return -ENOMEM;
    }
    /* copy_from_user returns number of bytes that could not be copied. On success, this will be zero.*/
    if (copy_from_user(saved_channel -> buffer, buffer, length) != 0) {
        return -EIO;
    }
    saved_channel -> msg_size = length; /* update message size in the channel */
    return length; /* return number of bytes written */
}


/* This structure will hold the functions to be called
 * when a process does something to the device we created */
struct file_operations Fops =
{
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
};


/* Initialize the module: Register the character device */
static int __init simple_init(void) 
{
    int rc = -1;
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops ); /* Register driver capabilities. major num = 235 */
    if (rc < 0) { /* Negative values signify an error */
        printk(KERN_ERR "ERROR: %s registration of %d failed \n", DEVICE_RANGE_NAME, MAJOR_NUM);
        return rc;
    }
    return 0;
}


static void __exit simple_cleanup(void)
{
    int i;
    channel *head_node, *curr_node;
    /* need to free channel nodes and channel buffers */
    for (i = 0; i < 257; i++) {
        head_node = minors_array[i];
        while (head_node != NULL) {
            if (head_node -> buffer != NULL) {
                kfree(head_node -> buffer);
            }
            curr_node = head_node -> next;
            kfree(head_node);
            head_node = curr_node;
        }
    }
    /* Unregister the device. Should always succeed */
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}


module_init(simple_init);
module_exit(simple_cleanup);