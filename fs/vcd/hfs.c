/*
  File hfs.c - routines for HFS partitions for cdfs

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

/* from /usr/src/linux/fs/hfs/part_tbl.c */

int cdfs_get_hfs_info(struct super_block *sb, unsigned track)
{
	return -1;	//doing nothing in order to speed up VCD mounting. No HFS support
}

void cdfs_copy_from_cdhfs(struct super_block * sb, int inode, unsigned int start, unsigned int stop, char * buf)
{
  int start_sector, start_byte, stop_sector, stop_byte, sector;
  unsigned int read_size=2048;
  cd * this_cd = cdfs_info(sb);      
  struct buffer_head * bh;
  unsigned start_lba=this_cd->track[inode].start_lba;
  char * temp;
  char * temp_start;
  int temp_length;
  char * buf_ptr;

  printk("hfs.c:cdfs_copy_from_cdhfs():AnyoneUseThis???\n");

  PRINT("-------------------------hfs.c:cdfs_copy_from_cdhfs()_ST\n");
  PRINT("start_lba=%d\n", start_lba);

  /* recalculate start and stop in bytes from the real start of the CD !! */
  start += start_lba*2048+ this_cd->track[inode].hfs_offset*512;
  stop  += start_lba*2048+ this_cd->track[inode].hfs_offset*512;

  start_sector = start/read_size;
  start_byte   = start - start_sector*read_size;
  stop_sector  = stop/read_size;
  stop_byte    = stop - stop_sector*read_size;
  
  PRINT("david: start_sector=%d  start_byte=%d  stop_sector=%d  stop_byte=%d\n",start_sector,start_byte,stop_sector,stop_byte);
  if (!stop_byte)
  {
    stop_sector--;
    stop_byte=2048;
  }

  PRINT("%d[%d-%d] -> 0x%x...0x%x  ... (%d,%d),(%d,%d)\n", 
         inode, start, stop, (int)buf, (int)buf+stop-start,
         start_sector,start_byte,stop_sector,stop_byte); 
  
  buf_ptr=buf;
  
  for (sector=start_sector; sector<=stop_sector; sector++)
  {
    PRINT("hfs.c:reading sector %d, lba=%d\n", sector, start_lba);
    if (!(bh = cdfs_bread(sb, sector, CD_FRAMESIZE)))
    { /* read 2048 bytes */
      PRINT("FAILED not this? not this?\n");
      return;
    }

    temp = bh->b_data;
    
    if (sector==start_sector) {
      temp_start  = temp+start_byte;
      if (sector!=stop_sector) 
        temp_length = read_size-start_byte;
      else
        temp_length = stop_byte-start_byte;
    } else if (sector==stop_sector) {
      temp_start  = temp;
      temp_length = stop_byte;
    } else {
      temp_start  = temp;
      temp_length = read_size;
    }
    
    PRINT("memcpy(0x%x, 0x%x, %d)\n", (int)buf_ptr, (int)temp_start, temp_length);
    
    memcpy(buf_ptr, (char*)temp_start, temp_length);

    brelse(bh);
    
    buf_ptr+=temp_length;
  }
};

struct file_operations cdfs_cdhfs_file_operations = {	//No one uses this
  .read             = generic_file_read,
  .mmap             = generic_file_mmap
};


int kcdfsd_add_cdhfs_request(struct file * file, struct page *page)
{
  printk("hfs.c:kcdfsd_add_cdhfs_request():AnyoneUseThis???\n");
  return kcdfsd_add_request(file->f_dentry, page, CDHFS_REQUEST);
}

struct address_space_operations cdfs_cdhfs_aops = {	//No one uses this
  .readpage         = kcdfsd_add_cdhfs_request
};


