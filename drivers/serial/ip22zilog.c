/*
 * ip22zilog.c
 *
 * Driver for Zilog serial chips found on SGI workstations and
 * servers.  This driver could actually be made more generic.
 *
 * This is based on the drivers/serial/sunzilog.c code as of 2.5.45 and the
 * old drivers/sgi/char/sgiserial.c code which itself is based of the original
 * drivers/sbus/char/zs.c code.  A lot of code has been simply moved over
 * directly from there but much has been rewritten.  Credits therefore go out
 * to David S. Miller, Eddie C. Dost, Peter Zaitcev, Ted Ts'o and Alex Buell
 * for their work there.
 *
 *  Copyright (C) 2002 David S. Miller (davem@redhat.com)
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/circ_buf.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/spinlock.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sgialib.h>
#include <asm/sgi/sgint23.h>
#include <asm/sgi/sgihpc.h>

#include <linux/serial_core.h>

#include "ip22zilog.h"

int ip22serial_current_minor = 64;

void ip22_do_break(void);

/*
 * On IP22 we need to delay after register accesses but we do not need to
 * flush writes.
 */
#define ZSDELAY()		udelay(5)
#define ZSDELAY_LONG()		udelay(20)
#define ZS_WSYNC(channel)	do { } while (0)

#define NUM_IP22ZILOG	1
#define NUM_CHANNELS	(NUM_IP22ZILOG * 2)

#define ZS_CLOCK		4915200 /* Zilog input clock rate. */
#define ZS_CLOCK_DIVISOR	16      /* Divisor this driver uses. */

/*
 * We wrap our port structure around the generic uart_port.
 */
struct uart_ip22zilog_port {
	struct uart_port		port;

	/* IRQ servicing chain.  */
	struct uart_ip22zilog_port	*next;

	/* Current values of Zilog write registers.  */
	unsigned char			curregs[NUM_ZSREGS];

	unsigned int			flags;
#define IP22ZILOG_FLAG_IS_CONS		0x00000004
#define IP22ZILOG_FLAG_IS_KGDB		0x00000008
#define IP22ZILOG_FLAG_MODEM_STATUS	0x00000010
#define IP22ZILOG_FLAG_IS_CHANNEL_A	0x00000020
#define IP22ZILOG_FLAG_REGS_HELD	0x00000040
#define IP22ZILOG_FLAG_TX_STOPPED	0x00000080
#define IP22ZILOG_FLAG_TX_ACTIVE	0x00000100

	unsigned int cflag;

	/* L1-A keyboard break state.  */
	int				kbd_id;
	int				l1_down;

	unsigned char			parity_mask;
	unsigned char			prev_status;
};

#define ZILOG_CHANNEL_FROM_PORT(PORT)	((struct zilog_channel *)((PORT)->membase))
#define UART_ZILOG(PORT)		((struct uart_ip22zilog_port *)(PORT))
#define IP22ZILOG_GET_CURR_REG(PORT, REGNUM)		\
	(UART_ZILOG(PORT)->curregs[REGNUM])
#define IP22ZILOG_SET_CURR_REG(PORT, REGNUM, REGVAL)	\
	((UART_ZILOG(PORT)->curregs[REGNUM]) = (REGVAL))
#define ZS_IS_CONS(UP)	((UP)->flags & IP22ZILOG_FLAG_IS_CONS)
#define ZS_IS_KGDB(UP)	((UP)->flags & IP22ZILOG_FLAG_IS_KGDB)
#define ZS_WANTS_MODEM_STATUS(UP)	((UP)->flags & IP22ZILOG_FLAG_MODEM_STATUS)
#define ZS_IS_CHANNEL_A(UP)	((UP)->flags & IP22ZILOG_FLAG_IS_CHANNEL_A)
#define ZS_REGS_HELD(UP)	((UP)->flags & IP22ZILOG_FLAG_REGS_HELD)
#define ZS_TX_STOPPED(UP)	((UP)->flags & IP22ZILOG_FLAG_TX_STOPPED)
#define ZS_TX_ACTIVE(UP)	((UP)->flags & IP22ZILOG_FLAG_TX_ACTIVE)

/* Reading and writing Zilog8530 registers.  The delays are to make this
 * driver work on the IP22 which needs a settling delay after each chip
 * register access, other machines handle this in hardware via auxiliary
 * flip-flops which implement the settle time we do in software.
 *
 * The port lock must be held and local IRQs must be disabled
 * when {read,write}_zsreg is invoked.
 */
static unsigned char read_zsreg(struct zilog_channel *channel,
				unsigned char reg)
{
	unsigned char retval;

	writeb(reg, &channel->control);
	ZSDELAY();
	retval = readb(&channel->control);
	ZSDELAY();

	return retval;
}

static void write_zsreg(struct zilog_channel *channel,
			unsigned char reg, unsigned char value)
{
	writeb(reg, &channel->control);
	ZSDELAY();
	writeb(value, &channel->control);
	ZSDELAY();
}

static void ip22zilog_clear_fifo(struct zilog_channel *channel)
{
	int i;

	for (i = 0; i < 32; i++) {
		unsigned char regval;

		regval = readb(&channel->control);
		ZSDELAY();
		if (regval & Rx_CH_AV)
			break;

		regval = read_zsreg(channel, R1);
		readb(&channel->data);
		ZSDELAY();

		if (regval & (PAR_ERR | Rx_OVR | CRC_ERR)) {
			writeb(ERR_RES, &channel->control);
			ZSDELAY();
			ZS_WSYNC(channel);
		}
	}
}

/* This function must only be called when the TX is not busy.  The UART
 * port lock must be held and local interrupts disabled.
 */
static void __load_zsregs(struct zilog_channel *channel, unsigned char *regs)
{
	int i;

	/* Let pending transmits finish.  */
	for (i = 0; i < 1000; i++) {
		unsigned char stat = read_zsreg(channel, R1);
		if (stat & ALL_SNT)
			break;
		udelay(100);
	}

	writeb(ERR_RES, &channel->control);
	ZSDELAY();
	ZS_WSYNC(channel);

	ip22zilog_clear_fifo(channel);

	/* Disable all interrupts.  */
	write_zsreg(channel, R1,
		    regs[R1] & ~(RxINT_MASK | TxINT_ENAB | EXT_INT_ENAB));

	/* Set parity, sync config, stop bits, and clock divisor.  */
	write_zsreg(channel, R4, regs[R4]);

	/* Set misc. TX/RX control bits.  */
	write_zsreg(channel, R10, regs[R10]);

	/* Set TX/RX controls sans the enable bits.  */
	write_zsreg(channel, R3, regs[R3] & ~RxENAB);
	write_zsreg(channel, R5, regs[R5] & ~TxENAB);

	/* Synchronous mode config.  */
	write_zsreg(channel, R6, regs[R6]);
	write_zsreg(channel, R7, regs[R7]);

	/* Don't mess with the interrupt vector (R2, unused by us) and
	 * master interrupt control (R9).  We make sure this is setup
	 * properly at probe time then never touch it again.
	 */

	/* Disable baud generator.  */
	write_zsreg(channel, R14, regs[R14] & ~BRENAB);

	/* Clock mode control.  */
	write_zsreg(channel, R11, regs[R11]);

	/* Lower and upper byte of baud rate generator divisor.  */
	write_zsreg(channel, R12, regs[R12]);
	write_zsreg(channel, R13, regs[R13]);
	
	/* Now rewrite R14, with BRENAB (if set).  */
	write_zsreg(channel, R14, regs[R14]);

	/* External status interrupt control.  */
	write_zsreg(channel, R15, regs[R15]);

	/* Reset external status interrupts.  */
	write_zsreg(channel, R0, RES_EXT_INT);
	write_zsreg(channel, R0, RES_EXT_INT);

	/* Rewrite R3/R5, this time without enables masked.  */
	write_zsreg(channel, R3, regs[R3]);
	write_zsreg(channel, R5, regs[R5]);

	/* Rewrite R1, this time without IRQ enabled masked.  */
	write_zsreg(channel, R1, regs[R1]);
}

/* Reprogram the Zilog channel HW registers with the copies found in the
 * software state struct.  If the transmitter is busy, we defer this update
 * until the next TX complete interrupt.  Else, we do it right now.
 *
 * The UART port lock must be held and local interrupts disabled.
 */
static void ip22zilog_maybe_update_regs(struct uart_ip22zilog_port *up,
				       struct zilog_channel *channel)
{
	if (!ZS_REGS_HELD(up)) {
		if (ZS_TX_ACTIVE(up)) {
			up->flags |= IP22ZILOG_FLAG_REGS_HELD;
		} else {
			__load_zsregs(channel, up->curregs);
		}
	}
}

static void ip22zilog_receive_chars(struct uart_ip22zilog_port *up,
				   struct zilog_channel *channel,
				   struct pt_regs *regs)
{
	struct tty_struct *tty = up->port.info->tty;

	while (1) {
		unsigned char ch, r1;

		if (unlikely(tty->flip.count >= TTY_FLIPBUF_SIZE)) {
			tty->flip.work.func((void *)tty);
			if (tty->flip.count >= TTY_FLIPBUF_SIZE)
				return;
		}

		r1 = read_zsreg(channel, R1);
		if (r1 & (PAR_ERR | Rx_OVR | CRC_ERR)) {
			writeb(ERR_RES, &channel->control);
			ZSDELAY();
			ZS_WSYNC(channel);
		}

		ch = readb(&channel->control);
		ZSDELAY();

		/* This funny hack depends upon BRK_ABRT not interfering
		 * with the other bits we care about in R1.
		 */
		if (ch & BRK_ABRT)
			r1 |= BRK_ABRT;

		ch = readb(&channel->data);
		ZSDELAY();

		ch &= up->parity_mask;

		if (ZS_IS_CONS(up) && (r1 & BRK_ABRT)) {
			/* Wait for BREAK to deassert to avoid potentially
			 * confusing the PROM.
			 */
			while (1) {
				ch = readb(&channel->control);
				ZSDELAY();
				if (!(ch & BRK_ABRT))
					break;
			}
			ip22_do_break();
			return;
		}

		/* A real serial line, record the character and status.  */
		*tty->flip.char_buf_ptr = ch;
		*tty->flip.flag_buf_ptr = TTY_NORMAL;
		up->port.icount.rx++;
		if (r1 & (BRK_ABRT | PAR_ERR | Rx_OVR | CRC_ERR)) {
			if (r1 & BRK_ABRT) {
				r1 &= ~(PAR_ERR | CRC_ERR);
				up->port.icount.brk++;
				if (uart_handle_break(&up->port))
					goto next_char;
			}
			else if (r1 & PAR_ERR)
				up->port.icount.parity++;
			else if (r1 & CRC_ERR)
				up->port.icount.frame++;
			if (r1 & Rx_OVR)
				up->port.icount.overrun++;
			r1 &= up->port.read_status_mask;
			if (r1 & BRK_ABRT)
				*tty->flip.flag_buf_ptr = TTY_BREAK;
			else if (r1 & PAR_ERR)
				*tty->flip.flag_buf_ptr = TTY_PARITY;
			else if (r1 & CRC_ERR)
				*tty->flip.flag_buf_ptr = TTY_FRAME;
		}
		if (uart_handle_sysrq_char(&up->port, ch, regs))
			goto next_char;

		if (up->port.ignore_status_mask == 0xff ||
		    (r1 & up->port.ignore_status_mask) == 0) {
			tty->flip.flag_buf_ptr++;
			tty->flip.char_buf_ptr++;
			tty->flip.count++;
		}
		if ((r1 & Rx_OVR) &&
		    tty->flip.count < TTY_FLIPBUF_SIZE) {
			*tty->flip.flag_buf_ptr = TTY_OVERRUN;
			tty->flip.flag_buf_ptr++;
			tty->flip.char_buf_ptr++;
			tty->flip.count++;
		}
	next_char:
		ch = readb(&channel->control);
		ZSDELAY();
		if (!(ch & Rx_CH_AV))
			break;
	}

	tty_flip_buffer_push(tty);
}

static void ip22zilog_status_handle(struct uart_ip22zilog_port *up,
				   struct zilog_channel *channel,
				   struct pt_regs *regs)
{
	unsigned char status;

	status = readb(&channel->control);
	ZSDELAY();

	writeb(RES_EXT_INT, &channel->control);
	ZSDELAY();
	ZS_WSYNC(channel);

	if (ZS_WANTS_MODEM_STATUS(up)) {
		if (status & SYNC)
			up->port.icount.dsr++;

		/* The Zilog just gives us an interrupt when DCD/CTS/etc. change.
		 * But it does not tell us which bit has changed, we have to keep
		 * track of this ourselves.
		 */
		if ((status & DCD) ^ up->prev_status)
			uart_handle_dcd_change(&up->port,
					       (status & DCD));
		if ((status & CTS) ^ up->prev_status)
			uart_handle_cts_change(&up->port,
					       (status & CTS));

		wake_up_interruptible(&up->port.info->delta_msr_wait);
	}

	up->prev_status = status;
}

static void ip22zilog_transmit_chars(struct uart_ip22zilog_port *up,
				    struct zilog_channel *channel)
{
	struct circ_buf *xmit = &up->port.info->xmit;

	if (ZS_IS_CONS(up)) {
		unsigned char status = readb(&channel->control);
		ZSDELAY();

		/* TX still busy?  Just wait for the next TX done interrupt.
		 *
		 * It can occur because of how we do serial console writes.  It would
		 * be nice to transmit console writes just like we normally would for
		 * a TTY line. (ie. buffered and TX interrupt driven).  That is not
		 * easy because console writes cannot sleep.  One solution might be
		 * to poll on enough port->xmit space becomming free.  -DaveM
		 */
		if (!(status & Tx_BUF_EMP))
			return;
	}

	if (ZS_REGS_HELD(up)) {
		__load_zsregs(channel, up->curregs);
		up->flags &= ~IP22ZILOG_FLAG_REGS_HELD;
	}

	if (ZS_TX_STOPPED(up)) {
		up->flags &= ~IP22ZILOG_FLAG_TX_STOPPED;
		goto disable_tx_int;
	}

	if (up->port.x_char) {
		writeb(up->port.x_char, &channel->data);
		ZSDELAY();
		ZS_WSYNC(channel);

		up->port.icount.tx++;
		up->port.x_char = 0;
		return;
	}

	if (uart_circ_empty(xmit) || uart_tx_stopped(&up->port))
		goto disable_tx_int;

	writeb(xmit->buf[xmit->tail], &channel->data);
	ZSDELAY();
	ZS_WSYNC(channel);

	xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
	up->port.icount.tx++;

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_event(&up->port, EVT_WRITE_WAKEUP);

	if (!uart_circ_empty(xmit))
		return;

disable_tx_int:
	up->curregs[R5] &= ~TxENAB;
	write_zsreg(ZILOG_CHANNEL_FROM_PORT(&up->port), R5, up->curregs[R5]);
}

static void ip22zilog_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct uart_ip22zilog_port *up = dev_id;

	while (up) {
		struct zilog_channel *channel
			= ZILOG_CHANNEL_FROM_PORT(&up->port);
		unsigned char r3;

		spin_lock(&up->port.lock);
		r3 = read_zsreg(channel, 3);

		/* Channel A */
		if (r3 & (CHAEXT | CHATxIP | CHARxIP)) {
			writeb(RES_H_IUS, &channel->control);
			ZSDELAY();
			ZS_WSYNC(channel);

			if (r3 & CHARxIP)
				ip22zilog_receive_chars(up, channel, regs);
			if (r3 & CHAEXT)
				ip22zilog_status_handle(up, channel, regs);
			if (r3 & CHATxIP)
				ip22zilog_transmit_chars(up, channel);
		}
		spin_unlock(&up->port.lock);

		/* Channel B */
		up = up->next;
		channel = ZILOG_CHANNEL_FROM_PORT(&up->port);

		spin_lock(&up->port.lock);
		if (r3 & (CHBEXT | CHBTxIP | CHBRxIP)) {
			writeb(RES_H_IUS, &channel->control);
			ZSDELAY();
			ZS_WSYNC(channel);

			if (r3 & CHBRxIP)
				ip22zilog_receive_chars(up, channel, regs);
			if (r3 & CHBEXT)
				ip22zilog_status_handle(up, channel, regs);
			if (r3 & CHBTxIP)
				ip22zilog_transmit_chars(up, channel);
		}
		spin_unlock(&up->port.lock);

		up = up->next;
	}
}

/* A convenient way to quickly get R0 status.  The caller must _not_ hold the
 * port lock, it is acquired here.
 */
static __inline__ unsigned char ip22zilog_read_channel_status(struct uart_port *port)
{
	struct zilog_channel *channel;
	unsigned long flags;
	unsigned char status;

	spin_lock_irqsave(&port->lock, flags);

	channel = ZILOG_CHANNEL_FROM_PORT(port);
	status = readb(&channel->control);
	ZSDELAY();

	spin_unlock_irqrestore(&port->lock, flags);

	return status;
}

/* The port lock is not held.  */
static unsigned int ip22zilog_tx_empty(struct uart_port *port)
{
	unsigned char status;
	unsigned int ret;

	status = ip22zilog_read_channel_status(port);
	if (status & Tx_BUF_EMP)
		ret = TIOCSER_TEMT;
	else
		ret = 0;

	return ret;
}

/* The port lock is not held.  */
static unsigned int ip22zilog_get_mctrl(struct uart_port *port)
{
	unsigned char status;
	unsigned int ret;

	status = ip22zilog_read_channel_status(port);

	ret = 0;
	if (status & DCD)
		ret |= TIOCM_CAR;
	if (status & SYNC)
		ret |= TIOCM_DSR;
	if (status & CTS)
		ret |= TIOCM_CTS;

	return ret;
}

/* The port lock is held and interrupts are disabled.  */
static void ip22zilog_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct uart_ip22zilog_port *up = (struct uart_ip22zilog_port *) port;
	struct zilog_channel *channel = ZILOG_CHANNEL_FROM_PORT(port);
	unsigned char set_bits, clear_bits;

	set_bits = clear_bits = 0;

	if (mctrl & TIOCM_RTS)
		set_bits |= RTS;
	else
		clear_bits |= RTS;
	if (mctrl & TIOCM_DTR)
		set_bits |= DTR;
	else
		clear_bits |= DTR;

	/* NOTE: Not subject to 'transmitter active' rule.  */ 
	up->curregs[R5] |= set_bits;
	up->curregs[R5] &= ~clear_bits;
	write_zsreg(channel, R5, up->curregs[R5]);
}

/* The port lock is held and interrupts are disabled.  */
static void ip22zilog_stop_tx(struct uart_port *port, unsigned int tty_stop)
{
	struct uart_ip22zilog_port *up = (struct uart_ip22zilog_port *) port;

	up->flags |= IP22ZILOG_FLAG_TX_STOPPED;
}

/* The port lock is held and interrupts are disabled.  */
static void ip22zilog_start_tx(struct uart_port *port, unsigned int tty_start)
{
	struct uart_ip22zilog_port *up = (struct uart_ip22zilog_port *) port;
	struct zilog_channel *channel = ZILOG_CHANNEL_FROM_PORT(port);
	unsigned char status;

	up->flags |= IP22ZILOG_FLAG_TX_ACTIVE;
	up->flags &= ~IP22ZILOG_FLAG_TX_STOPPED;

	/* Enable the transmitter.  */
	if (!(up->curregs[R5] & TxENAB)) {
		/* NOTE: Not subject to 'transmitter active' rule.  */ 
		up->curregs[R5] |= TxENAB;
		write_zsreg(channel, R5, up->curregs[R5]);
	}

	status = readb(&channel->control);
	ZSDELAY();

	/* TX busy?  Just wait for the TX done interrupt.  */
	if (!(status & Tx_BUF_EMP))
		return;

	/* Send the first character to jump-start the TX done
	 * IRQ sending engine.
	 */
	if (port->x_char) {
		writeb(port->x_char, &channel->data);
		ZSDELAY();
		ZS_WSYNC(channel);

		port->icount.tx++;
		port->x_char = 0;
	} else {
		struct circ_buf *xmit = &port->info->xmit;

		writeb(xmit->buf[xmit->tail], &channel->data);
		ZSDELAY();
		ZS_WSYNC(channel);

		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;

		if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
			uart_event(&up->port, EVT_WRITE_WAKEUP);
	}
}

/* The port lock is not held.  */
static void ip22zilog_stop_rx(struct uart_port *port)
{
	struct uart_ip22zilog_port *up = UART_ZILOG(port);
	struct zilog_channel *channel;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	channel = ZILOG_CHANNEL_FROM_PORT(port);

	/* Disable all RX interrupts.  */
	up->curregs[R1] &= ~RxINT_MASK;
	ip22zilog_maybe_update_regs(up, channel);

	spin_unlock_irqrestore(&port->lock, flags);
}

/* The port lock is not held.  */
static void ip22zilog_enable_ms(struct uart_port *port)
{
	struct uart_ip22zilog_port *up = (struct uart_ip22zilog_port *) port;
	struct zilog_channel *channel = ZILOG_CHANNEL_FROM_PORT(port);
	unsigned char new_reg;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	new_reg = up->curregs[R15] | (DCDIE | SYNCIE | CTSIE);
	if (new_reg != up->curregs[R15]) {
		up->curregs[R15] = new_reg;

		/* NOTE: Not subject to 'transmitter active' rule.  */ 
		write_zsreg(channel, R15, up->curregs[R15]);
	}

	spin_unlock_irqrestore(&port->lock, flags);
}

/* The port lock is not held.  */
static void ip22zilog_break_ctl(struct uart_port *port, int break_state)
{
	struct uart_ip22zilog_port *up = (struct uart_ip22zilog_port *) port;
	struct zilog_channel *channel = ZILOG_CHANNEL_FROM_PORT(port);
	unsigned char set_bits, clear_bits, new_reg;
	unsigned long flags;

	set_bits = clear_bits = 0;

	if (break_state)
		set_bits |= SND_BRK;
	else
		clear_bits |= SND_BRK;

	spin_lock_irqsave(&port->lock, flags);

	new_reg = (up->curregs[R5] | set_bits) & ~clear_bits;
	if (new_reg != up->curregs[R5]) {
		up->curregs[R5] = new_reg;

		/* NOTE: Not subject to 'transmitter active' rule.  */ 
		write_zsreg(channel, R5, up->curregs[R5]);
	}

	spin_unlock_irqrestore(&port->lock, flags);
}

static int ip22zilog_startup(struct uart_port *port)
{
	struct uart_ip22zilog_port *up = UART_ZILOG(port);
	struct zilog_channel *channel;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	channel = ZILOG_CHANNEL_FROM_PORT(port);
	up->prev_status = readb(&channel->control);

	/* Enable receiver and transmitter.  */
	up->curregs[R3] |= RxENAB;
	up->curregs[R5] |= TxENAB;

	/* Enable RX and status interrupts.  TX interrupts are enabled
	 * as needed.
	 */
	up->curregs[R1] |= EXT_INT_ENAB | INT_ALL_Rx;
	up->curregs[R9] |= MIE;
	ip22zilog_maybe_update_regs(up, channel);

	spin_unlock_irqrestore(&port->lock, flags);

	return 0;
}

static void ip22zilog_shutdown(struct uart_port *port)
{
	struct uart_ip22zilog_port *up = UART_ZILOG(port);
	struct zilog_channel *channel;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	channel = ZILOG_CHANNEL_FROM_PORT(port);

	/* Disable receiver and transmitter.  */
	up->curregs[R3] &= ~RxENAB;
	up->curregs[R5] &= ~TxENAB;

	/* Disable all interrupts and BRK assertion.  */
	up->curregs[R1] &= ~(EXT_INT_ENAB | TxINT_ENAB | RxINT_MASK);
	up->curregs[R5] &= ~SND_BRK;
	up->curregs[R9] &= ~MIE;
	ip22zilog_maybe_update_regs(up, channel);

	spin_unlock_irqrestore(&port->lock, flags);
}

/* Shared by TTY driver and serial console setup.  The port lock is held
 * and local interrupts are disabled.
 */
static void
ip22zilog_convert_to_zs(struct uart_ip22zilog_port *up, unsigned int cflag,
		       unsigned int iflag, int brg)
{

	/* Don't modify MIE. */
	up->curregs[R9] |= NV;

	up->curregs[R10] = NRZ;
	up->curregs[11] = TCBR | RCBR;

	/* Program BAUD and clock source. */
	up->curregs[4] &= ~XCLK_MASK;
	up->curregs[4] |= X16CLK;
	up->curregs[12] = brg & 0xff;
	up->curregs[13] = (brg >> 8) & 0xff;
	up->curregs[14] = BRSRC | BRENAB;

	/* Character size, stop bits, and parity. */
	up->curregs[3] &= ~RxN_MASK;
	up->curregs[5] &= ~TxN_MASK;
	switch (cflag & CSIZE) {
	case CS5:
		up->curregs[3] |= Rx5;
		up->curregs[5] |= Tx5;
		up->parity_mask = 0x1f;
		break;
	case CS6:
		up->curregs[3] |= Rx6;
		up->curregs[5] |= Tx6;
		up->parity_mask = 0x3f;
		break;
	case CS7:
		up->curregs[3] |= Rx7;
		up->curregs[5] |= Tx7;
		up->parity_mask = 0x7f;
		break;
	case CS8:
	default:
		up->curregs[3] |= Rx8;
		up->curregs[5] |= Tx8;
		up->parity_mask = 0xff;
		break;
	};
	up->curregs[4] &= ~0x0c;
	if (cflag & CSTOPB)
		up->curregs[4] |= SB2;
	else
		up->curregs[4] |= SB1;
	if (cflag & PARENB)
		up->curregs[4] |= PAR_ENAB;
	else
		up->curregs[4] &= ~PAR_ENAB;
	if (!(cflag & PARODD))
		up->curregs[4] |= PAR_EVEN;
	else
		up->curregs[4] &= ~PAR_EVEN;

	up->port.read_status_mask = Rx_OVR;
	if (iflag & INPCK)
		up->port.read_status_mask |= CRC_ERR | PAR_ERR;
	if (iflag & (BRKINT | PARMRK))
		up->port.read_status_mask |= BRK_ABRT;

	up->port.ignore_status_mask = 0;
	if (iflag & IGNPAR)
		up->port.ignore_status_mask |= CRC_ERR | PAR_ERR;
	if (iflag & IGNBRK) {
		up->port.ignore_status_mask |= BRK_ABRT;
		if (iflag & IGNPAR)
			up->port.ignore_status_mask |= Rx_OVR;
	}

	if ((cflag & CREAD) == 0)
		up->port.ignore_status_mask = 0xff;
}

/* The port lock is not held.  */
static void
ip22zilog_change_speed(struct uart_port *port, unsigned int cflag,
		      unsigned int iflag, unsigned int quot)
{
	struct uart_ip22zilog_port *up = (struct uart_ip22zilog_port *) port;
	unsigned long flags;
	int baud, brg;

	spin_lock_irqsave(&up->port.lock, flags);

	baud = (ZS_CLOCK / (quot * 16));
	brg = BPS_TO_BRG(baud, ZS_CLOCK / ZS_CLOCK_DIVISOR);

	ip22zilog_convert_to_zs(up, cflag, iflag, brg);

	if (UART_ENABLE_MS(&up->port, cflag))
		up->flags |= IP22ZILOG_FLAG_MODEM_STATUS;
	else
		up->flags &= ~IP22ZILOG_FLAG_MODEM_STATUS;

	up->cflag = cflag;

	ip22zilog_maybe_update_regs(up, ZILOG_CHANNEL_FROM_PORT(port));

	spin_unlock_irqrestore(&up->port.lock, flags);
}

static const char *ip22zilog_type(struct uart_port *port)
{
	return "IP22-Zilog";
}

/* We do not request/release mappings of the registers here, this
 * happens at early serial probe time.
 */
static void ip22zilog_release_port(struct uart_port *port)
{
}

static int ip22zilog_request_port(struct uart_port *port)
{
	return 0;
}

/* These do not need to do anything interesting either.  */
static void ip22zilog_config_port(struct uart_port *port, int flags)
{
}

/* We do not support letting the user mess with the divisor, IRQ, etc. */
static int ip22zilog_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	return -EINVAL;
}

static struct uart_ops ip22zilog_pops = {
	.tx_empty	=	ip22zilog_tx_empty,
	.set_mctrl	=	ip22zilog_set_mctrl,
	.get_mctrl	=	ip22zilog_get_mctrl,
	.stop_tx	=	ip22zilog_stop_tx,
	.start_tx	=	ip22zilog_start_tx,
	.stop_rx	=	ip22zilog_stop_rx,
	.enable_ms	=	ip22zilog_enable_ms,
	.break_ctl	=	ip22zilog_break_ctl,
	.startup	=	ip22zilog_startup,
	.shutdown	=	ip22zilog_shutdown,
	.change_speed	=	ip22zilog_change_speed,
	.type		=	ip22zilog_type,
	.release_port	=	ip22zilog_release_port,
	.request_port	=	ip22zilog_request_port,
	.config_port	=	ip22zilog_config_port,
	.verify_port	=	ip22zilog_verify_port,
};

static struct uart_ip22zilog_port *ip22zilog_port_table;
static struct zilog_layout **ip22zilog_chip_regs;
static int *ip22zilog_nodes;

static struct uart_ip22zilog_port *ip22zilog_irq_chain;
static int zilog_irq = -1;

static struct uart_driver ip22zilog_reg = {
	.owner		=	THIS_MODULE,
	.driver_name	=	"ttyS",
#ifdef CONFIG_DEVFS_FS
	.dev_name	=	"ttyS%d",
#else
	.dev_name	=	"ttyS",
#endif
	.major		=	TTY_MAJOR,
};

static void * __init alloc_one_table(unsigned long size)
{
	void *ret;

	ret = kmalloc(size, GFP_KERNEL);
	if (ret != NULL)
		memset(ret, 0, size);

	return ret;
}

static void __init ip22zilog_alloc_tables(void)
{
	ip22zilog_port_table = (struct uart_ip22zilog_port *)
		alloc_one_table(NUM_CHANNELS * sizeof(struct uart_ip22zilog_port));
	ip22zilog_chip_regs = (struct zilog_layout **)
		alloc_one_table(NUM_IP22ZILOG * sizeof(struct zilog_layout *));
	ip22zilog_nodes = (int *)
		alloc_one_table(NUM_IP22ZILOG * sizeof(int));

	if (ip22zilog_port_table == NULL ||
	    ip22zilog_chip_regs == NULL ||
	    ip22zilog_nodes == NULL) {
		panic("ip22zilog_init: Cannot alloc IP22-Zilog tables.");
	}
}

/* Get the address of the registers for IP22-Zilog instance CHIP.  */
static struct zilog_layout * __init get_zs(int chip)
{
	unsigned long base;

	if (chip < 0 || chip >= NUM_IP22ZILOG) {
		panic("IP22-Zilog: Illegal chip number %d in get_zs.", chip);
	}

	/* Not probe-able, hard code it. */
	base = (unsigned long) &hpc3mregs->ser1cmd;

	ip22zilog_nodes[chip] = 0;

	zilog_irq = SGI_SERIAL_IRQ;
	request_mem_region(base, 8, "IP22-Zilog");

	return (struct zilog_layout *) base;
}

#define ZS_PUT_CHAR_MAX_DELAY	2000	/* 10 ms */

static void ip22zilog_put_char(struct zilog_channel *channel, unsigned char ch)
{
	int loops = ZS_PUT_CHAR_MAX_DELAY;

	/* This is a timed polling loop so do not switch the explicit
	 * udelay with ZSDELAY as that is a NOP on some platforms.  -DaveM
	 */
	do {
		unsigned char val = readb(&channel->control);
		if (val & Tx_BUF_EMP) {
			ZSDELAY();
			break;
		}
		udelay(5);
	} while (--loops);

	writeb(ch, &channel->data);
	ZSDELAY();
	ZS_WSYNC(channel);
}

static void
ip22zilog_console_write(struct console *con, const char *s, unsigned int count)
{
	struct uart_ip22zilog_port *up = &ip22zilog_port_table[con->index];
	struct zilog_channel *channel = ZILOG_CHANNEL_FROM_PORT(&up->port);
	unsigned long flags;
	int i;

	spin_lock_irqsave(&up->port.lock, flags);
	for (i = 0; i < count; i++, s++) {
		ip22zilog_put_char(channel, *s);
		if (*s == 10)
			ip22zilog_put_char(channel, 13);
	}
	udelay(2);
	spin_unlock_irqrestore(&up->port.lock, flags);
}

static kdev_t ip22zilog_console_device(struct console *con)
{
	return mk_kdev(ip22zilog_reg.major, ip22zilog_reg.minor + con->index);
}

void
ip22serial_console_termios(struct console *con, char *options)
{
	int baud = 9600, bits = 8, cflag;
	int parity = 'n';
	int flow = 'n';
	char *dbaud;

	if (!serial_console)
		return;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	/*
	 * If the user doesn't supply a console=... option try to read the
	 * dbaud PROM variable - if this fails use 9600 baud and
	 * inform the user about the problem
	 */
	dbaud = ArcGetEnvironmentVariable("dbaud");
	if (dbaud)
		baud = simple_strtoul(dbaud, NULL, 10);
	else {
		printk("No dbaud variable, defaulting to 9600.\n");
		baud = 9600;
	}

	cflag = CREAD | HUPCL | CLOCAL;

	switch (baud) {
		case 150: cflag |= B150; break;
		case 300: cflag |= B300; break;
		case 600: cflag |= B600; break;
		case 1200: cflag |= B1200; break;
		case 2400: cflag |= B2400; break;
		case 4800: cflag |= B4800; break;
		case 9600: cflag |= B9600; break;
		case 19200: cflag |= B19200; break;
		case 38400: cflag |= B38400; break;
		default: baud = 9600; cflag |= B9600; break;
	}

	con->cflag = cflag | CS8;			/* 8N1 */
}

static int __init ip22zilog_console_setup(struct console *con, char *options)
{
	struct uart_ip22zilog_port *up = &ip22zilog_port_table[con->index];
	unsigned long flags;
	int baud, brg;

	printk("Console: ttyS%d (Zilog8530)\n",
	       (ip22zilog_reg.minor - 64) + con->index);

	/* Get firmware console settings.  */
	ip22serial_console_termios(con, options);

	/* Firmware console speed is limited to 150-->38400 baud so
	 * this hackish cflag thing is OK.
	 */
	switch (con->cflag & CBAUD) {
	case B150: baud = 150; break;
	case B300: baud = 300; break;
	case B600: baud = 600; break;
	case B1200: baud = 1200; break;
	case B2400: baud = 2400; break;
	case B4800: baud = 4800; break;
	default: case B9600: baud = 9600; break;
	case B19200: baud = 19200; break;
	case B38400: baud = 38400; break;
	};

	brg = BPS_TO_BRG(baud, ZS_CLOCK / ZS_CLOCK_DIVISOR);

	/*
	 * Temporary fix.
	 */
	spin_lock_init(&up->port.lock);

	spin_lock_irqsave(&up->port.lock, flags);

	up->curregs[R15] = BRKIE;

	ip22zilog_convert_to_zs(up, con->cflag, 0, brg);

	spin_unlock_irqrestore(&up->port.lock, flags);

	ip22zilog_set_mctrl(&up->port, TIOCM_DTR | TIOCM_RTS);
	ip22zilog_startup(&up->port);

	return 0;
}

static struct console ip22zilog_console = {
	.name	=	"ttyS",
	.write	=	ip22zilog_console_write,
	.device	=	ip22zilog_console_device,
	.setup	=	ip22zilog_console_setup,
	.flags	=	CON_PRINTBUFFER,
	.index	=	-1,
};

static int __init ip22zilog_console_init(void)
{
	int i;

	if (con_is_present())
		return 0;

	for (i = 0; i < NUM_CHANNELS; i++) {
		int this_minor = ip22zilog_reg.minor + i;

		if ((this_minor - 64) == (serial_console - 1))
			break;
	}
	if (i == NUM_CHANNELS)
		return 0;

	ip22zilog_console.index = i;
	register_console(&ip22zilog_console);
	return 0;
}

static void __init ip22zilog_prepare(void)
{
	struct uart_ip22zilog_port *up;
	struct zilog_layout *rp;
	int channel, chip;

	ip22zilog_irq_chain = up = &ip22zilog_port_table[0];
	for (channel = 0; channel < NUM_CHANNELS - 1; channel++)
		up[channel].next = &up[channel + 1];
	up[channel].next = NULL;

	for (chip = 0; chip < NUM_IP22ZILOG; chip++) {
		if (!ip22zilog_chip_regs[chip]) {
			ip22zilog_chip_regs[chip] = rp = get_zs(chip);

			up[(chip * 2) + 0].port.membase = (char *) &rp->channelA;
			up[(chip * 2) + 1].port.membase = (char *) &rp->channelB;
		}

		/* Channel A */
		up[(chip * 2) + 0].port.iotype = SERIAL_IO_MEM;
		up[(chip * 2) + 0].port.irq = zilog_irq;
		up[(chip * 2) + 0].port.uartclk = ZS_CLOCK;
		up[(chip * 2) + 0].port.fifosize = 1;
		up[(chip * 2) + 0].port.ops = &ip22zilog_pops;
		up[(chip * 2) + 0].port.type = PORT_IP22ZILOG;
		up[(chip * 2) + 0].port.flags = 0;
		up[(chip * 2) + 0].port.line = (chip * 2) + 0;
		up[(chip * 2) + 0].flags |= IP22ZILOG_FLAG_IS_CHANNEL_A;

		/* Channel B */
		up[(chip * 2) + 1].port.iotype = SERIAL_IO_MEM;
		up[(chip * 2) + 1].port.irq = zilog_irq;
		up[(chip * 2) + 1].port.uartclk = ZS_CLOCK;
		up[(chip * 2) + 1].port.fifosize = 1;
		up[(chip * 2) + 1].port.ops = &ip22zilog_pops;
		up[(chip * 2) + 1].port.type = PORT_IP22ZILOG;
		up[(chip * 2) + 1].port.flags = 0;
		up[(chip * 2) + 1].port.line = (chip * 2) + 1;
		up[(chip * 2) + 1].flags |= 0;
	}
}

static void __init ip22zilog_init_hw(void)
{
	int i;

	for (i = 0; i < NUM_CHANNELS; i++) {
		struct uart_ip22zilog_port *up = &ip22zilog_port_table[i];
		struct zilog_channel *channel = ZILOG_CHANNEL_FROM_PORT(&up->port);
		unsigned long flags;
		int baud, brg;

		spin_lock_irqsave(&up->port.lock, flags);

		if (ZS_IS_CHANNEL_A(up)) {
			write_zsreg(channel, R9, FHWRES);
			ZSDELAY_LONG();
			(void) read_zsreg(channel, R0);
		}

		if (ZS_IS_CONS(up)) {
			/* ip22zilog_console_setup takes care of this */
		} else {
			/* Normal serial TTY. */
			up->parity_mask = 0xff;
			up->curregs[R3] = RxENAB;
			up->curregs[R5] = TxENAB;
			up->curregs[R9] = NV | MIE;
			up->curregs[R10] = NRZ;
			up->curregs[R11] = TCBR | RCBR;
			baud = 9600;
			brg = BPS_TO_BRG(baud, (ZS_CLOCK / ZS_CLOCK_DIVISOR));
			up->curregs[R12] = (brg & 0xff);
			up->curregs[R13] = (brg >> 8) & 0xff;
			up->curregs[R14] = BRSRC | BRENAB;
			ip22zilog_maybe_update_regs(up, channel);
		}

		spin_unlock_irqrestore(&up->port.lock, flags);
	}
}

static int __init ip22zilog_ports_init(void)
{
	int ret;

	printk(KERN_INFO "Serial: IP22 Zilog driver.\n");

	ip22zilog_prepare();

	if (request_irq(zilog_irq, ip22zilog_interrupt, 0,
			"IP22-Zilog", ip22zilog_irq_chain)) {
		panic("IP22-Zilog: Unable to register zs interrupt handler.\n");
	}

	ip22zilog_init_hw();

	/* We can only init this once we have probed the Zilogs
	 * in the system.
	 */
	ip22zilog_reg.nr = NUM_CHANNELS;
	ip22zilog_reg.cons = &ip22zilog_console;

	ip22zilog_reg.minor = ip22serial_current_minor;
	ip22serial_current_minor += NUM_CHANNELS;

	ret = uart_register_driver(&ip22zilog_reg);
	if (ret == 0) {
		int i;

		for (i = 0; i < NUM_CHANNELS; i++) {
			struct uart_ip22zilog_port *up = &ip22zilog_port_table[i];

			uart_add_one_port(&ip22zilog_reg, &up->port);
		}
	}

	return ret;
}

static int __init ip22zilog_init(void)
{
	/* IP22 Zilog setup is hard coded, no probing to do.  */

	ip22zilog_alloc_tables();

	ip22zilog_ports_init();
	ip22zilog_console_init();

	return 0;
}

static void __exit ip22zilog_exit(void)
{
	int i;

	for (i = 0; i < NUM_CHANNELS; i++) {
		struct uart_ip22zilog_port *up = &ip22zilog_port_table[i];

		uart_remove_one_port(&ip22zilog_reg, &up->port);
	}

	uart_unregister_driver(&ip22zilog_reg);
}

module_init(ip22zilog_init);
module_exit(ip22zilog_exit);

EXPORT_NO_SYMBOLS;

/* David wrote it but I'm to blame for the bugs ...  */
MODULE_AUTHOR("Ralf Baechle <ralf@linux-mips.org>");
MODULE_DESCRIPTION("SGI Zilog serial port driver");
MODULE_LICENSE("GPL");
