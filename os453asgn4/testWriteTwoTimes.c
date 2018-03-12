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
/*this program is going to test that an open file 
can continue to append to the secret and fills
up the buffer such that it truncates the
string that would exceed the buffer length
its a buffer size of 32 in this case*/
int main(int argc, char **argv)
{
    int fd1, fd2, res, uid;
    char *msg = "The british are coming\n";
    fd1 = open("/dev/Secret", O_WRONLY);
    res = write(fd1,msg, strlen(msg));
    res = write(fd1,msg, strlen(msg));
    if (res < 0)
    {
      printf("Error when writing stuff that would exceed buffer\n");
    }
    res = close(fd1);
    res = close(fd2);
    return 0;
}
