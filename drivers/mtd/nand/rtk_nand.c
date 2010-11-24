/******************************************************************************
 * $Id: rtk_nand.c 257157 2009-07-30 12:34:36Z jimmy $
 * drivers/mtd/nand/rtk_nand.c
 * Overview: Realtek NAND Flash Controller Driver
 * Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2008-05-30 CMYu   create file
 *
 *******************************************************************************/
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/pm.h>
#include <asm/io.h>

/* Ken-Yu */
#include <linux/mtd/rtk_nand_reg.h>
#include <linux/mtd/rtk_nand.h>
#include <linux/bitops.h>
#include <mtd/mtd-abi.h>
#include <asm/r4kcache.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/mach-venus/platform.h>
//CMYu, 20090720, for CP
#include <asm/mach-venus/mcp.h>

#define BANNER  "Realtek NAND Flash Driver"
#define VERSION  "$Id: rtk_nand.c 257157 2009-07-30 12:34:36Z jimmy $"

#define MTDSIZE	(sizeof (struct mtd_info) + sizeof (struct nand_chip))
#define MAX_PARTITIONS	16
#define BOOTCODE	16*1024*1024	//16MB

/* Ken test and debug define: BEGIN */
#define RTK_DEBUG 0
#if RTK_DEBUG
      #define debug_nand(fmt, arg...)  printk(fmt, ##arg);
#else
      #define debug_nand(fmt, arg...)
#endif

#define KEN_MTD_PARTITION 0

#define KEN_TIMER_READ 0
#define KEN_TIMER_WRITE 0

#if (KEN_TIMER_WRITE || KEN_TIMER_READ)
int count_r=0, count_w=0;
unsigned long total_time_r, total_time_w;
#endif

#define RTK_TEST_RWE 0
#define RTK_TEST_OOB 0

#define DUMP_DATA	0
#define DUMP_OOB	1
#define BEFORE_CALL	0
#define AFTER_CALL	1
/* Ken test and debug define: END */

/* nand driver low-level functions */
static void rtk_nand_read_id(struct mtd_info *mtd, unsigned char id[5]);
static int rtk_read_oob(struct mtd_info *mtd, u16 chipnr, int page, int len, u_char *buf);
static int rtk_read_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page, 
			u_char *data, u_char *oob_buf);
static int rtk_write_oob(struct mtd_info *mtd, u16 chipnr, int page, int len, const u_char *buf);
static int rtk_write_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page, 
			const u_char *data, const u_char *oob_buf, int isBBT);
static int rtk_erase_block(struct mtd_info *mtd, u16 chipnr, int page);	
static void rtk_nand_suspend (struct mtd_info *mtd);
static void rtk_nand_resume (struct mtd_info *mtd);

/* Global Variables */
struct mtd_info *rtk_mtd; 
static DECLARE_MUTEX (sem);
static int page_size, oob_size, ppb;
static int RBA_PERCENT = 5;


/*
 * RTK NAND suspend:
 */
static void rtk_nand_suspend (struct mtd_info *mtd)
{
	
	if ( rtk_readl(REG_DMA_CTL3) & 0x02 ){
//		printk("%s : wait READ to complete.\n",__FUNCTION__);
		//wait DMA finished
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		/* check read result */
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
		//interval of two DMA
		udelay(20+60);
		//wait DMA finished
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		/* check read result */
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
//		printk("%s : READ is done.\n",__FUNCTION__);
	}else{
		printk("%s : wait WRITE to complete.\n",__FUNCTION__);
		//wait DMA finished
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		/* check read result */
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
		printk("%s : WRITE is done.\n",__FUNCTION__);
	}
}


static void resume_to_reset_reg(void)
{
	/*  set PIN_MUX to nand, not pcmcia */
	rtk_writel( 0x103, 0xb800036c );
	
	/* set up NF clock */
	// disable NF clock
	rtk_writel(rtk_readl(0xb800000c)& (~0x00800000), 0xb800000c);
	//nf clock 72MHZ
	rtk_writel( 0x02, 0xb8000034 );
	// enable NF clock
	rtk_writel(rtk_readl(0xb800000c)| (0x00800000), 0xb800000c);
 	
 	/* initial setting chip #0 */
	rtk_writel( ~(0x1 << 0), REG_CHIP_EN );
	/* Reset Nand Controller */
	rtk_writel( CMD_RESET, REG_CMD1 );
	rtk_writel( (0x80 | 0x00), REG_CTL );
	while( rtk_readl(REG_CTL) & 0x80 );
	
	/* disable EDO timing clock is below < 30 MHZ */
	rtk_writel(0x00, REG_MULTICHNL_MODE);
		
	/* Initial CLE, ALE setup time */
	rtk_writel( 0x00, REG_T1 );
	rtk_writel( 0x00, REG_T2 );
	rtk_writel( 0x00, REG_T3 );
	rtk_writel( 0x09, REG_DELAY_CNT );  // set T_WHR_ADL delay clock #
     
	/* Disable Table SRAM */
	rtk_writel( 0x00, REG_TABLE_CTL);	//disable

}


/*
 * RTK NAND resume:
 */
static void rtk_nand_resume (struct mtd_info *mtd)
{
	resume_to_reset_reg();
	if ( rtk_readl(REG_DMA_CTL3) & 0x02 ){
		printk("%s : wait READ to complete.\n",__FUNCTION__);
		//wait DMA finished
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		/* check read result */
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
		//interval of two DMA
		udelay(20+60);
		//wait DMA finished
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		/* check read result */
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
		printk("%s : READ is done.\n",__FUNCTION__);
	}else{
		printk("%s : wait WRITE to complete.\n",__FUNCTION__);
		//wait DMA finished
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		/* check read result */
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
		printk("%s : WRITE is done.\n",__FUNCTION__);
	}
}


/*
 * To dump nand data for debug
 */
static void rtk_dump_nand_data (__u8 *buf, int start, int size, __u8 *func_name, int line, 
					int timing, int type)
{
	printk("======= Dump %s %s calling [%s] [%d] =======\n", 
		type==DUMP_DATA?"Data":"OOB", 
		timing==BEFORE_CALL?"before":"after", 
		func_name, line);
	
	int j;
	int print_col = 16;			
	for ( j=start; j < start+size; j++){
		if ( !(j % print_col) )
			printk("[%p]:", &buf[j]);
		printk(" %02x", buf[j]);
		if ( (j % print_col) == print_col-1 )
			printk("\n");
	}
	printk("\n");
}


static void read_oob_from_PP(__u8 *r_oobbuf)
{
	unsigned int reg_oob, reg_num;
	int i;
	for ( i=0; i < (32/4); i++){
		reg_num = REG_BASE_ADDR + i*4;
		reg_oob = rtk_readl(reg_num);
		//printk("0x%lx = 0x%x\n", reg_num, reg_oob);
		r_oobbuf[i*4+0] = reg_oob & 0xff;
		r_oobbuf[i*4+1] = (reg_oob >> 8) & 0xff;
		r_oobbuf[i*4+2] = (reg_oob >> 16) & 0xff;
		r_oobbuf[i*4+3] = (reg_oob >> 24) & 0xff;
	}
}


static void write_oob_to_TableSram(__u8 *w_oobbuf, __u8 w_oob_1stB, int shift)
{
	unsigned int reg_oob, reg_num;
	int i;
	
	if (shift){
		reg_num = REG_BASE_ADDR;
		reg_oob = w_oob_1stB | (w_oobbuf[0] << 8) | (w_oobbuf[1] << 16) | (w_oobbuf[2] << 24);
		rtk_writel(reg_oob, reg_num);
	
		reg_num = REG_BASE_ADDR + 1*4;
		reg_oob = w_oobbuf[3];
		rtk_writel(reg_oob, reg_num);
	
		reg_num = REG_BASE_ADDR + 4*4;
		reg_oob = w_oobbuf[4] | (w_oobbuf[5] << 8) | (w_oobbuf[6] << 16) | (w_oobbuf[7] << 24);
		rtk_writel(reg_oob, reg_num);	
		
		reg_num = REG_BASE_ADDR + 8*4;
		reg_oob = w_oobbuf[8] | (w_oobbuf[9] << 8) | (w_oobbuf[10] << 16) | (w_oobbuf[11] << 24);
		rtk_writel(reg_oob, reg_num);
	
		reg_num = REG_BASE_ADDR + 12*4;
		reg_oob = w_oobbuf[12] | (w_oobbuf[13] << 8) | (w_oobbuf[14] << 16) | (w_oobbuf[15] << 24);
		rtk_writel(reg_oob, reg_num);	
	}else{
		for ( i=0; i < (oob_size/4); i++){
			reg_num = REG_BASE_ADDR + i*4;
			reg_oob = w_oobbuf[i*4+0] | (w_oobbuf[i*4+1] << 8) | (w_oobbuf[i*4+2] << 16) | (w_oobbuf[i*4+3] << 24);
			rtk_writel(reg_oob, reg_num);
		}	
	}
}


/* read nand flash id: Xfer mode */
static void rtk_nand_read_id(struct mtd_info *mtd, u_char id[5])
{
	debug_nand("---------[%s]----------\n", __FUNCTION__);
	int id_chain;

     //Set data count
     rtk_writel(0x05, REG_DATA_CNT1);	// will read 5 bytes
     rtk_writel(0x80, REG_DATA_CNT2);	// Access mode: other transfer mode
     
	//Set PP
	rtk_writel(0x0, REG_PP_RDY);	// Not through PP
	rtk_writel(0x01, REG_PP_CTL0);
	rtk_writel(0x0, REG_PP_CTL1);
	
	//Set command
	rtk_writel(CMD_READ_ID, REG_CMD1);
	rtk_writel(0x80, REG_CTL);	//TRANS_CMD
	while ( rtk_readl(REG_CTL) & 0x80 );
	
	//Set address	
	rtk_writel(0, REG_PAGE_ADR0);
	rtk_writel(0x7<<5, REG_PAGE_ADR2);
	rtk_writel(0x81, REG_CTL);	//TRANS_ADDR
	while ( rtk_readl(REG_CTL) & 0x80 );

#if 1
	//Enable MultiByteRead XFER mode
	rtk_writel(0x84, REG_CTL);	//TRANS_MBR
	while ( rtk_readl(REG_CTL) & 0x80 );
#else	//not read to PP, dump 5 bytes, for debug
	rtk_writel(0x82, REG_CTL);	//TRANS_MBR
	while ( rtk_readl(REG_CTL) & 0x80 );
	printk("rtk_readl(REG_DATA): 0x%lx\n", rtk_readl(REG_DATA));

	rtk_writel(0x82, REG_CTL);	//TRANS_MBR
	while ( rtk_readl(REG_CTL) & 0x80 );
	printk("rtk_readl(REG_DATA): 0x%lx\n", rtk_readl(REG_DATA));
	
	rtk_writel(0x82, REG_CTL);	//TRANS_MBR
	while ( rtk_readl(REG_CTL) & 0x80 );
	printk("rtk_readl(REG_DATA): 0x%lx\n", rtk_readl(REG_DATA));
	
	rtk_writel(0x82, REG_CTL);	//TRANS_MBR
	while ( rtk_readl(REG_CTL) & 0x80 );
	printk("rtk_readl(REG_DATA): 0x%lx\n", rtk_readl(REG_DATA));
	
	rtk_writel(0x82, REG_CTL);	//TRANS_MBR
	while ( rtk_readl(REG_CTL) & 0x80 );
	printk("rtk_readl(REG_DATA): 0x%lx\n", rtk_readl(REG_DATA));	
#endif
				
	//Reset PP
	rtk_writel(0x2, REG_PP_CTL0);
	
	//read PP #1
	rtk_writel(0x30, REG_SRAM_CTL);
	
	id_chain = rtk_readl(REG_PAGE_ADR0);
	id[0] = id_chain & 0xff;
	id[1] = (id_chain >> 8) & 0xff;
	id[2] = (id_chain >> 16) & 0xff;
	id[3] = (id_chain >> 24) & 0xff;

	id_chain = rtk_readl(REG_PAGE_ADR1);
	id[4] = id_chain & 0xff;

#if 0
	u8 i;
	debug_nand("rtk_nand_read_id: 5 bytes: ");
	for ( i = 0; i < 5; i++ ){
		//id[i] = rtk_readb(REG_PAGE_ADR0 + i);	//ex: Samsung: 5 bytes value : EC D3 10 A6 64
		debug_nand("0x%x ", id[i]);
	}
	debug_nand("\n ");
#endif	
	rtk_writel(0x0, REG_SRAM_CTL);	//# no omitted
}


/*
 * rtk_erase_block - erase block
 * @mtd: MTD device structure
 * @page: offset is the first page of the block
 */
static int rtk_erase_block(struct mtd_info *mtd, u16 chipnr, int page)
{	
	/* set up Global Variables, Cuz rtk_erase_block() will the first call in TEST_ERASE_ALL() */
	page_size = mtd->oobblock;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->oobblock;
	
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	
	//page must be block align
	if ( page & (ppb-1) ){
		printk("%s: page %d is not block alignment !!\n", __FUNCTION__, page);
		return -1;
	}

	//printk ("%s:chipnr=%d, block=%d,  page=%d, ppb=%d\n", __FUNCTION__, chipnr, page/ppb, page, ppb);

//=================================================================
#if 0	//for erase bbt test
	if ( page == 11136  || page == 15744 || page == 18496 ){	//block 174, 246, 289
	//if ( chipnr == 0 && (page == 260692  || page == 261775) ){
		printk("[%s] (BBT) erase is not completed at page %d\n", __FUNCTION__, page);
		return -1;	
	}
#endif	
//=================================================================

	/* try to grap the semaphore and see if the device is available */
	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}
	
	//Set ECC	
	rtk_writel( 0x20, REG_MULTICHNL_MODE);

	//Set address
	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (0x4<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  //addr_mode: 100
	rtk_writel( ((page>> 21)&0x7)<<5, REG_PAGE_ADR3 );
	
	// set erase command 
	rtk_writel( CMD_BLK_ERASE_C1, REG_CMD1 );
	rtk_writel( CMD_BLK_ERASE_C2, REG_CMD2 );
	rtk_writel( CMD_BLK_ERASE_C3, REG_CMD3 );	

	//Enable Auto mode
	rtk_writel( (1 << 7)|(0x01 << 3)|0x02, REG_AUTO_TRIG );
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );		

 	// execute command3 register and wait for executed completion
	rtk_writel(0x0d, REG_POLL_STATUS );
	while ( rtk_readl(REG_POLL_STATUS) & 0x01 );

#if 0	//for bbt erase test	
	if ( chipnr == 1 && page == 11264 ){ // HY27UG088G5B: block 176 fails
		up (&sem);
		printk("[%s] erasure is not completed at block %d\n", __FUNCTION__, page/ppb);
		return -1;
	}
#endif	

	//printk(" rtk_readl(REG_DATA) = 0x%x\n", rtk_readl(REG_DATA) );
	
	if ( rtk_readl(REG_DATA) & 0x01 ){ // IO0: 0: pass, 1: fail
		up (&sem);
		printk("[%s] erasure is not completed at block %d\n", __FUNCTION__, page/ppb);
		//protect bootcode(16MB) from updating BBT
		if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size )
			return 0;
		else
			return -1;
	}

	/* release semaphore */
	up (&sem);
	
	//after erase, mark the page is 0x01
	unsigned int chip_section = (chipnr * this->page_num) >> 5;	//because of __u32 
	unsigned int section = page >> 5;	//because of __u32 
	memset ( (__u32 *)(this->erase_page_flag)+ chip_section + section, 0xff, ppb>>3);	//byte alignment
	
#if 0
	int j;
	for ( j=section; j <section+4; j++){
		if ( !(j % 8)  ){
			printk("\n");
			printk("[%p]:", &(this->erase_page_flag[j]));
		}
		printk("[0x%x]", this->erase_page_flag[j]);
	}	
#endif	

	return 0;
}


/**
 * rtk_read_oob - read oob area
 * @mtd: MTD device structure
 * @page: offset to read from
 * @col: column address 
 * @len: number of oob bytes to read
 * @buf:	the databuffer to put oob, YAFFS pass the buffer size = mtd->oobsize
* 
 * NAND read out-of-band data from the spare area
 */
static int rtk_read_oob (struct mtd_info *mtd, u16 chipnr, int page, int len, u_char *oob_buf)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	int dram_sa, dma_len;
	int blank_all_one=0;
	/* "oob_buf" is used to save oob data.	it is aligned 8 is for DMA Dram address */
	int dma_counter = page_size >> 10;	//PP=1KB
	int buf_pos=0;
	uint8_t	auto_trigger_mode = 2;	//autocase: 010: CACD read, 2K page size
	uint8_t	addr_mode = 1;	//addr_mode: 001, CCPPP
	int page_len;
	
	//printk("[%s] chipnr=%d, block=%d, page=%d\n", 	__FUNCTION__, chipnr, page/ppb, page);
	
//========================================================
#if 0
	//if (page>8191 && page<8215)
	if (page == 8192)
		rtk_dump_nand_data ((__u8 *)oob_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_OOB); 
#endif
//========================================================
	
	//if ( page>=8192 && page<=8192*2 )
		//printk("[%s] page=%d, this->g_databuf=%p, oob_buf=%p\n", __FUNCTION__, page, this->g_databuf, oob_buf);
	
	if ( len>oob_size || !oob_buf ){
		printk("[%s] error: len>oob_size OR oob_buf is NULL\n", __FUNCTION__);
		return -1;
	}

	//flush DDR to cache to make data coherence
	RTK_FLUSH_CACHE((unsigned long) this->g_databuf, page_size);
	if ( oob_buf ) 
		RTK_FLUSH_CACHE((unsigned long) oob_buf, oob_size);
			
	/* try to grap the semaphore and see if the device is available */
	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}
	
	//enable blank check function
	rtk_writel(0x01, REG_BLANK_CHECK);
	
	//set data Xfer count
	rtk_writel(0x00, REG_DATA_CNT1);
	rtk_writel(0x82, REG_DATA_CNT2);	//data transfer count must be 0x200 at auto mode	

	//set page_len for auto mode
	//page_len = page_size >> 9;	//512B Unit
	page_len = 1024 >> 9;	//512B Unit
	rtk_writel(page_len, REG_PAGE_LEN);
	
	//Set PP
	rtk_writel(0x80, REG_PP_RDY);
	rtk_writel(0x00, REG_PP_CTL1);
	rtk_writel(0x01, REG_PP_CTL0);	//NFREG_PING_PONG_BUFFER_ENABLE

	//Disable Table SRAM
	//rtk_writel( 0x00, REG_TABLE_CTL);	//disable
	
	//Set read command
	rtk_writel(CMD_PG_READ_C1, REG_CMD1);
	rtk_writel(CMD_PG_READ_C2, REG_CMD2);
	rtk_writel(CMD_PG_READ_C3, REG_CMD3);

	//Set address
	rtk_writel(0x00, REG_COL_ADR0);	//nf code
	rtk_writel(0x00, REG_COL_ADR1);
	//rtk_writel( col & 0xff, REG_COL_ADR0 );
	//rtk_writel( (col >> 8) & 0xff, REG_COL_ADR1 );
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (addr_mode<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  
	rtk_writel( ((page>>21)&0x7)<<5, REG_PAGE_ADR3 );

	//Set ECC
	rtk_writel( 0x20, REG_MULTICHNL_MODE);
	rtk_writel( 0x80, REG_ECC_STOP);

	//Set DMA
	//enhance performance, remove them from below "for" loop
	//dma_len = (page_size) >> 9;	//512 B alignment
	dma_len = 1024 >> 9;	//512 B alignment
	//rtk_writel(0x04, REG_DMA1_CTL);	//DMA Max Packet Size = 512 bytes
	rtk_writel(dma_len, REG_DMA_CTL2);
	
	dram_sa = ( (uint32_t) this->g_databuf >> 3);
	rtk_writel(dram_sa, REG_DMA_CTL1);	
	//Enable Auto mode
	rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
	//Enable DMA Xfer
	rtk_writel(0x03, REG_DMA_CTL3);	//DMA_DIR_READ
	//wait DMA finished
	while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
	//printk("[%s][1]REG_DMA_CTL3=0x%lx\n", __FUNCTION__, rtk_readl(REG_DMA_CTL3));			
	/* check read result */
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );		
		
	//enable direct access to PP #3
	rtk_writel(0x00, REG_PP_RDY);
	rtk_writel(0x30 | 0x02, REG_SRAM_CTL);	
	read_oob_from_PP(oob_buf);
	// disable direct access PP
	rtk_writel(0x00, REG_SRAM_CTL); 
		
	dma_counter--;
	buf_pos++;				

	/*============================2nd-1K page==============================*/
	while(dma_counter>0){
		//Reset PP
		//rtk_writel( rtk_readl(REG_PP_CTL0)|0x02, REG_PP_CTL0);	
		//re-enable PP to read
		rtk_writel(0x80, REG_PP_RDY);	
		//Set address
		//set up column address to 1056 = 100-00100000
#if 0	//D read do not need set it
		col = buf_pos * (1024+32);
		rtk_writel( col & 0xff, REG_COL_ADR0 );
		rtk_writel( (col >> 8) & 0xff, REG_COL_ADR1 );			
#endif			
		//set DMA address
		dram_sa = ( (uint32_t)(this->g_databuf + buf_pos*1024) >> 3);
		rtk_writel(dram_sa, REG_DMA_CTL1);	
		//Enable Auto mode
		//rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
		//use D read to read 2nd-1K pag
		rtk_writel( (0x01 << 7)|(0x00 << 3)|0x04, REG_AUTO_TRIG );
		//Enable DMA Xfer
		rtk_writel(0x03, REG_DMA_CTL3);	//DMA_DIR_READ
		//wait DMA finished				
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		/* check read result */
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );

		//enable direct access to PP #3
		rtk_writel(0x00, REG_PP_RDY);
		rtk_writel(0x30 | 0x02, REG_SRAM_CTL);	
		read_oob_from_PP(oob_buf + 32*buf_pos);
		// disable direct access PP
		rtk_writel(0x00, REG_SRAM_CTL); 							
		
		dma_counter--;
		buf_pos++;
	}

	/* release semaphore */
	up (&sem);
	
//========================================================	
#if 0
	//if (page>8191 && page<8215)
	//if ( page==8192 )
	if ( page==8320 )
		rtk_dump_nand_data ((__u8 *)oob_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_OOB); 
#endif
//========================================================

	//check if the read oob is completed
	unsigned int chip_section = (chipnr * this->page_num) >> 5;	//because of __u32 
	unsigned int section = page >> 5;	//because of __u32 
	unsigned int index = page & (32-1);	
	/* because mounting will pop HW ECC Error after cool reboot 
	re-fill the this->erase_page_flag if we don't use erase function */
	blank_all_one = (rtk_readl(REG_BLANK_CHECK)>>1) & 0x01;
	if (blank_all_one)
		this->erase_page_flag[chip_section+section] =  (1<< index);
		
	if (rtk_readl(REG_ECC_STATE) & 0x0C){
		if (this->erase_page_flag[chip_section+section] & (1<< index) ){
			debug_nand("[%s] ECC pull high because just erase\n", __FUNCTION__);
		}else{
			//Un-correctable HW ECC error 
			if (rtk_readl(REG_ECC_STATE) & 0x08){		
				//protect bootcode(16MB) from updating BBT
				if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size )
					return 0;
				else
					return -1;
			}
			//Correctable HW ECC error
			if (rtk_readl(REG_ECC_STATE) & 0x04)		
				printk("[%s] Correctable HW ECC Error at page=%d\n", __FUNCTION__, page);
		}
	}

	return rc;
}


/*
 * rtk_read_ecc_page - read pages with hardware ecc 
 * @mtd: MTD device structure
 * @page: offset to read from
 * @data_buf: buffer to put data area
 * @oob_buf: buffer to put spare area
 */
static int rtk_read_ecc_page (struct mtd_info *mtd, u16 chipnr, unsigned int page, 
			u_char *data_buf, u_char *oob_buf)
{
#if KEN_TIMER_READ
	// Ken added
	struct timeval time1, time2;
	int usec,sec;
	do_gettimeofday(&time1);
#endif

	/* set up Global Variables, Cuz rtk_read_ecc_page() will the first call in rtk_nand_scan_bbt() */
	page_size = mtd->oobblock;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->oobblock;
	
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	int dram_sa, dma_len;
	int blank_all_one = 0;
	
	int dma_counter = page_size >> 10;	//PP=1KB
	int buf_pos=0;
	int page_len;
	
	//printk("[%s] page=%d, data_buf=%p, oob_buf=%p\n", __FUNCTION__, page, data_buf, oob_buf);
	
	if ( !oob_buf )
		memset(this->g_oobbuf, 0xff, oob_size);

	//flush DDR to cache to make data coherence
	RTK_FLUSH_CACHE((unsigned long) data_buf, page_size);
	if ( oob_buf )
		RTK_FLUSH_CACHE((unsigned long) oob_buf, oob_size);
	else
		RTK_FLUSH_CACHE((unsigned long) this->g_oobbuf, oob_size);
					
//========================================================
#if 0
	//if ( page == 8380 ){	
	if ( page == 0 ){	
		rtk_dump_nand_data ((__u8 *)data_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_DATA); 
		if (oob_buf)
			rtk_dump_nand_data ((__u8 *)oob_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_OOB); 
	}
#endif		
//========================================================
		               
	uint8_t	auto_trigger_mode = 2;	//autocase: 010: CACD read, 2K page size
	uint8_t	addr_mode = 1;	//addr_mode: 001, CCPPP

	/* try to grap the semaphore and see if the device is available */
	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}

	//enable blank check function
	rtk_writel(0x01, REG_BLANK_CHECK);
	
	//set data Xfer count
	rtk_writel( 0x00, REG_DATA_CNT1);
	rtk_writel( 0x82, REG_DATA_CNT2);	//data transfer count must be 0x200 at auto mode	

	//set page_len for auto mode
	//page_len = page_size >> 9;	//512B Unit
	page_len = 1024 >> 9;	//512B Unit, PP is 1KB
	rtk_writel( page_len, REG_PAGE_LEN);
	
	//Set PP
	rtk_writel(0x80, REG_PP_RDY);
	rtk_writel(0x00, REG_PP_CTL1);
	rtk_writel(0x01, REG_PP_CTL0);	//NFREG_PING_PONG_BUFFER_ENABLE

	//Disable Table SRAM
	//rtk_writel( 0x00, REG_TABLE_CTL);	//disable
	
	//Set read command
	rtk_writel(CMD_PG_READ_C1, REG_CMD1);
	rtk_writel(CMD_PG_READ_C2, REG_CMD2);
	rtk_writel(CMD_PG_READ_C3, REG_CMD3);

	//Set address
	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (addr_mode<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  
	rtk_writel( ((page>>21)&0x7)<<5, REG_PAGE_ADR3 );

	//Set ECC
	rtk_writel( 0x20, REG_MULTICHNL_MODE);
	rtk_writel( 0x80, REG_ECC_STOP);

	//Set DMA
	//enhance performance, remove them from below "for" loop
	//dma_len = (page_size) >> 9;	//512 B alignment
	dma_len = 1024 >> 9;	//512 B alignment
	//rtk_writel(0x04, REG_DMA1_CTL);	//DMA Max Packet Size = 512 bytes
	rtk_writel(dma_len, REG_DMA_CTL2);

	dram_sa = ( (uint32_t)data_buf >> 3);
	rtk_writel(dram_sa, REG_DMA_CTL1);	
	//Enable Auto mode
	rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
	//Enable DMA Xfer
	rtk_writel(0x03, REG_DMA_CTL3);	//DMA_DIR_READ
	//wait DMA finished
	while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
	/* check read result */
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );

	//printk ("read1: rtk_readl(REG_DMA_CTL3) = 0x%x\n", rtk_readl(REG_DMA_CTL3));
		
	//enable direct access to PP #3
	rtk_writel(0x00, REG_PP_RDY);
	rtk_writel(0x30 | 0x02, REG_SRAM_CTL);	
	if ( oob_buf ){
		read_oob_from_PP(oob_buf);
	}else{
		read_oob_from_PP(this->g_oobbuf);
	}
	// disable direct access PP
	rtk_writel(0x00, REG_SRAM_CTL); 

	dma_counter--;
	buf_pos++;

	/*============================2nd-1K page==============================*/
	while(dma_counter>0){
		//Reset PP
		//rtk_writel( rtk_readl(REG_PP_CTL0)|0x02, REG_PP_CTL0);	
		//re-enable PP to read
		rtk_writel(0x80, REG_PP_RDY);			
		//Set address
		//set up column address to 1056 = 100-00100000
#if 0	//D read do not need set it
		col = buf_pos * (1024+32);
		rtk_writel( col & 0xff, REG_COL_ADR0 );
		rtk_writel( (col >> 8) & 0xff, REG_COL_ADR1 );
#endif
		//set DMA address
		dram_sa = ( (uint32_t)(data_buf+buf_pos*1024) >> 3);
		rtk_writel(dram_sa, REG_DMA_CTL1);	
		//Enable Auto mode
		//rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
		//use D read to read 2nd-1K pag
		rtk_writel( (0x01 << 7)|(0x00 << 3)|0x04, REG_AUTO_TRIG );
		//Enable DMA Xfer
		rtk_writel(0x03, REG_DMA_CTL3);	//DMA_DIR_READ
		//wait DMA finished				
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		/* check read result */
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );		
		
		//printk ("read2: rtk_readl(REG_DMA_CTL3) = 0x%x\n", rtk_readl(REG_DMA_CTL3));
		
		//enable direct access to PP #3
		rtk_writel(0x00, REG_PP_RDY);
		rtk_writel(0x30 | 0x02, REG_SRAM_CTL);	
		if ( oob_buf ){
			read_oob_from_PP(oob_buf + 32*buf_pos);
		}else{
			read_oob_from_PP(this->g_oobbuf + 32*buf_pos);
		}
		// disable direct access PP
		rtk_writel(0x00, REG_SRAM_CTL); 							
			
		dma_counter--;
		buf_pos++;		
	}

	/* release semaphore */
	up (&sem);

#ifdef CONFIG_REALTEK_MCP
	//printk("[%s] this->mcp=%d\n", __FUNCTION__, this->mcp);
	if ( this->mcp==MCP_AES_ECB ){
		//printk("READ: MCP_AES_ECB\n");
		MCP_AES_ECB_Decryption(NULL, data_buf, data_buf, page_size);
	}else{
		//printk("Not Implement %s\n", this->mcp==MCP_AES_CBC?"MCP_AES_CBC":"MCP_AES_CTR");
	}
#endif

//=================================================================
#if 0	//for read bbt test
	//if ( page == 8200 ){	//at block=64
	//if ( page == 521700 ){	//at block=4075, test remap is bad, too
	//if ( page == 8380  || page == 9010 || page == 12708 ){	//at block=65, 70, 99 or HY27UG088G5B: 130, 140, 198
	if ( chipnr == 0 && (page == 260692  || page == 261775) ){	//HY27UG088G5B: block 4073, 4090
		printk("[%s] read is not completed at page %d\n", __FUNCTION__, page);
		return -1;	
	}
#endif	
//=================================================================

	//check if the read oob is completed
	unsigned int chip_section = (chipnr * this->page_num) >> 5;	//because of __u32 
	unsigned int section = page >> 5;	//because of __u32 
	unsigned int index = page & (32-1);	
	/* because mounting will pop HW ECC Error after cool reboot 
	re-fill the this->erase_page_flag if we don't use erase function */
	blank_all_one = (rtk_readl(REG_BLANK_CHECK)>>1) & 0x01;
	if (blank_all_one)
		this->erase_page_flag[chip_section+section] =  (1<< index);

	if (rtk_readl(REG_ECC_STATE) & 0x0C){
		if (this->erase_page_flag[chip_section+section] & (1<< index) ){
			debug_nand("[%s] ECC pull high because just erase OR after cold reboot\n", __FUNCTION__);
		}else{
			//Un-correctable HW ECC error 
			if (rtk_readl(REG_ECC_STATE) & 0x08){
				//protect bootcode(16MB) from updating BBT
				if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size )
					return 0;
				else
					return -1;
			}
			//Correctable HW ECC error
			if (rtk_readl(REG_ECC_STATE) & 0x04)		
				printk("[%s] Correctable HW ECC Error at page=%d\n", __FUNCTION__, page);
		}
	}
	
//========================================================
#if 0
	//if ( page == 8192 ){	
	if ( page == 0 ){	
		rtk_dump_nand_data ((__u8 *)data_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_DATA); 
		if (oob_buf)
			rtk_dump_nand_data ((__u8 *)oob_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_OOB); 
		else	
			rtk_dump_nand_data ((__u8 *)this->g_oobbuf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_OOB); 
	}
#endif	
//========================================================

#if KEN_TIMER_READ
	count_r++;
	do_gettimeofday(&time2);
	sec = time2.tv_sec - time1.tv_sec;
	usec = time2.tv_usec - time1.tv_usec;
	if(usec < 0){
		sec--;
		usec+=1000000;
	}
	total_time_r += sec*1000000+usec;

	//05.mp3: 5524414 Bytes
	if ( page_size == 4096 ){
		if (count_r==1350 )
			printk("[%s] page=%x total_time_r spends %ld MicroSec, count_r=%d===\n",
			__FUNCTION__, page, total_time_r, count_r);		
	}else{ 
		if (count_r==2700 )	
			printk("[%s] page=%x total_time_r spends %ld MicroSec, count_r=%d===\n",
				__FUNCTION__, page, total_time_r, count_r);
	}
#endif
	
	return rc;
}


/**
 * rtk_write_oob - write oob
 * @mtd: MTD device structure
 * @page: offset to read from
 * @col: column address 
 * @len: number of bytes to read
 * @oob_buf:	the databuffer to put spare data, YAFFS pass the buffer size = mtd->oobsize
* 
 * NAND write out-of-band data from the spare area
 */
static int rtk_write_oob(struct mtd_info *mtd, u16 chipnr, int page, int len, const u_char *oob_buf)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	unsigned int page_len, dram_sa, dma_len;
	unsigned char oob_1stB;
	int rc = 0;
	uint8_t	auto_trigger_mode = 1;	//autocase: 001: CADC write, 2K page size
	uint8_t	addr_mode = 1;	//addr_mode: 001, CCPPP
	
	//Cuz initial values 0xff will not change original data area values
	memset(this->g_databuf, 0xff, page_size);
	
	/* if I open RTK_TEST_MARK_BB, these must set first */
	page_size = mtd->oobblock;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->oobblock;
	
	//printk("[%s][%d] chipnr=%d, page=%d, page_size=%d \n", __FUNCTION__, __LINE__, chipnr, page, page_size);

	//protect bootcode from be written: 2*block~16MB(64 block)
	if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size ){
		printk("[%s] You have no permission to write this page %d\n", __FUNCTION__, page);
		return -2;
	}

	if ( len>oob_size || !oob_buf ){
		printk("[%s] error: len>oob_size OR oob_buf is NULL\n", __FUNCTION__);
		return -1;
	}

	//flush DDR to cache to make data coherence
	RTK_FLUSH_CACHE((unsigned long) this->g_databuf, page_size);
	if ( oob_buf ) 
		RTK_FLUSH_CACHE((unsigned long) oob_buf, oob_size);
		
//========================================================
#if 0
	//if ( page == 30 ){	//start from begining of nand
	//if ( page == 8192 + 30 ){	//start from begining 16 MB of nand
	//if ( page == 8380){
	if ( page == 8192){
		rtk_dump_nand_data ((__u8 *)data_buf, 1600, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_DATA); 
		rtk_dump_nand_data ((__u8 *)oob_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_OOB); 
	}
#endif
//========================================================	
	
	/* backup 1st bytes in spare area */
	/*
		In general, I should do this, but the performance is bad.
		yaffs_CheckChunkErased() will check the chunk status before 
		yaffs_WriteChunkWithTagsToNAND() in yaffs_WriteNewChunkWithTagsToNAND. 
		So I make another chioce because of this reason.
	*/
#if 0
	//__u8 *r_oobbuf = kmalloc( sizeof(oob_size), GFP_KERNEL );
	__u8 *r_oobbuf = kmalloc( sizeof(__u8)*32, GFP_KERNEL );
	//if ( rtk_read_oob(mtd, page, 0, oob_size, r_oobbuf) ){
	if ( rtk_read_oob(mtd, page, oob_size, r_oobbuf) ){
		printk ("%s: rtk_read_oob page=%d failed\n", __FUNCTION__, page);
		return 1;
	}
	oob_1stB = r_oobbuf[0];
	//printk("oob_1stB = 0x%x\n", oob_1stB);
	kfree(r_oobbuf);
#else
	if ( page == 0 || page == ppb )
		oob_1stB = BBT_TAG;
	else		
		oob_1stB = 0xFF;
#endif	

	/* try to grap the semaphore and see if the device is available */
	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}
	
	//write oob_buf to Table Sram #12
	rtk_writel(0x3e, REG_SRAM_CTL);	//enable direct access to Table Sram #12
	//rtk_write_oob() now is only used to label BB
	write_oob_to_TableSram((__u8 *)oob_buf, 0xFF, 0);
	// disable direct access PP
	rtk_writel(0x00, REG_SRAM_CTL); 		

	//====START Set write CMD register====	
	rtk_writel( 0x00, REG_DATA_CNT1);
 	rtk_writel( 0x42, REG_DATA_CNT2);	//auto mode: Table Sram
 	
 	//Set Auto Trigger page length
	page_len = page_size >> 9;	//512B Unit
	rtk_writel( page_len, REG_PAGE_LEN);
 		
	//Set PP
	rtk_writel(0x00, REG_PP_RDY);
	rtk_writel(0x00, REG_PP_CTL1);
	rtk_writel(0x00, REG_PP_CTL0);	//NFREG_PING_PONG_BUFFER_DISABLE
 
	//Disable Table SRAM
	//rtk_writel( 0x00, REG_TABLE_CTL);	//disable
 	//rtk_writel( 0xE1, REG_TABLE_CTL);	//enable
 	
	//Set write command
	rtk_writel(CMD_PG_WRITE_C1, REG_CMD1);
	rtk_writel(CMD_PG_WRITE_C2, REG_CMD2);
	rtk_writel(CMD_PG_WRITE_C3, REG_CMD3);
 	
	//Set address
	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (addr_mode<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  
	rtk_writel( ((page>>21)&0x7)<<5, REG_PAGE_ADR3 );
 		
	//Set ECC
	rtk_writel( 0x20, REG_MULTICHNL_MODE);
	rtk_writel( 0x80, REG_ECC_STOP);
	
	//enhance performance, remove them from below "for" loop
	dma_len = page_size >> 9;	//512B Unit
	//Set DMA, Xfer 1K-pagesize data
	//dma_len = (1024) >> 9;	//512B Unit
	rtk_writel(dma_len, REG_DMA_CTL2);

	dram_sa = ( (uint32_t)this->g_databuf >> 3);
	rtk_writel(dram_sa, REG_DMA_CTL1);	
	//Enable Auto mode
	rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
	//Enable DMA Xfer
	rtk_writel(0x01, REG_DMA_CTL3);	//DMA_DIR_WRITE 		
 	//Wait DMA done
 	while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
	/* check read result */
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
	
	//Hynix check "write" is ok.
 	// execute command3 register and wait for executed completion
	rtk_writel(0x0d, REG_POLL_STATUS );
	while ( rtk_readl(REG_POLL_STATUS) & 0x01 );

#if 0	//for test RTK_TEST_WRITE_OOB
	if ( page == 8350 ){
		up (&sem);
		printk("[%s] write oob is not completed at page %d\n", __FUNCTION__, page);
		return -1;
	}
#endif

	//printk("[%s] rtk_readl(REG_DATA) = %lx \n", __FUNCTION__, rtk_readl(REG_DATA));
	//check if the write is completed
	if ( rtk_readl(REG_DATA) & 0x01 ){ // IO0: 0: pass, 1: fail
		up (&sem);
		printk("[%s] write oob is not completed at page %d\n", __FUNCTION__, page);
		return -1;
	}

	/* release semaphore */
	up (&sem);

	//mark the page 0x00 after write data in it
	unsigned int chip_section = (chipnr * this->page_num) >> 5;	//because of __u32 
	unsigned int section = page >> 5;	//because of __u32 
	unsigned int index = page & (32-1);
	//printk("this->erase_page_flag[section]=%x\n", this->erase_page_flag[section]);
	this->erase_page_flag[chip_section+section] &= ~(1 << index);	//mark the bit is 0
	//printk("section=%d, index=%d, this->erase_page_flag[section]=%x\n", 
		//	section, index, this->erase_page_flag[chip_section+section]);

//========================================================
#if 0	//read page back to check 
	unsigned char data_buf1[page_size] __attribute__((__aligned__(8)));
	unsigned char oob_buf1[oob_size];
	rtk_read_ecc_page (mtd, page, data_buf1, oob_buf1);
	//if ( page == 30 ){	//start from begining of nand
	//if ( page == 8192 + 30 ){	//start from begining 16 MB of nand
	//if ( page == 8380 ){	
	if ( page == 8192){
		rtk_dump_nand_data ((__u8 *)data_buf1, 1600, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_DATA); 
		if (oob_buf1)
			rtk_dump_nand_data ((__u8 *)oob_buf1, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_OOB); 
	}
#endif	
//========================================================	
		
	return rc;
}


/*
 * rtk_write_ecc_page - write single page with hardware ecc check
 * @mtd: MTD device structure
 * @page: offset to read from
 * @data_buf: databuffer to put data area
 * @oob_buf: databuffer to put spare area
 */
int rtk_write_ecc_page (struct mtd_info *mtd, u16 chipnr, unsigned int page, 
			const u_char *data_buf, const u_char *oob_buf, int isBBT)	
{
#if KEN_TIMER_WRITE
	struct timeval time1, time2;
	int usec,sec;
	do_gettimeofday(&time1);
#endif	
	
	//printk ("%s:chipnr=%d, block=%d,  page=%d, ppb=%d\n", __FUNCTION__, chipnr, page/ppb, page, ppb);
	
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	uint8_t	auto_trigger_mode = 1;	//autocase: 001: CADC write, 2K page size
	uint8_t	addr_mode = 1;	//addr_mode: 001, CCPPP
	//int i;
	unsigned int page_len, dram_sa, dma_len;
	unsigned char oob_1stB;

	if ( !oob_buf )
		memset(this->g_oobbuf, 0xff, oob_size);

#ifdef CONFIG_REALTEK_MCP
	//printk("[%s] this->mcp=%d\n", __FUNCTION__, this->mcp);
	if ( this->mcp==MCP_AES_ECB ){
		//printk("WRITE: MCP_AES_ECB\n");
		MCP_AES_ECB_Encryption(NULL, data_buf, data_buf, page_size);
	}else{
		//printk("Not Implement %s\n", this->mcp==MCP_AES_CBC?"MCP_AES_CBC":"MCP_AES_CTR");
	}
#endif

	//flush DDR to cache to make data coherence
	RTK_FLUSH_CACHE((unsigned long) data_buf, page_size);
	if ( oob_buf )
		RTK_FLUSH_CACHE((unsigned long) oob_buf, oob_size);
	else
		RTK_FLUSH_CACHE((unsigned long) this->g_oobbuf, oob_size);
			
	//protect bootcode from be written: 2*block~16MB(64 block)
	if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size ){
		if ( isBBT && page<2*ppb ){
			printk("[%s] Updating BBT %d page=%d\n", __FUNCTION__, page/ppb, page%ppb);
		}else{
			printk("[%s] You have no permission to write page %d\n", __FUNCTION__, page);
			return -2;
		}
	}

//========================================================
#if 0
	//if ( page == 30 ){	//start from begining of nand
	//if ( page == 8192 + 30 ){	//start from begining 16 MB of nand
	//if ( page == 8380){	
	if ( page == 8192){
		rtk_dump_nand_data ((__u8 *)data_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_DATA); 
		if (oob_buf)
			rtk_dump_nand_data ((__u8 *)oob_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_OOB); 
		else	
			rtk_dump_nand_data ((__u8 *)this->g_oobbuf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_OOB); 
	}
#endif
//========================================================	

	/* backup 1st bytes in spare area */
	/*
		In general, I should do this, but the performance is bad.
		yaffs_CheckChunkErased() will check the chunk status before 
		yaffs_WriteChunkWithTagsToNAND() in yaffs_WriteNewChunkWithTagsToNAND. 
		So I make another chioce because of this reason.
	*/
#if 0
	//__u8 *r_oobbuf = kmalloc( sizeof(oob_size), GFP_KERNEL );
	__u8 *r_oobbuf = kmalloc( sizeof(__u8)*32, GFP_KERNEL );
	//if ( rtk_read_oob(mtd, page, 0, oob_size, r_oobbuf) ){
	if ( rtk_read_oob(mtd, chipnr, page, oob_size, r_oobbuf) ){
		printk ("%s: rtk_read_oob page=%d failed\n", __FUNCTION__, page);
		return 1;
	}
	oob_1stB = r_oobbuf[0];
	//printk("oob_1stB = 0x%x\n", oob_1stB);
	kfree(r_oobbuf);
#else
if ( page == 0 || page == 1 || page == ppb || page == ppb+1 )
		oob_1stB = BBT_TAG;
	else		
		oob_1stB = 0xFF;
#endif	

	/* try to grap the semaphore and see if the device is available */
	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}
	
	//write oob_buf data to Table Sram #12
	rtk_writel(0x3e, REG_SRAM_CTL);	//enable direct access to Table Sram #12
	if ( !oob_buf  && this->g_oobbuf ) {
		write_oob_to_TableSram(this->g_oobbuf, 0xFF, 0);
	}else{
		write_oob_to_TableSram((__u8 *)oob_buf, oob_1stB, 1);
	}
	// disable direct access PP
	rtk_writel(0x00, REG_SRAM_CTL); 		
	
	//====START Set write CMD register====	
	rtk_writel( 0x00, REG_DATA_CNT1);
	//rtk_writel( 0x02, REG_DATA_CNT2);	//auto mode: PP
 	rtk_writel( 0x42, REG_DATA_CNT2);	//auto mode: Table Sram
 	
 	//Set Auto Trigger page length
	page_len = page_size >> 9;	//512B Unit
	rtk_writel( page_len, REG_PAGE_LEN);
 		
	//Set PP
	rtk_writel(0x00, REG_PP_RDY);
	rtk_writel(0x00, REG_PP_CTL1);
	rtk_writel(0x00, REG_PP_CTL0);	//NFREG_PING_PONG_BUFFER_DISABLE
 
	//Set Table SRAM
	//rtk_writel( 0x00, REG_TABLE_CTL);	//disable
 	//rtk_writel( 0xE1, REG_TABLE_CTL);	//enable
 	
	//Set write command
	rtk_writel(CMD_PG_WRITE_C1, REG_CMD1);
	rtk_writel(CMD_PG_WRITE_C2, REG_CMD2);
	rtk_writel(CMD_PG_WRITE_C3, REG_CMD3);
 	
	//Set address
	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (addr_mode<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  
	rtk_writel( ((page>>21)&0x7)<<5, REG_PAGE_ADR3 );
 		
	//Set ECC
	rtk_writel( 0x20, REG_MULTICHNL_MODE);
	rtk_writel( 0x80, REG_ECC_STOP);
	
	//enhance performance, remove them from below "for" loop
	dma_len = page_size >> 9;	//512B Unit
	//Set DMA, Xfer 1K-pagesize data
	//dma_len = (1024) >> 9;	//512B Unit
	rtk_writel(dma_len, REG_DMA_CTL2);

	dram_sa = ( (uint32_t)data_buf >> 3);
	rtk_writel(dram_sa, REG_DMA_CTL1);	
	//Enable Auto mode
	rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
	//Enable DMA Xfer
	rtk_writel(0x01, REG_DMA_CTL3);	//DMA_DIR_WRITE 		
 	//Wait DMA done
 	while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
	/* check read result */
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
	
	//printk ("write: rtk_readl(REG_DMA_CTL3) = 0x%x\n", rtk_readl(REG_DMA_CTL3));
	
	//Hynix check "write" is ok.
 	// execute command3 register and wait for executed completion
	rtk_writel(0x0d, REG_POLL_STATUS );
	while ( rtk_readl(REG_POLL_STATUS) & 0x01 );

//=================================================================
#if 0	//for write bbt test
	//if ( page == 8200 ){	//at block=64
	//if ( page == 521700 ){	//at block=4075, test remap is bad, too
	if ( page == 8380  || page == 9010 || page == 12708 ){	//at block=65, 70, 99 or HY27UG088G5B: 130, 140, 198
	//if ( chipnr == 0 && (page == 260692  || page == 261775) ){	//HY27UG088G5B: block 4073, 4090
		up (&sem);
		printk("[%s] write is not completed at page %d\n", __FUNCTION__, page);
		return -1;	
	}
#endif	
//=================================================================
	
	//check if the write is completed
	if ( rtk_readl(REG_DATA) & 0x01 ){ // IO0: 0: pass, 1: fail
		up (&sem);
		printk("[%s] write is not completed at page %d\n", __FUNCTION__, page);
		return -1;
	}

	/* release semaphore */
	up (&sem);

	//mark the page 0x00 after write data in it
	unsigned int chip_section = (chipnr * this->page_num) >> 5;		//because of __u32 
	unsigned int section = page >> 5;		//because of __u32 
	unsigned int index = page & (32-1);
	//printk("this->erase_page_flag[section]=%x\n", this->erase_page_flag[chip_section+section]);
	this->erase_page_flag[chip_section+section] &= ~(1 << index);	//mark the bit is 0
	//printk("section=%d, index=%d, this->erase_page_flag[section]=%x\n", 
		//	section, index, this->erase_page_flag[chip_section+section]);

#if 0
	int j;
	for ( j=0; j < 512; j++){
		if ( !(j % 8)  ){
			printk("\n");
			printk("[%p]:", &(this->erase_page_flag[j]));
		}
		printk("[0x%x]", this->erase_page_flag[j]);
	}	
#endif	
		
//========================================================
#if 0	//read page back to check 
	unsigned char data_buf1[page_size] __attribute__((__aligned__(8)));
	unsigned char oob_buf1[oob_size];
	rtk_read_ecc_page (mtd, chipnr, page, data_buf1, oob_buf1);
	//if ( page == 30 ){	//start from begining of nand
	//if ( page == 8192 + 30 ){	//start from begining 16 MB of nand
	//if ( page == 8380 ){	
	if ( page == 8192){
		rtk_dump_nand_data ((__u8 *)data_buf1, 1600, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_DATA); 
		if (oob_buf1)
			rtk_dump_nand_data ((__u8 *)oob_buf1, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_OOB); 
	}
#endif	
//========================================================	

#if KEN_TIMER_WRITE
	count_w++;
	do_gettimeofday(&time2);
	sec = time2.tv_sec - time1.tv_sec;
	usec = time2.tv_usec - time1.tv_usec;
	if(usec < 0){
		sec--;
		usec+=1000000;
	}
	total_time_w += sec*1000000+usec;
	//05.mp3: 5524414 Bytes
	if ( page_size == 4096 ){
		if (count_w==1350 )
			printk("[%s] page=%x total_time_w spends %ld MicroSec, count_w=%d===\n",
			__FUNCTION__, page, total_time_w, count_w);
	}else{ 
		if (count_w==2700 )	//05.mp3
			printk("[%s] page=%x total_time_w spends %ld MicroSec, count_w=%d===\n",
			__FUNCTION__, page, total_time_w, count_w);
	}
#endif

	return rc;
}


/* command line partition*/
const char *ptypes[] = {"cmdlinepart", NULL};

#if KEN_MTD_PARTITION
#if 1
/* static partition */
static struct mtd_partition dft_parts[] = {
	{ .name	  = "nand_nda",
	  //.offset = 0x00000000,	//0th page
	  .offset = 0x01000000,	//first 16MB, nand boot
	  //.offset = 0x1FD00000,	//	509 MB, cross Die boundary
	  //.offset	  = 0x3FE00000,		//1022 MB, cross 2 x HY27UT088G2A Die boundary
	  .size	  = 0x1000000 },		//16MB
	  //.size	  = 0x4000000 },		//64MB
	  //.size	  = 0x8000000 },		//128MB	
	  //.size	  = 0xC000000 },	//192MB
	  //.size	  = 0x10000000 },	//256MB		    
	  //.size	  = 0x20000000 },	//512MB		    
	  //.size	  = 0x40000000 },	//1024MB		    	  
	  //.size	  = 0x07000000 },	//nand boot, 128MB
	  //.size	  = 0x3F000000 },	//nand boot,  1024MB-16MB, HY27UT088G2A
	  //.size	  = 0x7F000000 },	//nand boot,  1024MB-16MB, 2 x HY27UT088G2A
	 //.size	  = 0x3BE00000 },	//nand boot,  1024MB-RBA, HY27UT088G2A
	 //.size	  = 0x3F000000 },	//nand boot,  1024MB-16MB, HY27UG088G5B
	  //.size	  = 0x1F400000 },	//nand boot,  500MB, HY27UG088G5B
	 //{ .name	  = "nand_RBA",	//RBA: 200 block
	  //.offset = 0x3CE00000,
	  //.size	  = 0x3200000 },	  
};
#else	
/* static partition */
static struct mtd_partition dft_parts[] = {
	{ .name	  = "nand_P0",
	  .offset	= 0x1000000,	//first 16MB, nand boot
	  .size	= 0x3200000 },	//50MB
	{ .name	= "nand_P1",
	  .offset	= 0x4200000,
	  .size	= 0x3C00000 },	//60MB
	 { .name  = "nand_P2",
	  .offset = 0x7E00000,
	  .size	= 0x4600000 },	//70MB
	 { .name  = "nand_P3",
	  .offset = 0xC400000,
	  .size	= 0x5000000 },	//80MB
	{ .name	  = "nand_P4",
	  .offset = 0x11400000,
	  .size	= 0x5A00000 },	//90MB  
	{ .name	  = "nand_P5",
	  .offset 	= 0x16E00000,
	  .size	= 0x6400000 }, 	//100MB
	{ .name	  = "nand_P6",
	  .offset 	= 0x1D200000,
	  .size	= 0x1000000 }, 	//16MB
};
#endif
#endif	//KEN_MTD_PARTITION


/*
 * rtk_nand_profile: get nand flash profile
 */
static int rtk_nand_profile (void)
{
	//printk ("[%s] Initialize on-baord NAND\n", __FUNCTION__);
	/* RTK HW IP supports max 4 dies */
	int maxchips = 4;	
	struct nand_chip *this = (struct nand_chip *)rtk_mtd->priv;
	/* set up RBA_PERCENT, it can be tuned in future */
	this->RBA_PERCENT = RBA_PERCENT;
	if (rtk_nand_scan (rtk_mtd, maxchips) < 0 || rtk_mtd->size == 0){
		printk("%s: Error, cannot do nand_scan(on-board)\n", __FUNCTION__);
		return -ENODEV;
	}

	/* Capacity */
	if ( !(rtk_mtd->oobblock&(0x200-1)) )
		rtk_writel( rtk_mtd->oobblock >> 9, REG_PAGE_LEN);
	else{ 
		printk("Error: pagesize is not 512B Multiple");
		return -1;
	}

//=======KEN_MTD_PARTITION Test =========
#if KEN_MTD_PARTITION  
	char *ptype;
	int pnum = 0;
	struct mtd_partition *mtd_parts;

#ifdef CONFIG_MTD_CMDLINE_PARTS
	ptype = (char *)ptypes[0];
	pnum = parse_mtd_partitions (rtk_mtd, ptypes, &mtd_parts, 0);
	//printk ("CMDLINE part num = %d\n", pnum);
#endif

	/* static partitions */
	if ((pnum <= 0) && dft_parts) {
		mtd_parts = dft_parts;
		pnum = sizeof (dft_parts) / sizeof (struct mtd_partition);
		ptype = "ken-setting";
	}

	if (pnum > MAX_PARTITIONS) {
		printk ("%s: exceed the partition limitation\n", __FUNCTION__);
		pnum = MAX_PARTITIONS;
	}
	printk ("%s: Using %s partition definition\n", rtk_mtd->name, ptype);
	
	if (add_mtd_partitions (rtk_mtd, mtd_parts, pnum)) {
		printk("%s: Error, cannot add %s device\n", 
				__FUNCTION__, rtk_mtd->name);
		rtk_mtd->size = 0;
	}

#else		
//=============== Official Run  ================
	//rescure will pass partition table, otherwise I use whole nand flash as partition
	char *ptype;
	int pnum = 0;
	struct mtd_partition *mtd_parts;

#ifdef CONFIG_MTD_CMDLINE_PARTS
	ptype = (char *)ptypes[0];
	pnum = parse_mtd_partitions (rtk_mtd, ptypes, &mtd_parts, 0);
	//printk ("CMDLINE part num = %d\n", pnum);
#endif

	/* static partitions */
	if (pnum <= 0) {	
		printk(KERN_NOTICE "RTK: using the whole nand as a partitoin\n");
		if(add_mtd_device(rtk_mtd)) {
			printk(KERN_WARNING "RTK: Failed to register new nand device\n");
			return -EAGAIN;
		}
	}else{
		printk(KERN_NOTICE "RTK: using dynamic nand partition\n");
		if (add_mtd_partitions (rtk_mtd, mtd_parts, pnum)) {
			printk("%s: Error, cannot add %s device\n", 
					__FUNCTION__, rtk_mtd->name);
			rtk_mtd->size = 0;
			return -EAGAIN;
		}	
	}

#endif		//KEN_MTD_PARTITION

	return 0;
}


/*
 * rtk_read_proc_nandinfo: write /proc/nandinfo
 */
int rtk_read_proc_nandinfo(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	struct nand_chip *this = (struct nand_chip *) rtk_mtd->priv;
	int wlen = 0;

	wlen += sprintf(buf+wlen,"nand_PartNum:%s\n", rtk_mtd->PartNum);
	wlen += sprintf(buf+wlen,"nand_size:%u\n", this->device_size);
	wlen += sprintf(buf+wlen,"chip_size:%u\n", this->chipsize);
	wlen += sprintf(buf+wlen,"block_size:%u\n", rtk_mtd->erasesize);
	wlen += sprintf(buf+wlen,"page_size:%u\n", rtk_mtd->oobblock);
	wlen += sprintf(buf+wlen,"oob_size:%u\n", rtk_mtd->oobsize);
	wlen += sprintf(buf+wlen,"ppb:%u\n", rtk_mtd->erasesize/rtk_mtd->oobblock);
	wlen += sprintf(buf+wlen,"RBA:%u\n", this->RBA);
	wlen += sprintf(buf+wlen,"BBs:%u\n", this->BBs);
	
	return wlen;
}


static void display_version (void)
{
	const __u8 *revision;
	const __u8 *date;
	char *running = (__u8 *)VERSION;
	//printk("running=%s\n", running);
	strsep(&running, " ");
	strsep(&running, " ");
	revision = strsep(&running, " ");
	date = strsep(&running, " ");
	printk(BANNER " Rev:%s (%s)\n", revision, date);
}


/*
 * RTK NAND Main Initialization Routine
 */
static int __init rtk_nand_init (void)
{
	//Cuz All RTK SOC use the same rescure, I have to check the chip types.
	//Nand driver only supports chip type after mars
	if ( is_venus_cpu() || is_neptune_cpu() )
		return -1;

	if ( rtk_readl(0xb800000c) & 0x00800000 ){
		/* show nand flash driver revision */
		display_version();
	}else{ 
		printk(KERN_ERR "Nand Flash Clock is NOT Open. Installing fails.\n");
		return -1;	
	}

	/*  set PIN_MUX to nand, not pcmcia */
	rtk_writel( 0x103, 0xb800036c );
	
	/* set up NF clock */
	// disable NF clock
	rtk_writel(rtk_readl(0xb800000c)& (~0x00800000), 0xb800000c);
	//nf clock 72MHZ
	rtk_writel( 0x02, 0xb8000034 );
	// enable NF clock
	rtk_writel(rtk_readl(0xb800000c)| (0x00800000), 0xb800000c);

	struct nand_chip *this;
	int rc = 0;
	
	/* allocate and initial data structure */
	rtk_mtd = kmalloc (MTDSIZE, GFP_KERNEL);
	if ( !rtk_mtd ){
		printk("%s: Error, no enough memory for rtk_mtd\n",__FUNCTION__);
		rc = -ENOMEM;
		goto EXIT;
	}
	memset ( (char *)rtk_mtd, 0, MTDSIZE);
	rtk_mtd->priv = this = (struct nand_chip *)(rtk_mtd+1);
	
 	/* initial setting chip #0 */
	rtk_writel( ~(0x1 << 0), REG_CHIP_EN );
	/* Reset Nand Controller */
	rtk_writel( CMD_RESET, REG_CMD1 );
	rtk_writel( (0x80 | 0x00), REG_CTL );
	while( rtk_readl(REG_CTL) & 0x80 );
	
	//rtk_writel( readl(0xb800000c)|(1<<23), 0xb800000c );
	//debug_nand("NAND CLK: bit23: 0x%x\n", readl(0xb800000c));
	
	/* disable EDO timing clock is below < 30 MHZ */
	rtk_writel(0x00, REG_MULTICHNL_MODE);
		
	/* Initial CLE, ALE setup time */
	rtk_writel( 0x06, REG_T1 );
	rtk_writel( 0x06, REG_T2 );
	rtk_writel( 0x06, REG_T3 );
	rtk_writel( 0x09, REG_DELAY_CNT );  // set T_WHR_ADL delay clock #
     
	/* Interrupt Mask*/ 
	//rtk_writel( 0x09d, REG_ISREN );	//open Xfer, AutoMode, DMA, Polling Flash Status.
	//debug_nand("rtk_readl(REG_ISREN): 0x%lx\n", rtk_readl(REG_ISREN));

	/* Disable Table SRAM */
	rtk_writel( 0x00, REG_TABLE_CTL);	//disable
	
	/* set up callback function for MTD use */
	this->read_id		= rtk_nand_read_id;
	this->read_ecc_page 	= rtk_read_ecc_page;
	this->read_oob 		= rtk_read_oob;
	this->write_ecc_page	= rtk_write_ecc_page;
	this->write_oob		= rtk_write_oob;
	this->erase_block 	= rtk_erase_block;
	// CMYu, 20090422
	this->suspend		= rtk_nand_suspend;
	this->resume		= rtk_nand_resume;
	this->sync		= NULL;

	/* get nand flash profile */
	if( rtk_nand_profile() < 0 ){
		rc = -1;
		goto EXIT;
	}
	
	//set up global variables
	page_size = rtk_mtd->oobblock;
	oob_size = rtk_mtd->oobsize;
	ppb = (rtk_mtd->erasesize)/(rtk_mtd->oobblock);
	
	/* create /pro/nandinfo */
	create_proc_read_entry("nandinfo", 0, NULL, rtk_read_proc_nandinfo, NULL);
	
#if RTK_TEST_RWE
	int stress_page = 940*ppb+32;	//page = 940*64+32 = 60192
	TEST_RWE(rtk_mtd, stress_page);
#endif	

#if RTK_TEST_OOB
	TEST_OOB(rtk_mtd);
#endif	


EXIT:
	if (rc < 0) {
		if (rtk_mtd){
			del_mtd_partitions (rtk_mtd);
			if (this->g_databuf)
				kfree(this->g_databuf);
			if(this->g_oobbuf)
				kfree(this->g_oobbuf);
			if (this->erase_page_flag){
				unsigned int flag_size =  (this->numchips * this->page_num) >> 3;		//Kynix 1GB: 4096block, 128 page/block
				unsigned int mempage_order = get_order(flag_size);
				free_pages((unsigned long)this->erase_page_flag, mempage_order);
			}
			kfree(rtk_mtd);
		}
		remove_proc_entry("nandinfo", NULL);	    //release entry
	}else
		printk(KERN_INFO "Realtek Nand Flash Driver is successfully installing.\n");
	
	return rc;
}


void __exit rtk_nand_exit (void)
{
	if (rtk_mtd){
		del_mtd_partitions (rtk_mtd);
		struct nand_chip *this = (struct nand_chip *)rtk_mtd->priv;
		if (this->g_databuf)
			kfree(this->g_databuf);
		if(this->g_oobbuf)
			kfree(this->g_oobbuf);
		if (this->erase_page_flag){
			unsigned int flag_size =  (this->numchips * this->page_num) >> 3;		//Kynix 1GB: 4096block, 128 page/block
			unsigned int mempage_order = get_order(flag_size);
			free_pages((unsigned long)this->erase_page_flag, mempage_order);
		}
		kfree(rtk_mtd);
	}
	remove_proc_entry("nandinfo", NULL);	    //release entry
}

module_init(rtk_nand_init);
module_exit(rtk_nand_exit);
MODULE_AUTHOR("Ken Yu<Ken.Yu@realtek.com>");
MODULE_DESCRIPTION("Realtek NAND Flash Controller Driver");


//=============================Ken: RTK_TEST ===========================
#if RTK_TEST_RWE
int TEST_RWE(struct mtd_info *mtd, int page)
{
	int rc;
	int r_cnt=0, w_cnt=0, e_cnt=0;
	int isLastPage=0;
	int block = page/ppb;
	unsigned char oob_buf1[64] __attribute__((__aligned__(8)));
	unsigned char oob_buf[64] __attribute__((__aligned__(8)))= {
	0xa1, 0x34, 0x46, 0x76, 0x88, 0x13, 0x86, 0x67, 0x11, 0x34, 0x46, 0x22, 0x83, 0x13, 0x89, 0xdd,  							
	0xa5, 0x99, 0x35, 0x23, 0x11, 0x83, 0x44, 0x66, 0xa1, 0x5b, 0x7c, 0x71, 0x83, 0x13, 0x86, 0xd7, 
	0xab, 0xa1, 0x3d, 0x74, 0x81, 0x53, 0x76, 0x6a, 0xcd, 0x33, 0x7a, 0x56, 0xa8, 0x73, 0xaa, 0xe2, 
	0xef, 0x99, 0x11, 0x77, 0x8d, 0x16, 0xbb, 0xf2, 0xa9, 0x77, 0x42, 0x70, 0x8b, 0x13, 0xcc, 0x67
	};
	unsigned char data_buf1[2048] __attribute__((__aligned__(8)));
	unsigned char data_buf[2048] __attribute__((__aligned__(8)))= {
	0xa1, 0x34, 0x46, 0x76, 0x88, 0x13, 0x86, 0x67, 0x11, 0x34, 0x46, 0x22, 0x83, 0x13, 0x89, 0xdd,  							
	0xa5, 0x99, 0x35, 0x23, 0x11, 0x83, 0x44, 0x66, 0xa1, 0x5b, 0x7c, 0x71, 0x83, 0x13, 0x86, 0xd7, 
	0xab, 0xa1, 0x3d, 0x74, 0x81, 0x53, 0x76, 0x6a, 0xcd, 0x33, 0x7a, 0x56, 0xa8, 0x73, 0xaa, 0xe2, 
	0xef, 0x99, 0x11, 0x77, 0x8d, 0x16, 0xbb, 0xf2, 0xa9, 0x77, 0x42, 0x70, 0x8b, 0x13, 0xcc, 0x67, 	
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,								
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66
};

	//RTK_FLUSH_CACHE(data_buf, 2048);
	//RTK_FLUSH_CACHE(data_buf1, 2048);

	//rtk_dump_nand_data ((__u8 *)data_buf, 0, 256, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_DATA); 

#if 0
	if ( ( rc=rtk_erase_block(mtd, 0, (page/ppb)*ppb) ) < 0 ){
		printk("erase block %d fails, e_cnt=%d, w_cnt=%d, r_cnt=%d, rc=%d\n", page/ppb, e_cnt, w_cnt, r_cnt, rc);
	 }else
		e_cnt++;
	 
	 if ( ( rc=rtk_write_ecc_page(mtd, 0, page, data_buf,  oob_buf, 0) ) < 0 ){
	 	printk("write page %d fails, e_cnt=%d, w_cnt=%d, r_cnt=%d, rc=%d\n", page, e_cnt, w_cnt, r_cnt, rc);
	 }else
	 	w_cnt++;

	 if ( ( rc=rtk_read_ecc_page(mtd, 0, page, data_buf1,  oob_buf1) ) < 0 ){
	 	printk("read page %d fails, e_cnt=%d, w_cnt=%d, r_cnt=%d, rc=%d\n", page, e_cnt, w_cnt, r_cnt, rc);
	 }else
	 	r_cnt++;
		 
	printk("present rc=%d, e_cnt=%d, w_cnt=%d, r_cnt=%d\n", rc, e_cnt, w_cnt, r_cnt);
	rtk_dump_nand_data ((__u8 *)data_buf1, 0, 2048, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_DATA); 
	rtk_dump_nand_data ((__u8 *)oob_buf1, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_OOB); 

#else	//stress test

	while(1){
		 if ( ( rc=rtk_erase_block(mtd, 0, (page/ppb)*ppb) ) < 0 ){
		 	printk("erase block %d fails, e_cnt=%d, w_cnt=%d, r_cnt=%d, rc=%d\n", page/ppb, e_cnt, w_cnt, r_cnt, rc);
		 	printk("rtk_readl(REG_DATA) = 0x%x\n", rtk_readl(REG_DATA) );

			oob_buf[0] = 0x00;
			if ( isLastPage ){	//mark the block as bad
				rtk_write_oob(mtd, 0, block*ppb+ppb-1, oob_size, oob_buf);	//the last page
				rtk_write_oob(mtd, 0, block*ppb+ppb-2, oob_size, oob_buf);	//the (last-1) page
			}else{		//other nand: mark the block as bad
				rtk_write_oob(mtd, 0, block*ppb, oob_size, oob_buf);	//first page
				rtk_write_oob(mtd, 0, block*ppb+1, oob_size, oob_buf);	//2nd page
			}
			rtk_dump_nand_data ((__u8 *)oob_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_OOB); 
				
		 	break;
		 }else
		 	e_cnt++;

		 if ( ( rc=rtk_write_ecc_page(mtd, 0, page, data_buf,  oob_buf, 0) ) < 0 ){
		 	printk("write page %d fails, e_cnt=%d, w_cnt=%d, r_cnt=%d, rc=%d\n", page, e_cnt, w_cnt, r_cnt, rc);
		 	break;
		 }else
		 	w_cnt++;

		 if ( ( rc=rtk_read_ecc_page(mtd, 0, page, data_buf1,  oob_buf1) ) < 0 ){
		 	printk("read page %d fails, e_cnt=%d, w_cnt=%d, r_cnt=%d, rc=%d\n", page, e_cnt, w_cnt, r_cnt, rc);
		 	break;
		 }else
		 	r_cnt++;
		 
		 if ( !(e_cnt%1000) )
		 	printk("present rc=%d, e_cnt=%d, w_cnt=%d, r_cnt=%d\n", rc, e_cnt, w_cnt, r_cnt);
	}
#endif

	return 0;
}
#endif 


#if RTK_TEST_OOB
int TEST_OOB(struct mtd_info *mtd)
{
	int j;
	unsigned char data_buf1[64] __attribute__((__aligned__(8)));
	unsigned char data_buf[64] __attribute__((__aligned__(8)))= {
	0xa1, 0x34, 0x46, 0x76, 0x88, 0x13, 0x86, 0x67, 0x11, 0x34, 0x46, 0x22, 0x83, 0x13, 0x89, 0xdd,  							
	0xa5, 0x99, 0x35, 0x23, 0x11, 0x83, 0x44, 0x66, 0xa1, 0x5b, 0x7c, 0x71, 0x83, 0x13, 0x86, 0xd7, 
	0xab, 0xa1, 0x3d, 0x74, 0x81, 0x53, 0x76, 0x6a, 0xcd, 0x33, 0x7a, 0x56, 0xa8, 0x73, 0xaa, 0xe2, 
	0xef, 0x99, 0x11, 0x77, 0x8d, 0x16, 0xbb, 0xf2, 0xa9, 0x77, 0x42, 0x70, 0x8b, 0x13, 0xcc, 0x67
	};

	//RTK_FLUSH_CACHE(data_buf, 2048);
	//RTK_FLUSH_CACHE(data_buf1, 2048);
	rtk_dump_nand_data ((__u8 *)data_buf, 0, 64, (__u8 *)__FUNCTION__, __LINE__,BEFORE_CALL, DUMP_OOB); 
	//rtk_write_oob(mtd, 128, 0, 64, data_buf);
	//rtk_read_oob(mtd, 128, 0, 64, data_buf1);
	rtk_write_oob(mtd, 128, 64, data_buf, 0);
	rtk_read_oob(mtd, 128, 64, data_buf1);	
	rtk_dump_nand_data ((__u8 *)data_buf1, 0, 64, (__u8 *)__FUNCTION__, __LINE__,AFTER_CALL, DUMP_OOB); 
	return 0;
}
#endif 

