#include <stdarg.h>
#include <linux/kernel.h>
#include <linux/sched.h>

extern void prom_putchar(char);

void prom_printf(char *fmt, ...)
{
	va_list args;
	char ppbuf[1024];
	char *bptr;
#ifdef CONFIG_PRINTK_TIME
	unsigned long long t;
	unsigned long nanosec_rem;
#endif

	va_start(args, fmt);
	vsprintf(ppbuf, fmt, args);

	bptr = ppbuf;

#ifdef CONFIG_PRINTK_TIME
	t = sched_clock();
	nanosec_rem = do_div(t, 1000000000);
#if 1
	sprintf(bptr+strlen(bptr), "[%5lu.%06lu] ", (unsigned long)t, nanosec_rem/1000);
#else
/* This can be used to measure the time by "Count" register. */
	sprintf(bptr+strlen(bptr), "[%lu] ", (unsigned long) read_c0_count());
#endif
#endif

	while (*bptr != 0) {
		if (*bptr == '\n')
			prom_putchar('\r');

		prom_putchar(*bptr++);
	}
	va_end(args);
}
