/*****************************************************************************/

/*
 *	ezusb.c  --  Firmware download miscdevice for Anchorchips EZUSB microcontrollers.
 *
 *	Copyright (C) 1999
 *          Thomas Sailer (sailer@ife.ee.ethz.ch)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  History:
 *   0.1  26.05.99  Created
 *   0.2  23.07.99  Removed EZUSB_INTERRUPT. Interrupt pipes may be polled with
 *                  bulk reads.
 *                  Implemented EZUSB_SETINTERFACE, more sanity checks for EZUSB_BULK.
 *                  Preliminary ISO support
 *   0.3  01.09.99  Async Bulk and ISO support
 *   0.4  01.09.99  Set callback_frames to the total number of frames to make
 *                  it work with OHCI-HCD
 *        03.12.99  Now that USB error codes are negative, return them to application
 *                  instead of ENXIO
 *  $Id: ezusb.c,v 1.22 1999/12/03 15:06:28 tom Exp $
 */

/*****************************************************************************/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/socket.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

//#include <linux/spinlock.h>

#include "usb.h"
#include "ezusb.h"

/* --------------------------------------------------------------------- */

#define NREZUSB 1

static struct ezusb {
	struct semaphore mutex;
	struct usb_device *usbdev;
	struct list_head async_pending;
	struct list_head async_completed;
	wait_queue_head_t wait;
	spinlock_t lock;
} ezusb[NREZUSB];

struct async {
	struct list_head asynclist;
	struct ezusb *ez;
	void *userdata;
	unsigned datalen;
	void *context;
	urb_t urb;
};

/*-------------------------------------------------------------------*/

static struct async *alloc_async(unsigned int numisoframes)
{
	unsigned int assize = sizeof(struct async) + numisoframes * sizeof(iso_packet_descriptor_t);
	struct async *as = kmalloc(assize, GFP_KERNEL);
	if (!as)
		return NULL;
	memset(as, 0, assize);
	as->urb.number_of_packets = numisoframes;
	return as;
}

static void free_async(struct async *as)
{
	if (as->urb.transfer_buffer)
		kfree(as->urb.transfer_buffer);
	kfree(as);
}

/* --------------------------------------------------------------------- */

extern inline unsigned int ld2(unsigned int x)
{
	unsigned int r = 0;
	
	if (x >= 0x10000) {
		x >>= 16;
		r += 16;
	}
	if (x >= 0x100) {
		x >>= 8;
		r += 8;
	}
	if (x >= 0x10) {
		x >>= 4;
		r += 4;
	}
	if (x >= 4) {
		x >>= 2;
		r += 2;
	}
	if (x >= 2)
		r++;
	return r;
}

#if 0
/* why doesn't this work properly on i386? */
extern inline unsigned int ld2(unsigned int x)
{
	unsigned int r;

	__asm__("bsrl %1,%0" : "=r" (r) : "g" (x));
	return r;
}
#endif

/* --------------------------------------------------------------------- */

extern __inline__ void async_removelist(struct async *as)
{
	struct ezusb *ez = as->ez;
	unsigned long flags;

	spin_lock_irqsave(&ez->lock, flags);
	list_del(&as->asynclist);
	INIT_LIST_HEAD(&as->asynclist);
	spin_unlock_irqrestore(&ez->lock, flags);
}

extern __inline__ void async_newpending(struct async *as)
{
	struct ezusb *ez = as->ez;
	unsigned long flags;
	
	spin_lock_irqsave(&ez->lock, flags);
	list_add_tail(&as->asynclist, &ez->async_pending);
	spin_unlock_irqrestore(&ez->lock, flags);
}

extern __inline__ void async_removepending(struct async *as)
{
	struct ezusb *ez = as->ez;
	unsigned long flags;
	
	spin_lock_irqsave(&ez->lock, flags);
	list_del(&as->asynclist);
	INIT_LIST_HEAD(&as->asynclist);
	spin_unlock_irqrestore(&ez->lock, flags);
}

extern __inline__ struct async *async_getcompleted(struct ezusb *ez)
{
	unsigned long flags;
	struct async *as = NULL;

	spin_lock_irqsave(&ez->lock, flags);
	if (!list_empty(&ez->async_completed)) {
		as = list_entry(ez->async_completed.next, struct async, asynclist);
		list_del(&as->asynclist);
		INIT_LIST_HEAD(&as->asynclist);
	}
	spin_unlock_irqrestore(&ez->lock, flags);
	return as;
}

extern __inline__ struct async *async_getpending(struct ezusb *ez, void *context)
{
	unsigned long flags;
	struct async *as;
	struct list_head *p;

	spin_lock_irqsave(&ez->lock, flags);
	for (p = ez->async_pending.next; p != &ez->async_pending; ) {
		as = list_entry(p, struct async, asynclist);
		p = p->next;
		if (as->context != context)
			continue;
		list_del(&as->asynclist);
		INIT_LIST_HEAD(&as->asynclist);
		spin_unlock_irqrestore(&ez->lock, flags);
		return as;
	}
	spin_unlock_irqrestore(&ez->lock, flags);
	return NULL;
}

/* --------------------------------------------------------------------- */

static void async_completed(purb_t urb)
{
	struct async *as = (struct async *)urb->context;
	struct ezusb *ez = as->ez;
	unsigned cnt;

printk(KERN_DEBUG "ezusb: async_completed: status %d errcount %d actlen %d pipe 0x%x\n", 
       urb->status, urb->error_count, urb->actual_length, urb->pipe);
	spin_lock(&ez->lock);
	list_del(&as->asynclist);
	list_add_tail(&as->asynclist, &ez->async_completed);
	spin_unlock(&ez->lock);
	wake_up(&ez->wait);
}

static void destroy_all_async(struct ezusb *ez)
{
	struct async *as;
	unsigned long flags;

	spin_lock_irqsave(&ez->lock, flags);
	if (!list_empty(&ez->async_pending)) {
		as = list_entry(ez->async_pending.next, struct async, asynclist);
		list_del(&as->asynclist);
		INIT_LIST_HEAD(&as->asynclist);
		spin_unlock_irqrestore(&ez->lock, flags);
		/* discard_urb calls the completion handler with status == USB_ST_URB_KILLED */
		usb_unlink_urb(&as->urb);
		spin_lock_irqsave(&ez->lock, flags);
	}
	spin_unlock_irqrestore(&ez->lock, flags);
	while ((as = async_getcompleted(ez)))
		free_async(as);
}

/* --------------------------------------------------------------------- */

static loff_t ezusb_llseek(struct file *file, loff_t offset, int origin)
{
	struct ezusb *ez = (struct ezusb *)file->private_data;

	switch(origin) {
	case 1:
		offset += file->f_pos;
		break;
	case 2:
		offset += 0x10000;
		break;
	}
	if (offset < 0 || offset >= 0x10000)
		return -EINVAL;
	return (file->f_pos = offset);
}

static ssize_t ezusb_read(struct file *file, char *buf, size_t sz, loff_t *ppos)
{
	struct ezusb *ez = (struct ezusb *)file->private_data;
	unsigned pos = *ppos;
	unsigned ret = 0;
	unsigned len;
	unsigned char b[64];
	int i;

	if (*ppos < 0 || *ppos >= 0x10000)
		return -EINVAL;
	down(&ez->mutex);
	if (!ez->usbdev) {
		up(&ez->mutex);
		return -EIO;
	}
	while (sz > 0 && pos < 0x10000) {
		len = sz;
		if (len > sizeof(b))
			len = sizeof(b);
		if (pos + len > 0x10000)
			len = 0x10000 - pos;
		i = usb_control_msg(ez->usbdev, usb_rcvctrlpipe(ez->usbdev, 0), 0xa0, 0xc0, pos, 0, b, len, HZ);
		if (i < 0) {
			up(&ez->mutex);
			printk(KERN_WARNING "ezusb: upload failed pos %u len %u ret %d\n", pos, len, i);
			*ppos = pos;
			if (ret)
				return ret;
			return i;
		}
		if (copy_to_user(buf, b, len)) {
			up(&ez->mutex);
			*ppos = pos;
			if (ret)
				return ret;
			return -EFAULT;
		}
		pos += len;
		buf += len;
		sz -= len;
		ret += len;
	}
	up(&ez->mutex);
	*ppos = pos;
	return ret;
}

static ssize_t ezusb_write(struct file *file, const char *buf, size_t sz, loff_t *ppos)
{
	struct ezusb *ez = (struct ezusb *)file->private_data;
	unsigned pos = *ppos;
	unsigned ret = 0;
	unsigned len;
	unsigned char b[64];
	int i;

	if (*ppos < 0 || *ppos >= 0x10000)
		return -EINVAL;
	down(&ez->mutex);
	if (!ez->usbdev) {
		up(&ez->mutex);
		return -EIO;
	}
	while (sz > 0 && pos < 0x10000) {
		len = sz;
		if (len > sizeof(b))
			len = sizeof(b);
		if (pos + len > 0x10000)
			len = 0x10000 - pos;
		if (copy_from_user(b, buf, len)) {
			up(&ez->mutex);
			*ppos = pos;
			if (ret)
				return ret;
			return -EFAULT;
		}
		printk("writemem: %d %p %d\n",pos,b,len);
		i = usb_control_msg(ez->usbdev, usb_sndctrlpipe(ez->usbdev, 0), 0xa0, 0x40, pos, 0, b, len, HZ);
		if (i < 0) {
			up(&ez->mutex);
			printk(KERN_WARNING "ezusb: download failed pos %u len %u ret %d\n", pos, len, i);
			*ppos = pos;
			if (ret)
				return ret;
			return i;
		}
		pos += len;
		buf += len;
		sz -= len;
		ret += len;
	}
	up(&ez->mutex);
	*ppos = pos;
	return ret;
}

static int ezusb_open(struct inode *inode, struct file *file)
{
	struct ezusb *ez = &ezusb[0];

	down(&ez->mutex);
	while (!ez->usbdev) {
		up(&ez->mutex);
		if (!(file->f_flags & O_NONBLOCK)) {
			return -EIO;
		}
		schedule_timeout(HZ/2);
		if (signal_pending(current))
			return -EAGAIN;
		down(&ez->mutex);
	}
	up(&ez->mutex);
	file->f_pos = 0;
	file->private_data = ez;
	MOD_INC_USE_COUNT;
	return 0;
}

static int ezusb_release(struct inode *inode, struct file *file)
{
	struct ezusb *ez = (struct ezusb *)file->private_data;

	down(&ez->mutex);
	destroy_all_async(ez);
	up(&ez->mutex);
	MOD_DEC_USE_COUNT;
	return 0;
}

static int ezusb_control(struct usb_device *usbdev, unsigned char requesttype,
			 unsigned char request, unsigned short value, 
			 unsigned short index, unsigned short length,
			 unsigned int timeout, void *data)
{
	unsigned char *tbuf = NULL;
	unsigned int pipe;
	int i;

	if (length > PAGE_SIZE)
		return -EINVAL;
	/* __range_ok is broken; 
	   with unsigned short size, it gave
	   addl %si,%edx ; sbbl %ecx,%ecx; cmpl %edx,12(%eax); sbbl $0,%ecx
	*/
	if (requesttype & 0x80) {
		pipe = usb_rcvctrlpipe(usbdev, 0);
		if (length > 0 && !access_ok(VERIFY_WRITE, data, (unsigned int)length))
			return -EFAULT;
	} else
		pipe = usb_sndctrlpipe(usbdev, 0);
	if (length > 0) {
		if (!(tbuf = (unsigned char *)__get_free_page(GFP_KERNEL)))
			return -ENOMEM;
		if (!(requesttype & 0x80)) {
			if (copy_from_user(tbuf, data, length)) {
				free_page((unsigned long)tbuf);
				return -EFAULT;
			}
		}
	}
	i = usb_control_msg(usbdev, pipe, request, requesttype, value, index, tbuf, length, timeout);
	if (i < 0) {
		if (length > 0)
			free_page((unsigned long)tbuf);
		printk(KERN_WARNING "ezusb: EZUSB_CONTROL failed rqt %u rq %u len %u ret %d\n", 
		       requesttype, request, length, i);
		return i;
	}
	if (requesttype & 0x80 && length > 0 && copy_to_user(data, tbuf, length))
		i = -EFAULT;
	if (length > 0)
		free_page((unsigned long)tbuf);
	return i;
}

static int ezusb_bulk(struct usb_device *usbdev, unsigned int ep, unsigned int length, unsigned int timeout, void *data)
{
	unsigned char *tbuf = NULL;
	unsigned int pipe;
	unsigned long len2 = 0;
	int ret = 0;

	if (length > PAGE_SIZE)
		return -EINVAL;
	if ((ep & ~0x80) >= 16)
		return -EINVAL;
	if (ep & 0x80) {
		pipe = usb_rcvbulkpipe(usbdev, ep & 0x7f);
		if (length > 0 && !access_ok(VERIFY_WRITE, data, length))
			return -EFAULT;
	} else
		pipe = usb_sndbulkpipe(usbdev, ep & 0x7f);
	if (!usb_maxpacket(usbdev, pipe, !(ep & 0x80)))
		return -EINVAL;
	if (length > 0) {
		if (!(tbuf = (unsigned char *)__get_free_page(GFP_KERNEL)))
			return -ENOMEM;
		if (!(ep & 0x80)) {
			if (copy_from_user(tbuf, data, length)) {
				free_page((unsigned long)tbuf);
				return -EFAULT;
			}
		}
	}
	ret = usb_bulk_msg(usbdev, pipe, tbuf, length, &len2, timeout);
	if (ret < 0) {
		if (length > 0)
			free_page((unsigned long)tbuf);
		printk(KERN_WARNING "ezusb: EZUSB_BULK failed ep 0x%x len %u ret %d\n", 
		       ep, length, ret);
		return -ENXIO;
	}
	if (len2 > length)
		len2 = length;
	ret = len2;
	if (ep & 0x80 && len2 > 0 && copy_to_user(data, tbuf, len2))
		ret = -EFAULT;
	if (length > 0)
		free_page((unsigned long)tbuf);
	return ret;
}

static int ezusb_resetep(struct usb_device *usbdev, unsigned int ep)
{
	if ((ep & ~0x80) >= 16)
		return -EINVAL;
	usb_settoggle(usbdev, ep & 0xf, !(ep & 0x80), 0);
	return 0;
}

static int ezusb_setinterface(struct usb_device *usbdev, unsigned int interface, unsigned int altsetting)
{
	if (usb_set_interface(usbdev, interface, altsetting) < 0)
		return -EINVAL;
	return 0;
}

static int ezusb_setconfiguration(struct usb_device *usbdev, unsigned int config)
{
	if (usb_set_configuration(usbdev, config) < 0)
		return -EINVAL;
	return 0;
}

#define usb_sndintpipe(dev,endpoint)   ((PIPE_INTERRUPT << 30) | __create_pipe(dev,endpoint))
#define usb_rcvintpipe(dev,endpoint)   ((PIPE_INTERRUPT << 30) | __create_pipe(dev,endpoint) | USB_DIR_IN)

char* stuff[512];

static void int_compl(purb_t purb)
{
  printk("INT_COMPL\n");
  
}

static int ezusb_interrupt(struct ezusb *ez, struct ezusb_interrupt *ab)
{
  urb_t *urb;
  unsigned int pipe;

  urb=(urb_t*)kmalloc(sizeof(urb_t),GFP_KERNEL);
  if (!urb)
  {
    return -ENOMEM;
  }
  memset(urb,0,sizeof(urb_t));
  
 
  if ((ab->ep & ~0x80) >= 16)
    return -EINVAL;
  if (ab->ep & 0x80) 
    {
      pipe = usb_rcvintpipe(ez->usbdev, ab->ep & 0x7f);
      if (ab->len > 0 && !access_ok(VERIFY_WRITE, ab->data, ab->len))
	return -EFAULT;
    } 
  else
    pipe = usb_sndintpipe(ez->usbdev, ab->ep & 0x7f);
  
  memcpy(stuff,ab->data,64);
  urb->transfer_buffer=stuff;
  urb->transfer_buffer_length=ab->len;
  urb->complete=int_compl;
  urb->pipe=pipe;
  urb->dev=ez->usbdev;
  urb->interval=ab->interval;
  return usb_submit_urb(urb);
}

static int ezusb_requestbulk(struct ezusb *ez, struct ezusb_asyncbulk *ab)
{
	struct async *as = NULL;
	void *data = NULL;
	unsigned int pipe;
	int ret;

	if (ab->len > PAGE_SIZE)
		return -EINVAL;
	if ((ab->ep & ~0x80) >= 16)
		return -EINVAL;
	if (ab->ep & 0x80) {
		pipe = usb_rcvbulkpipe(ez->usbdev, ab->ep & 0x7f);
		if (ab->len > 0 && !access_ok(VERIFY_WRITE, ab->data, ab->len))
			return -EFAULT;
	} else
		pipe = usb_sndbulkpipe(ez->usbdev, ab->ep & 0x7f);
	if (!usb_maxpacket(ez->usbdev, pipe, !(ab->ep & 0x80)))
		return -EINVAL;
	if (ab->len > 0 && !(data = kmalloc(ab->len, GFP_KERNEL)))
		return -ENOMEM;
	if (!(as = alloc_async(0))) {
		if (data)
			kfree(data);
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&as->asynclist);
	as->ez = ez;
	as->userdata = ab->data;
	as->datalen = ab->len;
	as->context = ab->context;
	as->urb.dev = ez->usbdev;
	as->urb.pipe = pipe;
	as->urb.transfer_flags = 0;
	as->urb.transfer_buffer = data;
	as->urb.transfer_buffer_length = ab->len;
	as->urb.context = as;
	as->urb.complete = (usb_complete_t)async_completed;
	if (ab->len > 0 && !(ab->ep & 0x80)) {
		if (copy_from_user(data, ab->data, ab->len)) {
			free_async(as);
			return -EFAULT;
		}
		as->userdata = NULL; /* no need to copy back at completion */
	}
	async_newpending(as);
	if ((ret = usb_submit_urb(&as->urb))) {
 printk(KERN_DEBUG "ezusb: bulk: usb_submit_urb returned %d\n", ret);
		async_removepending(as);
		free_async(as);
		return -EINVAL; /* return ret; */
	}
	return 0;
}

static int ezusb_requestiso(struct ezusb *ez, struct ezusb_asynciso *ai, unsigned char *cmd)
{
	struct async *as;
	void *data = NULL;
	unsigned int maxpkt, pipe, dsize, totsize, i, j;
	int ret;

	if ((ai->ep & ~0x80) >= 16 || ai->framecnt < 1 || ai->framecnt > 128)
		return -EINVAL;
	if (ai->ep & 0x80)
		pipe = usb_rcvisocpipe(ez->usbdev, ai->ep & 0x7f);
	else
		pipe = usb_sndisocpipe(ez->usbdev, ai->ep & 0x7f);
	if (!(maxpkt = usb_maxpacket(ez->usbdev, pipe, !(ai->ep & 0x80))))
		return -EINVAL;
	dsize = maxpkt * ai->framecnt;
//printk(KERN_DEBUG "ezusb: iso: dsize %d\n", dsize);
	if (dsize > 65536) 
		return -EINVAL;
	if (ai->ep & 0x80)
		if (dsize > 0 && !access_ok(VERIFY_WRITE, ai->data, dsize))
			return -EFAULT;
	if (dsize > 0 && !(data = kmalloc(dsize, GFP_KERNEL)))
	{
		printk("dsize: %d failed\n",dsize);
		return -ENOMEM;
	}
	if (!(as = alloc_async(ai->framecnt))) {
		if (data)
			kfree(data);
		printk("alloc_async failed\n");			
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&as->asynclist);
	
	as->ez = ez;
	as->userdata = ai->data;
	as->datalen = dsize;
	as->context = ai->context;
	
	as->urb.dev = ez->usbdev;
	as->urb.pipe = pipe;
	as->urb.transfer_flags = USB_ISO_ASAP;
	as->urb.transfer_buffer = data;
	as->urb.transfer_buffer_length = dsize;
	as->urb.context = as;
	as->urb.complete = (usb_complete_t)async_completed;

	for (i = totsize = 0; i < as->urb.number_of_packets; i++) {
		as->urb.iso_frame_desc[i].offset = totsize;
		if (get_user(j, (int *)(cmd + i * sizeof(struct ezusb_isoframestat)))) {
			free_async(as);
			return -EFAULT;
		}
		as->urb.iso_frame_desc[i].length = j;
		totsize += j;
	}
	if (dsize > 0 && totsize > 0 && !(ai->ep & 0x80)) {
		if (copy_from_user(data, ai->data, totsize)) {
			free_async(as);
			return -EFAULT;
		}
		as->userdata = NULL; /* no need to copy back at completion */
	}
	async_newpending(as);
	if ((ret = usb_submit_urb(&as->urb))) {
 printk(KERN_DEBUG "ezusb: iso: usb_submit_urb returned %d\n", ret);
		async_removepending(as);
		free_async(as);
		return -EINVAL; /* return ret; */
	}
	return 0;
}

static int ezusb_terminateasync(struct ezusb *ez, void *context)
{
	struct async *as;
	int ret = 0;

	while ((as = async_getpending(ez, context))) {
		usb_unlink_urb(&as->urb);
		ret++;
	}
	return ret;
}

static int ezusb_asynccompl(struct async *as, void *arg)
{
	struct ezusb_asynccompleted *cpl;
	unsigned int numframes, cplsize, i;

	if (as->userdata) {
		if (copy_to_user(as->userdata, as->urb.transfer_buffer, as->datalen)) {
			free_async(as);
			return -EFAULT;
		}
	}
	numframes = as->urb.number_of_packets;
	cplsize = sizeof(struct ezusb_asynccompleted) + numframes * sizeof(struct ezusb_isoframestat);
	if (!(cpl = kmalloc(cplsize, GFP_KERNEL))) {
		free_async(as);
		return -ENOMEM;
	}
	cpl->status = as->urb.status;
	cpl->length = as->urb.actual_length;
	cpl->context = as->context;
	for (i = 0; i < numframes; i++) {
		cpl->isostat[i].length = as->urb.iso_frame_desc[i].length;
		cpl->isostat[i].status = as->urb.iso_frame_desc[i].status;
	}
	free_async(as);
	if (copy_to_user(arg, cpl, cplsize)) {
		kfree(cpl);
		return -EFAULT;
	}
	kfree(cpl);
	return 0;
}

static int ezusb_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ezusb *ez = (struct ezusb *)file->private_data;
	DECLARE_WAITQUEUE(wait, current);
	struct usb_proc_ctrltransfer pctrl;
	struct usb_proc_bulktransfer pbulk;
	struct usb_proc_old_ctrltransfer opctrl;
	struct usb_proc_old_bulktransfer opbulk;
	struct usb_proc_setinterface psetintf;
	struct ezusb_ctrltransfer ctrl;
	struct ezusb_bulktransfer bulk;
	struct ezusb_old_ctrltransfer octrl;
	struct ezusb_old_bulktransfer obulk;
	struct ezusb_setinterface setintf;
	struct ezusb_asyncbulk abulk;
	struct ezusb_asynciso aiso;
	struct ezusb_interrupt ezint;
	struct async *as;
	void *context;
	unsigned int ep, cfg;
	int i, ret = 0;

	down(&ez->mutex);
	if (!ez->usbdev) {
		up(&ez->mutex);
		return -EIO;
	}
	switch (cmd) {
	case USB_PROC_CONTROL:
		if (copy_from_user(&pctrl, (void *)arg, sizeof(pctrl))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_control(ez->usbdev, pctrl.requesttype, pctrl.request, 
				    pctrl.value, pctrl.index, pctrl.length, 
				    (pctrl.timeout * HZ + 500) / 1000, pctrl.data);
		break;

	case USB_PROC_BULK:
		if (copy_from_user(&pbulk, (void *)arg, sizeof(pbulk))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_bulk(ez->usbdev, pbulk.ep, pbulk.len, 
				 (pbulk.timeout * HZ + 500) / 1000, pbulk.data);
		break;

	case USB_PROC_OLD_CONTROL:
		if (copy_from_user(&opctrl, (void *)arg, sizeof(opctrl))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_control(ez->usbdev, opctrl.requesttype, opctrl.request, 
				    opctrl.value, opctrl.index, opctrl.length, HZ, opctrl.data);
		break;

	case USB_PROC_OLD_BULK:
		if (copy_from_user(&opbulk, (void *)arg, sizeof(opbulk))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_bulk(ez->usbdev, opbulk.ep, opbulk.len, 5*HZ, opbulk.data);
		break;

	case USB_PROC_RESETEP:
		if (get_user(ep, (unsigned int *)arg)) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_resetep(ez->usbdev, ep);
		break;
	
	case USB_PROC_SETINTERFACE:
		if (copy_from_user(&psetintf, (void *)arg, sizeof(psetintf))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_setinterface(ez->usbdev, psetintf.interface, psetintf.altsetting);
		break;

	case USB_PROC_SETCONFIGURATION:
		if (get_user(cfg, (unsigned int *)arg)) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_setconfiguration(ez->usbdev, cfg);
		break;

	case EZUSB_CONTROL:
		if (copy_from_user(&ctrl, (void *)arg, sizeof(ctrl))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_control(ez->usbdev, ctrl.requesttype, ctrl.request, 
				    ctrl.value, ctrl.index, ctrl.length, 
				    (ctrl.timeout * HZ + 500) / 1000, ctrl.data);
		break;

	case EZUSB_BULK:
		if (copy_from_user(&bulk, (void *)arg, sizeof(bulk))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_bulk(ez->usbdev, bulk.ep, bulk.len, 
				 (bulk.timeout * HZ + 500) / 1000, bulk.data);
		break;

	case EZUSB_OLD_CONTROL:
		if (copy_from_user(&octrl, (void *)arg, sizeof(octrl))) {
			ret = -EFAULT;
			break;
		}
		if (octrl.dlen != octrl.length) {
			ret = -EINVAL;
			break;
		}
		ret = ezusb_control(ez->usbdev, octrl.requesttype, octrl.request, 
				    octrl.value, octrl.index, octrl.length, HZ, octrl.data);
		break;

	case EZUSB_OLD_BULK:
		if (copy_from_user(&obulk, (void *)arg, sizeof(obulk))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_bulk(ez->usbdev, obulk.ep, obulk.len, 5*HZ, obulk.data);
		break;

	case EZUSB_RESETEP:
		if (get_user(ep, (unsigned int *)arg)) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_resetep(ez->usbdev, ep);
		break;
	
	case EZUSB_SETINTERFACE:
		if (copy_from_user(&setintf, (void *)arg, sizeof(setintf))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_setinterface(ez->usbdev, setintf.interface, setintf.altsetting);
		break;

	case EZUSB_SETCONFIGURATION:
		if (get_user(cfg, (unsigned int *)arg)) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_setconfiguration(ez->usbdev, cfg);
		break;

	case EZUSB_ASYNCCOMPLETED:
		as = NULL;
		current->state = TASK_INTERRUPTIBLE;
		add_wait_queue(&ez->wait, &wait);
		for (;;) {
			if (!ez->usbdev)
				break;
			if ((as = async_getcompleted(ez)))
				break;
			if (signal_pending(current))
				break;
			up(&ez->mutex);
			schedule();
			down(&ez->mutex);
		}
		remove_wait_queue(&ez->wait, &wait);
		current->state = TASK_RUNNING;
		if (as) {
			ret = ezusb_asynccompl(as, (void *)arg);
			break;
		}
		if (signal_pending(current)) {
			ret = -EINTR;
			break;
		}
		ret = -EIO;
		break;

	case EZUSB_ASYNCCOMPLETEDNB:
		if ((as = async_getcompleted(ez))) {
			ret = ezusb_asynccompl(as, (void *)arg);
			break;
		}
		ret = -EAGAIN;
		break;

	case EZUSB_REQUESTBULK:
		if (copy_from_user(&abulk, (void *)arg, sizeof(abulk))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_requestbulk(ez, &abulk);
		break;	

	case EZUSB_REQUESTISO:
		if (copy_from_user(&aiso, (void *)arg, sizeof(aiso))) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_requestiso(ez, &aiso, ((unsigned char *)arg)+sizeof(aiso));
		break;

	case EZUSB_TERMINATEASYNC:
		if (get_user(context, (void **)arg)) {
			ret = -EFAULT;
			break;
		}
		ret = ezusb_terminateasync(ez, context);
		break;

	case EZUSB_GETFRAMENUMBER:
		i = usb_get_current_frame_number(ez->usbdev);
		ret = put_user(i, (int *)arg);
		break;

	case EZUSB_INTERRUPT:
	  printk("INT START\n");
		if (copy_from_user(&ezint, (void *)arg, sizeof(ezint))) {
			ret = -EFAULT;
			break;
		}
		ret=ezusb_interrupt(ez,&ezint);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}
	up(&ez->mutex);
	return ret;
}

static struct file_operations ezusb_fops = {
	ezusb_llseek,
	ezusb_read,
	ezusb_write,
	NULL,  /* readdir */
	NULL,  /* poll */
	ezusb_ioctl,
	NULL,  /* mmap */
	ezusb_open,
	NULL,  /* flush */
	ezusb_release,
	NULL,  /* fsync */
	NULL,  /* fasync */
	NULL   /* lock */
};

/* --------------------------------------------------------------------- */

static void * ezusb_probe(struct usb_device *usbdev, unsigned int ifnum)
{
	struct ezusb *ez = &ezusb[0];
	struct usb_interface_descriptor *interface;
	struct usb_endpoint_descriptor *endpoint;

#undef KERN_DEBUG
#define KERN_DEBUG ""
	printk(KERN_DEBUG "ezusb: probe: vendor id 0x%x, device id 0x%x\n",
	       usbdev->descriptor.idVendor, usbdev->descriptor.idProduct);

	/* the 1234:5678 is just a self assigned test ID */
	if ((usbdev->descriptor.idVendor != 0x0547 || usbdev->descriptor.idProduct != 0x2131) 
	#if 1
		&&
	    (usbdev->descriptor.idVendor != 0x0547 || usbdev->descriptor.idProduct != 0x9999) &&
	    (usbdev->descriptor.idVendor != 0x1234 || usbdev->descriptor.idProduct != 0x5678)
	   #endif 
	    )
		return NULL;

	/* We don't handle multiple configurations */
	if (usbdev->descriptor.bNumConfigurations != 1)
		return NULL;

#if 0
	/* We don't handle multiple interfaces */
	if (usbdev->config[0].bNumInterfaces != 1)
		return NULL;
#endif

	down(&ez->mutex);
	if (ez->usbdev) {
		up(&ez->mutex);
		printk(KERN_INFO "ezusb: device already used\n");
		return NULL;
	}
	ez->usbdev = usbdev;
	if (usb_set_configuration(usbdev, usbdev->config[0].bConfigurationValue) < 0) {
		printk(KERN_ERR "ezusb: set_configuration failed\n");
		goto err;
	}

	interface = &usbdev->config[0].interface[0].altsetting[1];
	if (usb_set_interface(usbdev, 0, 1) < 0) {
		printk(KERN_ERR "ezusb: set_interface failed\n");
		goto err;
	}
	up(&ez->mutex);
	MOD_INC_USE_COUNT;
	return ez;

 err:
	up(&ez->mutex);
	ez->usbdev = NULL;
	return NULL;
}

static void ezusb_disconnect(struct usb_device *usbdev, void *ptr)
{
	struct ezusb *ez = (struct ezusb *)ptr;

	down(&ez->mutex);
	destroy_all_async(ez);
	ez->usbdev = NULL;
	up(&ez->mutex);
	wake_up(&ez->wait);
	MOD_DEC_USE_COUNT;
}

static struct usb_driver ezusb_driver = {
	"ezusb",
	ezusb_probe,
	ezusb_disconnect,
	{ NULL, NULL },
	&ezusb_fops,
	192
};

/* --------------------------------------------------------------------- */

int ezusb_init(void)
{
	unsigned u;

	/* initialize struct */
	for (u = 0; u < NREZUSB; u++) {
		init_MUTEX(&ezusb[u].mutex);
		ezusb[u].usbdev = NULL;
		INIT_LIST_HEAD(&ezusb[u].async_pending);
		INIT_LIST_HEAD(&ezusb[u].async_completed);
		init_waitqueue_head(&ezusb[u].wait);
		spin_lock_init(&ezusb[u].lock);
	}
	/* register misc device */
	usb_register(&ezusb_driver);
	printk(KERN_INFO "ezusb: Anchorchip firmware download driver registered\n");
	return 0;
}

void ezusb_cleanup(void)
{
	usb_deregister(&ezusb_driver);
}

/* --------------------------------------------------------------------- */

#ifdef MODULE

int minor = 192;

int init_module(void)
{
	return ezusb_init();
}

void cleanup_module(void)
{
	ezusb_cleanup();
}

#endif

/* --------------------------------------------------------------------- */
