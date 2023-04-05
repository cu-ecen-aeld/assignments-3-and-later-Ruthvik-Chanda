/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 * Reference: 1) https://github.com/cu-ecen-aeld/ldd3/blob/master/scull/main.c 
 * 	      2) Linux Device Drive Edition Chapter 3
 	      3)https://man7.org/linux/man-pages/man3/daemon.3.html
              4)CU-ECEN-AESD Github Repositories
              5) Coursera PPT Slides
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>   // file_operations
#include "aesdchar.h"
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>  
#include "aesd_ioctl.h"

int aesd_major = 0; // use dynamic major
int aesd_minor = 0;

MODULE_AUTHOR("Ruthvik R Chanda");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    filp->private_data = NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    int value = 0;
    unsigned long c = 0;
    int bytes_read = 0;
    size_t offset;
    struct aesd_buffer_entry *temp;
    struct aesd_dev *dev = filp->private_data;;
    
    value = mutex_lock_interruptible(&dev->lock);
    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    if (value!=0)
    {
	PDEBUG(KERN_ERR "Mutex was not acquired\n");
	goto handle_error;
    }

    temp = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &offset);

    if(temp==NULL)
        goto handle_error;

    if ((temp->size - offset) < count) 
    {
        *f_pos = *f_pos + (temp->size - offset);
        bytes_read = temp->size - offset;
    } 
    else 
    {
        *f_pos = *f_pos + count;
        bytes_read = count;
    }

    c = copy_to_user(buf, temp->buffptr+offset, bytes_read);
    if (c) 
    {
	retval = -EFAULT;
	goto handle_error;
    }

    retval = bytes_read;

    handle_error:
            mutex_unlock(&dev->lock);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0; 
    struct aesd_buffer_entry write_buffer;
    struct aesd_dev *dev = filp->private_data;
    int i, send_packet = 0; 
    int store = 0, temp_size = 0;
    char *temp_buff;
    int value = 0;
    const char *newbuffer;

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    value = mutex_lock_interruptible(&dev->lock);
    if (value !=0)
	{
		PDEBUG(KERN_ERR "Couldn't acquire Mutex\n");
		return -EFAULT;
	}

    temp_buff = (char *)kmalloc(count, GFP_KERNEL);
    if (temp_buff == NULL)
    {
        retval = -ENOMEM;
        goto error_handler;
    }
    
    if (copy_from_user(temp_buff, buf, count)) 
    {
        retval = -EFAULT;
		goto error_handler;
    }

    for (i = 0; i < count; i++) 
    {
        if (temp_buff[i] == '\n') 
        {
            send_packet = 1; 
            store = i+1; 
            break;
        }
    }

    if (dev->buffer_length == 0) 
    {
        dev->store_buffer = (char *)kmalloc(count, GFP_KERNEL);
        if (dev->store_buffer == NULL) 
        {
            retval = -ENOMEM;
            goto free_memory;
        }
        memcpy(dev->store_buffer, temp_buff, count);
        dev->buffer_length += count;
    } 
    else 
    {
        if (send_packet)
            temp_size = store;
        else
            temp_size = count;

        dev->store_buffer = (char *)krealloc(dev->store_buffer, dev->buffer_length + temp_size, GFP_KERNEL);
        if (dev->store_buffer == NULL) 
        {
            retval = -ENOMEM;
            goto free_memory;
        }
      
        memcpy(dev->store_buffer + dev->buffer_length, temp_buff, temp_size);
        dev->buffer_length += temp_size;        
    }
 
    if (send_packet) 
    {
        write_buffer.buffptr = dev->store_buffer;
        write_buffer.size = dev->buffer_length;
        newbuffer = aesd_circular_buffer_add_entry(&dev->circular_buffer, &write_buffer);
    
        if (newbuffer != NULL)
            kfree(newbuffer);
        
        dev->buffer_length = 0;
    } 

    retval = count;

    free_memory: 
            kfree(temp_buff);
    error_handler: 
            mutex_unlock(&dev->lock);
  
    return retval;
}

static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd, unsigned int write_cmd_offset)
{
    int i;
    long return_value = 0;
    struct aesd_dev *dev= NULL;
    dev=filp->private_data;

    if (mutex_lock_interruptible(&dev->lock)!=0)
    {
	PDEBUG(KERN_ERR "Mutex was not Acquired\n");
	return -EFAULT;
    }
    
    if ((write_cmd>=AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)||(write_cmd_offset>=dev->circular_buffer.entry[write_cmd].size)|| write_cmd>i)
    {
        PDEBUG(KERN_ERR "Invalid Offset\n");
        return_value = -EINVAL;
    }
    else
    {
        for (i=0;i<write_cmd;i++)
        {
            filp->f_pos=filp->f_pos + dev->circular_buffer.entry[i].size;
        }
        filp->f_pos=filp->f_pos+write_cmd_offset;
    }
    mutex_unlock(&dev->lock);
    return return_value;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_seekto seekto;
    long return_value = 0;
    
    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC)
    { 
    	return -ENOTTY;
    }
    if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR)
    { 
    	return -ENOTTY;
    }
    
    switch(cmd)
    {
        case AESDCHAR_IOCSEEKTO:
            if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)) != 0) 
            {
	     	return_value = -EFAULT;
            } 
            else 
            {
            	return_value = aesd_adjust_file_offset(filp,seekto.write_cmd,seekto.write_cmd_offset);
            }
	    break;

    default:
    	return -ENOTTY;
    	break;
    }
    
    return return_value;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t pos;
    int i;
    loff_t size = 0;
    struct aesd_dev *dev= NULL;
    dev = filp->private_data;
    struct aesd_buffer_entry *buf_entry = NULL;

    if (mutex_lock_interruptible(&dev->lock)!=0)
    {
	PDEBUG(KERN_ERR "Mutex was not Acquired\n");
	return -ERESTARTSYS;
    }

    AESD_CIRCULAR_BUFFER_FOREACH(buf_entry, &dev->circular_buffer, i)
    {
        size = size + buf_entry->size;
    }
    pos = fixed_size_llseek(filp, off, whence, size);
    mutex_unlock(&dev->lock);
    return pos;
}

struct file_operations aesd_fops = {
    .owner = THIS_MODULE,
    .read = aesd_read,
    .write = aesd_write,
    .open = aesd_open,
    .release = aesd_release,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_ioctl
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0)
    {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));

    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if (result)
    {
        unregister_chrdev_region(dev, 1);
    }
    return result;
}

void aesd_cleanup_module(void)
{
    int count = 0;
    struct aesd_buffer_entry *buffer_element;
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    cdev_del(&aesd_device.cdev);

    AESD_CIRCULAR_BUFFER_FOREACH(buffer_element, &aesd_device.circular_buffer, count)
    {
        if (buffer_element->buffptr != NULL)
        {
            kfree(buffer_element->buffptr);
            buffer_element->size = 0;
        }
    }

    mutex_destroy(&aesd_device.lock);
    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
