#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "message_slot.h"

int main(int argc, char** argv)
{
    int fd, ret_val, msg_size;
    char *file_path, *msg;
    unsigned int channel_id;

    if (argc != 4) {
        perror("ERROR: wrong number of arguments \n");
        exit(1);
    }

    file_path = argv[1];
    channel_id = atoi(argv[2]); /* should be converted to int */
    msg = argv[3];
    msg_size = strlen(msg); /* strlen doesn't include the terminating null character (as requested) */

    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("ERROR: can't open device file \n");
        exit(1);
    }
    ret_val = ioctl(fd, MSG_SLOT_CHANNEL, channel_id); /* set channel */
    if (ret_val != 0) {
        perror("ERROR: ioctl failed \n");
        exit(1);
    }
    ret_val = write(fd, msg, msg_size);
    if (ret_val != msg_size) {
        perror("ERROR: write failed \n");
        exit(1);
    }

    close(fd);
    return 0;
}