
#include <assert.h>
#include "kernel_cc.h"
#include "kernel_dev.h"
#include "kernel_sched.h"
#include "kernel_streams.h"
#include "kernel_proc.h"

/*************************************

      Devices and device drivers

 *************************************/

DCB DT[MAX_TERMINALS];


/*
_____________________________________________________________________________________________


           =============================================================

                              The null device driver

           =============================================================
*/

//_____________________________________________________________________________________________

int nulldev_read(void* dev, char *buf, unsigned int size)
{
  memset(buf, 0, size);
  return size;
}
//_____________________________________________________________________________________________



//_____________________________________________________________________________________________

int nulldev_write(void* dev, const char* buf, unsigned int size)
{
    /* Here, we do not copy anything, therefore simply return
       a value equal to the argument.
     */
    return size;
}

//_____________________________________________________________________________________________



//_____________________________________________________________________________________________

int nulldev_close(void* dev) 
{
  return 0;
}

//_____________________________________________________________________________________________


//_____________________________________________________________________________________________

void* nulldev_open(uint minor)
{
  return NULL;
}

//_____________________________________________________________________________________________

/*
⣿⣿⣿⣿⠏⠌⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⠀⠀⠸⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⠃⠀⣠⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⡿⠃⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⠃⠀⠀⣾⣿⣿⣿⣿⣿⣦⠀⠈⠻⣿⣿⣿⣿
⣿⠀⠀⠀⣿⣿⣿⠟⠉⠉⠉⢃⣤⠀⠈⢿⣿⣿
⣿⠀⠀⠀⢸⣿⡟⠀⠀⠀⠀⢹⣿⣧⠀⠀⠙⣿
⣿⡆⠀⠀⠈⠻⡅⠀⠀⠀⠀⣸⣿⠿⠇⠀⠀⢸
⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠔⠛⠁⠀⠀⠀⣠⣿
⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⣿
⣿⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣾⣿⣿⣿⣿
⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⣠⣿⣿⣿⣿⣿⣿
⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠘⣿⣿⣿⣿⣿⣿⣿
⣿⣿⠁⠀⠀⠀⠀⠀⠀⠀⠀⢹⣿⣿⣿⣿⣿⣿
⣿⠟⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣿⣿⣿⣿⣿
⡟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿

*/
// WE STOLE THIS, WE SIMPLY BORROWED INFINATELY IN kernel_pipes.c
//_____________________________________________________________________________________________

static file_ops nulldev_fops = {
  .Open = nulldev_open,
  .Read = nulldev_read,
  .Write = nulldev_write,
  .Close = nulldev_close
};

//_____________________________________________________________________________________________

/*
_____________________________________________________________________________________________


            ===========================================================

                             The serial device driver

            ===========================================================

_____________________________________________________________________________________________
*/
        /* forward */

   void serial_rx_handler();
   void serial_tx_handler();

typedef struct serial_device_control_block {
  uint devno;
  Mutex spinlock;
  CondVar rx_ready;
} serial_dcb_t;

serial_dcb_t serial_dcb[MAX_TERMINALS];

//_____________________________________________________________________________________________



/*
_____________________________________________________________________________________________

              ===========================================================
                  Interrupt-driven driver for serial-device reads.
              ===========================================================
*/


void serial_rx_handler()
{
  int pre = preempt_off;

  /* 
    We do not know which terminal is
    ready, so we must signal them all !
   */
  for(int i=0;i<bios_serial_ports();i++) {
    serial_dcb_t* dcb = &serial_dcb[i];
    Cond_Broadcast(&dcb->rx_ready);
  }
  if(pre) preempt_on;
}
//_____________________________________________________________________________________________

/*
               ===========================================================
                        Read from the device, sleeping if needed.
               ===========================================================

*/


int serial_read(void* dev, char *buf, unsigned int size)
{
  serial_dcb_t* dcb = (serial_dcb_t*)dev;

  preempt_off;            /* Stop preemption */

  uint count =  0;

  while(count<size) {
    int valid = bios_read_serial(dcb->devno, &buf[count]);
    
    if (valid) {
      count++;
    }
    else if(count==0) {
      kernel_wait(&dcb->rx_ready, SCHED_IO);
    }
    else
      break;
  }

  preempt_on;           /* Restart preemption */

  return count;
}
//_____________________________________________________________________________________________




/*
_____________________________________________________________________________________________

                ===========================================================
                            A polling driver for serial writes
                ===========================================================

*/



                              /* Interrupt driver */

void serial_tx_handler()
{
  /* nothing to do all day but stay in bed you are in the army now*/
}

//_____________________________________________________________________________________________




/* 
_____________________________________________________________________________________________
                
                ===========================================================
                        Write call 
                        This is currently a polling driver.
                ===========================================================

*/
int serial_write(void* dev, const char* buf, unsigned int size)
{
  serial_dcb_t* dcb = (serial_dcb_t*)dev;

  unsigned int count = 0;
  while(count < size) {
    int success = bios_write_serial(dcb->devno, buf[count] );

    if(success) {
      count++;
    } 
    else if(count==0)
    {
      yield(SCHED_IO);
    }
    else
      break;
  }

  return count;  
}
//_____________________________________________________________________________________________



//_____________________________________________________________________________________________

int serial_close(void* dev) 
{
  return 0;
}
//_____________________________________________________________________________________________



//_____________________________________________________________________________________________

void* serial_open(uint term)
{
  assert(term<bios_serial_ports());
  return & serial_dcb[term];  
}

//_____________________________________________________________________________________________



//_____________________________________________________________________________________________

file_ops serial_fops = {
  .Open = serial_open,
  .Read = serial_read,
  .Write = serial_write,
  .Close = serial_close
};

//_____________________________________________________________________________________________





//_____________________________________________________________________________________________

                   /************************************************

                                   The device table

                  *************************************************/

DCB devtable[DEV_MAX];


//_____________________________________________________________________________________________

void initialize_devices()
{

  devtable[DEV_NULL].type = DEV_NULL;
  devtable[DEV_NULL].devnum = 1;
  devtable[DEV_NULL].dev_fops = nulldev_fops;

  devtable[DEV_SERIAL].type = DEV_SERIAL;
  devtable[DEV_SERIAL].devnum = bios_serial_ports();
  devtable[DEV_SERIAL].dev_fops = serial_fops;

  /* Initialize the serial devices */
  for(int i=0; i<bios_serial_ports(); i++) {
    serial_dcb[i].devno = i;
    serial_dcb[i].rx_ready = COND_INIT;
    serial_dcb[i].spinlock = MUTEX_INIT;
  }

  cpu_interrupt_handler(SERIAL_RX_READY, serial_rx_handler);
  cpu_interrupt_handler(SERIAL_TX_READY, serial_tx_handler);
}

//_____________________________________________________________________________________________


//_____________________________________________________________________________________________

int device_open(Device_type major, uint minor, void** obj, file_ops** ops)
{
  assert(major < DEV_MAX);  
  if(minor >= devtable[major].devnum)
    return -1;
  *obj = devtable[major].dev_fops.Open(minor);
  *ops = &devtable[major].dev_fops;
  return 0;
}
//_____________________________________________________________________________________________

//_____________________________________________________________________________________________

uint device_no(Device_type major)
{
  return devtable[major].devnum;
}
//_____________________________________________________________________________________________


