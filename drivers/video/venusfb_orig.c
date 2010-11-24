/*
 * linux/drivers/video/venusfb.c - 
*
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <venus.h>
#include <linux/radix-tree.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/blkpg.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>


#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/sched.h>
#include <linux/dcache.h>
#include <linux/auth.h>
#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

#include "venusfb.h"
#define  VIDEO_MEMORY_LENGTH     1024*1024*3//(1*1024*1024+128*1024)*2

#define  TMPBUFLEN   2048

#define DCU_REG_BASE 0xB8008000
#define DC_SYS_MISC ((volatile unsigned int *)0xB8008004)
#define DC_PICT_SET ((volatile unsigned int *)0xB8008000)
#define DC_PICT_SET_OFFSET ((volatile unsigned int *)0xB8008034)

#define DBG_PRINT(s, args...) printk(s, ##args)
#define GRAPHIC_DCU_INDEX_BASE   59


#ifdef SIMULATOR
DWORD SB2_TILE_ACCESS_SET;
DWORD DC_PICT_SET;

//Command buffer manupolated registers
DWORD    SE_CmdBase;
DWORD    SE_CmdLimit;
DWORD    SE_CmdRdptr;
DWORD    SE_CmdWrptr;
DWORD    SE_CmdfifoState;
DWORD    SE_CNTL;
DWORD    SE_INT_STATUS;
DWORD    SE_INT_ENABLE;
DWORD    SE_INST_CNT;

//Streaming engine registers
DWORD SE_AddrMap;
DWORD SE_Color_Key;
DWORD SE_Def_Alpha;
DWORD SE_Rslt_Alpha;
DWORD SE_Src_Alpha;
DWORD SE_Src_Color;
DWORD SE_Clr_Format;
DWORD SE_BG_Color;
DWORD SE_BG_Alpha;

#define write_l(a, b)    (((b)) = (a))
#define readl(a)    ((a))

#else
#define write_l(a, b) {DBG_PRINT("write (0x%8x to reg(0x%8x)\n", (uint32_t)(a), (uint32_t)(b)); writel(a, b);}
#endif
#define endian_swap_32(a)   (((a)>>24) | (((a)>>8) & 0xFF00) | (((a)<<8) & 0xFF0000) | (((a)<<24) & 0xFF000000))


int32_t 	g_pixelFormat   = 0xFFFFFFFF; 

struct 	fb_se_dev *  p_global_se_dev=NULL;

static  struct  venusfb_info   global_fbi;



static struct venusfb_rgb rgb_8 = {
	.red	= { .offset = 0,  .length = 8/*4*/, },
	.green	= { .offset = 0,  .length = 8/*4*/, },
	.blue	= { .offset = 0,  .length = 8/*4*/, },
	.transp = { .offset = 0,  .length = 0, },
};

static struct venusfb_rgb  rgb_16 = {
	.red	= { .offset = 11, .length = 5, },
	.green	= { .offset = 5,  .length = 6, },
	.blue	= { .offset = 0,  .length = 5, },
	.transp = { .offset = 0,  .length = 0, },
};
static struct venusfb_rgb  rgb_24 = {
	.red	= { .offset = 0, .length = 8, },
	.green	= { .offset =8,  .length = 8, },
	.blue	= { .offset = 16,  .length = 8, },
	.transp = { .offset = 0,  .length = 0, },
};
static struct venusfb_rgb  rgb_32 = {
	.red	= { .offset = 0, .length = 8, },
	.green	= { .offset =8,  .length = 8, },
	.blue	= { .offset = 16,  .length = 8, },
	.transp = { .offset = 24,  .length = 8, },
};

uint32_t dwValue = 0x7;

static struct venusfb_mach_info venus_video_info __initdata = {
	.pixclock	= 39721/*??*/,	.bpp		= 16,/*??*/
	.xres		= 720/*640*/,		        .yres		= 576/*480*/,
	.hsync_len	= /*95*/0,		        .vsync_len	= /*2*/0,
	.left_margin	= /*40*/0,		.upper_margin	= /*32*/0,
	.right_margin	= /*24*/0,		.lower_margin	= /*11*/0,
	.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,/*??*/

};


static  struct radix_tree_root	dvr_tree;
static  spinlock_t              dvr_tree_lock;
// constant declaration
static const int	pli_signature = 0x77000000;
static  const int	max_buddy_size = 1 << (12+MAX_ORDER-1);
static const int	buddy_id = 0x10000;
static const int	max_cache_size = 1 << 12;
static  const int	cache_id = 0x20000;






static int cal_order(int num)
{
	int cnt = 0, val = 1 << 12;

	while (num > val) {
		val <<= 1;
		cnt++;
	}

	return cnt;
}


/*
*
*  Internal routines
*/
static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;
	return (length);
}


static struct venusfb_mach_info * 
venusfb_get_machine_info(struct venusfb_mach_info *m_info)
{
	struct venusfb_mach_info *p_inf = NULL;
	p_inf= m_info;
	return p_inf;
}



/*
*        now ,I can't use se
*	   interrupt   hanle
*/
static
irqreturn_t  se_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
#if 0
	struct se_dev *dev = dev_id;
	uint32_t int_status = readl(SE_INT_STATUS);

	if(int_status & INT_SYNC/* interrupt souirce is from SYNC command*/)
	{
		uint32_t tmp;
		//DBG_PRINT("se interrupt = %x\n", int_status);
		//clear interrupt
		write_l(SE_CLR_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_STATUS));

		tmp = dev->hw_counter.low;
		dev->hw_counter.low += (INST_CNT_MASK+1);
		if(tmp > dev->hw_counter.low )	//check if overflow happen
			dev->hw_counter.high++;

		//DBG_PRINT("[se driver] SE_REG(SE_INST_CNT) = 0x%x\n", readl(SE_REG(SE_INST_CNT)));
		//DBG_PRINT("hw_counter.high = 0x%x\n", dev->hw_counter.high);
		//DBG_PRINT("hw_counter.low = 0x%x\n", dev->hw_counter.low);
		//enable interrupt
		write_l(SE_CLR_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_ENABLE));
		return IRQ_HANDLED;
	}
	//DBG_PRINT("not se interrupt = %x\n", int_status);
#endif
	return IRQ_NONE;

}

/********************************************
 * Function :
 * Description :
 * Input :   struct fb_se_dev : hold driver data
 *              nLen : number of bytes
 *              pdwBuf : command buffer
 * Output : 
 *******************************************/
static void
WriteCmd(struct fb_se_dev *dev, uint8_t *pBuf, int nLen)
{
#if 0
	int ii;
	uint8_t *writeptr;
	uint8_t *pWptr;
	uint8_t *pWptrLimit;

	dev->sw_counter.low++;
	if(dev->sw_counter.low == 0){
		dev->sw_counter.high++;
	}
	if( (dev->sw_counter.low & INST_CNT_MASK) == 0){
	   
		//Insert a SYNC command and enable interrupt for updating the upper 48 bits counter in software.
		uint32_t dwValue = SYNC;
		//recursive call
		WriteCmd(dev, (uint8_t *)&dwValue, sizeof(uint32_t));      //recursive
	}

	DBG_PRINT("[queue size, wptr, rptr] =");
	DBG_PRINT("[0x%x, ", dev->size);
	DBG_PRINT("0x%x, ", readl(SE_REG(SE_CmdWrptr)));
	DBG_PRINT("0x%x]\n", readl(SE_REG(SE_CmdRdptr)));
	DBG_PRINT("SE_REG(SE_INST_CNT) %x\n", readl(SE_REG(SE_INST_CNT)));
	DBG_PRINT("SE_REG(SE_CmdFifoState) %x\n", readl(SE_REG(SE_CmdFifoState)));
	DBG_PRINT("SE_REG(SE_CNTL) %x\n", readl(SE_REG(SE_CNTL)));
	DBG_PRINT("SE_REG(SE_INT_STATUS) %x\n", readl(SE_REG(SE_INT_STATUS)));
	
	writeptr = (uint8_t *)readl(SE_REG(SE_CmdWrptr));

	
#ifndef SIMULATOR
	while(1)
	{
		uint8_t *readptr = (uint8_t *)readl(SE_REG(SE_CmdRdptr));
		if(readptr <= writeptr)
		{
			readptr += dev->size; 
		}
		if((writeptr+nLen) < readptr)
		{
			break;
		}
		DBG_PRINT("while loop w=%x r=%x\n", (uint32_t)writeptr, (uint32_t)readptr);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(1);
	}
#endif

	pWptrLimit = (uint8_t *)(dev->CmdBuf + dev->size);

	while(1){ 
		//handle the condition that command buffer wrapped around inside a 2-/3-word command
		//causes SE decording error
		//restore write pointer afater recusive call
		pWptr = dev->CmdBuf + dev->wrptr;
		//if((nLen > 4) && ((pWptr + sizeof(uint32_t)) == pWptrLimit))
		if((nLen > 4) && (pWptr + nLen) >= pWptrLimit)
		{
			uint32_t cmd_word = 0x00000071; //dummy command by writing 0 to SE_Input register

			//recursive call
			WriteCmd(dev, (uint8_t *)&cmd_word, sizeof(uint32_t));
		}
		else
		{
			break;
		}
	}

	DBG_PRINT("kernel land [");
	
	//Start writing command words to the ring buffer.
	for(ii=0; ii<nLen; ii+=sizeof(uint32_t))
	{
		DBG_PRINT("(%x-%x)-", (uint32_t)pWptr, *(uint32_t *)(pBuf + ii));
		*(uint32_t *)((uint32_t)pWptr | 0xA0000000) = endian_swap_32(*(uint32_t *)(pBuf + ii));

		pWptr += sizeof(uint32_t);
		if(pWptr >= pWptrLimit)
			pWptr = dev->CmdBuf;
	}
	DBG_PRINT("]\n");

	dev->wrptr += nLen;
	if(dev->wrptr >= dev->size)
		dev->wrptr -= dev->size;

	//convert to physical address
	pWptr -= dev->v_to_p_offset;
	write_l( SE_GO | SE_CLR_WRITE_DATA, SE_REG(SE_CNTL));
	write_l((uint32_t)pWptr, SE_REG(SE_CmdWrptr));    
	write_l( SE_GO | SE_SET_WRITE_DATA, SE_REG(SE_CNTL));
#endif
	return;
}



/*
*             start   se
*
*/
static void setup_se(void)
{
#if 0
	int result;
	struct fb_se_dev *se_dev; /* device information */

	DBG_PRINT(KERN_INFO "justin---setup_se(void)\n");
	se_dev = kmalloc(sizeof(struct fb_se_dev) ,GFP_KERNEL);
	if (!se_dev){
		printk("error: can not kmalloc (struct fb_se_dev) \n ");
		return NULL;
	}

	memset(se_dev, 0, sizeof(struct fb_se_dev));
	
	sema_init(&se_dev->sem,1);
	
 	if (down_interruptible(&se_dev->sem))
		return -ERESTARTSYS;


	if(!se_dev->CmdBuf){
		
		printk("setup_se(): if(!dev->CmdBuf \n");		
 		int result;
		uint32_t *pPhysAddr;

		DBG_PRINT("se initialization\n");

		se_dev->size = SE_COMMAND_ENTRIES*sizeof(uint32_t);
		se_dev->CmdBuf = kmalloc(se_dev->size, GFP_KERNEL);
		if(NULL == se_dev->CmdBuf ){
			printk("error:dev->CmdBuf = kmalloc(dev->size, GFP_KERNEL); \n");
			return  -3;
		}
		pPhysAddr = (uint32_t *)__pa(se_dev->CmdBuf);
		DBG_PRINT("CmdBuf virt addr = %x, phy addr = %x\n", (uint32_t)se_dev->CmdBuf, (uint32_t)pPhysAddr);
		se_dev->v_to_p_offset = (int32_t)((uint8_t *)se_dev->CmdBuf - (uint32_t)pPhysAddr);

		se_dev->wrptr = 0;
		se_dev->CmdBase = pPhysAddr;
		se_dev->CmdLimit = pPhysAddr + SE_COMMAND_ENTRIES;

		write_l((uint32_t)se_dev->CmdBase, SE_REG(SE_CmdBase));
		write_l((uint32_t)se_dev->CmdLimit, SE_REG(SE_CmdLimit));
		write_l((uint32_t)se_dev->CmdBase, SE_REG(SE_CmdRdptr));
		write_l((uint32_t)se_dev->CmdBase, SE_REG(SE_CmdWrptr));
		write_l(0, SE_REG(SE_INST_CNT));

		result = request_irq(SE_IRQ, se_irq_handler, SA_INTERRUPT|SA_SHIRQ, "fb_se", se_dev);
		if(result){
			
			DBG_PRINT(KERN_INFO "se: can't get assigned irq%i\n", SE_IRQ);
		}
		
		//enable interrupt
		write_l(SE_SET_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_ENABLE));

		//start SE
		write_l(/*readl(SE_REG(SE_CNTL)) |*/ SE_GO | SE_SET_WRITE_DATA, SE_REG(SE_CNTL));
	}

	
	p_global_se_dev = se_dev;	

	up(&se_dev->sem);

#endif
	return 0;        

}


/*
*
*	release se
*/
#if 0
int se_release(struct inode *inode, struct file *filp)
{
	//struct se_dev *dev = filp->private_data; /* for other methods */
	struct se_dev *dev = p_global_se_dev;
	//dev->initialized = 0;
	dev->hw_counter.low = 0;
	dev->hw_counter.high = 0;
	write_l(SE_CLR_WRITE_DATA | INT_SYNC, SE_REG(SE_INT_ENABLE));
	//stop SE
	write_l(SE_GO | SE_CLR_WRITE_DATA, SE_REG(SE_CNTL));
	
	free_irq(SE_IRQ, dev);
	
	DBG_PRINT(KERN_INFO "se release\n");
	return 0;
}
#endif


/********************************************
 * Function :
 * Description :
 * Input : 
 * Output : 
 *******************************************/
//HRESULT
int 
WriteReg(uint32_t reg_addr, uint32_t dwValue)
{
#if 0
	dwValue = ((uint32_t)dwValue << 8) | ((uint32_t)reg_addr << 4) | WRITE_REG;
	//WriteCmd(1, &dwValue);
	// if(count & 0x3)
	//	return 0;
	//pthread_mutex_lock(global_se_dev->lock_mutex);
	WriteCmd(p_global_se_dev, (uint8_t *)&dwValue, 1<<2);
	//pthread_mutex_unlock(global_se_dev->lock_mutex);
#endif
	return  1;
}



//HRESULT
static int
set_dcu_mem(uint32_t idx, uint32_t addr, uint32_t pitch)
{
#if 0
	ioc_dcu_info info;
	info.index = GRAPHIC_DCU_INDEX_BASE + idx;
	info.addr = addr;
	info.pitch = pitch;
	
	DBG_PRINT("now in set_dcu_mem()\n");

	write_l( PreparePictSetWriteData(info.index, info.pitch, info.addr),
                             DCU_REG(DC_PICT_SET));
       write_l( DC_PICT_SET_OFFSET_pict_set_num(info.index)
                     | DC_PICT_SET_OFFSET_pict_set_offset_x(0)
                     | DC_PICT_SET_OFFSET_pict_set_offset_y(0),
                     DCU_REG(DC_PICT_SET_OFFSET));
	DBG_PRINT("venus SetDcuMem=SE_IOC_SET_DCU_INFO %x\n", PreparePictSetWriteData(info.index, info.pitch, info.addr));
	DBG_PRINT("index=%x, pitch=%x, addr=%x\n", info.index, info.pitch, info.addr);

	
#define TEST_VOUT

#ifdef TEST_VOUT
	write_l( (((info.addr) >> (11+((readl(DCU_REG(DC_SYS_MISC)) >> 16) & 0x3))) & 0x1FFF), ((volatile unsigned int *)0xB80050A0));
#endif
#endif
	//return S_OK;
	return  1;
}



static void show_buddy_info(void)
{
	pg_data_t *pgdat = &contig_page_data;
	struct zone *zone;
	struct zone *node_zones = pgdat->node_zones;
	unsigned long flags;
	int order;
	//long cached;

	for (zone = node_zones; zone - node_zones < MAX_NR_ZONES; ++zone) {
		if (!zone->present_pages)
			continue;

		spin_lock_irqsave(&zone->lock, flags);
		printk("Node %d, zone %8s ", pgdat->node_id, zone->name);
		for (order = 0; order < MAX_ORDER; ++order)
			printk("%6lu ", zone->free_area[order].nr_free);
		printk("\nfree pages:%6lu\n", zone->free_pages);
		spin_unlock_irqrestore(&zone->lock, flags);
	}
	//cached = get_page_cache_size() - total_swapcache_pages - nr_blockdev_pages();
	//printk("cached size:%6lu\n", cached);
}


static void show_zone_info(void)
{
	struct zone *zone;
	int cnt = 0;

	for (zone = contig_page_data.node_zones; zone; zone = next_zone(zone)) {
		struct per_cpu_pageset *pset;
		int i;
		printk(KERN_WARNING "   zone%d\tfree size:%d\tpresent size:%d\n", cnt++, (int)zone->free_pages, (int)zone->present_pages);
		printk(KERN_WARNING "   min:%d\tlow:%d\t\thigh:%d\n", (int)zone->pages_min, (int)zone->pages_low, (int)zone->pages_high);
		printk(KERN_WARNING "   active:%d\tinactive:%d\n", (int)zone->nr_active, (int)zone->nr_inactive);

		pset = &zone->pageset[0];
		for (i = 0; i < ARRAY_SIZE(pset->pcp); i++) {
			struct per_cpu_pages *pcp;		
			pcp = &pset->pcp[i];
			printk(KERN_WARNING "   per_cpu_pages[%d]: %d\n", i, (int)pcp->count);
		}
	}
	print_pagevec_count();
}




/*
*  when I insmode the 'venusfb.ko'  , the kernel alawys tell it can't find the  
*   -- radix_tree_preload
*  so i delete these codes,and doing this will not affect my routine
*
*/
static int record_insert(int type, unsigned long addr)
{
	#if 0
	int *ptr;
	int error;
	ptr = kmalloc(8, GFP_KERNEL);
	ptr[0] = type;
	ptr[1] = addr;
	addr = (addr&0x0fffffff) >> 2;	// use WORD as minimum unit
	error = radix_tree_preload(GFP_KERNEL);
	if (error == 0) {
		spin_lock_irq(&dvr_tree_lock);
		error = radix_tree_insert(&dvr_tree, addr, ptr);
		if (error) {
			printk("radix tree insert error...\n");
			kfree((void *)ptr);
		}
		spin_unlock_irq(&dvr_tree_lock);
		radix_tree_preload_end();
	}
	#endif
	return 0;// error;
}


static void data_cache_flush(unsigned long addr, int size)
{
	for ( ; size > 0; size -= CACHE_LINE_LENGTH) {
		__asm__ __volatile__ ("cache 0x15, (%0);": :"r"(addr));
		addr += 16;
	}
}



/*
*
*	allocate a free memory for vidio card  (1M bytes) which is reserved by kernel,
*      
*/
static int vneusfb_map_video_memory(struct venusfb_info * fbi)
{
	int order;
	int ret;
	DBG_PRINT("enter  vneusfb_map_video_memory() \n");


	fbi->fb.fix.smem_len = VIDEO_MEMORY_LENGTH;//+512*1024;

	fbi->map_size = PAGE_ALIGN(fbi->fb.fix.smem_len/* + PAGE_SIZE*/);
	printk("fbi->map_size = %ld\n",fbi->map_size);

	order = cal_order(fbi->map_size);
	// modified by EJ
	ret = dvr_malloc(PAGE_SIZE << order);
	if (!ret)
		show_buddy_info();
	
	if (ret) {
		//#ifdef	CONFIG_REALTEK_ADVANCED_RECLAIM
		int diff;
		int tmp, cnt = 0;
		struct page *page;

		diff = (1 << (12+order)) -fbi->map_size;
		if (diff >= 0x10000) {
			//printk("=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*\n");
			//printk("req: %x, get: %x \n", arg, 1 << (12+order));
			cnt = 1;
			tmp = (1 << (12+order-cnt));
			while (tmp < fbi->map_size) {
				//printk("tmp value: %x \n", tmp);
				cnt++;
				tmp += (1 << (12+order-cnt));
			}
			//printk("tmp value: %x \n", tmp);
			//printk("cnt value: %d \n", cnt);
			page = virt_to_page((void *)ret+tmp);
			atomic_inc(&page->_count);
			free_pages(ret+tmp, order-cnt);
			printk("reclaim size: %x \n", 1 << (12+order-cnt));
			printk("=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*\n");
		}

		if (record_insert(pli_signature | buddy_id | (cnt << 8) | order, (unsigned long)ret)) {
			//#else
			//if (record_insert(pli_signature | buddy_id | order, (unsigned long)ret)) {
			//#endif
			free_pages((unsigned long)ret, order);
			return 0;
		}
		
		data_cache_flush(ret, fbi->map_size);


	//if allocation still  failed	
	} else {

		return ret;//ret = 0

	}

	printk("ret = 0x%x\n",ret);

	/*virtual address used in kernel space*/
	fbi->fb.screen_base = ret /* + PAGE_SIZE*/;

	//printk("vneusfb_map_video_memory--------fbi->fb.screen_base=0x%x\n",fbi->fb.screen_base);
	/*virtual address used in user space*/
	fbi->map_virt_addr = (ret  & 0x0fffffff) + 0x44000000;//+0x8850;//xu_add

	fbi->map_phy_addr = (unsigned long)(ret & 0x0fffffff);

	//	fbi->fb.fix.smem_start = fbi->map_phy_addr;
	fbi->fb.fix.smem_start = __pa(fbi->fb.screen_base);

	//printk("fbi->map_virt_addr= 0x%x  \n fbi->map_phy_addr = 0x%x\n fbi->fb.fix.smem_start = 0x%x\n ",fbi->map_virt_addr,fbi->map_phy_addr,fbi->fb.fix.smem_start);
	
	return  fbi->fb.screen_base? 0 : -ENOMEM;


}




static ssize_t
venusfb_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	struct inode *inode = file->f_dentry->d_inode;
	int fbidx = iminor(inode);
	struct fb_info  *info = registered_fb[fbidx];
	char tmpbuf[TMPBUFLEN];//2048

	if (!info || ! info->screen_base)
		return -ENODEV;

	if (p >= info->fix.smem_len)
	    return 0;
	if (count >= info->fix.smem_len)
	    count = info->fix.smem_len;
	
	if (count + p > info->fix.smem_len)
		count = info->fix.smem_len - p;
	
	if (count > sizeof(tmpbuf))
		count = sizeof(tmpbuf);
	
	if (count) {
		
	    char *base_addr;
	    base_addr = info->screen_base;
	    memcpy_fromio(&tmpbuf, base_addr+p, count);
	    count -= copy_to_user(buf, &tmpbuf, count);
	    if (!count)
		return -EFAULT;
	    *ppos += count;
	}
	return count;
}




int venusfb_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{

	/** fixme **/
	return 0;

}



static ssize_t
venusfb_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	struct inode *inode = file->f_dentry->d_inode;
	int fbidx = iminor(inode);
	struct fb_info *info = registered_fb[fbidx];
	unsigned long p = *ppos;
	size_t c;
	int err;
	char tmpbuf[TMPBUFLEN];



	if (!info || !info->screen_base)
		return -ENODEV;

	if (p > info->fix.smem_len)
	    return -ENOSPC;
	
	if (count >= info->fix.smem_len)
		count = info->fix.smem_len;
		err = 0;
	
	if (count + p > info->fix.smem_len) {
	    count = info->fix.smem_len - p;
	    err = -ENOSPC;
	}

	p += (unsigned long)info->screen_base;
	c = count;
	
	while (c) {
		
	    int len = c > sizeof(tmpbuf) ? sizeof(tmpbuf) : c;
	    err = -EFAULT;
	    if (copy_from_user(&tmpbuf, buf, len))
		    break;
	    memcpy_toio(p, &tmpbuf, len);
	    c -= len;
	    p += len;
	    buf += len;
	    *ppos += len;
	}
	
	if (count-c){

		return (count-c);

	}

	return err;
}


static int
venusfb_setcolreg(u_int regno, u_int red, u_int green,
	      u_int blue, u_int transp, struct fb_info *info)
{


#if 0
	u32  color;

	if (regno >= 256)  /* no. of hw registers */
		return 1;

	red   >>= 8;
	green >>= 8;
	blue  >>= 8;

	#if 0
	if (info.var.grayscale) {
		/* gray = 0.30*R + 0.59*G + 0.11*B */
		color = ((red * 77) +
			 (green * 151) +
			 (blue * 28)) >> 8;
		
	} else {
	
		color = ((red << 16) |
			 (green << 8) |
			 (blue));
	}
	#endif

	if (info->var.bits_per_pixel == 32) {
		#if 0
		((u32 *)(info->pseudo_palette))[regno] =
			(red   << info->var.red.offset)   |
			(green << info->var.green.offset) |
			(blue  << info->var.blue.offset);
		#endif
	} else {
		#if 0
		((u32 *)(info->pseudo_palette))[regno] = regno;
		#endif
	}
		

	//PIXEL_FORMAT  pixelFormat  = ???//pass a value to it
		
	g_pixelFormat = Format_32_888A;
	
	if(g_pixelFormat == Format_32_888A)
			
		WriteReg(REG_SE_Clr_Format, Format_32 | 0x04);
	else
		WriteReg(REG_SE_Clr_Format, g_pixelFormat);


	if(g_pixelFormat == Format_32){
    
		WriteReg(REG_SE_Src_Alpha, color >> 24);
		
	}else if(g_pixelFormat == Format_32_888A){
    
		WriteReg(REG_SE_Src_Alpha, color & 0xFF);
		color = color >> 8;;
	}
	
	WriteReg(REG_SE_Src_Color, color);
#endif


}



/*
 * Note that we are entered with the kernel locked.
 */
static int
venusfb_mmap(struct fb_info *info, struct file *file, struct vm_area_struct *vma)
{
	printk("venusfb_mmap ************\n");
	vma->vm_start = global_fbi.map_virt_addr;//0x43300000;
	vma->vm_end =  vma->vm_start + VIDEO_MEMORY_LENGTH;//0x43319000;
       return 0;
}


/*
*  		Pan or Wrap the Display
*
*  		This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
*/
static int venusfb_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *info)
{

#if 0
	if (var->vmode & FB_VMODE_YWRAP) {
		if (var->yoffset < 0
		    || var->yoffset >= info->var.yres_virtual
		    || var->xoffset)
			return -EINVAL;
	} else {
		if (var->xoffset + var->xres > info->var.xres_virtual ||
		    var->yoffset + var->yres > info->var.yres_virtual)
			return -EINVAL;
	}
	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;
	if (var->vmode & FB_VMODE_YWRAP)
		info->var.vmode |= FB_VMODE_YWRAP;
	else
		info->var.vmode &= ~FB_VMODE_YWRAP;
#endif

	return 0;
}




static int
venusfb_blank(int blank_mode, struct fb_info *info)
{
	struct venusfb_info *fb = (struct venusfb_info *) info;
	//int enable = (blank_mode == 0) ? ENABLE : DISABLE;

	/*FIXME*/
	return 0;
}


/* This routine actually sets the video mode. It's in here where we
 * the hardware state info->par and fix which can be affected by the 
 * change in par. For this driver it doesn't do much. 
 */
static int venusfb_set_par(struct fb_info *info)
{
	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);
	return 0;
}


/*
 *   venusfb_check_var():
 *    Round up in the following order: bits_per_pixel, xres,
 *    yres, xres_virtual, yres_virtual, xoffset, yoffset, grayscale,
 *    bitfields, horizontal timing, vertical timing.
 */
static int
venusfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct venusfb_info *fbi = (struct venusfb_info *)info;
	int rgbidx;

	if (var->xres < MIN_XRES)
		var->xres = MIN_XRES;
	if (var->yres < MIN_YRES)
		var->yres = MIN_YRES;
	if (var->xres > fbi->max_xres)
		var->xres = fbi->max_xres;
	if (var->yres > fbi->max_yres)
		var->yres = fbi->max_yres;
	
	var->xres_virtual = max(var->xres_virtual, var->xres);
	var->yres_virtual = max(var->yres_virtual, var->yres);

	DBG_PRINT("var->bits_per_pixel=%d\n", var->bits_per_pixel);
	
	switch (var->bits_per_pixel) {
		
	case 4:
		rgbidx = RGB_8;
		break;
	case 8:
		rgbidx = RGB_8;
		break;
	case 16:
		rgbidx = RGB_16;
		break;
	case 32:
		rgbidx = RGB_32;
		
	default:
		return -EINVAL;
	}

	/*
	 * Copy the RGB parameters for this display
	 * from the machine specific parameters.
	 */
	var->red    = fbi->rgb[rgbidx]->red;
	var->green  = fbi->rgb[rgbidx]->green;
	var->blue   = fbi->rgb[rgbidx]->blue;
	var->transp	= fbi->rgb[rgbidx]->transp;

	DBG_PRINT("RGBT length = %d:%d:%d:%d\n",
	var->red.length, var->green.length, var->blue.length,
		var->transp.length);

	//DBG_PRINT("while loop w=%x r=%x\n", (uint32_t)writeptr, (uint32_t)readptr);

	DBG_PRINT("RGBT offset = %d:%d:%d:%d\n",
		var->red.offset, var->green.offset, var->blue.offset,
		var->transp.offset);

	return 0;
}


extern void cfb_copyarea(struct fb_info *p, const struct fb_copyarea *area);
extern  void cfb_imageblit(struct fb_info *p, const struct fb_image *image);
extern  int soft_cursor(struct fb_info *info, struct fb_cursor *cursor);


/* ------------ Interfaces to hardware functions ------------ */

static struct fb_ops venusfb_ops = {
	.owner		= THIS_MODULE,
	//.fb_open        = venusfb_open,
	.fb_read		= venusfb_read,
	.fb_write		= venusfb_write,
	.fb_check_var	= venusfb_check_var,
	.fb_set_par	= venusfb_set_par,
	.fb_setcolreg	= venusfb_setcolreg,
	.fb_blank		= venusfb_blank,
	//.fb_ioctl       = venusfb_ioctl,
	//.fb_fillrect	= 	cfb_fillrect,
	//.fb_copyarea	= cfb_copyarea,
	//.fb_imageblit	= cfb_imageblit,
	//.fb_cursor	= soft_cursor,
	.fb_mmap	= venusfb_mmap,
	.fb_pan_display =venusfb_pan_display,
};


/* Fake monspecs to fill in fbinfo structure */
static struct fb_monspecs  monspecs __initdata = {
	.hfmin	= 30000,
	.hfmax	= 70000,
	.vfmin	= 50,
	.vfmax	= 65,
};

/*
 *  Initialization
 */
static struct venusfb_info *  __init  venusfb_init_fbinfo(struct device *dev)
{
	struct venusfb_info  *fbi;
	struct venusfb_mach_info   *inf;

	printk("now: start venusfb_init_fbinfo(struct device *dev)\n");
	fbi = kmalloc(sizeof(struct venusfb_info) + sizeof(u32) * 16,GFP_KERNEL);
	if (!fbi){
		printk("error:can't allocate space for fbi from kerenl\n");
		return -2;
	}

	memset(fbi, 0, sizeof(struct venusfb_info) + sizeof(u32) * 16);
	fbi->dev = dev;// now I don't use the 'dev'

	strcpy(fbi->fb.fix.id, VENUS1261);

	fbi->fb.fix.type	= FB_TYPE_PACKED_PIXELS;
	fbi->fb.fix.type_aux	= 0;
	fbi->fb.fix.xpanstep	= 0;
	fbi->fb.fix.ypanstep	= 0;
	fbi->fb.fix.ywrapstep	= 0;
	fbi->fb.fix.accel	=   FB_ACCEL_NONE;//FB_ACCEL_ATI_RAGE128;//xu_change //

	fbi->fb.var.nonstd	= 0;
	fbi->fb.var.activate	= FB_ACTIVATE_NOW;
	fbi->fb.var.height	= -1;
	fbi->fb.var.width	= -1;
	fbi->fb.var.accel_flags	= 0;
	fbi->fb.var.vmode	= FB_VMODE_NONINTERLACED;

	fbi->fb.fbops		= &venusfb_ops;
	fbi->fb.flags		= FBINFO_DEFAULT;
	fbi->fb.monspecs	= monspecs;
	//fbi->fb.pseudo_palette	= (fbi + 1);
	
	fbi->rgb[RGB_8]	= &rgb_8;
	fbi->rgb[RGB_16]	= &rgb_16;
	fbi->rgb[RGB_24]	= &rgb_24;
	fbi->rgb[RGB_32]	= &rgb_32;

	inf = venusfb_get_machine_info(&venus_video_info);


	fbi->max_bpp				= inf->bpp;
	fbi->max_yres			= inf->yres;
	fbi->max_xres			= inf->xres;
	
	fbi->fb.var.xres			= inf->xres;
	fbi->fb.var.xres_virtual		= inf->xres;
	fbi->fb.var.yres			= inf->yres;
	fbi->fb.var.yres_virtual		= inf->yres;
	fbi->fb.var.bits_per_pixel	= inf->bpp;
	fbi->fb.var.pixclock		= inf->pixclock;
	fbi->fb.var.hsync_len		= inf->hsync_len;
	fbi->fb.var.vsync_len		= inf->vsync_len;	
	fbi->fb.var.left_margin		= inf->left_margin;
	fbi->fb.var.right_margin	= inf->right_margin;

	fbi->fb.var.upper_margin	= inf->upper_margin;
	fbi->fb.var.lower_margin	= inf->lower_margin;
	fbi->fb.var.sync			= inf->sync;
	
	//fbi->fb.var.grayscale		= inf->cmap_greyscale;
	//fbi->cmap_inverse		= inf->cmap_inverse;
	//fbi->cmap_static		= inf->cmap_static;

	//fbi->state			= C_STARTUP;
	//fbi->task_state			= (u_char)-1;

	fbi->fb.fix.smem_len		= (fbi->max_xres) *( fbi->max_yres) *(fbi->max_bpp)/8;
	fbi->fb.fix.line_length   	= 1536;//get_line_length(fbi->fb.var.xres_virtual,fbi->fb.var.bits_per_pixel);
	
	printk(" now : end  venusfb_init_fbinfo:\n");
	printk("fbi->fb.fix.smem_len = %d\n",fbi->fb.fix.smem_len);
	printk("fb.fix.line_length = %d\n",fbi->fb.fix.line_length);
	
	//init_waitqueue_head(&fbi->ctrlr_wait);
	//INIT_WORK(&fbi->task, sa1100fb_task, fbi);
	//init_MUTEX(&fbi->venus_sem);

	return fbi;
	
}



static int __init venusfb_probe(struct  device *dev)
{
	struct venusfb_info *fbi = NULL;
	int ret;
	int  lPitch;
		
	/*initiate   fbi*/
	fbi = venusfb_init_fbinfo(dev);
	if (!fbi){
		printk("error: can't allocate memory for fbi\n");
		return -1;
	}

	/*setup  se*/
	//setup_se();//del 20080123

	/* Initialize video memory */
	ret = vneusfb_map_video_memory(fbi);
	if (ret)
		goto failed;

	
	/*
	 * This makes sure that our colour bitfield
	 * descriptors are correctly initialised.
	 */
	venusfb_check_var(&fbi->fb.var, &fbi->fb);

	/* set dcu register */	
	lPitch = get_line_length(fbi->fb.var.xres, fbi->max_bpp);
	lPitch = 	((lPitch+0xff)>>8)<<8;
	printk("lPitch = %d\n",lPitch);
	//set_dcu_mem(0, fbi->map_phy_addr,lPitch);


	/**** register my framebuffer to  /dev/fb/0 ******/
	ret = register_framebuffer(&fbi->fb);
	if (ret < 0)
		goto failed;

	global_fbi = *fbi;

	
	//kfree(fbi);//a big bug

	return 0;

failed:

	printk("justin:  register_framebuffer --failed:\n");
	kfree(fbi);
	return ret;

}


int __init
venusfb_init(void)
{

	if (fb_get_options("venus1261fb", NULL))
		return -ENODEV;
	
	venusfb_probe(NULL);
	
}


/*
 *  Cleanup
 */
static void __exit
venusfb_cleanup(void)
{

	struct venusfb_info *fbi = NULL;
	printk("venusfb_cleanup(void)\n");
	fbi = &global_fbi ;
	kfree(fbi->fb.screen_base);
	unregister_framebuffer(&fbi->fb);
	kfree(fbi);

}

module_init(venusfb_init);
module_exit(venusfb_cleanup);

MODULE_AUTHOR("justin_xu  <yiping_xu@realsil.com.cn>");
MODULE_DESCRIPTION("Framebuffer driver for venus1261");
MODULE_LICENSE("GPL v2");


