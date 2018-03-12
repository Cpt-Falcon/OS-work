#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include <minix/syslib.h>
#include "secret.h"
#include <unistd.h>
#include <sys/ucred.h>
#include <sys/ioc_secret.h>
#include <minix/const.h>

int main(int argc, char **argv)
{
    int fd, res, uid;
    char *msg = "Hello World";
    fd = open("/dev/Secret", O_WRONLY);
    printf("Opening... fd=%d\n", fd);
    res = write(fd,msg, strlen(msg));
    printf("Writing... res=%d\n", res);
    if (argc > 1 && 0 != (uid=atoi(argv[1]))) 
    {
 	
	if(res = ioctl(fd, SSGRANT, &uid) ) 
	    perror("ioctl");

            printf("Trying to change owner to %d...res = %d\n",
               uid, res);
    }
    res = close(fd);

    return 0;
}
