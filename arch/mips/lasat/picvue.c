/* 
 * Picvue PVC160206 display driver
 *
 * Brian Murphy <brian@murphy.dk> 
 *
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/lasat/lasat.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>

#include "picvue.h"

#define PVC_BUSY		0x80
#define PVC_NLINES		2
#define PVC_DISPMEM		80
#define PVC_LINELEN		PVC_DISPMEM / PVC_NLINES

static struct {
	u32 *reg;
	u32 data_shift;
	u32 data_mask;
	u32 e;
	u32 rw;
	u32 rs;
} pvc_defs = { (void *)PVC_REG, PVC_DATA_SHIFT, PVC_DATA_M,
	PVC_E, PVC_RW, PVC_RS };

DECLARE_MUTEX(pvc_sem);

static inline void pvc_reg_write(u32 val) 
{
	*pvc_defs.reg = val;
}

static inline u32 pvc_reg_read(void) 
{
	u32 tmp = *pvc_defs.reg;
	return tmp;
}

static inline void pvc_write_byte(u32 data, u8 byte)
{
	data |= pvc_defs.e;
	pvc_reg_write(data);
	data &= ~pvc_defs.data_mask;
	data |= byte << pvc_defs.data_shift;
	pvc_reg_write(data);
	ndelay(220);
	pvc_reg_write(data & ~pvc_defs.e);
	ndelay(220);
}

static inline u8 pvc_read_byte(u32 data)
{
	u8 byte;

	data |= pvc_defs.e;
	pvc_reg_write(data);
	ndelay(220);
	byte = (pvc_reg_read() & pvc_defs.data_mask) >> pvc_defs.data_shift;
	data &= ~pvc_defs.e;
	pvc_reg_write(data);
	ndelay(220);
	return byte;
}

static inline u8 pvc_read_data(void)
{
	u32 data = pvc_reg_read();
	u8 byte;
	data |= pvc_defs.rw; 
	data &= ~pvc_defs.rs;
	pvc_reg_write(data);
	ndelay(40);
	byte = pvc_read_byte(data);
	data |= pvc_defs.rs; 
	pvc_reg_write(data);
	return byte;
}

#define TIMEOUT 1000
static inline int pvc_wait(void)
{
	int i = TIMEOUT;
	int err = 0;

	while ((pvc_read_data() & PVC_BUSY) && i)
		i--;
	if (i == 0)
		err = -ETIME;

	return err;
}

#define MODE_INST 0
#define MODE_DATA 1
static inline void pvc_write(u8 byte, int mode)
{
	u32 data = pvc_reg_read();
	data &= ~pvc_defs.rw;
	if (mode == MODE_DATA)
		data |= pvc_defs.rs;
	else
		data &= ~pvc_defs.rs;
	pvc_reg_write(data);
	ndelay(40);
	pvc_write_byte(data, byte);
	if (mode == MODE_DATA)
		data &= ~pvc_defs.rs;
	else
		data |= pvc_defs.rs;
	pvc_reg_write(data);
	pvc_wait();
}

void pvc_write_string(const unsigned char *str, u8 addr, int line) {
	int i = 0;

	if (line > 0 && (PVC_NLINES > 1))
		addr += 0x40 * line;
	pvc_write(0x80 | addr, MODE_INST);

	while (*str != 0 && i < PVC_LINELEN) {
		pvc_write(*str++, MODE_DATA);
		i++;
	}
}

#define BM_SIZE			8
#define MAX_PROGRAMMABLE_CHARS	8
int pvc_program_cg(int charnum, u8 bitmap[BM_SIZE]) {
	int i;
	int addr;

	if (charnum > MAX_PROGRAMMABLE_CHARS)
		return -ENOENT;

	addr = charnum * 8;
	pvc_write(0x40 | addr, MODE_INST);

	for (i=0; i<BM_SIZE; i++)
		pvc_write(bitmap[i], MODE_DATA);
	return 0;
}

#define FUNC_SET_CMD	0x20
#define  EIGHT_BYTE	(1 << 4)
#define  FOUR_BYTE	0
#define  TWO_LINES	(1 << 3)
#define  ONE_LINE	0
#define  LARGE_FONT	(1 << 2)
#define  SMALL_FONT	0
static inline void pvc_funcset(u8 cmd) {
	pvc_write(FUNC_SET_CMD | (cmd & (EIGHT_BYTE|TWO_LINES|LARGE_FONT)), MODE_INST);
}

#define ENTRYMODE_CMD		0x4
#define  AUTO_INC		(1 << 1)
#define  AUTO_DEC		0
#define  CURSOR_FOLLOWS_DISP	(1 << 0)
static inline void pvc_entrymode(u8 cmd) {
	pvc_write(ENTRYMODE_CMD | (cmd & (AUTO_INC|CURSOR_FOLLOWS_DISP)), MODE_INST);
}

#define DISP_CNT_CMD	0x08
#define  DISP_OFF	0
#define  DISP_ON	(1 << 2)
#define  CUR_ON		(1 << 1)
#define  CUR_BLINK	(1 << 0)
void inline pvc_dispcnt(u8 cmd) {
	pvc_write(DISP_CNT_CMD | (cmd & (DISP_ON|CUR_ON|CUR_BLINK)), MODE_INST);
}

#define MOVE_CMD	0x10
#define  DISPLAY	(1 << 3)
#define  CURSOR		0
#define  RIGHT		(1 << 2)
#define  LEFT		0
void pvc_move(u8 cmd) {
	pvc_write(MOVE_CMD | (cmd & (DISPLAY|RIGHT)), MODE_INST);
}

#define CLEAR_CMD	0x1
void pvc_clear(void) {
	pvc_write(CLEAR_CMD, MODE_INST);
}

#define HOME_CMD	0x2
void pvc_home(void) {
	pvc_write(HOME_CMD, MODE_INST);
}

int pvc_init(void)
{
	u8 cmd = EIGHT_BYTE;

	if (PVC_NLINES == 2)
		cmd |= (SMALL_FONT|TWO_LINES);
	else
		cmd |= (LARGE_FONT|ONE_LINE);
	pvc_funcset(cmd);
	pvc_dispcnt(DISP_ON);
	pvc_entrymode(AUTO_INC);

	return 0;
}

module_init(pvc_init);
MODULE_LICENSE("GPL");
