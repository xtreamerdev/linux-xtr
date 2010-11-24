/*
  
  File cdXA.c - routines for frames <>2048 and <> 2352 byte frames for cdfs
  
  Copyright (c) 1999, 2000, 2001 by Michiel Ronsse 
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
  
*/


#include "all.h"
#include <linux/cdrom.h>
//#include <asm/delay.h>
extern int First_Video_LBA;
extern int DVD_disc;
extern int CD_TYPE;
extern int EACD_disc;
int cdfs_read_iso_data(struct super_block *s, int lba, unsigned char* buff);

struct file_operations cdfs_cdXA_file_operations = {
  read:               generic_file_read,
  mmap:               generic_file_mmap
};

struct address_space_operations cdfs_cdXA_aops = {
  readpage:           kcdfsd_add_cdXA_request
};

int kcdfsd_add_cdXA_request(struct file * file, struct page *page)
{
	return kcdfsd_add_request(file->f_dentry, page, CDXA_REQUEST);
}

int cdfs_read_raw_frame(struct super_block * sb, int lba, unsigned char *buf)
{ 
	/*    
	struct cdrom_msf *msf;
	msf = (struct cdrom_msf*) buf;
	msf->cdmsf_min0   = (lba + CD_MSF_OFFSET) / CD_FRAMES / CD_SECS;
	msf->cdmsf_sec0   = (lba + CD_MSF_OFFSET) / CD_FRAMES % CD_SECS;
	msf->cdmsf_frame0 = (lba + CD_MSF_OFFSET) % CD_FRAMES;
	return cdfs_ioctl(sb, CDROMREADMODE2, (unsigned long)msf);
	*/
	static struct cdrom_generic_command cgc;
	static struct request_sense buffer2;
	unsigned char *frame;
	int ret;

	frame = kmalloc(CD_FRAMESIZE_RAW, GFP_KERNEL);	
	if (!frame)
	{
		printk("memory allocation failed\n");
		return -5;
	}
	memset(&cgc, 0, sizeof(cgc));
	cgc.sense = &buffer2;
	cgc.buffer = frame;
	cgc.buflen= CD_FRAMESIZE_RAW;
	cgc.data_direction=CGC_DATA_READ;
	cgc.quiet = 1;
	cgc.stat = 1;
	cgc.cmd[0]=GPCMD_READ_CD;
	cgc.cmd[1]= 0<<2;
	cgc.cmd[2]=(lba>>24)&0xff;      // MSB
	cgc.cmd[3]=(lba>>16)&0xff;      
	cgc.cmd[4]=(lba>>8 )&0xff;      
	cgc.cmd[5]=(lba    )&0xff;      // LSB
	cgc.cmd[8]=1;			//Transfer length in blocks LSB
	cgc.cmd[9]=0xf8;
	ret=cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)&cgc );
	if (!ret) memcpy(buf, frame+16, CD_FRAMESIZE_RAW0);  
	kfree(frame);
	return(ret);

}

int cdfs_read_raw_frame2(struct super_block * sb, int lba, unsigned char *buf,unsigned int data_size)
{
	/*     
	struct cdrom_msf *msf;
	msf = (struct cdrom_msf*) buf;
	msf->cdmsf_min0   = (lba + CD_MSF_OFFSET) / CD_FRAMES / CD_SECS;
	msf->cdmsf_sec0   = (lba + CD_MSF_OFFSET) / CD_FRAMES % CD_SECS;
	msf->cdmsf_frame0 = (lba + CD_MSF_OFFSET) % CD_FRAMES;

	if(data_size==2048)
	{
		PRINTC("david0213: data_size==2048, using CDROMREADMODE2\n");
		return cdfs_ioctl(sb, CDROMREADMODE2, (unsigned long)msf);
	}
	else
	{
		PRINTC("david0213: data_size!=2048, using CDROMREADRAW\n");
		return cdfs_ioctl(sb, CDROMREADRAW, (unsigned long)msf);	//david 1014
	}
	*/
	static struct cdrom_generic_command cgc;
	static struct request_sense buffer2;
	unsigned char *frame;
	int ret;
	
	memset(&cgc, 0, sizeof(cgc));	

	if(data_size==2048)
	{
		PRINTC("david0213: data_size==2048, using CDROMREADMODE2\n");
		frame = kmalloc(CD_FRAMESIZE_RAW, GFP_KERNEL);	
		if (!frame)
		{
			printk("memory allocation failed in cdfs_read_raw_frame2\n");
			return 0;
		}
		cgc.sense = &buffer2;
		cgc.buffer = frame;
		cgc.buflen=CD_FRAMESIZE_RAW;
		cgc.data_direction=CGC_DATA_READ;
		cgc.quiet = 1;
		cgc.stat = 1;
		cgc.cmd[0]=GPCMD_READ_CD;
		cgc.cmd[1]= 0<<2;
		cgc.cmd[2]=(lba>>24)&0xff;      // MSB
		cgc.cmd[3]=(lba>>16)&0xff;      
		cgc.cmd[4]=(lba>>8 )&0xff;      
		cgc.cmd[5]=(lba    )&0xff;      // LSB
		cgc.cmd[8]=1;			//Transfer length in blocks LSB
		cgc.cmd[9]=0xf8;
		ret=cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)&cgc );
		if (!ret) memcpy(buf, frame+16, CD_FRAMESIZE_RAW0);
		kfree(frame);
		return(ret);
	}
	else
	{
		PRINTC("david0213: data_size!=2048, using CDROMREADRAW\n");
		cgc.sense = &buffer2;
		cgc.buffer = buf;
		cgc.buflen=CD_FRAMESIZE_RAW;
		cgc.data_direction=CGC_DATA_READ;
		cgc.quiet = 1;
		cgc.stat = 1;
		cgc.cmd[0]=GPCMD_READ_CD;
		cgc.cmd[2]=(lba>>24)&0xff;      // MSB
		cgc.cmd[3]=(lba>>16)&0xff;      
		cgc.cmd[4]=(lba>>8 )&0xff;      
		cgc.cmd[5]=(lba    )&0xff;	// LSB
		cgc.cmd[8]=1;			//Transfer length in blocks LSB
		cgc.cmd[9]=0xf8;
		return(cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)&cgc ));
	}
}

int cdfs_get_XA_info(struct super_block * sb, int inode)
{
	char *frame;
	cd * this_cd = cdfs_info(sb);
	track_info * this_track = &(this_cd->track[inode]);
	unsigned start_lba = this_track->start_lba;
	int status;
	int i=0;
	//frame = kmalloc(CD_FRAMESIZE_RAW0, GFP_KERNEL);	//C01
	frame = kmalloc(2348, GFP_KERNEL);	//C01
	PRINTM("C01 kmalloc addr = 0x %x, size = %6d for frame\n",(unsigned int)frame,2348);
	if (!frame)
	{
		printk("memory allocation failed\n");
		return 0;
	}
	//if((status = cdfs_read_raw_frame(sb, start_lba, frame)))
	if((status = (DVD_disc)?cdfs_read_iso_data(sb, start_lba, frame):cdfs_read_raw_frame(sb, start_lba, frame)))
	{
		printk("<vcd module> cdfs_get_XA_info_1 : Reading sector %d in disc SP fail!!! status=%d\n", start_lba, status);
		//if fails here, the cd-folder will be a mass.
		PRINTM("C01 kfree   addr = 0x %x for frame\n",(unsigned int)frame);
		kfree(frame);	//C01
		return -5;
	}
	PRINTC("david0614_tmp092 xa_data_size=%d\n",this_track->xa_data_size);
	if (frame[0] == frame[4] &&
	  frame[1] == frame[5] &&
	  frame[2] == frame[6] &&
	  frame[3] == frame[7] &&
	  frame[2] != 0 &&
	  frame[0] < 100 &&
	  frame[1] < 8)
	{
		//printk("david0518: case1\n");
		//this_track->xa_data_size   = (frame[2] & 0x20) ? 2352 : 2048;
		if((this_track->xa_data_size)!=2352)
			this_cd->track[i].xa_data_size=2048;
		this_track->xa_data_offset = 8;
	}
	else
	{
		//printk("david0518: case2\n");
		this_track->xa_data_size   = 2048;
		this_track->xa_data_offset = 0;
	}
	// Get type & title
	if((status = cdfs_read_raw_frame(sb, 150, frame)))
	{
		printk("<vcd module> cdfs_get_XA_info_2 : Reading sector %d in disc SP fail!!! status=%d\n", 150, status);
		PRINTM("C01 kfree   addr = 0x %x for frame\n",(unsigned int)frame);
		kfree(frame);	//C01
		return 0;
	}
	strncpy(this_cd->videocd_type, frame+this_track->xa_data_offset, 8);
	this_cd->videocd_type[8]=0;
	strncpy(this_cd->videocd_title, frame+this_track->xa_data_offset+10, 16);
	this_cd->videocd_title[16]=0;
	PRINTM("C01 kfree   addr = 0x %x for frame\n",(unsigned int)frame);
	kfree(frame);	//C01
	return 1;
}

int vcdfs_get_xa_sector_info(struct super_block * sb, unsigned  int start_lba, unsigned int *data_size, unsigned int *data_offset)
{
	unsigned char *frame;
	int status;
	*data_size=2048;
	*data_offset=0;
	
	frame = kmalloc(2348, GFP_KERNEL);	//C01
	PRINTC("cdxa.c:vcdfs_get_sector_info()\n");
	
	//if((status = cdfs_read_raw_frame(sb, start_lba, frame)))
	if((status = (DVD_disc)?cdfs_read_iso_data(sb, start_lba, frame):cdfs_read_raw_frame(sb, start_lba, frame)))
	{
		printk("<vcd module> vcdfs_get_xa_sector_info : Reading sector %d in disc SP fail!!! status=%d => file error!!!\n", start_lba, status);
		kfree(frame);
		return -5;
	}

	if (DVD_disc)
		CD_TYPE=DVD_DISC;
	
	if (frame[0] == frame[4] &&		//M2ªºª¬ªp
	  frame[1] == frame[5] &&
	  frame[2] == frame[6] &&
	  frame[3] == frame[7] &&
	  //frame[2] == 0 &&	//david0615
	  //frame[0] < 256 &&   //always true
	  //frame[1] < 8) {	//david0615
	  frame[1] < 32) {
		*data_size   = (frame[2] & 0x20) ? 2352 : 2048;
		PRINTC("david0518: case3 data_size=2352 or 2048 M2\n");
		if((start_lba>=First_Video_LBA)&&(!EACD_disc)&&(CD_TYPE!=DATA_DISC))
		{
			*data_size=2352;
			PRINTC("M2F2 data_size=%d\n",*data_size);
		}
		else
		{
			PRINTC("M2F1 data_size=%d\n",*data_size);
		}
		*data_offset = 8;
	}
	else if((start_lba>=First_Video_LBA)&&((CD_TYPE==VCD_DISC)||(CD_TYPE==AVCD_DISC)||(CD_TYPE==SVCD_DISC)))
	{	//especially for badly recorded VCD SVCD disc, Mantis #3701
		printk("<vcd module> Get VCD AVCD SVCD with wrong sector header......\n");
		*data_size=2352;
		PRINTC("M2F2 data_size=%d\n",*data_size);
		*data_offset = 8;
	}
	else					//M1
	{
		*data_size   = 2048;
		*data_offset = 0;
		PRINTC("david0518: case4 data_size=2048 M1\n");
		PRINTC("M1 data_size=%d\n",*data_size);
	}
	PRINTC("david0614_A: Get data_size=%d\n",*data_size);
	kfree(frame);
	return 0;
}

int cdfs_copy_from_cdXA(struct super_block * sb, struct inode * inode, unsigned int start, unsigned int stop, char * buf)
{
	struct iso_inode_info *ei = ISOFS_I(inode);
	int start_sector, start_byte, stop_sector, stop_byte, sector;
  	int status=0;
	unsigned start_lba = ei->i_first_extent;
	cd * this_cd = cdfs_info(sb);
	inode_info *p=(inode_info *)inode->u.generic_ip;
	unsigned int data_size   = p->xa_data_size;
	unsigned int data_offset = p->xa_data_offset;
	int ret;

	if(p->xa_data_offset==-1)
	{
		ret=vcdfs_get_xa_sector_info(sb, ei->i_first_extent, &(p->xa_data_size), &(p->xa_data_offset));
		data_size   = p->xa_data_size;
		data_offset = p->xa_data_offset;
		if(ret)
		{
			p->xa_data_offset=-5;
			return -5;
		}
	}
	else if(p->xa_data_offset==-5)
	{
		PRINTC("<vcd module> Reading this file fail!!!!!!!\n");
		return -5;
	}

	start_sector = start / data_size;
	start_byte   = start % data_size;
	stop_sector  = stop  / data_size;
	stop_byte    = stop  % data_size;

	if (!stop_byte)
	{            /* empty frame */
		stop_sector -= 1;
		stop_byte    = data_size;// Diego Rodriguez <diegoro@hotmail.com>
	}
	PRINTC("%d[%d-%d] -> 0x%x...0x%x  ... (%d,%d),(%d,%d)\n",
		inode, start, stop, (int)buf, (int)buf+stop-start,
		start_sector,start_byte,stop_sector,stop_byte);
	for (sector=start_sector; sector<=stop_sector; sector++)
	{
		int lba=sector+start_lba;
		if (this_cd->cache_sector == lba)
		{
			PRINTC("using cache\n");
		}
		else
		{
			this_cd->cache_sector = lba;
	PRINTC("david0614_tmp094 xa_data_size=%d\n",p->xa_data_size);
			//if((status = cdfs_read_raw_frame2(sb, lba, this_cd->cache,p->xa_data_size)))
			if((status = (DVD_disc)?cdfs_read_iso_data(sb, lba, this_cd->cache):cdfs_read_raw_frame2(sb, lba, this_cd->cache,p->xa_data_size)))
			{
				printk("<vcd module> Reading sector %d in disc SP fail!!! status=%d\n", lba, status);
				return status;
			}
		}

		{
			char * copy_start;
			int copy_length;
			if (sector==start_sector)
			{
				copy_start  = this_cd->cache+data_offset+start_byte;
				if (sector!=stop_sector)
				{
					copy_length = data_size-start_byte;
				}
				else
				{
					copy_length = stop_byte-start_byte;
				}
			}
			else if (sector==stop_sector)
			{
				copy_start  = this_cd->cache+data_offset;
				copy_length = stop_byte;
			}
			else
			{
				copy_start  = this_cd->cache+data_offset;
				copy_length = data_size;
			}
			
/*			
			if(copy_length==2048)
				memcpy(buf, (char*)copy_start, copy_length);
			else
				memcpy(buf, ((char*)copy_start)-24, copy_length);
*/

			PRINTC("david1014: p->xa_data_size=%d\n",p->xa_data_size);

			if(p->xa_data_size==2048)
			{
				//printk("david0213 path 1\n");
				memcpy(buf, (char*)copy_start, copy_length);
			}
			else
			{
				//printk("david0213 path 2\n");
				memcpy(buf, ((char*)copy_start)-8, copy_length);
			}
			buf+=copy_length;
		}
	}
	return status;
}
/*
int cdfs_read_iso_data(struct super_block *s, int lba, unsigned char* buff)
{
	struct buffer_head * bh;
	//printk("reading secter %6d in the DVD disc_ST\n",lba);
	if (!(bh = sb_bread(s, lba)))	//D01
	{
		printk("reading secter %6d in the DVD disc_SP fail address of bh=%lu\n",lba,bh);
		//brelse(bh);
		//bh=NULL;
		return -5;
	}
	else
	{
		//printk("reading secter %6d in the DVD disc_SP OK address of bh=%lu\n",lba,bh);
		memcpy(buff,bh->b_data,2048);
		brelse(bh);	//D01
		//bh=NULL;
		return 0;
	}
}
*/

int cdfs_read_iso_data(struct super_block *s, int lba, unsigned char* buff)
{
	struct buffer_head * bh;
	//printk("reading secter %6d in the DVD disc_ST\n",lba);
	bh = sb_bread(s, lba);	//D01
	//printk("reading secter %6d in the DVD disc_SP\n",lba);
	memcpy(buff,bh->b_data,2048);
	if (bh) brelse(bh);	//D01
	return 0;
}


