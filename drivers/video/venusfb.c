/*
 * linux/drivers/video/venusfb.c - 
 * 
 *
 * Modes / Resolution:
 *
 *      | 720x480  720x576  1280x720 1920x1080
 *  ----+-------------------------------------
 *  8bit|  0x101    0x201    0x401    0x801   
 * 16bit|  0x102    0x202    0x402    0x802   
 * 32bit|  0x103    0x204    0x404    0x804   
 *
 * args example:  video=venusfb:0x202 (this is default setting)
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

#include <platform.h>
#include "venusfb.h"

#define TMPBUFLEN   2048

#define DBG_PRINT(s, args...) printk(s, ##args)

static VENUSFB_MACH_INFO venus_video_info __initdata = {
	.pixclock       = 720*576*60,
	.xres           = 720,
	.yres           = 576,
	.bpp            = 16,
	.pitch			= 768*2,
	.videomemory	= NULL,
	.phyAddr		= NULL,
	.videomemorysize= 768 * 2 * 576,
	.videomemorysizeorder = 8, // 2^8 * 4KB = 1MB
	.storageMode	= 0,
};

static struct fb_monspecs monspecs __initdata = {
	.hfmin	= 30000,
	.hfmax	= 70000,
	.vfmin	= 50,
	.vfmax	= 65,
};

/*
 * Internal routines
 */

static void setup_venus_video_info(char *options) {
	char *this_opt;
	uint32_t MIN_TILE_HEIGHT;
	short height;

	// retrieve MIN_TILE_HEIGHT
	MIN_TILE_HEIGHT = 8 << ((readl(IOA_DC_SYS_MISC) >> 16) & 0x3);

	// default settings
	venus_video_info.xres = 720;
	venus_video_info.yres = 576;
	venus_video_info.bpp = 16;

	while((this_opt = strsep(&options, ",")) != NULL) {
		if(!*this_opt)
			continue;
		if(strncmp(this_opt, "0x101", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 480;
			venus_video_info.bpp = 8;
		}
		else if(strncmp(this_opt, "0x102", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 480;
			venus_video_info.bpp = 16;
		}
		else if(strncmp(this_opt, "0x104", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 480;
			venus_video_info.bpp = 32;
		}
		else if(strncmp(this_opt, "0x201", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 576;
			venus_video_info.bpp = 8;
		}
		else if(strncmp(this_opt, "0x202", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 576;
			venus_video_info.bpp = 16;
		}
		else if(strncmp(this_opt, "0x204", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 576;
			venus_video_info.bpp = 32;
		}
		else if(strncmp(this_opt, "0x401", 5) == 0) {
			venus_video_info.xres = 1280;
			venus_video_info.yres = 720;
			venus_video_info.bpp = 8;
		}
		else if(strncmp(this_opt, "0x402", 5) == 0) {
			venus_video_info.xres = 1280;
			venus_video_info.yres = 720;
			venus_video_info.bpp = 16;
		}
		else if(strncmp(this_opt, "0x404", 5) == 0) {
			venus_video_info.xres = 1280;
			venus_video_info.yres = 720;
			venus_video_info.bpp = 32;
		}
		else if(strncmp(this_opt, "0x801", 5) == 0) {
			venus_video_info.xres = 1920;
			venus_video_info.yres = 1080;
			venus_video_info.bpp = 8;
		}
		else if(strncmp(this_opt, "0x802", 5) == 0) {
			venus_video_info.xres = 1920;
			venus_video_info.yres = 1080;
			venus_video_info.bpp = 16;
		}
		else if(strncmp(this_opt, "0x804", 5) == 0) {
			venus_video_info.xres = 1280;
			venus_video_info.yres = 960;
			venus_video_info.bpp = 32;
		}
	}

	// tune height
	height = ((venus_video_info.yres - 1) / MIN_TILE_HEIGHT + 1) * MIN_TILE_HEIGHT;
	
	if(is_mars_cpu()) {
		venus_video_info.storageMode = 1; // sequential mode

    	// adjust pitch to the multiple of 16
		venus_video_info.pitch = ((venus_video_info.xres * (venus_video_info.bpp / 8) + 0x0F) >> 4 ) << 4;
	}
	else {
		uint32_t ii;

		venus_video_info.storageMode = 0; // block mode

    	// adjust pitch to the multiple of 256
		venus_video_info.pitch = ((venus_video_info.xres * (venus_video_info.bpp / 8) + 0xFF) >> 8 ) << 8;

		// adjust pitch to power of 2 in 256 bytes unit
		for(ii=256; ; ii*=2) {
			if(venus_video_info.pitch <= ii) {
				venus_video_info.pitch = ii;
				break;
			}
		}
	}

	// calculate pixel clock
	venus_video_info.pixclock = venus_video_info.xres * venus_video_info.yres * 60;

	// calculate required video memory size
	venus_video_info.videomemorysize =  venus_video_info.pitch * height;

	// calculate required videomemorysizeorder
	if(venus_video_info.videomemorysize == (1 << (fls(venus_video_info.videomemorysize) - 1)))
		venus_video_info.videomemorysizeorder = fls(venus_video_info.videomemorysize) - 13;
	else
		venus_video_info.videomemorysizeorder = fls(venus_video_info.videomemorysize) - 12;
}

//
// Frame Buffer Operation Functions 
//

static int
venusfb_open(struct fb_info *info, int user) 
{
	struct venusfb_info *fbi = (struct venusfb_info *) info;
	atomic_inc(&fbi->ref_count);

	return 0;
}

static int
venusfb_release(struct fb_info *info, int user)
{
	struct venusfb_info *fbi = (struct venusfb_info *) info;
	atomic_dec(&fbi->ref_count);

	return 0;
}

#if 0
static ssize_t
venusfb_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
};

static ssize_t 
venusfb_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
};

#endif

static int 
venusfb_ioctl(struct inode *inode, struct file *filp,
				unsigned int cmd, unsigned long arg, struct fb_info *info)
{
	struct venusfb_info *fbi = (struct venusfb_info *) info;

	int err = 0;
	int retval = 0;

	if (_IOC_TYPE(cmd) != VENUS_FB_IOC_MAGIC) 
		return -ENOTTY;
	else if (_IOC_NR(cmd) > VENUS_FB_IOC_MAXNR) 
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) 
		return -EFAULT;

	if (!capable (CAP_SYS_ADMIN))
		return -EPERM;

	switch(cmd) {
	case VENUS_FB_IOC_GET_MACHINE_INFO:
		if(copy_to_user(arg, &(fbi->video_info), sizeof(VENUSFB_MACH_INFO)) != 0)
			return -EFAULT;
		retval = 0;
		break;
	default:
		retval = -ENOIOCTLCMD;
	}

	return retval;
}

/*
 * Note that we are entered with the kernel locked.
 */

#if 0
static int venusfb_mmap(struct fb_info *info, struct file *file, struct vm_area_struct *vma)
{
	unsigned long off;
	unsigned long start;
	u32 len;

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	off = vma->vm_pgoff << PAGE_SHIFT;

	/* frame buffer memory */
	start = info->fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
	if (off >= len) {
		/* memory mapped io */
		off -= len;
		if (info->var.accel_flags) {
			unlock_kernel();
			return -EINVAL;
		}
		start = info->fix.mmio_start;
		len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.mmio_len);
	}
	start &= PAGE_MASK;
	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO | VM_RESERVED;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			     vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}
#endif

// sanity check
static int
venusfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct venusfb_info *fbi = (struct venusfb_info *) info;

	const struct venusfb_rgb rgb_8 = {
		.red	= { .offset = 0,  .length = 8, },
		.green	= { .offset = 0,  .length = 8, },
		.blue	= { .offset = 0,  .length = 8, },
		.transp = { .offset = 0,  .length = 0, },
	};

	const struct venusfb_rgb rgb_16 = {
		.red	= { .offset = 11, .length = 5, },
		.green	= { .offset = 5,  .length = 6, },
		.blue	= { .offset = 0,  .length = 5, },
		.transp = { .offset = 0,  .length = 0, },
	};

	const struct venusfb_rgb rgb_32 = {
		.red	= { .offset = 0,  .length = 8, },
		.green	= { .offset = 8,  .length = 8, },
		.blue	= { .offset = 16, .length = 8, },
		.transp = { .offset = 24, .length = 8, },
	};

	// resolution check
	if(var->xres == 720) {
		if(var->yres != 480 && var->yres != 576)
			var->yres = 576;
	} 
	else if(var->xres == 1280) {
		if(var->yres != 720)
			var->yres = 720;
	}
	else if(var->xres == 1920) {
		if(var->yres != 1080)
			var->yres = 1080;
	}
	else {
		var->xres = 720;
		var->yres = 576;
	}

	if(var->xres_virtual != var->xres)
		var->xres_virtual = var->xres;

	if(var->yres_virtual != var->yres)
		var->yres_virtual = var->yres;

	// bpp check
	if (var->bits_per_pixel <= 8)
		var->bits_per_pixel = 8;
	else if (var->bits_per_pixel <= 16)
		var->bits_per_pixel = 16;
	else if (var->bits_per_pixel <= 32)
		var->bits_per_pixel = 32;
	else
		return -EINVAL;

	// Memory limit check
	if((var->xres * var->yres * var->bits_per_pixel) < fbi->video_info.videomemorysize)
		return -ENOMEM;

	// RGB offset calibration
	switch(var->bits_per_pixel) {
	case 8:
		var->red	= rgb_8.red;
		var->green	= rgb_8.green;
		var->blue 	= rgb_8.blue;
		var->transp	= rgb_8.transp;
		break;
	default:
	case 16:
		var->red	= rgb_16.red;
		var->green	= rgb_16.green;
		var->blue 	= rgb_16.blue;
		var->transp	= rgb_16.transp;
		break;
	case 32:
		var->red	= rgb_32.red;
		var->green	= rgb_32.green;
		var->blue 	= rgb_32.blue;
		var->transp	= rgb_32.transp;
		break;		
	}

	var->xoffset		= 0;
	var->yoffset		= 0;
	var->grayscale		= 0;

	var->pixclock		= 1000*1000*1000 / fbi->video_info.pixclock; // in pico-second unit
	var->left_margin	= 0;
	var->right_margin	= 0;
	var->upper_margin	= 0;
	var->lower_margin	= 0;
	var->hsync_len		= 0;
	var->vsync_len		= 0;
	var->sync			= FB_SYNC_EXT;
	var->nonstd			= 0;
	var->activate		= FB_ACTIVATE_NOW;
	var->height			= 1;
	var->width			= 1;
	var->accel_flags	= 0;
	var->vmode			= FB_VMODE_NONINTERLACED;
	
	// sync with fbi->video_info
	fbi->video_info.pixclock	= var->xres * var->yres * 60;
	fbi->video_info.xres 		= var->xres;
	fbi->video_info.yres 		= var->yres;
	fbi->video_info.bpp			= var->bits_per_pixel;

	if(is_mars_cpu())
		fbi->video_info.pitch 		= ((fbi->video_info.xres * (fbi->video_info.bpp / 8) + 0x0F) >> 4 ) << 4;
	else {
		uint32_t ii;
		fbi->video_info.pitch 		= ((fbi->video_info.xres * (fbi->video_info.bpp / 8) + 0xFF) >> 8 ) << 8;

		// adjust pitch to power of 2 in 256 bytes unit
		for(ii=256; ; ii*=2) {
			if(fbi->video_info.pitch <= ii) {
				fbi->video_info.pitch = ii;
				break;
			}
		}
	}

	fbi->fb.fix.line_length = fbi->video_info.pitch;

	return 0;
}

static int venusfb_set_par(struct fb_info *info)
{
	return 0;
}

/* ------------ Interfaces to hardware functions ------------ */

static struct fb_ops venusfb_ops = {
	.owner			= THIS_MODULE,
	.fb_open        = venusfb_open,
	.fb_release		= venusfb_release,
	.fb_read		= NULL, // venusfb_read # enable if mmap doesn't work
	.fb_write		= NULL, // venusfb_write # enable if mmap doesn't work
	.fb_check_var	= venusfb_check_var,
	.fb_set_par		= venusfb_set_par,
	.fb_ioctl       = venusfb_ioctl,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor		= soft_cursor,
	.fb_mmap		= NULL, // venusfb_mmap,
};

/*
 *  Initialization
 */
static struct venusfb_info *  __init venusfb_init_fbinfo(struct device *dev)
{
	struct venusfb_info *fbi = NULL;
	const struct venusfb_rgb rgb_8 = {
		.red	= { .offset = 0,  .length = 8, },
		.green	= { .offset = 0,  .length = 8, },
		.blue	= { .offset = 0,  .length = 8, },
		.transp = { .offset = 0,  .length = 0, },
	};

	const struct venusfb_rgb rgb_16 = {
		.red	= { .offset = 11, .length = 5, },
		.green	= { .offset = 5,  .length = 6, },
		.blue	= { .offset = 0,  .length = 5, },
		.transp = { .offset = 0,  .length = 0, },
	};

	const struct venusfb_rgb rgb_32 = {
		.red	= { .offset = 0,  .length = 8, },
		.green	= { .offset = 8,  .length = 8, },
		.blue	= { .offset = 16, .length = 8, },
		.transp = { .offset = 24, .length = 8, },
	};

	// allocate memory for surface
	// modified by EJ
	if (!(venus_video_info.videomemory = dvr_malloc(PAGE_SIZE << venus_video_info.videomemorysizeorder))) {
		printk("VenusFB: __get_free_pages(%d) failed ..\n", venus_video_info.videomemorysizeorder);
		return fbi;
	}

	venus_video_info.phyAddr = (void *)(virt_to_phys(venus_video_info.videomemory) | SB2_GRAPHIC_OFFSET);
	memset(venus_video_info.videomemory, 0, venus_video_info.videomemorysize);

	// fill venusfb_info
	fbi = framebuffer_alloc(sizeof(struct venusfb_info) - sizeof(struct fb_info), dev);

	if (!fbi) {
		printk("VenusFB: framebuffer_alloc(%d) failed ..\n", sizeof(struct venusfb_info) - sizeof(struct fb_info));
		goto err;
	}

	memset(fbi, 0, sizeof(struct venusfb_info));
	fbi->dev 			= dev;
	fbi->map_virt_addr	= venus_video_info.videomemory;
	fbi->map_phy_addr 	= (void *)(virt_to_phys(venus_video_info.videomemory) | SB2_GRAPHIC_OFFSET);
	fbi->map_size		= venus_video_info.videomemorysize;
	fbi->video_info 	= venus_video_info;

	// fill [struct fb_fix_screeninfo] : Non-changeable properties 
	strcpy(fbi->fb.fix.id, "venusfb");
	fbi->fb.fix.smem_start	= (virt_to_phys(venus_video_info.videomemory) | SB2_GRAPHIC_OFFSET);
	fbi->fb.fix.smem_len	= venus_video_info.videomemorysize;
	fbi->fb.fix.type		= FB_TYPE_PACKED_PIXELS;
	fbi->fb.fix.type_aux	= 0;
	fbi->fb.fix.visual		= FB_VISUAL_TRUECOLOR;
	fbi->fb.fix.xpanstep	= 0;
	fbi->fb.fix.ypanstep	= 0;
	fbi->fb.fix.ywrapstep	= 0;
	fbi->fb.fix.line_length = venus_video_info.pitch;
	fbi->fb.fix.accel		= FB_ACCEL_NONE;

	// fill [struct fb_var_screeninfo] : This is used to set "feature"
	fbi->fb.var.xres			= venus_video_info.xres;
	fbi->fb.var.xres_virtual	= venus_video_info.xres;
	fbi->fb.var.yres			= venus_video_info.yres;
	fbi->fb.var.yres_virtual	= venus_video_info.yres;
	fbi->fb.var.xoffset			= 0;
	fbi->fb.var.yoffset			= 0;
	fbi->fb.var.bits_per_pixel	= venus_video_info.bpp;
	fbi->fb.var.grayscale		= 0;

	switch(fbi->fb.var.bits_per_pixel) {
	case 8:
		fbi->fb.var.red		= rgb_8.red;
		fbi->fb.var.green	= rgb_8.green;
		fbi->fb.var.blue 	= rgb_8.blue;
		fbi->fb.var.transp 	= rgb_8.transp;
		break;
	default:
	case 16:
		fbi->fb.var.red		= rgb_16.red;
		fbi->fb.var.green	= rgb_16.green;
		fbi->fb.var.blue 	= rgb_16.blue;
		fbi->fb.var.transp 	= rgb_16.transp;
		break;
	case 32:
		fbi->fb.var.red		= rgb_32.red;
		fbi->fb.var.green	= rgb_32.green;
		fbi->fb.var.blue 	= rgb_32.blue;
		fbi->fb.var.transp 	= rgb_32.transp;
		break;		
	}

	fbi->fb.var.pixclock		= 1000*1000*1000 / venus_video_info.pixclock; // in pico-second unit
	fbi->fb.var.left_margin		= 0;
	fbi->fb.var.right_margin	= 0;
	fbi->fb.var.upper_margin	= 0;
	fbi->fb.var.lower_margin	= 0;
	fbi->fb.var.hsync_len		= 0;
	fbi->fb.var.vsync_len		= 0;
	fbi->fb.var.sync			= FB_SYNC_EXT;
	fbi->fb.var.nonstd			= 0;
	fbi->fb.var.activate		= FB_ACTIVATE_NOW;
	fbi->fb.var.height			= 1;
	fbi->fb.var.width			= 1;
	fbi->fb.var.accel_flags		= 0;
	fbi->fb.var.vmode			= FB_VMODE_NONINTERLACED;

	// fbops for character device support
	fbi->fb.fbops				= &venusfb_ops;
	fbi->fb.flags				= FBINFO_DEFAULT;
	fbi->fb.monspecs			= monspecs;

	fbi->fb.screen_base			= (char __iomem *)venus_video_info.videomemory;
	fbi->fb.screen_size			= venus_video_info.videomemorysize;

err:
	return fbi;
}

static int __init venusfb_probe(struct device *dev)
{
	struct venusfb_info *fbi = NULL;
	int ret;

	/* initialize fbi */
	fbi = venusfb_init_fbinfo(dev);
	if (!fbi){
		printk("VenusFB: error: can't allocate memory for fbi\n");
		return -ENOMEM;
	}

	venusfb_check_var(&fbi->fb.var, &fbi->fb);

	// framebuffer device registeration
	ret = register_framebuffer(&fbi->fb);
	if (ret < 0)
		goto failed;

	printk("VenusFB: loading successful (%dx%dx%dbbp)\n", fbi->video_info.xres, fbi->video_info.yres, fbi->video_info.bpp);

	return 0;

failed:

	kfree(fbi);
	return ret;
}

static int venusfb_remove(struct device *device)
{
	struct fb_info *info = dev_get_drvdata(device);

	printk("VenusFB: removing..\n");
	if(info) {
		unregister_framebuffer(info);
		kfree(venus_video_info.videomemory);
		framebuffer_release(info);
	}
	return 0;
}

static struct device_driver venusfb_driver = {
	.name	= "venusfb",
	.bus	= &platform_bus_type,
	.probe	= venusfb_probe,
	.remove = venusfb_remove,
};

static struct platform_device venusfb_device = {
	.name	= "venusfb",
	.id		= 0,
};

/*
 * Init mem=
 */
static int __init venusfb_init(void)
{
	int ret;
	char *options = NULL;

	printk("VenusFB: Framebuffer device driver for Realtek Media Processors\n");

	if (fb_get_options("venusfb", &options))
		return -ENODEV;

	setup_venus_video_info(options);

	ret = driver_register(&venusfb_driver);

	if (!ret) {
		ret = platform_device_register(&venusfb_device);
		if (ret)
			driver_unregister(&venusfb_driver);
	}

	return ret;
}


/*
 * Cleanup
 */
static void __exit venusfb_cleanup(void)
{
	platform_device_unregister(&venusfb_device);
	driver_unregister(&venusfb_driver);
}

module_init(venusfb_init);
module_exit(venusfb_cleanup);

MODULE_AUTHOR("Chih-pin Wu, Realtek Semiconductor");
MODULE_DESCRIPTION("Framebuffer driver for Venus/Neptune/Mars");
MODULE_LICENSE("GPL v2");
