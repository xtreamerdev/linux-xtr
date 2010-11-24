#ifndef _LINUX_CPREG_H
#define _LINUX_CPREG_H


//#define 	CP_DEBUG

#ifdef CP_DEBUG
#define idecpinfo(fmt, args...) \
            printk(KERN_INFO "ide-cp info: " fmt, ## args)
#else
#define idecpinfo(fmt, args...) 
#endif

/* Content Protection ctl's  are numbered 0x880n */

#define CPIO_DECSS_ON	0x8801	/* put DECSS setting & turn on */
#define CPIO_DECSS_OFF	0x8802	/* turn DECSS off */

#endif	/* _LINUX_HDREG_H */
