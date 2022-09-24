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

static int pivot_probe(struct platform_device *pdev);
static int pivot_remove(struct platform_device *pdev);
static int pivot_open(struct inode *i, struct file *f);
static int pivot_close(struct inode *i, struct file *f);
static ssize_t pivot_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t pivot_write(struct file *f, const char __user *buf, size_t length, loff_t *off);
static ssize_t pivot_mmap(struct file *f, struct vm_area_struct *vma_s);
static int __init pivot_init(void);
static void __exit pivot_exit(void);

struct device_info
{
  unsigned long mem_start;
  unsigned long mem_end;
  void __iomem *base_addr;
};

static struct cdev *my_cdev;
static dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct device_info *pivot = NULL; //naziv
static struct device_info *bram  = NULL;//naziv
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
	.mmap = pivot_mmap
};

static struct of_device_id device_of_match[] = {
	{ .compatible = "xlnx,pivot", }, //proveri naziv u device tree
	{ .compatible = "xlnx,bram"}, //proveri naziv u device tree
	{ /* end of list */ },
};

MODULE_DEVICE_TABLE(of, device_of_match); //proveri naziv

static struct platform_driver pivot_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= device_of_match,
	},
	.probe		= pivot_probe,
	.remove	= pivot_remove,
};

int device_fsm = 0;

static int pivot_probe(struct platform_device *pdev)
{
  struct resource *r_mem;
  int rc = 0;
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem) {
		printk(KERN_ALERT "invalid address\n");
		return -ENODEV;
	}
  printk(KERN_INFO "Starting probing.\n");

  switch (device_fsm)
    {
    case 0: // device pivot
      pivot = (struct device_info *) kmalloc(sizeof(struct device_info), GFP_KERNEL);
      if (!pivot)
        {
          printk(KERN_ALERT "Cound not allocate fft2 device\n");
          return -ENOMEM;
        }
      pivot->mem_start = r_mem->start;
      pivot->mem_end   = r_mem->end;
      if(!request_mem_region(pivot->mem_start, pivot->mem_end - pivot->mem_start+1, DRIVER_NAME))
        {
          printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)pivot->mem_start);
          rc = -EBUSY;
          goto error1;
        }
      pivot->base_addr = ioremap(pivot->mem_start, pivot->mem_end - pivot->mem_start + 1);
      if (!pivot->base_addr)
        {
          printk(KERN_ALERT "[PROBE]: Could not allocate pivot iomem\n");
          rc = -EIO;
          goto error2;
        }
      ++device_fsm;
      printk(KERN_INFO "[PROBE]: Finished probing pivot.\n");
      return 0;
      error2:
        release_mem_region(pivot->mem_start, pivot->mem_end - pivot->mem_start + 1);
		kfree(pivot);
      error1:
        return rc;
      break;

    case 1: // device bram
      bram = (struct device_info *) kmalloc(sizeof(struct device_info), GFP_KERNEL);
      if (!bram)
        {
          printk(KERN_ALERT "Cound not allocate bram device\n");
          return -ENOMEM;
        }
      bram->mem_start = r_mem->start;
      bram->mem_end   = r_mem->end;
      if(!request_mem_region(bram->mem_start, bram->mem_end - bram->mem_start+1, DRIVER_NAME))
        {
          printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)bram->mem_start);
          rc = -EBUSY;
          goto error5;
        }
      bram->base_addr = ioremap(bram->mem_start, bram->mem_end - bram->mem_start + 1);
      if (!bram->base_addr)
        {
          printk(KERN_ALERT "[PROBE]: Could not allocate bram iomem\n");
          rc = -EIO;
          goto error6;
        }
      printk(KERN_INFO "[PROBE]: Finished probing bram.\n");
      return 0;
      error6:
        release_mem_region(bram->mem_start, bram->mem_end - bram->mem_start + 1);
		kfree(bram);
      error5:
        return rc;
      break;

    default:
      printk(KERN_INFO "[PROBE] Device FSM in illegal state. \n");
      return -1;
    }
  printk(KERN_INFO "Succesfully probed driver\n");
  return 0;
}

static int pivot_remove(struct platform_device *pdev)
{
  switch (device_fsm)
    {
    case 0: //device pivot
      printk(KERN_ALERT "pivot device platform driver removed\n");
      iowrite32(0, pivot->base_addr); //0 ili reset 
      iounmap(pivot->base_addr);
      release_mem_region(pivot->mem_start, pivot->mem_end - pivot->mem_start + 1);
      kfree(pivot);
      break;

    case 1: //device bram
      printk(KERN_ALERT "bram platform driver removed\n");
      iowrite32(0, bram->base_addr);
      iounmap(bram->base_addr);
      release_mem_region(bram->mem_start, bram->mem_end - bram->mem_start + 1);
      kfree(bram);
      --device_fsm;
      break;

    default:
      printk(KERN_INFO "[REMOVE] Device FSM in illegal state. \n");
      return -1;
    }
  printk(KERN_INFO "Succesfully removed driver\n");
  return 0;
}

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
int p=0;
int end_read=0;
 
static ssize_t pivot_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	char buff[length+1];
	long int len=0;
	u32 start_reg=0;
	u32 ready_reg=0;
	//u32 counter_reg=0;
	u32 bram_val=0;
	int minor = MINOR(f->f_inode->i_rdev);
	
	printk(KERN_INFO "addr=%d,len=%ld,end_read=%d\n",p,len,end_read);
	if(end_read==1)
	{
		end_read=0;
		printk(KERN_INFO "Reading complete\n");
		return 0;
	}
	
	switch(minor)
	{
		case 0://device pivot
		printk(KERN_INFO "Reading from pivot device\n");
		if(down_interruptible(&sem_ip))
			return -ERESTARTSYS;
		start_reg=ioread32(pivot->base_addr);
		ready_reg=ioread32(pivot->base_addr+4);
		if(ready_reg==1)
			wake_up_interruptible(&readyQ);
		//counter_reg=ioread32(pivot->base_addr+8);
		len=scnprintf(buf,BUFF_SIZE,"Start=%d,Ready=%d\n",start_reg,ready_reg);
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
		while(ioread32(pivot->base_addr+4)==0)
		{
			up(&sem_ip);
			if(wait_event_interruptible(readyQ,(ioread32(pivot->base_addr+4)==1)))
				return -ERESTARTSYS;
			if(down_interruptible(&sem_ip))
				return -ERESTARTSYS;
		}
		if(ioread32(pivot->base_addr+4)==1)
		{
			printk(KERN_INFO "Reading from bram device\n");
			bram_val=ioread32(bram->base_addr+p*4);
			if(p<ROWSIZE*COLSIZE+1)
			{
				len=scnprintf(buf,BUFF_SIZE,"%u",bram_val);
			}
			*offset+=len;
			if(copy_to_user(buffer,buff,len))
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

static ssize_t pivot_write(struct file *f, const char __user *buf, size_t length, loff_t *off)
{
	char buf[length+1];
	int minor = MINOR(f->f_inode->i_rdev);
	unsigned int pos=0,reg_val=0,bram_val=0;
	if(copy_from_user(buf,buffer,length))
		return -EFAULT;
	buf[length]='\0';
	switch(minor)
	{
		case 0: //device pivot
		if(down_interruptible(&sem_ip))
			return -ERESTARTSYS;
		sscanf(buf,"%u",&reg_val);
		if(start_val==1)
		{
			while(ioread32(pivot->base_addr+4)==0)
			{
				up(&sem_ip);
				if(wait_event_interruptible(readyQ,(ioread32(pivot->base_addr+4)==1)))
					return -ERESTARTSYS;
				if(down_interruptible(&sem_ip))
					return -ERESTARTSYS;
			}
			if(ioread32(pivot->base_addr+4)==1)
			{
				iowrite32(reg_val,pivot->base_addr+0);	
			}
		}
		else
			iowrite32(reg_val,pivot->base_addr+0);
		up(&sem_ip);
		printk(KERN_INFO "Wrote succesfully to start register value %u\n",reg_val);		
		break;
		
		case 1:
		if(down_interruptible(&sem_ip))
			return -ERESTARTSYS;
		while(ioread32(pivot->base_addr+4)==0)
		{
			up(&sem_ip);
			if(wait_event_interruptible(readyQ,(ioread32(pivot->base_addr+4)==1)))
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
			iowrite32(bram_val,bram->base_addr+(pos*4));
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
static ssize_t pivot_mmap(struct file *f, struct vm_area_struct *vma_s)
{
	
}

static int __init pivot_init(void)
{
   printk(KERN_INFO "\n");
   printk(KERN_INFO "PIVOT driver starting insmod.\n");

	sema_init(&sem_bram,1);
	sema_init(&sem_ip,1);
	
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

   return platform_driver_register(&pivot_driver);

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
	platform_driver_unregister(&pivot_driver);
	cdev_del(my_cdev);
  device_destroy(my_class, MKDEV(MAJOR(my_dev_id),1));
  device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
  class_destroy(my_class);
  unregister_chrdev_region(my_dev_id,1);
  printk(KERN_INFO "FFT2 driver exited.\n");
}


module_init(pivot_init);
module_exit(pivot_exit);