/*
  File cddata.c - routines for 2048 byte frames for cdfs

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

#define CD_FRAMESIZE       2048 /* bytes per frame, "cooked" mode */

unsigned cdfs_data_bmap(struct super_block * sb, int inode, int block)
{
  int result;
  cd * this_cd = cdfs_info(sb);
  track_info * this_track = &(this_cd->track[inode]);
  int session;
  printk("cddata.c:cdfs_data_bmap():AnyoneUseThis???\n");
  if ((this_track->type == BOOT) || (this_track->type == HFS) || (block<20) || this_cd->single)  
    /* 20 sectors seems to be OK for ISO */ 

    result = this_track->start_lba+block;

  else {
    
    /* ISO past sector 18 */
    result = block;
    
    /* post processing */
    for (session=0; session < this_cd->nr_iso_sessions; session++)
    {
      PRINT("this is sector %d, checking session %d: %d-%d\n", result, session, 
            this_cd->lba_iso_sessions[session].start,
            this_cd->lba_iso_sessions[session].stop);
      if ((this_cd->lba_iso_sessions[session].start<=result)
          && (result<=this_cd->lba_iso_sessions[session].stop)) 
        goto exit;    /* OK, reading from an ISO session */
      if ((this_cd->lba_iso_sessions[session].stop<result)
          && (result<this_cd->lba_iso_sessions[session].start)) {
        result = 0;
        goto exit;    /* not OK, reading between two ISO sessions => we force 0*/
      }
    }

    /* We only get here if we read past the last session => we force 0 */
    result = 0;

  }

 exit: 

  //PRINT("BMAP('%s', block %d) => sector %d\n", this_track->name, block, result);

  return result;
}

struct file_operations cdfs_cddata_file_operations = {
  .read             = generic_file_read,
  .mmap             = generic_file_readonly_mmap,
  .sendfile         = generic_file_sendfile
};

struct address_space_operations cdfs_cddata_aops = {
  .readpage         = kcdfsd_add_cddata_request
};

int kcdfsd_add_cddata_request(struct file * file, struct page *page)
{
  //printk("cddata.c:kcdfsd_add_cddata_request():AnyoneUseThis???\n");
  return kcdfsd_add_request(file->f_dentry, page, CDDATA_REQUEST);
}

int cdfs_read_rawDATA_frame(struct super_block * sb, unsigned lba, unsigned char *buf)
{
  struct buffer_head * bh;
  printk("cddata.c:cdfs_read_rawDATA_frame():AnyoneUseThis???\n");
  bh=cdfs_bread(sb, lba, CD_FRAMESIZE);
  if (!bh) {
    // dit gebeurt indien track1!=ISO is, bv. De Morgen CD
    printk("cdfs_read_rawDATA(%x, %u, %x) FAILED!\n", (unsigned)sb, lba, (unsigned)buf);
  } else {
    memcpy(buf, bh->b_data, CD_FRAMESIZE);
    brelse(bh);
  }
  return 0;
}

void cdfs_copy_from_cddata(struct super_block * sb, int inode, unsigned int start,
	unsigned int stop, char * buf){
  int start_sector, start_byte, stop_sector, stop_byte, sector;
  int status;
  cd * this_cd = cdfs_info(sb);
  unsigned int data_size = CD_FRAMESIZE;
  printk("cddata.c:cdfs_copy_from_cddata():AnyoneUseThis???\n");
    
  start_sector = start / data_size;
  start_byte   = start % data_size;
  stop_sector  = stop  / data_size;
  stop_byte    = stop  % data_size;

  if (!stop_byte) {            /* empty frame */
    stop_sector -= 1;
    stop_byte    = CD_FRAMESIZE;
  }
  
  PRINT("%d[%d-%d] -> 0x%x...0x%x  ... (%d,%d),(%d,%d)\n",
        inode, start, stop, (int)buf, (int)buf+stop-start,
        start_sector,start_byte,stop_sector,stop_byte);

  for (sector=start_sector; sector<=stop_sector; sector++) {
    
    unsigned lba = cdfs_data_bmap(sb, inode, sector);
    
    if (!(this_cd->cache_sector == lba))
    {
    	this_cd->cache_sector = lba;
    	if((status = cdfs_read_rawDATA_frame(sb, lba, this_cd->cache)))
    	{
    		printk("copy_from_cddata(%d): ioctl failed: %d\n", lba, status);
    		return;
    	}
    }


    
    {
      char * copy_start;
      int copy_length;
      
      if (sector==start_sector) {
        copy_start  = this_cd->cache+start_byte;
        if (sector!=stop_sector)
          copy_length = data_size-start_byte;
        else
          copy_length = stop_byte-start_byte;
      } else if (sector==stop_sector) {
        copy_start  = this_cd->cache;
        copy_length = stop_byte;
      } else {
        copy_start  = this_cd->cache;
        copy_length = data_size;
      }
      PRINT("memcpy(0x%x, 0x%x, %d)\n", (int)buf, (int)copy_start, copy_length);      
      memcpy(buf, (char*)copy_start, copy_length);
      buf+=copy_length;
    }

  }
}


