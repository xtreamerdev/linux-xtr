# Rules for making the NTFS driver.
 


EXTRA_CFLAGS = -DNTFS_VERSION=\"2.1.28\"

ifeq ($(CONFIG_NTFS_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

ifneq ($(CONFIG_NTFS_RW),y)
obj-$(CONFIG_NTFS_FS) += ntfs.o
ntfs-objs := aops.o attrib.o collate.o compress.o debug.o dir.o file.o \
             index.o inode.o mft.o mst.o namei.o runlist.o super.o sysctl.o \
             unistr.o upcase.o
else
obj-$(CONFIG_NTFS_FS) += rtfs.bin

endif




