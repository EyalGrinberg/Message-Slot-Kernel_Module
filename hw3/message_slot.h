#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235 /* all files have the same major number, which is hard-coded to 235 */
#define DEVICE_RANGE_NAME "message_slot"
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long) /* for ioctl */


/* each minor will have a linked list of all the channels it uses */
typedef struct channel {
    unsigned int id;
    int minor;
    char *buffer;
    int msg_size;
    struct channel *next;
} channel;

#endif