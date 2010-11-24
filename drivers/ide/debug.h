
#include <linux/mm.h>


#define NEPTUNE_MAX_SG_SEGMENT		31
#define NEPTUNE_DMA_BUF_LEN		(32*PAGE_SIZE)

// Added by Richard Kuo 10/18/2005
#define	VENUS_DMA_BUFFER

#ifdef VENUS_DMA_BUFFER
#define	VENUS_MAX_SG_SEGMENT            	31
#define	VENUS_DMA_BUF_LEN               	(32*PAGE_SIZE)
#endif
//***************************

//@ #define	IDE_FPGA_BOARD

// #define         IDE_1394_IRQ

// #define 	IDE_DEBUG
#ifdef IDE_DEBUG
#define ideinfo(fmt, args...) \
        if (ide_debug) \
            printk(KERN_INFO "ide info: " fmt, ## args)
#else
#define ideinfo(fmt, args...) 
#endif

// #define 	IDECD_DEBUG
#ifdef IDECD_DEBUG
#define idecdinfo(fmt, args...) \
           printk(KERN_INFO "ide-cd info: " fmt, ## args)
#else
#define idecdinfo(fmt, args...) 
#endif


//#define 	IDEDISK_DEBUG
#ifdef IDEDISK_DEBUG
#define idediskinfo(fmt, args...) \
           printk(KERN_INFO "ide-disk info: " fmt, ## args)
#else
#define idediskinfo(fmt, args...) 
#endif
