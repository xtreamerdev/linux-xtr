/*
 * sysctl.c
 *
 * Thomas Horsten <thh@lasat.com>
 * Copyright (C) 2000 LASAT Networks A/S.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Routines specific to the LASAT boards
 */
#include <asm/lasat/lasat.h>

#include <linux/config.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "sysctl.h"
#include "ds1603.h"

static char lasat_firmware_version[176];

static DECLARE_MUTEX(lasat_info_sem);

/* Strategy function to write EEPROM after changing string entry */ 
int sysctl_lasatstring(ctl_table *table, int *name, int nlen,
		void *oldval, size_t *oldlenp,
		void *newval, size_t newlen, void **context)
{
	int r;
	down(&lasat_info_sem);
	r = sysctl_string(table, name, 
			  nlen, oldval, oldlenp, newval, newlen, context);
	if (r < 0) {
		up(&lasat_info_sem);
		return r;
	}
	if (newval && newlen) {
		lasat_write_eeprom_info();
	}
	up(&lasat_info_sem);
	return 1;
}


/* And the same for proc */
int proc_dolasatstring(ctl_table *table, int write, struct file *filp,
		       void *buffer, size_t *lenp)
{
	int r;
	down(&lasat_info_sem);
	r = proc_dostring(table, write, filp, buffer, lenp);
	if ( (!write) || r) {
		up(&lasat_info_sem);
		return r;
	}
	lasat_write_eeprom_info();
	up(&lasat_info_sem);
	return 0;
}

/* proc function to write EEPROM after changing int entry */ 
int proc_dolasatint(ctl_table *table, int write, struct file *filp,
		       void *buffer, size_t *lenp)
{
	int r;
	down(&lasat_info_sem);
	r = proc_dointvec(table, write, filp, buffer, lenp);
	if ( (!write) || r) {
		up(&lasat_info_sem);
		return r;
	}
	lasat_write_eeprom_info();
	up(&lasat_info_sem);
	return 0;
}

/* Same for vendor ID that's only a char in EEPROM struct */
int proc_dolasatvendor(ctl_table *table, int write, struct file *filp,
		       void *buffer, size_t *lenp)
{
	int r;
	down(&lasat_info_sem);
	r = proc_dointvec(table, write, filp, buffer, lenp);
	if ( (!write) || r) {
		up(&lasat_info_sem);
		return r;
	}
	lasat_board_info.li_eeprom_info.vendid = 
		lasat_board_info.li_vendid & 0xff;
	lasat_board_info.li_vendid &= 0xff;
	lasat_write_eeprom_info();
	up(&lasat_info_sem);
	return 0;
}

static int rtctmp;

#ifdef CONFIG_DS1603
/* proc function to read/write RealTime Clock */ 
int proc_dolasatrtc(ctl_table *table, int write, struct file *filp,
		       void *buffer, size_t *lenp)
{
	int r;
	down(&lasat_info_sem);
	if (!write) {
		rtctmp = ds1603_read();
		/* check for time < 0 and set to 0 */
		if (rtctmp < 0)
			rtctmp = 0;
	}
	r = proc_dointvec(table, write, filp, buffer, lenp);
	if ( (!write) || r) {
		up(&lasat_info_sem);
		return r;
	}
	ds1603_set(rtctmp);
	up(&lasat_info_sem);
	return 0;
}
#endif

/* Sysctl for setting the IP addresses */
int sysctl_lasat_intvec(ctl_table *table, int *name, int nlen,
		    void *oldval, size_t *oldlenp,
		    void *newval, size_t newlen, void **context)
{
	int r;
	down(&lasat_info_sem);
	r = sysctl_intvec(table, name, nlen, oldval, oldlenp, newval, newlen, context);
	if (r < 0) {
		up(&lasat_info_sem);
		return r;
	}
	if (newval && newlen) {
		lasat_write_eeprom_info();
	}
	up(&lasat_info_sem);
	return 1;
}

#if CONFIG_DS1603
/* Same for RTC */
int sysctl_lasat_rtc(ctl_table *table, int *name, int nlen,
		    void *oldval, size_t *oldlenp,
		    void *newval, size_t newlen, void **context)
{
	int r;
	down(&lasat_info_sem);
	rtctmp = ds1603_read();
	if (rtctmp < 0)
		rtctmp = 0;
	r = sysctl_intvec(table, name, nlen, oldval, oldlenp, newval, newlen, context);
	if (r < 0) {
		up(&lasat_info_sem);
		return r;
	}
	if (newval && newlen) {
		ds1603_set(rtctmp);
	}
	up(&lasat_info_sem);
	return 1;
}
#endif

#ifdef CONFIG_INET
static char proc_lasat_ipbuf[32];
/* Parsing of IP address */
int proc_lasat_ip(ctl_table *table, int write, struct file *filp,
		       void *buffer, size_t *lenp)
{
	int len;
        unsigned int ip;
	char *p, c;

	if (!table->data || !table->maxlen || !*lenp ||
	    (filp->f_pos && !write)) {
		*lenp = 0;
		return 0;
	}

	down(&lasat_info_sem);
	if (write) {
		len = 0;
		p = buffer;
		while (len < *lenp) {
			if(get_user(c, p++)) {
				up(&lasat_info_sem);
				return -EFAULT;
			}
			if (c == 0 || c == '\n')
				break;
			len++;
		}
		if (len >= sizeof(proc_lasat_ipbuf)-1) 
			len = sizeof(proc_lasat_ipbuf) - 1;
		if (copy_from_user(proc_lasat_ipbuf, buffer, len))
		{
			up(&lasat_info_sem);
			return -EFAULT;
		}
		proc_lasat_ipbuf[len] = 0;
		filp->f_pos += *lenp;
		/* Now see if we can convert it to a valid IP */
		ip = in_aton(proc_lasat_ipbuf);
		*(unsigned int *)(table->data) = ip;
		lasat_write_eeprom_info();
	} else {
		ip = *(unsigned int *)(table->data);
		sprintf(proc_lasat_ipbuf, "%d.%d.%d.%d",
			(ip      ) & 0xff,
			(ip >>  8) & 0xff,
			(ip >> 16) & 0xff,
			(ip >> 24) & 0xff);
		len = strlen(proc_lasat_ipbuf);
		if (len > *lenp)
			len = *lenp;
		if (len)
			if(copy_to_user(buffer, proc_lasat_ipbuf, len)) {
				up(&lasat_info_sem);
				return -EFAULT;
			}
		if (len < *lenp) {
			if(put_user('\n', ((char *) buffer) + len)) {
				up(&lasat_info_sem);
				return -EFAULT;
			}
			len++;
		}
		*lenp = len;
		filp->f_pos += len;
	}
	up(&lasat_info_sem);
	return 0;
}
#endif /* defined(CONFIG_INET) */

static int sysctl_lasat_eeprom_value(ctl_table *table, int *name, int nlen, 
				     void *oldval, size_t *oldlenp, 
				     void *newval, size_t newlen,
				     void **context)
{
	int r;

	down(&lasat_info_sem);
	r = sysctl_intvec(table, name, nlen, oldval, oldlenp, newval, newlen, context);
	if (r < 0) {
		up(&lasat_info_sem);
		return r;
	}

	if (newval && newlen)
	{
		if (name && *name == LASAT_PRID)
			lasat_board_info.li_eeprom_info.prid = *(int*)newval;
		if (name && *name == LASAT_DEBUGACCESS)
			lasat_board_info.li_eeprom_info.debugaccess = *(char*)newval;

		lasat_write_eeprom_info();
		lasat_init_board_info();
	}
	up(&lasat_info_sem);

	return 0;
}

int proc_lasat_eeprom_value(ctl_table *table, int write, struct file *filp,
		       void *buffer, size_t *lenp)
{
	int r;
	down(&lasat_info_sem);
	r = proc_dointvec(table, write, filp, buffer, lenp);
	if ( (!write) || r) {
		up(&lasat_info_sem);
		return r;
	}
	if (filp && filp->f_dentry)
	{
		if (!strcmp(filp->f_dentry->d_name.name, "prid"))
			lasat_board_info.li_eeprom_info.prid = lasat_board_info.li_prid;
		if (!strcmp(filp->f_dentry->d_name.name, "debugaccess"))
			lasat_board_info.li_eeprom_info.debugaccess = lasat_board_info.li_debugaccess;
	}
	lasat_write_eeprom_info();	
	up(&lasat_info_sem);
	return 0;
}

extern int lasat_boot_to_service;

#ifdef CONFIG_SYSCTL

static ctl_table lasat_table[] = {
	{LASAT_CPU_HZ, "cpu-hz", &lasat_board_info.li_cpu_hz, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_BUS_HZ, "bus-hz", &lasat_board_info.li_bus_hz, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_MODEL, "bmid", &lasat_board_info.li_bmid, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_SERIAL, "serial", &lasat_board_info.li_serial, sizeof(lasat_board_info.li_serial),
	 0444, NULL, &proc_dostring, &sysctl_string},
	{LASAT_PARTNO, "partno", &lasat_board_info.li_partno, sizeof(lasat_board_info.li_partno),
	 0444, NULL, &proc_dostring, &sysctl_string},
	{LASAT_EDHAC, "edhac", &lasat_board_info.li_edhac, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_EADI, "eadi", &lasat_board_info.li_eadi, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_LEASEDLINE, "leasedline", &lasat_board_info.li_leasedline, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_ISDN, "isdn", &lasat_board_info.li_isdn, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_HIFN, "hifn", &lasat_board_info.li_hifn, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_USVER, "us-version", &lasat_board_info.li_usversion, sizeof(int),
	 0444, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_PRID, "prid", &lasat_board_info.li_prid, sizeof(int),
	 0644, NULL, &proc_lasat_eeprom_value, &sysctl_lasat_eeprom_value},
	{LASAT_VENDID, "vendid", &lasat_board_info.li_vendid, sizeof(int),
	 0644, NULL, &proc_dolasatvendor, &sysctl_intvec},
#ifdef CONFIG_INET
	{LASAT_IPADDR, "ipaddr", &lasat_board_info.li_eeprom_info.ipaddr, sizeof(int),
	 0644, NULL, &proc_lasat_ip, &sysctl_lasat_intvec},
	{LASAT_NETMASK, "netmask", &lasat_board_info.li_eeprom_info.netmask, sizeof(int),
	 0644, NULL, &proc_lasat_ip, &sysctl_lasat_intvec},
#endif
	{LASAT_PASSWORD, "passwd_hash", &lasat_board_info.li_eeprom_info.passwd_hash, sizeof(lasat_board_info.li_eeprom_info.passwd_hash),
	 0600, NULL, &proc_dolasatstring, &sysctl_lasatstring},
	{LASAT_SERVICEFLAG, "serviceflag", &lasat_board_info.li_eeprom_info.serviceflag, sizeof(lasat_board_info.li_eeprom_info.serviceflag),
	 0644, NULL, &proc_dolasatint, &sysctl_lasat_intvec},
	{LASAT_SBOOT, "boot-service", &lasat_boot_to_service, sizeof(int),
	 0666, NULL, &proc_dointvec, &sysctl_intvec},
#if CONFIG_DS1603
	{LASAT_RTC, "rtc", &rtctmp, sizeof(int),
	 0644, NULL, &proc_dolasatrtc, &sysctl_lasat_rtc},
#endif
	{LASAT_VER, "version", &lasat_firmware_version, 176,
	 0444, NULL, &proc_dostring, &sysctl_string},
	{LASAT_SERVICEVER, "service-version", (char*)0xbfc00100/* fixed position */, 64,
	 0444, NULL, &proc_dostring, &sysctl_string},
#if 0
	{LASAT_WANLED, "wan_led", &lasat_wan_led, sizeof(int),
	 0644, NULL, &proc_dointvec, &sysctl_intvec},
#endif
 	{LASAT_UPGRADE, "upgrade_eeprom", &lasat_board_info.li_eeprom_upgrade_version, sizeof(int),
	 0644, NULL, &proc_dointvec, &sysctl_intvec},
 	{LASAT_VPN_KBPS, "vpn_kbps", &lasat_board_info.li_vpn_kbps, sizeof(int),
	 0644, NULL, &proc_dointvec, &sysctl_intvec},
 	{LASAT_TUNNELS, "vpn_tunnels", &lasat_board_info.li_vpn_tunnels, sizeof(int),
	 0644, NULL, &proc_dointvec, &sysctl_intvec},
 	{LASAT_CLIENTS, "vpn_clients", &lasat_board_info.li_vpn_clients, sizeof(int),
	 0644, NULL, &proc_dointvec, &sysctl_intvec},
	{LASAT_VENDSTR, "vendstr", &lasat_board_info.li_vendstr, sizeof(lasat_board_info.li_vendstr),
	 0444, NULL, &proc_dostring, &sysctl_string},
	{LASAT_NAMESTR, "namestr", &lasat_board_info.li_namestr, sizeof(lasat_board_info.li_namestr),
	 0444, NULL, &proc_dostring, &sysctl_string},
	{LASAT_TYPESTR, "typestr", &lasat_board_info.li_typestr, sizeof(lasat_board_info.li_typestr),
	 0444, NULL, &proc_dostring, &sysctl_string},
	{LASAT_DEBUGACCESS, "debugaccess", &lasat_board_info.li_debugaccess, sizeof(int),
	 0644, NULL, &proc_lasat_eeprom_value, &sysctl_lasat_eeprom_value},
	{0}
};

#define CTL_LASAT 1	// CTL_ANY ???
static ctl_table lasat_root_table[] = {
	{ CTL_LASAT, "lasat", NULL, 0, 0555, lasat_table },
	{ 0 }
};

static int __init lasat_register_sysctl(void)
{
	struct ctl_table_header *lasat_table_header;

	lasat_table_header =
		register_sysctl_table(lasat_root_table, 0);

	return 0;
}

__initcall(lasat_register_sysctl);
#endif /* CONFIG_SYSCTL */
