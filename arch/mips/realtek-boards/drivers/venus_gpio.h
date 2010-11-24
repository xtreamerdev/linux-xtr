#define MIS_ISR				((volatile unsigned int *)0xb801b00c)
#define MIS_UMSK_ISR		((volatile unsigned int *)0xb801b008)
#define MIS_UMSK_ISR_GP0A	((volatile unsigned int *)0xb801b010)
#define MIS_UMSK_ISR_GP0DA	((volatile unsigned int *)0xb801b014)
#define MIS_UMSK_ISR_GP1A	((volatile unsigned int *)0xb801b038)
#define MIS_UMSK_ISR_GP1DA	((volatile unsigned int *)0xb801b03c)

#define MIS_GP0DIR		((volatile unsigned int *)0xb801b100)
#define MIS_GP1DIR		((volatile unsigned int *)0xb801b104)
#define MIS_GP0DAT0		((volatile unsigned int *)0xb801b108)
#define MIS_GP1DAT0		((volatile unsigned int *)0xb801b10c)
#define MIS_GP0DATI		((volatile unsigned int *)0xb801b110)
#define MIS_GP1DATI		((volatile unsigned int *)0xb801b114)
#define MIS_GP0IE		((volatile unsigned int *)0xb801b118)
#define MIS_GP1IE		((volatile unsigned int *)0xb801b11c)
#define MIS_GP0DP		((volatile unsigned int *)0xb801b120)
#define MIS_GP1DP		((volatile unsigned int *)0xb801b124)
#define MIS_GPDEB		((volatile unsigned int *)0xb801b128)

#define VENUS_GPIO_IRQ		3       /* HW1 +2 = 3 */

#define VENUS_GPIO_IOC_MAGIC		'r'
#define VENUS_GPIO_ENABLE_INT		_IOW(VENUS_GPIO_IOC_MAGIC, 1, int)
#define VENUS_GPIO_DISABLE_INT		_IOW(VENUS_GPIO_IOC_MAGIC, 2, int)
#define VENUS_GPIO_SET_ASSERT		_IOW(VENUS_GPIO_IOC_MAGIC, 3, int)
#define VENUS_GPIO_SET_DEASSERT		_IOW(VENUS_GPIO_IOC_MAGIC, 4, int)
#define VENUS_GPIO_SET_SW_DEBOUNCE	_IOW(VENUS_GPIO_IOC_MAGIC, 5, int) 
#define VENUS_GPIO_CHOOSE_GPDEB1	_IOW(VENUS_GPIO_IOC_MAGIC, 6, int)
#define VENUS_GPIO_CHOOSE_GPDEB2	_IOW(VENUS_GPIO_IOC_MAGIC, 7, int)
#define VENUS_GPIO_CHOOSE_GPDEB3	_IOW(VENUS_GPIO_IOC_MAGIC, 8, int)
#define VENUS_GPIO_CHOOSE_GPDEB4	_IOW(VENUS_GPIO_IOC_MAGIC, 9, int)
#define VENUS_GPIO_RESET_INT_STATUS	_IO(VENUS_GPIO_IOC_MAGIC, 10)

#define VENUS_GPIO_IOC_MAXNR	12
