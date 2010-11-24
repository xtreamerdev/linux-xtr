#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/devfs_fs_kernel.h>
#include "cec.h"
#include "cec_dev.h"

#define MAX_CEC_CNT             4
static cec_dev          node_list[MAX_CEC_CNT];
static dev_t            devno_base;



/*------------------------------------------------------------------
 * Func : create_cec_dev_node
 *
 * Desc : create a cec device node
 *
 * Parm : device : handle of cec device
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int create_cec_dev_node(cec_device* device)
{               
    char name[10];                 
    int i;    
    for (i=0; i<MAX_CEC_CNT; i++)
    {
        if (node_list[i].device==NULL)
        {                                  
	        if (cdev_add(&node_list[i].cdev, devno_base + i, 1)<0)
	        {		    
	            cec_warning("cec warning : register character dev failed\n");    
	            return -1;
            }
                        
            sprintf(name, "cec/%d", i);
            devfs_mk_cdev(devno_base + i, S_IFCHR|S_IRUSR|S_IWUSR, name);                    
            node_list[i].device = device;                        
            
            return 0;
        }
    }
    
    return -1;
}



/*------------------------------------------------------------------
 * Func : remove_cec_dev_node
 *
 * Desc : remove a cec device node
 *
 * Parm : device : cec device to be unregistered
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
void remove_cec_dev_node(cec_device* device)
{   
    int i = 0;
    char name[10];    
    
    for (i=0; i<MAX_CEC_CNT; i++)
    {
        if (node_list[i].device==device)
        {
            sprintf(name, "cec/%d", i);
            devfs_remove(name); 
            cdev_del(&node_list[i].cdev);
            node_list[i].device = NULL;
            return;
        }
    }
}



/*------------------------------------------------------------------
 * Func : cec_dev_open
 *
 * Desc : open function of cec dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int cec_dev_open(struct inode *inode, struct file *file)
{
    unsigned int i = iminor(inode);
	return (node_list[i].device) ? 0 : -ENODEV;	  	
}



/*------------------------------------------------------------------
 * Func : cec_dev_release
 *
 * Desc : release function of cec dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int cec_dev_release(
    struct inode*           inode, 
    struct file*            file
    )
{    
    unsigned int i  = iminor(inode);
    cec_device* dev = node_list[i].device;
    
    if (dev)
    {
        cec_driver* drv = (cec_driver*) to_cec_driver(dev->dev.driver);                
        drv->enable(dev, 0);
    }
                
	return 0;
}




/*------------------------------------------------------------------
 * Func : cec_dev_ioctl
 *
 * Desc : ioctl function of cec dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *        cmd   : control command
 *        arg   : arguments
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int cec_dev_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    unsigned long           arg
    )
{    
    unsigned int i  = iminor(inode);            
    cec_device* dev = node_list[i].device;    
    cec_driver* drv;    
    cec_msg  msg;    
    cm_buff* cmb = NULL;    
    int      len;                        		
    
    if (!dev)
        return -ENODEV;
        
    drv = (cec_driver*) to_cec_driver(dev->dev.driver);            
    
    switch(cmd)
    {        
    case CEC_ENABLE:            
        return drv->enable(dev, (arg) ? 1 : 0);                
        
    case CEC_SET_LOGICAL_ADDRESS:                       
        return drv->set_logical_addr(dev, (unsigned char) arg);
                    
            
    case CEC_SEND_MESSAGE:                 
        
        if (copy_from_user(&msg, (cec_msg __user *)arg, sizeof(cec_msg)))              
			return -EFAULT;               
			                
        if ((cmb = alloc_cmb(msg.len))==NULL)
            return -ENOMEM;
            
        if (copy_from_user(cmb_put(cmb, msg.len), (unsigned char __user *)msg.buf, msg.len))
            return -EFAULT;                    
                    
        return drv->xmit(dev, cmb, 0);      // BLOCK I/O        
        
    case CEC_RCV_MESSAGE:                               
        
        if (copy_from_user(&msg, (cec_msg __user *)arg, sizeof(cec_msg)))              
			return -EFAULT;     
			
        cmb = drv->read(dev, 0);       // BLOCK I/O        

        if (cmb)
        {
            if (cmb->len > msg.len) 
            {
                cec_warning("cec : read message failed, msg size (%d) more than msg buffer size(%d)\n", cmb->len, msg.len);                            
                kfree_cmb(cmb);
                return -ENOMEM;
            }
            
            len = cmb->len;                                    
            
            if (copy_to_user((unsigned char __user *) msg.buf, cmb->data, cmb->len)<0)
            {
                cec_warning("cec : read message failed, copy msg to user failed\n");
                kfree_cmb(cmb);                
                return -EFAULT;
            }
            
            kfree_cmb(cmb);                        
            return len;
        }
        
        return -EFAULT;                    
             
    default:    
        cec_warning("cec : unknown ioctl cmd %08x\n", cmd);        
        return -EFAULT;          
    }        
}



static struct file_operations cec_dev_fops = 
{
	.owner		= THIS_MODULE,	
	.ioctl		= cec_dev_ioctl,
	.open		= cec_dev_open,
	.release	= cec_dev_release,
};





/*------------------------------------------------------------------
 * Func : cec_dev_module_init
 *
 * Desc : cec dev init function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static int __init cec_dev_module_init(void)
{        
    int i;
    
    if (alloc_chrdev_region(&devno_base, 0, MAX_CEC_CNT, "cec")!=0)    
        return -EFAULT;

	devfs_mk_dir("cec");	  
		   
    for (i=0; i<MAX_CEC_CNT; i++) 
    {   
        cdev_init(&node_list[i].cdev, &cec_dev_fops);        
        node_list[i].device = NULL;        
    }
		
    return 0;
}



/*------------------------------------------------------------------
 * Func : cec_dev_module_exit
 *
 * Desc : cec dev module exit function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static void __exit cec_dev_module_exit(void)
{   
    int i = 0;            
    
    for (i=0; i<MAX_CEC_CNT; i++)    
    {
        if (node_list[i].device)
            remove_cec_dev_node(node_list[i].device);
    }
    
    devfs_remove("cec");
    unregister_chrdev_region(devno_base, MAX_CEC_CNT);
}



module_init(cec_dev_module_init);
module_exit(cec_dev_module_exit);
