#define MIS_ISR					((volatile unsigned int *)0xb801b00c)
#define MIS_DUMMY				((volatile unsigned int *)0xb801b030)
#define MIS_IR_PSR				((volatile unsigned int *)0xb801b400)
#define MIS_IR_PER				((volatile unsigned int *)0xb801b404)
#define MIS_IR_SF				((volatile unsigned int *)0xb801b408)
#define MIS_IR_DPIR				((volatile unsigned int *)0xb801b40c)
#define MIS_IR_CR				((volatile unsigned int *)0xb801b410)
#define MIS_IR_RP				((volatile unsigned int *)0xb801b414)
#define MIS_IR_SR				((volatile unsigned int *)0xb801b418)
#define MIS_IR_RAW_CTRL			((volatile unsigned int *)0xb801b41c)
#define MIS_IR_RAW_FF			((volatile unsigned int *)0xb801b420)
#define MIS_IR_RAW_SAMPLE_TIME	((volatile unsigned int *)0xb801b424)
#define MIS_IR_RAW_WL			((volatile unsigned int *)0xb801b428)
#define MIS_IR_RAW_DEB			((volatile unsigned int *)0xb801b42c)

#define VENUS_IR_IRQ   3       /* HW1 +2 = 3 */

#define VENUS_IR_MAJOR			234
#define VENUS_IR_DEVICE_NUM		1
#define VENUS_IR_MINOR_RP		50
#define VENUS_IR_DEVICE_FILE	"venus_irrp"

#define VENUS_IR_IOC_MAGIC			'r'
#define VENUS_IR_IOC_SET_IRIOTCDP		_IOW(VENUS_IR_IOC_MAGIC, 1, int)
#define VENUS_IR_IOC_FLUSH_IRRP			_IOW(VENUS_IR_IOC_MAGIC, 2, int)
#define VENUS_IR_IOC_SET_PROTOCOL		_IOW(VENUS_IR_IOC_MAGIC, 3, int)
#define VENUS_IR_IOC_SET_DEBOUNCE		_IOW(VENUS_IR_IOC_MAGIC, 4, int)
#define VENUS_IR_IOC_SET_IRPSR			_IOW(VENUS_IR_IOC_MAGIC, 5, int)
#define VENUS_IR_IOC_SET_IRPER			_IOW(VENUS_IR_IOC_MAGIC, 6, int)
#define VENUS_IR_IOC_SET_IRSF			_IOW(VENUS_IR_IOC_MAGIC, 7, int)
#define VENUS_IR_IOC_SET_IRCR			_IOW(VENUS_IR_IOC_MAGIC, 8, int)
#define VENUS_IR_IOC_SET_DRIVER_MODE	_IOW(VENUS_IR_IOC_MAGIC, 9, int)
#define VENUS_IR_IOC_MAXNR			9
