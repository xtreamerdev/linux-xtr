#used for MIPS kernel compilation	//For Demo system, Nucom Board

#
# Makefile for the Linux VCD filesystem routines.
#

obj-$(CONFIG_VCD) += vcd.o

#vcd-objs-y 			:= namei.o inode.o dir.o util.o rock.o export.o joliet.o cddata.o
vcd-objs-y 			:= namei.o inode.o dir.o util.o rock.o export.o cddata.o
vcd-objs-y 			+= audio.o cdxa.o daemon.o utils.o iso.o toc.o hfs.o

vcd-objs-$(CONFIG_JOLIET)	+= joliet.o
vcd-objs-$(CONFIG_ZISOFS)	+= compress.o
vcd-objs			:= $(vcd-objs-y)

