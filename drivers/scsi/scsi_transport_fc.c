/* 
 *  FiberChannel transport specific attributes exported to sysfs.
 *
 *  Copyright (c) 2003 Silicon Graphics, Inc.  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/init.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_transport.h>
#include <scsi/scsi_transport_fc.h>

static void transport_class_release(struct class_device *class_dev);

struct class fc_transport_class = {
	.name = "fc_transport",
	.release = transport_class_release,
};

static __init int fc_transport_init(void)
{
	return class_register(&fc_transport_class);
}

static void __exit fc_transport_exit(void)
{
	class_unregister(&fc_transport_class);
}

static int fc_setup_transport_attrs(struct scsi_device *sdev)
{
	/* FIXME: Callback into the driver */
	fc_node_name(sdev) = -1;
	fc_port_name(sdev) = -1;
	fc_port_id(sdev) = -1;

	return 0;
}

static void transport_class_release(struct class_device *class_dev)
{
	struct scsi_device *sdev = transport_class_to_sdev(class_dev);
	put_device(&sdev->sdev_gendev);
}

#define fc_transport_show_function(field, format_string, cast)			\
static ssize_t									\
show_fc_transport_##field (struct class_device *cdev, char *buf)		\
{										\
	struct scsi_device *sdev = transport_class_to_sdev(cdev);		\
	struct fc_transport_attrs *tp;						\
	tp = (struct fc_transport_attrs *)&sdev->transport_data;		\
	return snprintf(buf, 20, format_string, cast tp->field);		\
}

#define fc_transport_rd_attr(field, format_string)				\
	fc_transport_show_function(field, format_string, )			\
static CLASS_DEVICE_ATTR( field, S_IRUGO, show_fc_transport_##field, NULL)

#define fc_transport_rd_attr_cast(field, format_string, cast)			\
	fc_transport_show_function(field, format_string, (cast))		\
static CLASS_DEVICE_ATTR( field, S_IRUGO, show_fc_transport_##field, NULL)

/* the FiberChannel Tranport Attributes: */
fc_transport_rd_attr_cast(node_name, "0x%llx\n", unsigned long long);
fc_transport_rd_attr_cast(port_name, "0x%llx\n", unsigned long long);
fc_transport_rd_attr(port_id, "0x%06x\n");

struct class_device_attribute *fc_transport_attrs[] = {
	&class_device_attr_node_name,
	&class_device_attr_port_name,
	&class_device_attr_port_id,
	NULL
};

struct scsi_transport_template fc_transport_template = {
	.attrs = fc_transport_attrs,
	.class = &fc_transport_class,
	.setup = &fc_setup_transport_attrs,
	.cleanup = NULL,
	.size = sizeof(struct fc_transport_attrs) - sizeof(unsigned long),
};
EXPORT_SYMBOL(fc_transport_template);

MODULE_AUTHOR("Martin Hicks");
MODULE_DESCRIPTION("FC Transport Attributes");
MODULE_LICENSE("GPL");

module_init(fc_transport_init);
module_exit(fc_transport_exit);
