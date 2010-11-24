#ifndef    _VENUSFB_H_
#define    _VENUSFB_H_

#define IOA_DC_SYS_MISC ((volatile unsigned int *)0xb8008004)
#define SB2_GRAPHIC_OFFSET 0xe0000000

typedef enum _tagDisplayMode 
{
	DISPLAY_1080,
	DISPLAY_720,
	DISPLAY_576,
	DISPLAY_480
} DISPLAY_MODE;

typedef enum _PIXELFORMAT
{   
    Format_8,           //332
    Format_16,          //565
    Format_32,          //A888
    Format_32_888A,     //888A
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

typedef struct venusfb_mach_info {
	int		pixclock;
	short	xres;
	short	yres;
	char	bpp;					// in bits
	int		pitch;					// in bytes
	void 	*videomemory;			// virtual address (in-kernel)
	void	*phyAddr;				// physical address
	int		videomemorysize;		// in bytes
	int		videomemorysizeorder; 	// in 2^n (4KBytes unit)
    int		storageMode;            // 0 :  block mode, 1 : sequential mode
} VENUSFB_MACH_INFO;

struct venusfb_info {
	struct fb_info	fb;
	struct device	*dev;

	/*
	 * These are the addresses we mapped
	 * the framebuffer memory region to.
	 */

	char *  		map_virt_addr;
	void			*map_phy_addr;
	u_int			map_size;

	VENUSFB_MACH_INFO video_info;

	atomic_t 		ref_count;
};

/*
 * Minimum X and Y resolutions
 */

#define VENUS_FB_IOC_MAGIC				'f'
#define VENUS_FB_IOC_GET_MACHINE_INFO	_IOR(VENUS_FB_IOC_MAGIC, 1, struct venusfb_mach_info)
#define VENUS_FB_IOC_MAXNR				8

#define MIN_XRES	256
#define MIN_YRES	64
#endif
