/*
   File audio.c - routines for 2352 byte frames cdfs

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
#include <asm/byteorder.h>
#include <linux/cdrom.h>
extern int CD_TYPE;
int cdfs_read_cd(struct super_block *s, struct cdrom_read_audio *cdda, char* buff);

void cdfs_make_header(char * temp, unsigned int size)
{
  /* RIFF header */
  strcpy(             temp,     "RIFF");
  *(u32 *) (temp+ 4) = cpu_to_le32(size-8);    /* size of RIFF */

  /* WAVE file */
  strcpy(             temp+ 8,  "WAVE");

  /* WAVE format*/
  strcpy(             temp+12,  "fmt ");
  *(u32 *) (temp+16) = cpu_to_le32(16);        /* size of format descriptor */
  /* common fields */
  *(u16 *) (temp+20) = cpu_to_le16(1);         /* format category: WAVE_FORMAT_PCM */
  *(u16 *) (temp+22) = cpu_to_le16(2);         /* number of channels */
  *(u32 *) (temp+24) = cpu_to_le32(44100);     /* sample rate */
  *(u32 *) (temp+28) = cpu_to_le32(44100*4);   /* bytes/sec */
  *(u16 *) (temp+32) = cpu_to_le16(4);         /* data block size */
  /* format specific fields (PCM) */
  *(u16 *) (temp+34) = cpu_to_le16(16);        /* bits per sample */

  /* WAVE data */
  strcpy(             temp+36,  "data");
  *(u32 *) (temp+40) = cpu_to_le32(size-44);   /* size of data chunk */
}

int cdfs_copy_from_cd(struct super_block * sb, int inode, unsigned int start, unsigned int stop, char * buf)
{
  int start_sector, start_byte, stop_sector, stop_byte, sector;
  int status=0;
  struct cdrom_read_audio cdda;
  unsigned int read_size=CD_FRAMESIZE_RAW;	//CD_FRAMESIZE_RAW=2352
  cd * this_cd = cdfs_info(sb);      
  //unsigned start_lba=this_cd->track[inode].start_lba;
  unsigned start_lba=this_cd->track[inode].start_lba;
  char * temp, * temp2;
  char * temp_start;
  int temp_length;

  start_lba=this_cd->track[inode].start_lba;
  
/* cache */
/*
  char * temp, * temp2;
  char * temp_start;
  int temp_length;
*/

  temp = this_cd->cache;

  start_sector = start/read_size;
  start_byte   = start%read_size;
  stop_sector  = stop/read_size;
  stop_byte    = stop%read_size;

  start_sector += start_lba;
  stop_sector  += start_lba;

  if (!stop_byte)
  {	/* empty frame */
  	stop_sector -= 1;
  	stop_byte    = CD_FRAMESIZE_RAW;	//CD_FRAMESIZE_RAW=2352
  }

  cdda.addr_format = CDROM_LBA;	//Using CDROM_LBA format
  cdda.nframes     = CACHE_SIZE;
  cdda.buf         = temp;

  // testen of eindadres>CD
  for (sector=start_sector; sector<=stop_sector; sector++)
  {

    PRINT("cache holds [%d-%d], we want sector=%d\n", this_cd->cache_sector, this_cd->cache_sector+CACHE_SIZE-1,  sector);

    if (!((this_cd->cache_sector<=sector) && (sector<this_cd->cache_sector+CACHE_SIZE)))
    {
      this_cd->cache_sector = cdda.addr.lba = sector;

	  //printk("<vcd module> Reading sector %d in ACD ST \n",cdda.addr.lba);
          status = cdfs_read_cd(sb, (&cdda), cdda.buf);
	  
      if (status)
      {
		  printk("<vcd module> Reading sector %d in ACD SP fail!!! status=%d\n", cdda.addr.lba, status);
		  return status;
      }
      /*
      else
		  printk("<vcd module> Reading sector %d in ACD SP OK status=%d\n", cdda.addr.lba, status);
      */
    }
    temp2=temp+(sector-this_cd->cache_sector)*CD_FRAMESIZE_RAW;	//CD_FRAMESIZE_RAW=2352
    if (sector==start_sector)
    {
		temp_start  = temp2+start_byte;
		if (sector!=stop_sector) 
		{
			temp_length = read_size-start_byte;
		}
		else
		{
			temp_length = stop_byte-start_byte;
		}
    }
    else if (sector==stop_sector)
    {
		temp_start  = temp2;
		temp_length = stop_byte;
    }
    else
    {
		temp_start  = temp2;
		temp_length = read_size;
    }
    memcpy(buf, (char*)temp_start, temp_length);
    buf += temp_length;
  }
  return status;
}

int cdfs_cdda_file_read(struct inode * inode, char * buf, size_t count, unsigned start, int raw)
{
  unsigned stop=start+count;

  int status=0;
  if (raw)	//For audio CD raw=0, CDDA will not enter here unless using -option, let raw=1
  { //Audio CD will not enter here
    status=cdfs_copy_from_cd(inode->i_sb, inode->i_ino, start, stop, buf);
  }
  else
  {
    //#define WAV_HEADER_SIZE 44 @ all.h
    //PRINT("david: start=%d,  stop=%d\n",start,stop);
    if (start < WAV_HEADER_SIZE)
    {
      if (stop > WAV_HEADER_SIZE)
      {		  //Enter here once to make the file header
		  //david: recurrsively calling subroutine!!
		  status=cdfs_cdda_file_read( inode,  buf,                        WAV_HEADER_SIZE-start,  start , 0 );
		  status=cdfs_cdda_file_read( inode,  buf+WAV_HEADER_SIZE-start,  stop-WAV_HEADER_SIZE,   WAV_HEADER_SIZE, 0 );
      }
	  else
	  {
		  char temp[44];
		  cdfs_make_header(temp, inode->i_size);
		  memcpy(buf, temp+start, stop-start);
      }
	}
	else
	{
		start -= WAV_HEADER_SIZE;
		stop  -= WAV_HEADER_SIZE;
		status=cdfs_copy_from_cd(inode->i_sb, inode->i_ino, start, stop, buf);
    }
  }
  return status;
}

struct file_operations cdfs_cdda_file_operations = {
read:               generic_file_read,
mmap:               generic_file_mmap
};


int kcdfsd_add_cdda_request(struct file * file, struct page *page)
{
  PRINT("david: Do nothing, later will return call daemon.c:kcdfsd_add_request() with CDDA_REQUEST=%d\n",CDDA_REQUEST);
  return kcdfsd_add_request(file->f_dentry, page, CDDA_REQUEST);
}

int kcdfsd_add_cdda_raw_request(struct file * file, struct page *page)
{
  return kcdfsd_add_request(file->f_dentry, page, CDDA_RAW_REQUEST);
}

struct address_space_operations cdfs_cdda_aops = {
readpage:           kcdfsd_add_cdda_request
};

struct address_space_operations cdfs_cdda_raw_aops = {
readpage:           kcdfsd_add_cdda_raw_request
};

int cdfs_read_cd(struct super_block *s, struct cdrom_read_audio *cdda, char* buff)
{
	static struct cdrom_generic_command cgc;
	static struct request_sense buffer2;
	int ret;
	int sector;
	ret=0;
	sector=(*cdda).addr.lba;
	cgc.sense = &buffer2;
	cgc.buffer = buff;
	cgc.buflen=CD_FRAMESIZE_RAW;
	cgc.data_direction=CGC_DATA_READ;
	cgc.quiet = 1;
	cgc.stat = 1;
	cgc.cmd[0]=GPCMD_READ_CD;
	//cgc.cmd[1]=1<<2; //Expected Sector Type
	cgc.cmd[1]=0;
	cgc.cmd[2]=(sector>>24)&0xff;      //Sector MSB
	cgc.cmd[3]=(sector>>16)&0xff;      //Sector
	cgc.cmd[4]=(sector>>8 )&0xff;      //Sector
	cgc.cmd[5]=(sector    )&0xff;      //Sector LSB
	cgc.cmd[6]=0;	//Transfer length in blocks MSB
	cgc.cmd[7]=0;	//Transfer length in blocks
	cgc.cmd[8]=1;	//Transfer length in blocks LSB
	//cgc.cmd[9]=1<<4;
	cgc.cmd[9]=0xf8;
	//cgc.cmd[10]=2;
	cgc.cmd[10]=0;
	cgc.cmd[11]=0;
	//printk("Reading sector %2d ST\n",sector);
	ret=cdfs_ioctl( s, CDROM_SEND_PACKET, (unsigned int)&cgc );
	//printk("Reading sector %2d SP and ret=%d asc=%d ascq=%d\n",sector,ret,(cgc.sense)->asc,(cgc.sense)->ascq);
	return ret;
}


