/* 
 * Copyright (C) 2001, 2002 Jeff Dike (jdike@karaya.com)
 * Licensed under the GPL
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <sched.h>
#include <sys/socket.h>
#include "kern_util.h"
#include "chan_user.h"
#include "helper.h"
#include "user_util.h"
#include "user.h"
#include "os.h"

struct xterm_chan {
	int pid;
	int helper_pid;
	char *title;
	int device;
	int raw;
	struct termios tt;
	unsigned long stack;
};

void *xterm_init(char *str, int device, struct chan_opts *opts)
{
	struct xterm_chan *data;

	if((data = malloc(sizeof(*data))) == NULL) return(NULL);
	*data = ((struct xterm_chan) { pid :		-1, 
				       helper_pid :	-1,
				       device :		device, 
				       title :		opts->xterm_title,
				       raw : 		opts->raw,
				       stack :		opts->tramp_stack } );
	return(data);
}

/* Only changed by xterm_setup, which is a setup */
static char *terminal_emulator = "xterm";
static char *title_switch = "-T";
static char *exec_switch = "-e";

static int __init xterm_setup(char *line, int *add)
{
	*add = 0;
	terminal_emulator = line;

	line = strchr(line, ',');
	if(line == NULL) return(0);
	*line++ = '\0';
	if(*line) title_switch = line;

	line = strchr(line, ',');
	if(line == NULL) return(0);
	*line++ = '\0';
	if(*line) exec_switch = line;

	return(0);
}

__uml_setup("xterm=", xterm_setup,
"xterm=<terminal emulator>,<title switch>,<exec switch>\n"
"    Specifies an alternate terminal emulator to use for the debugger,\n"
"    consoles, and serial lines when they are attached to the xterm channel.\n"
"    The values are the terminal emulator binary, the switch it uses to set\n"
"    its title, and the switch it uses to execute a subprocess,\n"
"    respectively.  The title switch must have the form '<switch> title',\n"
"    not '<switch>=title'.  Similarly, the exec switch must have the form\n"
"    '<switch> command arg1 arg2 ...'.\n"
"    The default values are 'xterm=xterm,-T,-e'.  Values for gnome-terminal\n"
"    are 'xterm=gnome-terminal,-t,-x'.\n\n"
);

int xterm_open(int input, int output, int primary, void *d)
{
	struct xterm_chan *data = d;
	unsigned long stack;
	int pid, fd, new;
	char title[256], file[] = "/tmp/xterm-pipeXXXXXX";
	char *argv[] = { terminal_emulator, title_switch, title, exec_switch, 
			 "/usr/lib/uml/port-helper", "-uml-socket",
			 file, NULL };

	fd = mkstemp(file);
	if(fd < 0){
		printk("xterm_open : mkstemp failed, errno = %d\n", errno);
		return(-errno);
	}

	if(unlink(file)){
		printk("xterm_open : unlink failed, errno = %d\n", errno);
		return(-errno);
	}
	close(fd);

	fd = create_unix_socket(file, sizeof(file));
	if(fd < 0){
		printk("xterm_open : create_unix_socket failed, errno = %d\n", 
		       errno);
		return(-errno);
	}

	sprintf(title, data->title, data->device);
	stack = data->stack;
	pid = run_helper(NULL, NULL, argv, &stack);
	if(pid < 0){
		printk("xterm_open : run_helper failed\n");
		return(-1);
	}

	if(data->stack == 0) free_stack(stack, 0);

	new = os_rcv_fd(fd, &data->helper_pid);
	if(new < 0){
		printk("xterm_open : os_rcv_fd failed, errno = %d\n", -new);
		return(new);
	}

	tcgetattr(new, &data->tt);
	if(data->raw) raw(new, 0);

	data->pid = pid;
	return(new);
}

void xterm_close(int fd, void *d)
{
	struct xterm_chan *data = d;
	
	if(data->pid != -1) kill(data->pid, SIGKILL);
	data->pid = -1;
	if(data->helper_pid != -1) kill(data->helper_pid, SIGKILL);
	data->helper_pid = -1;
	close(fd);
}

void xterm_free(void *d)
{
	free(d);
}

int xterm_console_write(int fd, const char *buf, int n, void *d)
{
	struct xterm_chan *data = d;

	return(generic_console_write(fd, buf, n, &data->tt));
}

struct chan_ops xterm_ops = {
	init:		xterm_init,
	open:		xterm_open,
	close:		xterm_close,
	read:		generic_read,
	write:		generic_write,
	console_write:	xterm_console_write,
	window_size:	generic_window_size,
	free:		xterm_free,
	winch:		1,
};

/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
