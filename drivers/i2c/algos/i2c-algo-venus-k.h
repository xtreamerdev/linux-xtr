/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * Copyright (C) 2005 Chih-pin Wu <wucp@realtek.com.tw>
 */

#ifndef I2C_ALGO_VENUS_H
#define I2C_ALGO_VENUS_H

#include <linux/i2c.h>

struct i2c_algo_venus_data {
	void *data;	/* private data for lowlevel routines */
	
	/*
	int (*masterRead)(u16 addr, unsigned short flags, unsigned char *buf, unsigned int len);
	int (*masterWrite)(u16 addr, unsigned short flags, unsigned char *buf, unsigned int len);
	*/
	int (*masterXfer)(struct i2c_msg *msgs, int num);
};

int i2c_venus_add_bus(struct i2c_adapter *);
int i2c_venus_del_bus(struct i2c_adapter *);

#endif /* I2C_ALGO_VENUS_H */
