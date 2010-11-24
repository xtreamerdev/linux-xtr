/* 
  File iso.c - routines for ISO segments and boot images for cdfs
  
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

struct iso_primary_descriptor * cdfs_get_iso_info(struct super_block *sb, int track_no)
{
  cd * this_cd = cdfs_info(sb);
  struct buffer_head * bh;
  int block;
  struct iso_primary_descriptor * iso_info = NULL;

  iso_info=(struct iso_primary_descriptor *)kmalloc(sizeof(struct iso_primary_descriptor), GFP_KERNEL);	//ISO01
  PRINTM("ISO01 kmalloc addr = 0x %x, size = %6d for iso_info\n",(unsigned int)iso_info,sizeof(struct iso_primary_descriptor));
  
  if (iso_info==NULL)
  {
    printk(FSNAME ": kmalloc failed in cdfs_get_iso_info\n");
    return NULL;
  }

  block = this_cd->track[track_no].start_lba+16;  /* ISO info at sector 16 */

  if (!(bh = cdfs_bread(sb, block, CD_FRAMESIZE)))
  {	  //cdfs_bread fail condition
    	  PRINTM("ISO01 kfree   addr = 0x %x for iso_info since data is not read........\n",(unsigned int)iso_info);
    	  kfree(iso_info);	//ISO01
	  return NULL;
  }
  if (!strncmp(bh->b_data+1,"CD001",5))
  {
    PRINT("david: strncmp match 'CD001'\n");

    memcpy(iso_info, bh->b_data, sizeof(struct iso_primary_descriptor));   /* ISO session */
  }
  else
  {
    PRINTM("ISO01 kfree   addr = 0x %x for iso_info\n",(unsigned int)iso_info);
    kfree(iso_info);	//ISO01
    iso_info=NULL;	/* DATA, but no ISO */
  }
  brelse(bh);
  return iso_info;
}


