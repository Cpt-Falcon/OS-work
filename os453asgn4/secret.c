/*Author: Aubrey Russell, Chris Voncina
Program: Assignment 4 driver
Date:  2/27/2016
Description: This is the MINIX driver for
the secret keeper and it is used by processes
to store a secret that can only be read by the user 
who owns the secret */

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
#define SECRET_SIZE 32 /*Buffer size*/
#define O_RDWR 0x6 /*read and write constant*/
#define O_WRONLY 0x2 /*write consant*/
#define O_RDONLY 0x4 /*read constant*/
/*
 * Function prototypes for the secret driver.
 */
FORWARD _PROTOTYPE( int secret_ioctl,    (struct driver *d, message *m) );
FORWARD _PROTOTYPE( char * secret_name,   (void) );
FORWARD _PROTOTYPE( int secret_open,      (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int secret_close,     (struct driver *d, message *m) );
FORWARD _PROTOTYPE( struct device * secret_prepare, (int device) );
FORWARD _PROTOTYPE( int secret_transfer,  (int procnr, int opcode,
                                          u64_t position, iovec_t *iov,
                                          unsigned nr_req) );
FORWARD _PROTOTYPE( void secret_geometry, (struct partition *entry) );

/* SEF functions and variables. */
FORWARD _PROTOTYPE( void sef_local_startup, (void) );
FORWARD _PROTOTYPE( int sef_cb_init, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int sef_cb_lu_state_save, (int) );
FORWARD _PROTOTYPE( int lu_state_restore, (void) );

/* Entry points to the secret driver. */
PRIVATE struct driver secret_tab =
{
    secret_name,
    secret_open,
    secret_close,
    secret_ioctl,
    secret_prepare,
    secret_transfer,
    nop_cleanup,
    secret_geometry,
    nop_alarm,
    nop_cancel,
    nop_select,
    do_nop,
};

/** Represents the /dev/secret device. */
PRIVATE struct device secret_device;

PRIVATE int open_counter; /*the number of fds*/
PRIVATE int curread = 0; /*current read position*/
PRIVATE char secret[SECRET_SIZE]; /*secret pointer */
PRIVATE size_t cursize = 0; /*current size of secret*/
PRIVATE size_t curpos = 0; /*current write position*/
PRIVATE struct ucred owner; /*owner of the secret*/
PRIVATE struct ucred tempowner; /*compare to owner to determine permission*/

/*function to get the name of this device, similar to hello_name*/
PRIVATE char * secret_name(void)
{
    return "secret";
}

/*transfer ownership between different user ids*/
PRIVATE int secret_ioctl(d, m)
    struct driver *d;
    message *m;
{
    int res;
    uid_t grantee;

    if (m->REQUEST != SSGRANT)
    { /*verify the SSGRANT request corresponding to secret*/
       return ENOTTY;
    }
    
    res = getnucred(m->IO_ENDPT, &tempowner);
    /*if the getnucred system calls fails, report it*/
    if (res < 0)
    {
       perror("getnucred failure in ioctl");
       return res;
    }

    if (owner.uid != tempowner.uid)
    { /*verify user permissions*/
       return EACCES;
    }

    res = sys_safecopyfrom(m ->IO_ENDPT, (vir_bytes)m->IO_GRANT, 0, 
                           (vir_bytes) &grantee, sizeof(grantee), D);/*
                           get the grantee as specified in spec*/
    if (res < 0)
    { /*check that sys safe copy from worked*/
       perror("sys_safecopyfrom failed in ioctl");
       return res;
    }
    owner.uid = grantee; /*set the new grantee*/
    return res;
}

/*open the driver and check permissions and sizes to make sure able
to open*/
PRIVATE int secret_open(d, m)
    struct driver *d;
    message *m;
{
    int error;
    if (cursize == 0)
    { /*if the secret is empty, set a new owner of the secret*/
        error = getnucred(m->IO_ENDPT, &owner);
        if (error < 0)
        {
            perror("getnucred in secret_open failed: ");
            return error;
        }
    }
    error = getnucred(m->IO_ENDPT, &tempowner); /*check that 
    the owner is theone who is granting the permission*/
    
    if (error < 0) /*verify that getnucred worked successfuly*/
    { 
        perror("getnucred in secret_open failed: ");
        return error;
    }

    if (owner.uid != tempowner.uid)
    {
       return EACCES;    /*access error*/
    }
    /*check to make sure not read write*/
    if (m->COUNT & O_RDWR == O_RDWR)
    {
        return EACCES; 
    }
    /*check if no open files and if there aren't
    and there is stuff in the buffer, then 
    return ENOSPC error in anything
    but read mode--this is for making
    sure nothing can write when there are
    no extra open calls and another
    program tries to write into the buffer*/
    if (open_counter == 0 && cursize > 0)
    {
       if (m->COUNT != O_RDONLY)
       {
           return ENOSPC;
       }
    }
    /*the file has already been opened
    by the same process, prevent
    opening multiple times by same process*/
    if (open_counter > 0 && owner.pid == tempowner.pid)
    {
        return EACCES;
    }
    open_counter++;
    return OK;
}

/*called to let the driver know a file descriptor is closing*/
PRIVATE int secret_close(d, m)
    struct driver *d;
    message *m;
{
    open_counter--;
    if (curread > 0 && open_counter == 0)
    { /*checks if a read has occured and the last file descriptor
    has been closed, and then clears the secret*/
       cursize = 0;
       curpos = 0;
       curread = 0;
    }
    return OK;
}

/*not important here but used to prepare the driver base and sizes*/
PRIVATE struct device * secret_prepare(dev)
    int dev;
{
    secret_device.dv_base.lo = 0;
    secret_device.dv_base.hi = 0;
    secret_device.dv_size.lo = 0;
    secret_device.dv_size.hi = 0;
    return &secret_device;
}

/*primary logic, read and write to the secret if there is room*/
PRIVATE int secret_transfer(proc_nr, opcode, position, iov, nr_req)
    int proc_nr;
    int opcode;
    u64_t position;
    iovec_t *iov;
    unsigned nr_req;
{
    int bytes, ret;
    struct ucred tempowner;
   switch (opcode)
    {
       case DEV_GATHER_S:/*get secret read data from secret*/
        if (curpos > 0 && curread < cursize)
        {
           
          ret = sys_safecopyto(proc_nr, iov->iov_addr, 0,
                              (vir_bytes) (secret + curread),
                              curpos, D);
          if (ret < 0)
          {
             perror("GATHER sys_safecopyto failed: ");
             return ret;
          }
           curpos -= cursize; /*move the position back*/
          curread = cursize; /*make so read doesn't cover what 
          it just read*/
           iov->iov_size -= iov->iov_size; /*change iov size so
          program using driver knows how many bytes were read*/
       }
        else
        {
            ret = 0;
          curpos = cursize;
        }
        
        break;
       case DEV_SCATTER_S:
        if ((iov->iov_size + cursize) <= SECRET_SIZE)
        {/*if the size is still in the limit*/
          ret = sys_safecopyfrom(proc_nr, iov->iov_addr, 0,
                                   (vir_bytes) (secret + cursize),
                                    iov->iov_size, D);
          if (ret < 0)
          {  /*error handling for safecopyfrom system call*/
             perror("SCATTER sys_safecopyfrom failure: ");
          }
           cursize += iov->iov_size; /*indicate bytes written to secret*/
               curpos += iov->iov_size; /*adjust the current position
               to the end so writing can append if multiple file descript
               ors*/
           iov->iov_size -= iov->iov_size;/*tell calling program how
          many bytes were written*/
        }
       else
        {/*if there are file descriptors open and the buffer still isn't
       full, then write in the difference to fill the gap*/
          ret = sys_safecopyfrom(proc_nr, iov->iov_addr, 0,
              (vir_bytes) (secret + cursize),
              SECRET_SIZE - cursize, D);
          if (ret < 0)
          {
             perror("SCATTER sys_safecopyfrom failure: ");
          }
           cursize = curpos = SECRET_SIZE;
           iov->iov_size = 0;
           ret = SECRET_SIZE - cursize;
           return ENOSPC; /*return ENOSPC since couldn't fill
          buffer entirely*/
        }
        break;
       default:
          return EINVAL;
    }
    return ret;
}

/*geometry initialization*/
PRIVATE void secret_geometry(entry)
    struct partition *entry;
{
    entry->cylinders = 0;
    entry->heads     = 0;
    entry->sectors   = 0;
}

/*used to save the state of driver in case of crash or restart--
used to restore important variables*/
PRIVATE int sef_cb_lu_state_save(int state) {
/* Save the state. */
    int error;
    error = ds_publish_mem("open_counter", (void *) &open_counter, 
    sizeof(int),DSF_OVERWRITE);/*save fd counter*/
    if (error < 0)
    {
       perror("ds_publish_mem open_counter error: ");
       return error;
    }

    error = ds_publish_mem("curread", (void *) &curread, sizeof(int), 
    DSF_OVERWRITE); /*save current read position*/
    if (error < 0)
    {
       perror("ds_publish_mem curread error: ");
       return error;
    }

    error = ds_publish_mem("secret",(void *) secret, cursize *sizeof(char),
    DSF_OVERWRITE); /*save secret block*/
    if (error < 0)
    {
       perror("ds_publish_mem secret error: ");
       return error;
    }

    error = ds_publish_mem("owner", (void *) &owner,sizeof(struct ucred), 
    DSF_OVERWRITE); /*save ownership permission struct*/
    if (error < 0)
    {
       perror("ds_publish_mem owner error: ");
       return error;
    }

    error = ds_publish_mem("cursize",(void *) &cursize, sizeof(int), 
    DSF_OVERWRITE); /*save current secret size*/
    if (error < 0)
    {
       perror("ds_publish_mem cursize error: ");
       return error;
    }

    error = ds_publish_mem("curpos", (void *)&curpos, sizeof(int), 
    DSF_OVERWRITE); /*save current writting position*/
    if (error < 0)
    {
       perror("ds_publish_mem curpos error: ");
       return error;
    }
    return OK;
}

/*restores all the variables listed in the above function
such that the driver may continue running normally if the driver
needs to be restarted by the reincarnation server for whatever
reason*/
PRIVATE int lu_state_restore() {
/* Restore the state. */
    size_t length;
    struct ucred credpoint;
    int ret, del, error;

    ret = ds_retrieve_mem("curpos", (char *)&curpos, &length);
    del = ds_delete_mem("curpos");
    if(ret < 0 || del < 0)
    {
       error = ret < del ? ret : del;
       perror("problem with retrieve or delete curpos: ");
       return error;
    }

    ret = ds_retrieve_mem("open_counter",  (char *)&open_counter, &length);
    del = ds_delete_mem("open_counter");
    if (ret < 0 || del < 0)
    {
       error = ret < del ? ret : del;
       perror("problem with retrieve or delete open_counter: ");
       return error;
    }

    ret = ds_retrieve_mem("curread", (char *)&curread, &length);
    del = ds_delete_mem("curread");
    if (ret < 0 || del < 0)
    {
       error = ret < del ? ret : del;
       perror("problem with retrieve or delete curread: ");
       return error;
    }

    ret = ds_retrieve_mem("cursize", (char *)&cursize, &length);
    del = ds_delete_mem("cursize");
    if (ret < 0 || del < 0)
    {
       error = ret < del ? ret : del;
       perror("problem with retireve or delete cursize: ");
       return error;
    }

    ret = ds_retrieve_mem("secret", secret, &length);
    del = ds_delete_mem("secret");
    if (ret < 0 || del < 0)
    {
       error = ret < del ? ret : del;
       perror("problem with retrieve or delete secret: ");
       return error;
    }

    ret = ds_retrieve_mem("owner", (char *)&owner, &length);
    del = ds_delete_mem("owner");
    if (ret < 0 || del < 0)
    {
       error = ret < del ? ret : del;
       perror("problem with retrieve or delete owner: ");
       return error;
    }

    return OK;
}

/*used to initialize the driver and callback functions*/
PRIVATE void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /*
     * Register live update callbacks.
     */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

/*handles new states of the driver and its corresponding logic*/
PRIVATE int sef_cb_init(int type, sef_init_info_t *info)
{
/* Initialize the hello driver. */
    int do_announce_driver = TRUE;

    open_counter = 0;
    switch(type) {
        case SEF_INIT_FRESH:
            cursize = curpos = 0;
            owner.pid = owner.uid = owner.gid = -1;
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;

        break;

        case SEF_INIT_RESTART:
            lu_state_restore();
        break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        driver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}
/*startup the driver*/
PUBLIC int main(int argc, char **argv)
{
    /*
     * Perform initialization.
     */
    sef_local_startup();

    /*
     * Run the main loop.
     */
    driver_task(&secret_tab, DRIVER_STD);
    return OK;
}
