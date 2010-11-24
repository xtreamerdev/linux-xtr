/*
   File daemon.c - kernel thread routines for asynchronuous I/O

   Initial code written by Chih-Chung Chang

   Copyright (c) 2000, 2001 by Michiel Ronsse

   Support for kmap by  Peter Korf <peter@niendo.de>
   
   NULL checking after kmalloc + Clean daemon removing by Thibault Mondary <thibm82@yahoo.fr>

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

int kcdfsd_pid = 0;
static int kcdfsd_running = 0;
static DECLARE_WAIT_QUEUE_HEAD(kcdfsd_wait);
static LIST_HEAD(kcdfsd_req_list);       /* List of requests needing servicing */

struct kcdfsd_req {
  struct list_head req_list;
  struct dentry *dentry;
  struct page *page;
  unsigned request_type;
};

int kcdfsd_add_request(struct dentry *dentry, struct page *page, unsigned request)
{
  	struct kcdfsd_req *req = NULL;
  	int count=0;
	if (!PageLocked(page)) BUG();
	        if (PageError(page)){
		printk("<vcd module> page %lu error!!\n", page->index);
		unlock_page(page);
		return(-EIO);
	}
		       
	do {
     		req = (struct kcdfsd_req *) kmalloc (sizeof (struct kcdfsd_req), GFP_KERNEL);	//D01
		PRINTM("D01 kmalloc addr = 0x %x, size = %6d for req\n",(unsigned int)req,sizeof(struct kcdfsd_req));
    		count++;
  	} while (req==NULL && count <5);
   
	if (req != NULL)
	{
		INIT_LIST_HEAD (&req->req_list);
		req->dentry = dentry;
		req->page = page;
		req->request_type = request;
		preempt_disable();
		list_add_tail (&req->req_list, &kcdfsd_req_list);
		preempt_enable();
		wake_up (&kcdfsd_wait);
		return(0);
	}
	unlock_page(page);
	return (-ENOMEM);
}

static void kcdfsd_process_request(void)
{
	struct list_head * tmp;
	struct kcdfsd_req * req;
	struct page * page;
	struct inode * inode;
	unsigned request;
	struct iso_inode_info *ei;
	int return_status=0;
	while (!list_empty (&kcdfsd_req_list))
	{    	/* Grab the next entry from the beginning of the list */
		preempt_disable();
		tmp = kcdfsd_req_list.next;
		if ((unsigned long)tmp < (unsigned long)0x80000000) 
		{
			printk("tmp: %x, tmp->prev: %x, tmp->next: %x", (int)tmp, (int)tmp->prev, (int)tmp->next);
			BUG();
		}
		req = list_entry (tmp, struct kcdfsd_req, req_list);
		list_del (tmp);
		preempt_enable();
		page = req->page;
		inode = req->dentry->d_inode;
		ei = ISOFS_I(inode);		// added by Frank 94/9/6
		request = req->request_type;
		if (!PageLocked(page))
			BUG();
		if(inode)
		{
		switch (request)
		{
		case CDDA_REQUEST:
		case CDDA_RAW_REQUEST:
		{
			cd *this_cd = cdfs_info (inode->i_sb);
			char *p;
			track_info *this_track = &(this_cd->track[inode->i_ino]);
			//struct super_block *sb = inode->i_sb;	//inode->i_sb : pointer to superblock object
			//this_track->avi=this_cd->track[10].start_lba;
			this_track->avi=0;
			this_track->avi_offset=0;
			this_track->avi_swab=0;
			
			{
				return_status=cdfs_cdda_file_read (inode,
					p = (char *) kmap (page),
					1 << PAGE_CACHE_SHIFT,
					(page->index << PAGE_CACHE_SHIFT) + ((this_track->avi) ? this_track->avi_offset : 0),
					(request == CDDA_RAW_REQUEST));
			}
		}
		break;
		
		case CDXA_REQUEST:
			return_status=cdfs_copy_from_cdXA(inode->i_sb,
				inode,
				page->index << PAGE_CACHE_SHIFT,
				(page->index + 1) << PAGE_CACHE_SHIFT,
				(char *)kmap(page));
			break;

		case CDDATA_REQUEST:
			/*
			return_status=cdfs_copy_from_cddata(inode->i_sb,
				inode->i_ino,
				page->index << PAGE_CACHE_SHIFT,
				(page->index + 1) << PAGE_CACHE_SHIFT,
				(char *)kmap(page));
			*/
			return_status=0;
			break;
		case CDHFS_REQUEST:
			/*
			return_status=cdfs_copy_from_cdhfs(inode->i_sb,
				inode->i_ino,
				page->index << PAGE_CACHE_SHIFT,
				(page->index + 1) << PAGE_CACHE_SHIFT,
				(char *)kmap(page));
			*/
			return_status=0;
			break;
		}
		}
		else
		{
			return_status=1;
		}
		if( (!return_status) && (!PageError(page)) )
			SetPageUptodate (page);
		else
			SetPageError(page);
		kunmap (page);
		unlock_page (page);
		PRINTM("D01 kfree   addr = 0x %x for req\n",(unsigned int)req);
		kfree(req);	//D01
	}
}

int kcdfsd_thread(void *unused)
{
  kcdfsd_running = 1;

  /* This thread doesn't need any user-level access,
   * so get rid of all our resources  */
   //in sys/kerner/exit.c, it says:
   //Put all the gunge required to become a kernel thread without
   //attached user resources in one place where it belongs.
   //void daemonize(const char *name, ...)

  daemonize("vcd_module");

  /* Allow SIGTERM to quit properly when removing module */
  /* By default with daemonize all signals are dropped */
  allow_signal(SIGTERM);

  /* Send me a signal to get me die */
  do {
    kcdfsd_process_request();
    //interruptible_sleep_on(&kcdfsd_wait);
    wait_event_interruptible(kcdfsd_wait, !list_empty(&kcdfsd_req_list));
  } while (!signal_pending(current));
  kcdfsd_running = 0;
  return 0;
}

void kcdfsd_cleanup_thread()
{
  int ret;

  ret = kill_proc(kcdfsd_pid, SIGTERM, 1);
  if (!ret)
  {
    /* Wait 10 seconds */
    int count = 10 * HZ;

    while (kcdfsd_running && --count)
    {
	    current->state = TASK_INTERRUPTIBLE;
	    schedule_timeout(1);
    }
    if (!count)
    {
	    printk(FSNAME": Giving up on killing k"FSNAME"d!\n");
    }
  }
}


