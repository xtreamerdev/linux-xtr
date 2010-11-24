/*

  File toc.c - superblock and module routines for cdfs
  
  Copyright (c) 2003 by Laurent Pinchart
  
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

extern int CD_TYPE;
extern int First_AVCD;
extern int TRACK_NO;
extern int SESSION_NO;
extern int VIDEOCD_NO;
extern int DVD_disc;
extern int EACD_disc;

int cdfs_toc_read( struct super_block *sb ,int mounting_acd)
{
	if(DVD_disc)
		return cdfs_toc_read_dvd(sb);
	else
		return cdfs_toc_read_cd(sb,mounting_acd);
}

int cdfs_toc_read_cd( struct super_block *sb , int mounting_acd)
{
    int i, t;
    int DataType;
    int PureVCD;
    int PureACD;
    int no_audio;
    int no_data;
    struct cdrom_tochdr hdr;
    struct cdrom_tocentry entry;
    cd * this_cd = cdfs_info( sb );
    DataType=0;
    PureVCD=0;
    PureACD=0;
    no_audio=0;
    no_data=0;
    if ( cdfs_ioctl( sb, CDROMREADTOCHDR, (unsigned long)&hdr ) )
    {
        printk( "<vcd module> Reading track number fails.... 1 ioctl(CDROMREADTOCHDR) failed, print associated data...\n" );
	this_cd->tracks=find_track_info(sb);
	if(!this_cd->tracks)
		return -1;
    }
    else
    {
	    this_cd->tracks = hdr.cdth_trk1 - hdr.cdth_trk0 + 1;
	    PRINTT( "toc.c:cdfs_toc_read: CD contains %d tracks: %d-%d\n", this_cd->tracks, hdr.cdth_trk0, hdr.cdth_trk1 );
    }
    /* Collect track info */
    entry.cdte_format = CDROM_LBA;
    this_cd->track[0].start_lba  = 0;
    this_cd->track[1].start_lba  = 0;
    this_cd->track[2].start_lba  = 0;
    for ( t = this_cd->tracks; t >= 0; --t )
    {
        i = T2I(t);	// T2I: Convert track to inode number, i=t+3

        entry.cdte_track = ( t == this_cd->tracks ) ? CDROM_LEADOUT : t+1;
        PRINTT( "Read track %d/%d/%d\n", entry.cdte_track, t, i );
        if ( cdfs_ioctl( sb, CDROMREADTOCENTRY, (unsigned long)&entry ) )
        {
	    printk( "<vcd module> Reading track lba_ST fails.... 2 ioctl(CDROMREADTOCENTRY) failed\n" );
            return -1;
        }
        this_cd->track[i].start_lba  = entry.cdte_addr.lba;
        this_cd->track[i].stop_lba   = this_cd->track[i+1].start_lba - 1;
        if ( t != this_cd->tracks )	/* all tracks but the LEADOUT */
        {
        	this_cd->track[i].type = ( entry.cdte_ctrl & CDROM_DATA_TRACK ) ? DATA : AUDIO;	//Determine track type
        	this_cd->track[i].time = get_seconds();
		PRINTT("this_cd->track[%d].type=%d\n",i,this_cd->track[i].type);
        	PRINTT("entry.cdte_ctrl=%d  CDROM_DATA_TRACK=%d  DATA=%d  AUDIO=%d\n",entry.cdte_ctrl,CDROM_DATA_TRACK,DATA,AUDIO);
        	if(this_cd->track[i].type==AUDIO)	//Audio Track
        	{
        		no_audio++;
        	}
        	else if(this_cd->track[i].type==DATA)	//Data or VCD Track
        	{
        		no_data++;
        	}
        	else	//Other Track
        	{
        		PRINTT("=> OTHERS TYPE.\n");
        	}
        	DataType+=this_cd->track[i].type;
        	PureVCD+=2;
        }
    }
    
    if(mounting_acd)
    {
	    if ((DataType==this_cd->tracks)&&SESSION_NO==1)
	    {
		    PRINTT("This disc => Pure ACD one session\n");	//ACD
		    CD_TYPE=ACD_DISC;
	    }
	    else if ((DataType==(this_cd->tracks+1))&&SESSION_NO==2)	//EACD
	    {
		    PRINTT("This disc => EACD 2 session\n");
		    CD_TYPE=ACD_DISC;
	    }
	    else	//BAD DISC
	    {       //david0821 added to check if someone goes the wrong way.....
		    PRINTT("BAD_DISC\n");
		    CD_TYPE=BAD_DISC;
		    return -1;
	    }
    }
    else if ((DataType==PureVCD)&&(SESSION_NO==1)&&(this_cd->tracks>1)&&(no_data>1))	//VCD
    {
	CD_TYPE=VCD_DISC;
    }
    else if (DataType==PureVCD)			//DATA_CD
    {
	CD_TYPE=DATA_DISC;
    }
    else if ((SESSION_NO==1)&&(no_data>1))	//AVCD
    {
	CD_TYPE=AVCD_DISC;
	First_AVCD=no_data+1;
    }
    else if (SESSION_NO!=1)		//EACD
    {
	CD_TYPE=EACD_DISC;
    }
    else	//other disc => data
    {
    	PRINTT("Not supported yet\n");
	CD_TYPE=DATA_DISC;
    }
    PRINTT("Summary of this CD, track_no = this_CD->tracks = %d\n",this_cd->tracks);
    return 0;
}

int find_track_info2(struct super_block *sb, int track_no)
{
	struct cdrom_generic_command *cgc;
	int ret;
	int i;
	ret=0;
	if (!(cgc = kmalloc(sizeof(struct cdrom_generic_command), GFP_KERNEL)))	//T01
	{
		return -ENOMEM;
	}
	PRINTM("T01 kmalloc addr = 0x %x, size = %6d for cgc\n",(unsigned int)cgc,sizeof(struct cdrom_generic_command));
	memset(cgc,   0,sizeof(struct cdrom_generic_command));
	if (!(cgc->sense = kmalloc(sizeof(struct request_sense), GFP_KERNEL)))	//T02
	{
		return -ENOMEM;
	}
	PRINTM("T02 kmalloc addr = 0x %x, size = %6d for cgc->sense\n",(unsigned int)cgc->sense,sizeof(struct request_sense));
	memset(cgc->sense, 0,sizeof(struct request_sense));
	if (!(cgc->buffer = kmalloc(36, GFP_KERNEL)))	//T03
	{
		return -ENOMEM;
	}
	PRINTM("T03 kmalloc addr = 0x %x, size = %6d for cgc->buffer\n",(unsigned int)cgc->buffer,36);
	memset(cgc->buffer,0,36);

	cgc->buflen=36;
	cgc->data_direction=CGC_DATA_READ;
	cgc->quiet = 1;
	cgc->stat = 1;
	cgc->cmd[0]=GPCMD_READ_TRACK_RZONE_INFO;
	//cgc->cmd[0]=0x52;
	cgc->cmd[1]=1;			//Address/Number Type
	cgc->cmd[5]=track_no;
	cgc->cmd[7]=0;
	cgc->cmd[8]=36;
	ret=cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)cgc );

	printk("For track_no=%d INFO_L=%d\n",track_no,cgc->buffer[ 0]<< 8|cgc->buffer[ 1]);
	printk("For track_no=%d TRK_NO=%d\n",track_no,cgc->buffer[ 2]);
	printk("For track_no=%d SES_NO=%d\n",track_no,cgc->buffer[ 3]);
	printk("For track_no=%d T_mode=%d\n",track_no,cgc->buffer[ 5]);
	printk("For track_no=%d D_mode=%d\n",track_no,cgc->buffer[ 6]);
	printk("For track_no=%d LBS_ST=%d\n",track_no,cgc->buffer[ 8]<<24|cgc->buffer[ 9]<<16|cgc->buffer[10]<<8|cgc->buffer[11]);
	printk("For track_no=%d NWA   =%d\n",track_no,cgc->buffer[12]<<24|cgc->buffer[13]<<16|cgc->buffer[14]<<8|cgc->buffer[15]);
	printk("For track_no=%d F_blk =%d\n",track_no,cgc->buffer[16]<<24|cgc->buffer[17]<<16|cgc->buffer[18]<<8|cgc->buffer[19]);
	printk("For track_no=%d F_size=%d\n",track_no,cgc->buffer[20]<<24|cgc->buffer[21]<<16|cgc->buffer[22]<<8|cgc->buffer[23]);
	printk("For track_no=%d SIZE  =%d\n",track_no,cgc->buffer[24]<<24|cgc->buffer[25]<<16|cgc->buffer[26]<<8|cgc->buffer[27]);
	printk("For track_no=%d LAST  =%d\n",track_no,cgc->buffer[28]<<24|cgc->buffer[29]<<16|cgc->buffer[30]<<8|cgc->buffer[31]);
	printk("For track_no=%d TRK_NO=%d\n",track_no,cgc->buffer[32]);
	printk("For track_no=%d SES_NO=%d\n",track_no,cgc->buffer[33]);

	for(i=0;i<34;i++)
		printk("data[%d]=%d\n",i,cgc->buffer[i]);

	PRINTM("T03 kfree   addr = 0x %x for cgc->buffer\n",(unsigned int)cgc->buffer);
	kfree(cgc->buffer);	//T03
	PRINTM("T02 kfree   addr = 0x %x for cgc->sense\n",(unsigned int)cgc->sense);
	kfree(cgc->sense);	//T02
	PRINTM("T01 kfree   addr = 0x %x for cgc\n",(unsigned int)cgc);
	kfree(cgc);		//T01
	return ret;
}

int find_disc_info(struct super_block *sb)
{
	struct cdrom_generic_command *cgc;
	int session_no;
	int track_no;
	int ret;
	int t;
	int i;
	cd * this_cd = cdfs_info( sb );

	ret=0;
	//printk("find_disc_info_ST\n");
	if (!(cgc = kmalloc(sizeof(struct cdrom_generic_command), GFP_KERNEL)))//T04
	{
		return -ENOMEM;
	}
	PRINTM("T04 kmalloc addr = 0x %x, size = %6d for cgc\n",(unsigned int)cgc,sizeof(struct cdrom_generic_command));
	memset(cgc,   0,sizeof(struct cdrom_generic_command));
	if (!(cgc->sense = kmalloc(sizeof(struct request_sense), GFP_KERNEL)))	//T05
	{
		return -ENOMEM;
	}
	PRINTM("T05 kmalloc addr = 0x %x, size = %6d for cgc->sense\n",(unsigned int)cgc->sense,sizeof(struct request_sense));
	memset(cgc->sense, 0,sizeof(struct request_sense));
	if (!(cgc->buffer = kmalloc(34, GFP_KERNEL)))	//T06
	{
		return -ENOMEM;
	}
	PRINTM("T06 kmalloc addr = 0x %x, size = %6d for cgc->buffer\n",(unsigned int)cgc->buffer,34);
	memset(cgc->buffer,0,34);
	cgc->buflen=34;
	cgc->data_direction=CGC_DATA_READ;
	cgc->quiet = 1;
	cgc->stat = 1;
	cgc->cmd[0]=GPCMD_READ_DISC_INFO;
	cgc->cmd[8]=34;
	ret=cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)cgc );
	//printk("cgc->cmd[%d]=%d\n",0,cgc->cmd[0]);
	track_no  =cgc->buffer[6]-cgc->buffer[5]+1;
	session_no=cgc->buffer[4]-cgc->buffer[3]+1;
	//printk("There are %2d sessions in this disc\n",session_no);
	//printk("There are %2d tracks   in the last session\n",track_no);
	PRINTM("T06 kfree   addr = 0x %x for cgc->buffer\n",(unsigned int)cgc->buffer);
	kfree(cgc->buffer);	//T06
	PRINTM("T05 kfree   addr = 0x %x for cgc->sense\n",(unsigned int)cgc->sense);
	kfree(cgc->sense);	//T05
	PRINTM("T04 kfree   addr = 0x %x for cgc\n",(unsigned int)cgc);
	kfree(cgc);		//T04	


	PRINTT( "toc.c:cdfs_toc_read: CD contains %d tracks.\n", this_cd->tracks);
	for ( t = this_cd->tracks; t >= 0; --t )
	{
		i = T2I(t);	// T2I: Convert track to inode number, i=t+3
		if ( t != this_cd->tracks )	/* all tracks but the LEADOUT */
		{
			if(this_cd->track[i].type==AUDIO)
			{
        			PRINTT("=> 音樂 for track %d\n",t+1);
			}
			else if(this_cd->track[i].type==DATA)
			{
				PRINTT("=> 資料  (VCD算是資料) for track %d\n",t+1);
			}
			else
			{
				PRINTT("=> 其他資料型態\n");
				PRINTT("others\n");
			}
		}
	}
	
	//printk("find_disc_info_SP\n");
	
	return ret;
}

int cdfs_toc_read_dvd( struct super_block *sb )
{
	struct cdrom_generic_command *cgc;
	int session_no;
	int track_no;
	int ret;
	int i, t;
	int DataType;
	int PureVCD;
	int PureACD;
	int no_audio;
	int no_data;
	int lba_st;
	int lba_sp;
	int track_size;
	cd * this_cd = cdfs_info( sb );
	DataType=0;
	PureVCD=0;
	PureACD=0;
	no_audio=0;
	no_data=0;
	ret=0;
	if (!(cgc = kmalloc(sizeof(struct cdrom_generic_command), GFP_KERNEL)))	//T07
	{
		return -ENOMEM;
	}
	PRINTM("T07 kmalloc addr = 0x %x, size = %6d for cgc\n",(unsigned int)cgc,sizeof(struct cdrom_generic_command));
	memset(cgc,   0,sizeof(struct cdrom_generic_command));
	if (!(cgc->sense = kmalloc(sizeof(struct request_sense), GFP_KERNEL)))	//T08
	{
		return -ENOMEM;
	}
	PRINTM("T08 kmalloc addr = 0x %x, size = %6d for cgc->sense\n",(unsigned int)cgc->sense,sizeof(struct request_sense));
	memset(cgc->sense, 0,sizeof(struct request_sense));
	if (!(cgc->buffer = kmalloc(34, GFP_KERNEL)))	//T09
	{
		return -ENOMEM;
	}
	PRINTM("T09 kmalloc addr = 0x %x, size = %6d for cgc->buffer\n",(unsigned int)cgc->buffer,34);
	memset(cgc->buffer,0,34);
	cgc->buflen=34;
	cgc->data_direction=CGC_DATA_READ;
	cgc->quiet = 1;
	cgc->stat = 1;
	cgc->cmd[0]=GPCMD_READ_DISC_INFO;
	cgc->cmd[8]=34;
	ret=cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)cgc );
	PRINTT("cgc->cmd[%d]=%d\n",0,cgc->cmd[0]);
	track_no  =cgc->buffer[6]-cgc->buffer[5]+1;
	session_no=cgc->buffer[4]-cgc->buffer[3]+1;
	PRINTT("There are %2d tracks   in this disc\n",track_no);
	PRINTT("There are %2d sessions in this disc\n",session_no);
	PRINTM("T09 kfree   addr = 0x %x for cgc->buffer\n",(unsigned int)cgc->buffer);
	kfree(cgc->buffer);	//T09
	PRINTM("T08 kfree   addr = 0x %x for cgc->sense\n",(unsigned int)cgc->sense);
	kfree(cgc->sense);	//T08
	PRINTM("T07 kfree   addr = 0x %x for cgc\n",(unsigned int)cgc);
	kfree(cgc);		//T07
    	this_cd->tracks = track_no;
	PRINTT( "This DVD contains %d tracks.\n", this_cd->tracks );
	/* Collect track info */

	this_cd->track[0].start_lba  = 0;
	this_cd->track[1].start_lba  = 0;
	this_cd->track[2].start_lba  = 0;
	for ( t = this_cd->tracks; t >= 0; --t )
	{
		i = T2I(t);	// T2I: Convert track to inode number, i=t+3
		cgc->buflen=36;
		cgc->data_direction=CGC_DATA_READ;
		cgc->quiet = 1;
		cgc->stat = 1;
		cgc->cmd[0]=GPCMD_READ_TRACK_RZONE_INFO;
		//cgc->cmd[0]=0x52;
		cgc->cmd[1]=1;			//Address/Number Type
		//cgc->cmd[5]=track_no;
		cgc->cmd[5]=t;
		cgc->cmd[7]=0;
		cgc->cmd[8]=36;
		ret=cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)cgc );
		
		/*
		printk("For track_no=%d INFO_L=%d\n",track_no,cgc->buffer[ 0]<< 8|cgc->buffer[ 1]);
		printk("For track_no=%d TRK_NO=%d\n",track_no,cgc->buffer[ 2]);
		printk("For track_no=%d SES_NO=%d\n",track_no,cgc->buffer[ 3]);
		printk("For track_no=%d T_mode=%d\n",track_no,cgc->buffer[ 5]);
		printk("For track_no=%d D_mode=%d\n",track_no,cgc->buffer[ 6]);
		printk("For track_no=%d LBS_ST=%d\n",track_no,cgc->buffer[ 8]<<24|cgc->buffer[ 9]<<16|cgc->buffer[10]<<8|cgc->buffer[11]);
		printk("For track_no=%d NWA   =%d\n",track_no,cgc->buffer[12]<<24|cgc->buffer[13]<<16|cgc->buffer[14]<<8|cgc->buffer[15]);
		printk("For track_no=%d F_blk =%d\n",track_no,cgc->buffer[16]<<24|cgc->buffer[17]<<16|cgc->buffer[18]<<8|cgc->buffer[19]);
		printk("For track_no=%d F_size=%d\n",track_no,cgc->buffer[20]<<24|cgc->buffer[21]<<16|cgc->buffer[22]<<8|cgc->buffer[23]);
		printk("For track_no=%d LAST  =%d\n",track_no,cgc->buffer[28]<<24|cgc->buffer[29]<<16|cgc->buffer[30]<<8|cgc->buffer[31]);
		printk("For track_no=%d TRK_NO=%d\n",track_no,cgc->buffer[32]);
		printk("For track_no=%d SES_NO=%d\n",track_no,cgc->buffer[33]);
		*/
		/*
		for(k=0;k<34;k++)
			printk("data[%d]=%d\n",k,cgc->buffer[k]);
		*/
		
		//i = T2I(t);	// T2I: Convert track to inode number, i=t+3

		//entry.cdte_track = ( t == this_cd->tracks ) ? CDROM_LEADOUT : t+1;
		//printk("For track_no=%d SIZE  =%d\n",track_no,cgc->buffer[24]<<24|cgc->buffer[25]<<16|cgc->buffer[26]<<8|cgc->buffer[27]);

		track_size=cgc->buffer[24]<<24|cgc->buffer[25]<<16|cgc->buffer[26]<<8|cgc->buffer[27];

		lba_st=cgc->buffer[ 8]<<24|cgc->buffer[ 9]<<16|cgc->buffer[10]<<8|cgc->buffer[11];
		lba_st=0;
		this_cd->track[i].start_lba = lba_st;
		lba_sp=cgc->buffer[24]<<24|cgc->buffer[25]<<16|cgc->buffer[26]<<8|cgc->buffer[27];
		this_cd->track[i].stop_lba  = lba_sp-2;
		this_cd->track[i].track_size  = track_size-1+100000;
		this_cd->track[i].size  = track_size-1+100000;

		//i=t+2;
		//printk("Track %d LBA：Start[%d]: %d\n", i-2, i, this_cd->track[i].start_lba);
		if ( t != this_cd->tracks )	/* all tracks but the LEADOUT */
		{
			//this_cd->track[i].type = ( entry.cdte_ctrl & CDROM_DATA_TRACK ) ? DATA : AUDIO;
			this_cd->track[i].type = 2; //now only support DVD iso966
			PRINTT("this_cd->track[%d].type=%d\n",i,this_cd->track[i].type);
			this_cd->track[i].time = get_seconds();
			DataType+=this_cd->track[i].type;
			PureVCD+=2;
		}
	}
	return 0;
}

int find_session_no(struct super_block *sb)
{
	struct cdrom_generic_command *cgc;
	int session_no;
	if (!(cgc = kmalloc(sizeof(struct cdrom_generic_command), GFP_KERNEL)))	//T10
	{
		return -ENOMEM;
	}
	PRINTM("T10 kmalloc addr = 0x %x, size = %6d for cgc\n",(unsigned int)cgc,sizeof(struct cdrom_generic_command));
	memset(cgc,   0,sizeof(struct cdrom_generic_command));
	if (!(cgc->sense = kmalloc(sizeof(struct request_sense), GFP_KERNEL)))	//T11
	{
		return -ENOMEM;
	}
	PRINTM("T11 kmalloc addr = 0x %x, size = %6d for cgc->sense\n",(unsigned int)cgc->sense,sizeof(struct request_sense));
	memset(cgc->sense, 0,sizeof(struct request_sense));
	if (!(cgc->buffer = kmalloc(34, GFP_KERNEL)))	//T12
	{
		return -ENOMEM;
	}
	PRINTM("T12 kmalloc addr = 0x %x, size = %6d for cgc->buffer\n",(unsigned int)cgc->buffer,34);
	memset(cgc->buffer,0,34);
	cgc->buflen=34;
	cgc->data_direction=CGC_DATA_READ;
	cgc->quiet = 1;
	cgc->stat = 1;
	cgc->cmd[0]=GPCMD_READ_DISC_INFO;
	cgc->cmd[8]=34;
	cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)cgc );
	session_no=cgc->buffer[4]-cgc->buffer[3]+1;

	/*
	int i;
	printk("session_no=%d\n",session_no);
	for(i=0;i<24;i++)
	{
		printk("buffer[%d]=%d\n",i,cgc->buffer[i]);
	}
	*/

	
	PRINTM("T12 kfree   addr = 0x %x for cgc->buffer\n",(unsigned int)cgc->buffer);
	kfree(cgc->buffer);	//T12
	PRINTM("T11 kfree   addr = 0x %x for cgc->sense\n",(unsigned int)cgc->sense);
	kfree(cgc->sense);	//T11
	PRINTM("T10 kfree   addr = 0x %x for cgc\n",(unsigned int)cgc);
	kfree(cgc);		//T10
	return session_no;
}

int isEACD(struct super_block *sb)
{
	int i, t;
	int result=0;
	struct cdrom_tochdr hdr;
	struct cdrom_tocentry entry;   
	cd * this_cd;
	if (!(this_cd = cdfs_info2(sb) = kmalloc(sizeof(cd), GFP_KERNEL)))//T13
	{
		return -ENOMEM;
	}
	PRINTM("T13 kmalloc addr = 0x %x, size = %6d for this_cd\n",(unsigned int)this_cd,sizeof(cd));
	if ( cdfs_ioctl( sb, CDROMREADTOCHDR, (unsigned long)&hdr ) )
	{
        	printk( "<vcd module> Reading track number fails.... 3 ioctl(CDROMREADTOCHDR) failed\n" );
		return -1;
	}
	this_cd->tracks = hdr.cdth_trk1 - hdr.cdth_trk0 + 1;
	PRINTT( "toc.c:cdfs_toc_read: CD contains %d tracks: %d-%d\n", this_cd->tracks, hdr.cdth_trk0, hdr.cdth_trk1 );
	entry.cdte_format = CDROM_LBA;
	this_cd->track[0].start_lba  = 0;
	this_cd->track[1].start_lba  = 0;
	this_cd->track[2].start_lba  = 0;
	for ( t = this_cd->tracks; t >= 0; --t )
	{
		i = T2I(t);	// T2I: Convert track to inode number, i=t+3
		entry.cdte_track = ( t == this_cd->tracks ) ? CDROM_LEADOUT : t+1;
		PRINTT( "Read track %d/%d/%d\n", entry.cdte_track, t, i );
		if ( cdfs_ioctl( sb, CDROMREADTOCENTRY, (unsigned long)&entry ) )
		{
        		printk( "<vcd module> Reading track number fails.... 4 ioctl(CDROMREADTOCENTRY) failed\n" );
			return -1;
		}
		if ( t != this_cd->tracks )	/* all tracks but the LEADOUT */
		{
			this_cd->track[i].type = ( entry.cdte_ctrl & CDROM_DATA_TRACK ) ? DATA : AUDIO;
		}
	}
	for(t=0;t<this_cd->tracks;t++)
	{
		PRINTT("track %d data_type=%d\n",t+1,this_cd->track[t+3].type);
		result+=this_cd->track[t+3].type;
	}
	PRINTM("T13 kfree   addr = 0x %x for this_cd\n",(unsigned int)this_cd);
	kfree(this_cd);	//T13
	PRINTT("isEACD_SP and result=%d\n",result);
	if((result==(this_cd->tracks+1))&&(this_cd->tracks>1))
	{
		PRINTT("This is an Enhanced Music CD\n");
		return 1;
	}
	else
	{
		PRINTT("This is NOT an Enhanced Music CD\n");
		return 0;
	}
}

int find_track_info(struct super_block *sb)
{
	struct cdrom_generic_command *cgc;
	int session_no;
	int i;
	int ret;
	if (!(cgc = kmalloc(sizeof(struct cdrom_generic_command), GFP_KERNEL)))	//T10
	{
		return -ENOMEM;
	}
	PRINTM("T10 kmalloc addr = 0x %x, size = %6d for cgc\n",(unsigned int)cgc,sizeof(struct cdrom_generic_command));
	memset(cgc,   0,sizeof(struct cdrom_generic_command));
	if (!(cgc->sense = kmalloc(sizeof(struct request_sense), GFP_KERNEL)))	//T11
	{
		return -ENOMEM;
	}
	PRINTM("T11 kmalloc addr = 0x %x, size = %6d for cgc->sense\n",(unsigned int)cgc->sense,sizeof(struct request_sense));
	memset(cgc->sense, 0,sizeof(struct request_sense));
	if (!(cgc->buffer = kmalloc(34, GFP_KERNEL)))	//T12
	{
		return -ENOMEM;
	}
	PRINTM("T12 kmalloc addr = 0x %x, size = %6d for cgc->buffer\n",(unsigned int)cgc->buffer,34);
	memset(cgc->buffer,0,34);
	cgc->buflen=34;
	cgc->data_direction=CGC_DATA_READ;
	cgc->quiet = 1;
	cgc->stat = 1;
	cgc->cmd[0]=GPCMD_READ_DISC_INFO;
	cgc->cmd[8]=34;
	cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)cgc );
	session_no=cgc->buffer[4]-cgc->buffer[3]+1;
	printk("session_no=%d\n",session_no);
	for(i=0;i<24;i++)
	{
		printk("buffer[%d]=%d\n",i,cgc->buffer[i]);
	}
	ret=cgc->buffer[6];
/*
	for(i=0;i<cgc->buffer[6];i++)
	{
		find_track_info2(sb, i);
	}
*/
	PRINTM("T12 kfree   addr = 0x %x for cgc->buffer\n",(unsigned int)cgc->buffer);
	kfree(cgc->buffer);	//T12
	PRINTM("T11 kfree   addr = 0x %x for cgc->sense\n",(unsigned int)cgc->sense);
	kfree(cgc->sense);	//T11
	PRINTM("T10 kfree   addr = 0x %x for cgc\n",(unsigned int)cgc);
	kfree(cgc);		//T10
	return ret;
}

int readtocentry(struct super_block *sb)
{
	struct cdrom_generic_command *cgc;
	int i;
	int ret;
	if (!(cgc = kmalloc(sizeof(struct cdrom_generic_command), GFP_KERNEL)))	//T10
	{
		return -ENOMEM;
	}
	PRINTM("T10 kmalloc addr = 0x %x, size = %6d for cgc\n",(unsigned int)cgc,sizeof(struct cdrom_generic_command));
	memset(cgc,   0,sizeof(struct cdrom_generic_command));
	if (!(cgc->sense = kmalloc(sizeof(struct request_sense), GFP_KERNEL)))	//T11
	{
		return -ENOMEM;
	}
	PRINTM("T11 kmalloc addr = 0x %x, size = %6d for cgc->sense\n",(unsigned int)cgc->sense,sizeof(struct request_sense));
	memset(cgc->sense, 0,sizeof(struct request_sense));
	if (!(cgc->buffer = kmalloc(34, GFP_KERNEL)))	//T12
	{
		return -ENOMEM;
	}
	PRINTM("T12 kmalloc addr = 0x %x, size = %6d for cgc->buffer\n",(unsigned int)cgc->buffer,34);
	memset(cgc->buffer,0,34);
	cgc->buflen=34;
	cgc->data_direction=CGC_DATA_READ;
	cgc->quiet = 1;
	cgc->stat = 1;
	//cgc->cmd[0]=GPCMD_READ_DISC_INFO;
	cgc->cmd[0]=0x43;
	cgc->cmd[1]=0;
	cgc->cmd[6]=0;
	cgc->cmd[8]=0x0c;
	cgc->cmd[9]=0x40;
	cdfs_ioctl( sb, CDROM_SEND_PACKET, (unsigned int)cgc );
	
	for(i=0;i<12;i++)
	{
		printk("buffer[%d]=%d\n",i,cgc->buffer[i]);
	}
	ret=cgc->buffer[6];
/*
	for(i=0;i<cgc->buffer[6];i++)
	{
		find_track_info2(sb, i);
	}
*/
	PRINTM("T12 kfree   addr = 0x %x for cgc->buffer\n",(unsigned int)cgc->buffer);
	kfree(cgc->buffer);	//T12
	PRINTM("T11 kfree   addr = 0x %x for cgc->sense\n",(unsigned int)cgc->sense);
	kfree(cgc->sense);	//T11
	PRINTM("T10 kfree   addr = 0x %x for cgc\n",(unsigned int)cgc);
	kfree(cgc);		//T10
	return ret;
}


