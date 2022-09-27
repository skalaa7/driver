#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/dma-mapping.h>  //dma access
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/semaphore.h>
#include <linux/wait.h>

MODULE_AUTHOR ("Embedjedi");
MODULE_DESCRIPTION("Test Driver for PIVOT IP.");
MODULE_LICENSE("Dual BSD/GPL");
//MODULE_ALIAS("custom:vga_dma controller");

#define DRIVER_NAME "PIVOT" //naziv
#define BUFF_SIZE 100
#define NUMOFVAR 50
#define NUMOFSLACK 50
#define ROWSIZE (NUMOFSLACK+1)
#define COLSIZE (NUMOFSLACK+NUMOFVAR+1)
#define START_REG (0x00)
#define READY_REG (0x04)
#define COUNTER_REG (0x08)


static int pivot_open(struct inode *i, struct file *f);
static int pivot_close(struct inode *i, struct file *f);
static ssize_t pivot_read(struct file *f, char __user *buffer, size_t len, loff_t *off);
static ssize_t pivot_write(struct file *f, const char __user *buffer, size_t length, loff_t *off);
//static ssize_t pivot_mmap(struct file *f, struct vm_area_struct *vma_s);
static int __init pivot_init(void);
static void __exit pivot_exit(void);
void do_pivoting();

static struct cdev *my_cdev;
static dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
struct semaphore sem_ip;
struct semaphore sem_bram;
DECLARE_WAIT_QUEUE_HEAD(readyQ);
static struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = pivot_open,
	.release = pivot_close,
	.read = pivot_read,
	.write = pivot_write,
	//.mmap = pivot_mmap
};

static int pivot_open(struct inode *i, struct file *f)
{
		printk(KERN_INFO "Succesfully opened file\n");
		return 0;
}

static int pivot_close(struct inode *i, struct file *f)
{
		printk(KERN_INFO "Succesfully closed file\n");
		return 0;
}
//READ AND WRITE
int end_read=0;
unsigned int bram[ROWSIZE*COLSIZE+1];
int start=0,ready=1;
int p=0;
 
static ssize_t pivot_read(struct file *f, char __user *buffer, size_t len, loff_t *off)
{
	char buf[BUFF_SIZE];
	long int len=0;
	//u32 counter_reg=0;
	u32 bram_val=0;
	
	int minor = MINOR(f->f_inode->i_rdev);
	
	printk(KERN_INFO "i=%d,len=%ld,end_read=%d\n",i,len,end_read);
	if(end_read==1)
	{
		end_read=0;
		printk(KERN_INFO "Reading complete\n");
		return 0;
	}
	
	switch(minor)
	{
		case 0://device pivot
		if(down_interruptible(&sem_ip))
			return -ERESTARTSYS;
		printk(KERN_INFO "Reading from pivot device\n");
		if(ready==1)
			wake_up_interruptible(&readyQ);
		len=scnprintf(buf,BUFF_SIZE,"Start=%d,Ready=%d\n",start,ready);
		if(copy_to_user(buffer,buf,len)
		{
			printk(KERN_ERR "Copy to user does not work\n");
			return -EFAULT;
		}
		up(&sem_ip);
		end_read=1;
		break;
		
		case 1://device bram
		if(down_interruptible(&sem_ip))
			return -ERESTARTSYS;
		while(ready==0)
		{
			up(&sem_ip);
			if(wait_event_interruptible(readyQ,(ready==1)))
				return -ERESTARTSYS;
			if(down_interruptible(&sem_ip))
				return -ERESTARTSYS;
		}
		if(ready==1)
		{
			up(&sem_ip);
			if(down_interruptible(&sem_bram))
				return -ERESTARTSYS;
			printk(KERN_INFO "Reading from bram device\n");
			bram_val=bram[p];
			if(p<ROWSIZE*COLSIZE+1)
			{
			len=scnprintf(buf,BUFF_SIZE,"%u",bram_val);
			}
			*offset+=len;
			if(copy_to_user(buffer,buf,len))
			{
				printk(KERN_ERR "Copy to user does not work\n");
				return -EFAULT;
			}
			p++;
			if(p==ROWSIZE*COLSIZE+1)
			{
				printk(KERN_INFO "Succesfully read from bram\n");
				p=0;
				end_read=1;
			}
			up(&sem_bram);
		}
		else
			up(&sem_ip);
		break;
		
		default:
		printk(KERN_ERR "Invalid minor\n");
		end_read=1;
	}
	return len;	
}

static ssize_t pivot_write(struct file *f, const char __user *buffer, size_t length, loff_t *off)
{
	char buf[length+1];
	int minor = MINOR(f->f_inode->i_rdev);
	unsigned int pos=0,rbram_val=0,start_val=0;
	if(copy_from_user(buf,buffer,length))
		return -EFAULT;
	buf[length]='\0';
	switch(minor)
	{
		case 0: //device pivot
		if(down_interruptible(&sem_ip))
			return -ERESTARTSYS;
		sscanf(buf,"%u",&start_val);
		if(start_val==1)
		{
			while(ready==0)
			{
			up(&sem_ip);
			if(wait_event_interruptible(readyQ,(ready==1)))
				return -ERESTARTSYS;
			if(down_interruptible(&sem_ip))
				return -ERESTARTSYS;
			}
			if(ready==1)
			{
				start=start_val;
				do_pivoting();
				wake_up_interruptible(&readyQ);
			}
		}
		else
			start=start_val;
		up(&sem_ip);
		printk(KERN_INFO "Wrote succesfully to start register value %u\n",start_val);
		break;
		case 1:
		if(down_interruptible(&sem_ip))
			return -ERESTARTSYS;
		while(ready==0)
		{
			up(&sem_ip);
			if(wait_event_interruptible(readyQ,(ready==1)))
				return -ERESTARTSYS;
			if(down_interruptible(&sem_ip))
				return -ERESTARTSYS;
		}
		if(ready==1)
		{
			up(&sem_ip);
			if(down_interruptible(&sem_bram))
				return -ERESTARTSYS;
			sscanf(buf,"%d,%u",&pos,&bram_val);
			bram[pos]=bram_val;
			printk(KERN_INFO "Wrote succesfully into bram. pos = %d, val = %d\n", pos,bram_val);
			up(&sem_bram);
		}
		else
			up(&sem_ip);
		break;
		default:
		printk(KERN_ERR "Invalid minor\n");
		
	}
	
	
}
//////////////
void do_pivoting()
{
	ready=0;
	unsigned int newRow[COLSIZE];
	unsigned int pivotColVal[ROWSIZE];
	unsigned int pivot;
	int pivotRow=0;
	unsigned int pivotCol;
	unsigned int temp1,temp2;
	int i=0;
	int j=0;
	pivotRow=0;
	pivotCol=bram[ROWSIZE*COLSIZE];
	pivot=bram[pivotRow*COLSIZE+pivotCol];
	for(i=0;i<COLSIZE;i++)
	{
		newRow[i] = bram[pivotRow*COLSIZE+i]/pivot;//kako float
		bram[pivotRow*COLSIZE+i]=newRow[i];
	}
	for(j=1;j<ROWSIZE;j++)
	{
		pivotColVal[j] = bram[j*COLSIZE+pivotCol];
	}
	for(j=1;j<ROWSIZE-1;j=j+2)
	{
		for(i=0;i<COLSIZE;i++)
		{
			temp1 = bram[j*COLSIZE+i]-newRow[i]*pivotColVal[j];
			bram[j*COLSIZE+i]=temp1;
			temp2 = bram[(j+1)*COLSIZE+i]-newRow[i]*pivotColVal[j+1];
			bram[(j+1)*COLSIZE+i]=temp2;
		}
	}
	ready=1;
}

static int __init pivot_init(void)
{
   printk(KERN_INFO "\n");
   printk(KERN_INFO "PIVOT driver starting insmod.\n");
	
	sema_init(&sem_bram,1);
	sema_init(&sem_ip,1);
	
	for(i=0;i<ROWSIZE*COLSIZE+1;i++)//Initialize bram
		bram[i]=0;
		
   if (alloc_chrdev_region(&my_dev_id, 0, 2, "pivot_region") < 0){
      printk(KERN_ERR "failed to register char device\n");
      return -1;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "pivot_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");

   if (device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),0), NULL, "xlnx,pivot") == NULL) {
      printk(KERN_ERR "failed to create device pivot\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created - pivot\n");

   if (device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),1), NULL, "xlnx,bram") == NULL) {
     printk(KERN_ERR "failed to create device bram\n");
     goto fail_2;
   }
   printk(KERN_INFO "device created - bram\n");

	my_cdev = cdev_alloc();
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;

	if (cdev_add(my_cdev, my_dev_id, 2) == -1)
	{
      printk(KERN_ERR "failed to add cdev\n");
      goto fail_3;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "PIVOT driver initialized.\n");

   //return platform_driver_register(&pivot_driver);

    fail_3:
     device_destroy(my_class, MKDEV(MAJOR(my_dev_id),1));
	  fail_2:
     device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

static void __exit pivot_exit(void)
{
  printk(KERN_INFO "PIVOT driver starting rmmod.\n");
	//platform_driver_unregister(&pivot_driver);
	cdev_del(my_cdev);
  device_destroy(my_class, MKDEV(MAJOR(my_dev_id),1));
  device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
  class_destroy(my_class);
  unregister_chrdev_region(my_dev_id,1);
  printk(KERN_INFO "FFT2 driver exited.\n");
}


module_init(pivot_init);
module_exit(pivot_exit);