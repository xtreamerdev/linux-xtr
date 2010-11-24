/*
 * i2c-algo-venus.c: i2c driver algorithms for Realtek Venus DVR
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * Copyright (C) 2005 Chih-pin Wu <wucp@realtek.com.tw>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include <linux/i2c.h>
//#include <linux/i2c-algo-venus.h>
#include "i2c-algo-venus.h"

static int venus_xfer(struct i2c_adapter *i2c_adap, struct i2c_msg *msgs, int num)
{
	struct i2c_algo_venus_data *adap = i2c_adap->algo_data;
	int err = 0;

	err = adap->masterXfer(i2c_adap, msgs, num);

	return err;
}

static u32 venus_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm venus_algo = {
	.name			= "Venus algorithm",
#define I2C_ALGO_VENUS 0x1b0000        /* Venus algorithm, FIXME, move to i2c-id.h later */
	.id				= I2C_ALGO_VENUS,
	.master_xfer	= venus_xfer,
	.functionality	= venus_func,
};

/* 
 * registering functions to load algorithms at runtime 
 */
int i2c_venus_add_bus(struct i2c_adapter *adap)
{
	adap->id |= venus_algo.id;
	adap->algo = &venus_algo;

	return i2c_add_adapter(adap);
}


int i2c_venus_del_bus(struct i2c_adapter *adap)
{
	return i2c_del_adapter(adap);
}

EXPORT_SYMBOL(i2c_venus_add_bus);
EXPORT_SYMBOL(i2c_venus_del_bus);

MODULE_AUTHOR("Chih-pin Wu <wucp@realtek.com.tw>");
MODULE_DESCRIPTION("I2C-Bus Venus algorithm");
MODULE_LICENSE("GPL");
