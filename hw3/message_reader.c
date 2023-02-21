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
    int fd, ret_val;
    char *file_path;
    unsigned int channel_id;
    char buffer[128];

    if (argc != 3) {
        perror("ERROR: wrong number of arguments \n");
        exit(1);
    }

    file_path = argv[1];
    channel_id = atoi(argv[2]); /* should be converted to int */

    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("ERROR: can't open device file \n");
        exit(1);
    }
    ret_val = ioctl(fd, MSG_SLOT_CHANNEL, channel_id); /* set channel id */
    if (ret_val != 0) {
        perror("ERROR: ioctl failed \n");
        exit(1);
    }
    ret_val = read(fd, buffer, 128); /* read from slot to a buffer */
    if (ret_val < 0) {
        perror("ERROR: read failed \n");
        exit(1);
    }
    close(fd);

    ret_val = write(1, buffer, ret_val); /* write buffer content to STDOUT */
    if (ret_val < 0) {
        perror("ERROR: write failed \n");
        exit(1);
    }
    
    return 0;
}