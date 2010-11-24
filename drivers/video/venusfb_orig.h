#ifndef    _VENUSFB_H_
#define    _VENUSFB_H_

#include    "se_driver.h"
#include  "dcu_reg.h"
#include  <linux/swap.h>
#define	VENUS1261    "venus1261"

#define	SE_IRQ 5

#define 	RGB_8	(0)
#define 	RGB_16	(1)
#define 	RGB_24	(2)
#define 	RGB_32	(3)

#define 	NR_RGB	4

#define CACHE_LINE_LENGTH	16
//typedef  int  irqreturn_t; 
typedef enum _PIXELFORMAT
{   
    Format_8,           //332
    Format_16,          //565
    Format_32,          //A888
    Format_32_888A,      //888A
    Format_2_IDX,
    Format_4_IDX,
    Format_8_IDX,
    Format_1555,
    Format_4444_ARGB,
    Format_4444_YUVA,
    Format_8888_YUVA,
    Format_5551,
    Format_4444_RGBA,
    NO_OF_PIXEL_FORMAT
} PIXEL_FORMAT;


struct fb_se_dev {
	//int initialized;
	void *CmdBuf;  /* Pointer to first quantum set */
	void *CmdBase;
	void *CmdLimit;
	int wrptr;
	int v_to_p_offset;
	int size;
	se_cmd_counter sw_counter;
	se_cmd_counter hw_counter;
	struct semaphore sem;     /* mutual exclusion semaphore     */
	struct semaphore empty_sem;     /* mutual exclusion semaphore     */
      struct cdev cdev;   /* Char device structure          */
};


/*
 * These are the bitfields for each
 * display depth that we support.
 */
struct venusfb_rgb {
	struct fb_bitfield	red;
	struct fb_bitfield	green;
	struct fb_bitfield	blue;
	struct fb_bitfield	transp;
};

struct venusfb_info {
	
	struct fb_info		fb;
	struct device		*dev;
	struct venusfb_rgb	*rgb[NR_RGB];
	
	u_int			max_bpp;
	u_int			max_xres;
	u_int			max_yres;

	/*
	 * These are the addresses we mapped
	 * the framebuffer memory region to.
	 */

	char *  			map_virt_addr;
	unsigned long			map_phy_addr;//dma_addr_t
	u_int				map_size;

	u_char *			screen_virt;
	u32*   		 	screen_phy;//dma_addr_t
	
	u16 *			palette_cpu;
	dma_addr_t		palette_dma;
	u_int				palette_size;
	
	u_int				cmap_inverse:1,
					cmap_static:1,
					unused:30;


#if 0
	volatile u_char		state;
	volatile u_char		task_state;
	//struct semaphore	ctr_venus_sem;
	wait_queue_head_t	ctr_venus_wait;
	struct work_struct	task;

#endif

	struct semaphore  venus_sem;

};

struct venusfb_mach_info{
	
	u_long		pixclock;
	u_short		xres;
	u_short		yres;

	u_char		bpp;
	u_char		hsync_len;
	u_char		left_margin;
	u_char		right_margin;

	u_char		vsync_len;
	u_char		upper_margin;
	u_char		lower_margin;
	u_char		sync;

	u_int		cmap_greyscale:1,
				cmap_inverse:1,
				cmap_static:1,
				unused:29;
};


/*
 * Minimum X and Y resolutions
 */
#define MIN_XRES	64
#define MIN_YRES	64
#endif














