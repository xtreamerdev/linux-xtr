#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/devfs_fs_kernel.h>
#include "scd.h"
#include "scd_dev.h"


#define MAX_SCD_CNT     2
static scd_dev          node_list[MAX_SCD_CNT];
static dev_t            devno_base;


/*------------------------------------------------------------------
 * Func : create_scd_dev_node
 *
 * Desc : create a scd device node
 *
 * Parm : device : handle of scd device
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int create_scd_dev_node(scd_device* device)
{               
    char name[10];                 
    int i;    
    for (i=0; i<MAX_SCD_CNT; i++)
    {
        if (node_list[i].device==NULL)
        {                                  
	        if (cdev_add(&node_list[i].cdev, devno_base + i, 1)<0)
	        {		    
	            SC_WARNING("scd warning : register character dev failed\n");    
	            return -1;
            }
                        
            sprintf(name, "smartcard/%d", i);
            devfs_mk_cdev(devno_base + i, S_IFCHR|S_IRUSR|S_IWUSR, name);                    
            node_list[i].device = device;                        
            
            return 0;
        }
    }
    
    return -1;
}



/*------------------------------------------------------------------
 * Func : remove_scd_dev_node
 *
 * Desc : remove a scd device node
 *
 * Parm : device : scd device to be unregistered
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
void remove_scd_dev_node(scd_device* device)
{   
    int i = 0;
    char name[10];    
    
    for (i=0; i<MAX_SCD_CNT; i++)
    {
        if (node_list[i].device==device)
        {
            sprintf(name, "smartcard/%d", i);
            devfs_remove(name); 
            cdev_del(&node_list[i].cdev);
            node_list[i].device = NULL;
            return;
        }
    }
}



/*------------------------------------------------------------------
 * Func : scd_dev_open
 *
 * Desc : open function of scd dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int scd_dev_open(struct inode *inode, struct file *file)
{
    unsigned int i = iminor(inode);
    scd_device*  dev = node_list[i].device;
    
    if (dev)
    {
        scd_driver* drv = (scd_driver*) to_scd_driver(dev->dev.driver);                
        drv->enable(dev, 1);
        return 0;
    }
    
	return -ENODEV;	  	
}



/*------------------------------------------------------------------
 * Func : scd_dev_release
 *
 * Desc : release function of scd dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int scd_dev_release(
    struct inode*           inode, 
    struct file*            file
    )
{    
    unsigned int i  = iminor(inode);
    scd_device* dev = node_list[i].device;
    
    if (dev)
    {
        scd_driver* drv = (scd_driver*) to_scd_driver(dev->dev.driver);                
        drv->enable(dev, 0);
    }
                
	return 0;
}




/*------------------------------------------------------------------
 * Func : scd_dev_ioctl
 *
 * Desc : ioctl function of scd dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *        cmd   : control command
 *        arg   : arguments
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int scd_dev_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    unsigned long           arg
    )
{    
    unsigned int i  = iminor(inode);            
    scd_device* dev = node_list[i].device;    
    scd_driver* drv;    
    scd_atr     atr;
    scd_atr*    p_atr;            
    sc_msg_buff buff;            
    sc_buff*    scb;    
    int         len;
    scd_param   param;
    
    if (!dev)
        return -ENODEV;
        
    drv = (scd_driver*) to_scd_driver(dev->dev.driver);            
        
	switch (cmd)
    {
    case SCD_SETPARAM:                
        
        if (copy_from_user(&param, (scd_param __user *)arg, sizeof(scd_param)))
			return -EFAULT;
			
        return drv->set_param(dev, &param);
    
    case SCD_GETPARAM:                
        
        if (drv->get_param(dev, &param)<0)
            return -EFAULT;
        
        return copy_to_user((scd_param __user *)arg, &param, sizeof(scd_param));			
                
	case SCD_ACTIVE:                    return drv->activate(dev);
	case SCD_DEACTIVE:                  return drv->deactivate(dev);	        
	case SCD_RESET:                     return drv->reset(dev);     	          
	case SCD_CARD_DETECT:               return drv->get_card_status(dev);
    case SCD_POLL_CARD_STATUS_CHANGE:   return drv->poll_card_status(dev);
    case SCD_GET_ATR:
        
        if (drv->get_atr(dev, &atr))    
            return -1;        
                
        p_atr = (scd_atr *)arg;                    				    			          
        	
        put_user(atr.length, &p_atr->length);
            	
        for(i=0; i < atr.length; i++)        
            put_user(atr.data[i], &p_atr->data[i]);        
            
        return 0;

    /*
	case SCD_PARSE_ATR:
		Scd_ParseATR(cardDev);
		break;
	
		
	case SCD_SET_PARAM_USING_ATR:
		scd_info("SCD_SET_PARAM_USING_ATR\n");
		Scd_SetParametersUsingATR(cardDev);
		break;
		
    */
    		
	case SCD_READ:			
	
	    if (copy_from_user(&buff, (sc_msg_buff __user *)arg, sizeof(sc_msg_buff)))
			return -EFAULT;

		scb = drv->read(dev);        
        
		if (scb)
        {   
            if (scb->len > buff.length) 
            {
                SC_WARNING("read message failed, msg size (%d) more than msg buffer size(%d)\n", scb->len, buff.length);
                kfree_scb(scb);
                return -ENOMEM;
            }            
            
            len = scb->len;                        
            
            if (copy_to_user((unsigned char __user *) buff.p_data, scb->data, scb->len)<0)
            {
                kfree_scb(scb);
                return -EFAULT;
            }
            
            kfree_scb(scb);
            return len;
        }
	   
        return -EFAULT;       			                                            
    
	case SCD_WRITE:							    
		
		if (copy_from_user(&buff, (sc_msg_buff __user *)arg, sizeof(sc_msg_buff)))
			return -EFAULT;
			                
        if ((scb = alloc_scb(buff.length))==NULL)
            return -ENOMEM;
            
        if (copy_from_user(scb_put(scb, buff.length), (unsigned char __user *)buff.p_data, buff.length))
            return -EFAULT;
                    
        return drv->xmit(dev, scb);
            
	default:		
		return -EFAULT;          
	}
	
    return 0;          
}



static struct file_operations scd_dev_fops = 
{
	.owner		= THIS_MODULE,	
	.ioctl		= scd_dev_ioctl,
	.open		= scd_dev_open,
	.release	= scd_dev_release,
};



/*------------------------------------------------------------------
 * Func : scd_dev_module_init
 *
 * Desc : scd dev init function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int __init scd_dev_module_init(void)
{        
    int i;
    
    if (alloc_chrdev_region(&devno_base, 0, MAX_SCD_CNT, "smartcard")!=0)    
        return -EFAULT;

	devfs_mk_dir("smartcard");	  
		   
    for (i=0; i<MAX_SCD_CNT; i++) 
    {   
        cdev_init(&node_list[i].cdev, &scd_dev_fops);        
        node_list[i].device = NULL;        
    }
		
    return 0;
}



/*------------------------------------------------------------------
 * Func : scd_dev_module_exit
 *
 * Desc : scd dev module exit function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
void __exit scd_dev_module_exit(void)
{   
    int i = 0;            
    
    for (i=0; i<MAX_SCD_CNT; i++)    
    {
        if (node_list[i].device)
            remove_scd_dev_node(node_list[i].device);
    }
    
    devfs_remove("smartcard");
    unregister_chrdev_region(devno_base, MAX_SCD_CNT);
}
