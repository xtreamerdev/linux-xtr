/*
 *	Real Time Clock interface for STFPC320 of STMicroelectronics. It
 *      uses I2C to communicate with that chip.
 *
 *      Copyright (C) 2008 Colin
 */

#define STFPC320_RTC_VERSION	"1.0"
#define	STFPC320_DRV_NAME	"stfpc320"

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <asm/irq.h>

#include <linux/i2c.h>
#include <asm/rtc.h>
#include <asm/time.h>
#include <linux/bcd.h>


static struct i2c_client *save_client=NULL;
static struct i2c_driver stfpc320_driver;
static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { CONFIG_SENSORS_STFPC320_ADDR, I2C_CLIENT_END };
static struct i2c_client_address_data addr_data = {
	.normal_i2c		= normal_addr,
	.normal_i2c_range	= ignore,
	.probe			= ignore,
	.probe_range		= ignore,
	.ignore			= ignore,
	.ignore_range		= ignore,
	.force			= ignore,
};

unsigned long stfpc320_rtc_get_time(void)
{
	unsigned int sec, min, hour, day, mon, year;
	unsigned char data2=0xC1, data[7];
	struct i2c_msg i2cmsg[3] = {
		{save_client->addr, 0, 1, &data2},
		{save_client->addr, 0, 1, data},
		{save_client->addr, I2C_M_RD, 7, data}
	};

	data[0] = 0x73;
	if (i2c_transfer(save_client->adapter, i2cmsg, 3) != 3) {
		printk("Stfpc320 get time by I2C error.\n");
		return 0;
	}
	sec = data[0]&0x7F;
	min = data[1]&0x7F;
	hour = data[2]&0x3F;
	day = data[4]&0x3F;
	mon = data[5]&0x1F;
	year = data[6]&0x7F;
//printk("aaa%x %x %x %x %x %x %x\n", sec, min, hour, data[3], day, mon, year);
	BCD_TO_BIN(sec);
	BCD_TO_BIN(min);
	BCD_TO_BIN(hour);
	BCD_TO_BIN(day);
	BCD_TO_BIN(mon);
	BCD_TO_BIN(year);
	day += 1;
	mon += 1;
	year+=2000;

	return mktime(year, mon, day, hour, min, sec);
}

int stfpc320_rtc_set_time(unsigned long second)
{
	unsigned int sec, min, hour, day, mon, year;
	unsigned char data[8], data2=0xC1;
	struct rtc_time time;
	struct i2c_msg i2cmsg[2] = {
		{save_client->addr, 0, 1, &data2},
		{save_client->addr, 0, 8, data}
	};

	to_tm(second, &time);
	sec = time.tm_sec;
	min = time.tm_min;
	hour = time.tm_hour;
	day = time.tm_mday;
	mon = time.tm_mon;
	year = time.tm_year;
	day -= 1;
	year -= 2000;
	BIN_TO_BCD(sec);
	BIN_TO_BCD(min);
	BIN_TO_BCD(hour);
	BIN_TO_BCD(day);
	BIN_TO_BCD(mon);
	BIN_TO_BCD(year);
	data[1] = sec;
	data[2] = min;
	data[3] = hour;
	data[4] = time.tm_wday+1;
	data[5] = day;
	data[6] = mon;
	data[7] = year;
//printk("bbb%x %x %x %x %x %x %x\n", sec, min, hour, data[4], day, mon, year);
	data[0] = 0x40;
	if (i2c_transfer(save_client->adapter, i2cmsg, 2) != 2) {
		printk("Stfpc320 set time by I2C error.\n");
		return -ENODEV;
	}

	return 0;
}

unsigned long stfpc320_rtc_alarm_get_time(void)
{
	unsigned int sec, min, hour, day, mon;
	unsigned char data[5], data2=0xCA;
	struct i2c_msg i2cmsg[3] = {
		{save_client->addr, 0, 1, &data2},
		{save_client->addr, 0, 1, data},
		{save_client->addr, I2C_M_RD, 5, data}
	};

	data[0] = 0x73;
	if (i2c_transfer(save_client->adapter, i2cmsg, 3) != 3) {
		printk("Stfpc320 alarm get time by I2C error.\n");
		return 0;
	}
	sec = data[4]&0x7F;
	min = data[3]&0x7F;
	hour = data[2]&0x3F;
	day = data[1]&0x3F;
	mon = data[0]&0x1F;
//printk("ccc%x %x %x %x %x\n", sec, min, hour, day, mon);
	BCD_TO_BIN(sec);
	BCD_TO_BIN(min);
	BCD_TO_BIN(hour);
	BCD_TO_BIN(day);
	BCD_TO_BIN(mon);
	day += 1;
	mon += 1;
	printk("For stfpc320, the year will always be \"2000\" because it doesn't have year field for alarm.\n");

	return mktime(2000, mon, day, hour, min, sec);
}

int stfpc320_rtc_alarm_set_time(unsigned long second)
{
	unsigned int sec, min, hour, day, mon;
	unsigned char data[8], data2=0xCA;
	struct rtc_time time;

	struct i2c_msg i2cmsg[2] = {
		{save_client->addr, 0, 1, &data2},
		{save_client->addr, 0, 8, data}
	};

	to_tm(second, &time);
	sec = time.tm_sec;
	min = time.tm_min;
	hour = time.tm_hour;
	day = time.tm_mday;
	mon = time.tm_mon;
	day -= 1;
	BIN_TO_BCD(sec);
	BIN_TO_BCD(min);
	BIN_TO_BCD(hour);
	BIN_TO_BCD(day);
	BIN_TO_BCD(mon);
	data[1] = mon | 0xA0;
	data[2] = day;
	data[3] = hour;
	data[4] = min;
	data[5] = sec;
//printk("ddd%x %x %x %x %x %x\n", sec, min, hour, data[4], day, mon);
	data[0] = 0x40;
	if (i2c_transfer(save_client->adapter, i2cmsg, 2) != 2) {
		printk("Stfpc320 alarm set time by I2C error.\n");
		return -ENODEV;
	}
	printk("For stfpc320, the year will always be ignored because it doesn't have year field for alarm.\n");

	return 0;
}

static int stfpc320_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *client;
	int ret;
	unsigned char data1, data2;
	struct i2c_msg i2cmsg[3] = {
		{addr, 0, 1, &data1},
		{addr, 0, 1, &data2},
		{addr, I2C_M_RD, 7, &data2}
	};

	client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!client)
		return -ENOMEM;

	memset(client, 0, sizeof(struct i2c_client));
	strncpy(client->name, STFPC320_DRV_NAME, I2C_NAME_SIZE);
	client->flags = I2C_DF_NOTIFY;
	client->addr = addr;
	client->adapter = adap;
	client->driver = &stfpc320_driver;

	data1 = 0xCF;
	data2 = 0x73;
	ret = i2c_transfer(client->adapter, i2cmsg, 3);
	if (ret != 3) {
		printk("Stfpc320 cannot probe device.\n");
		kfree(client);
		save_client=NULL;
		return -ENODEV;
	}

	if ((ret = i2c_attach_client(client)) != 0) {
		kfree(client);
		save_client=NULL;
		return ret;
	}
	save_client = client;

	return 0;
}

static int stfpc320_attach(struct i2c_adapter *adap)
{
return stfpc320_probe(adap, CONFIG_SENSORS_STFPC320_ADDR, 0);
//	return i2c_probe(adap, &addr_data, stfpc320_probe);
}

static int stfpc320_detach(struct i2c_client *client)
{
	int ret;

	if ((ret = i2c_detach_client(client)) == 0) {
		kfree(i2c_get_clientdata(client));
	}
	save_client=NULL;
	
	return ret;
}

static struct i2c_driver stfpc320_driver = {
	.owner		= THIS_MODULE,
	.name		= STFPC320_DRV_NAME,
	.flags		= I2C_DF_NOTIFY,
	.attach_adapter	= stfpc320_attach,
	.detach_client	= stfpc320_detach,
};

static int __init stfpc320_init(void)
{
	int ret;
	struct timespec time;

	printk("RTC Driver, v%s, of STFPC320\n", STFPC320_RTC_VERSION);

	if ((ret = i2c_add_driver(&stfpc320_driver)))
		return ret;

	if(!save_client)
		return 0;
		
	time.tv_sec = stfpc320_rtc_get_time();
	time.tv_nsec = 0;
	do_settimeofday(&time);
	local_irq_disable();
	rtc_get_time = stfpc320_rtc_get_time;
	rtc_set_time = stfpc320_rtc_set_time;
	rtc_alarm_get_time = stfpc320_rtc_alarm_get_time;
	rtc_alarm_set_time = stfpc320_rtc_alarm_set_time;
	local_irq_enable();
	return 0;
}

static void __exit stfpc320_exit(void)
{
	local_irq_disable();
	rtc_set_default_funcs();
	local_irq_enable();
	i2c_del_driver(&stfpc320_driver);
}


module_init(stfpc320_init);
module_exit(stfpc320_exit);

MODULE_AUTHOR("Colin");
MODULE_LICENSE("GPL");
