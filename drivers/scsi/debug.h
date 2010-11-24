//#define MARS_DEBUG//////////////////////
#ifdef MARS_DEBUG
#define marsinfo(fmt, args...) \
           printk(KERN_INFO "mars info: " fmt, ## args)
#else
#define marsinfo(fmt, args...)
#endif

//#define MARS_LOLO_DEBUG///////////////////////
#ifdef MARS_LOLO_DEBUG
#define marslolo(fmt, args...) \
           printk(KERN_INFO "mars info: " fmt, ## args)
#else
#define marslolo(fmt, args...)
#endif

//#define MARS_FLOW_DEBUG//////////////////////
#ifdef MARS_FLOW_DEBUG
#define marsflow(fmt, args...) \
           printk(KERN_INFO "mars info: " fmt, ## args)
#else
#define marsflow(fmt, args...)
#endif

//#define SCSI_DISK_DEBUG/////////////////==>sd.c
#ifdef SCSI_DISK_DEBUG
#define scsidisk(fmt, args...) \
           printk(KERN_INFO "scsi disk: " fmt, ## args)
#else
#define scsidisk(fmt, args...)
#endif

//#define SCSI_CD_DEBUG/////////////////==>sr.c
#ifdef SCSI_CD_DEBUG
#define scsicd(fmt, args...) \
           printk(KERN_INFO "scsi cd: " fmt, ## args)
#else
#define scsicd(fmt, args...)
#endif

//#define MARS_TEMP_DEBUG//////////////////////
#ifdef MARS_TEMP_DEBUG
#define marstemp(fmt, args...) \
           printk(KERN_INFO "mars info: " fmt, ## args)
#else
#define marstemp(fmt, args...)
#endif


//#define SCSI_DEBUG//////////////////////==>scsi.c
#ifdef SCSI_DEBUG
#define scsi_debug(fmt, args...) \
           printk(KERN_INFO "scsi.c:" fmt, ## args)
#else
#define scsi_debug(fmt, args...)
#endif

//#define LIB_DEBUG//////////////////////==>scsi_lib.c
#ifdef LIB_DEBUG
#define lib_debug(fmt, args...) \
           printk(KERN_INFO "scsi_lib:" fmt, ## args)
#else
#define lib_debug(fmt, args...)
#endif

//#define ERROR_DEBUG//////////////////////==>scsi_error.c
#ifdef ERROR_DEBUG
#define error_debug(fmt, args...) \
           printk(KERN_INFO "scsi_error:" fmt, ## args)
#else
#define error_debug(fmt, args...)
#endif

//#define SCAN_DEBUG//////////////////////==>scsi_scan.c
#ifdef SCAN_DEBUG
#define scan_debug(fmt, args...) \
           printk(KERN_INFO "scsi_scan:" fmt, ## args)
#else
#define scan_debug(fmt, args...)
#endif

//#define HOST_DEBUG//////////////////////==>host.c
#ifdef HOST_DEBUG
#define host_debug(fmt, args...) \
           printk(KERN_INFO "scsi_host:" fmt, ## args)
#else
#define host_debug(fmt, args...)
#endif

//#define IOCTL_DEBUG//////////////////////==>scsi_ioctl.c
#ifdef IOCTL_DEBUG
#define ioctl_debug(fmt, args...) \
           printk(KERN_INFO "scsi_ioctl:" fmt, ## args)
#else
#define ioctl_debug(fmt, args...)
#endif

//////////////////////////////
#define CHG_BUF_LEN             /*001*/
#define POLLING_IPF             /*002*/
//#define SHOW_SY_CMD           /*006*/
#define FORCE_SYNC              /*007*/
//#define SHOW_TF_DATA          /*010*/
//#define SHOW_PRD              /*011*/
//#define SHOW_IPF                /*017*/
#define HEAT_DOWN12p5           /*018*/
//#define HEAT_DOWN25           /*019*/
//#define WAIT_PHY_CAL          /*023*/
#define CHK_PHY_STA             /*024*/
#define CONFIG_MARS_PM        	/*025*/
//#define EN_SSC        		/*026*/

//#define TEST_MDIO        		/*027*/



