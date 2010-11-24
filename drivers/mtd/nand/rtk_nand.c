/******************************************************************************
 * $Id: rtk_nand.c 268236 2009-09-17 05:57:24Z ken.yu $
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
#define VERSION  "$Id: rtk_nand.c 268236 2009-09-17 05:57:24Z ken.yu $"

#define MTDSIZE	(sizeof (struct mtd_info) + sizeof (struct nand_chip))
#define MAX_PARTITIONS	16
#define BOOTCODE	16*1024*1024	//16MB


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
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
		udelay(20+60);
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
	}else{
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
	}
}


static void resume_to_reset_reg(void)
{
	rtk_writel( 0x103, 0xb800036c );
	rtk_writel(rtk_readl(0xb800000c)& (~0x00800000), 0xb800000c);
	rtk_writel( 0x02, 0xb8000034 );
	rtk_writel(rtk_readl(0xb800000c)| (0x00800000), 0xb800000c);
 	
	rtk_writel( ~(0x1 << 0), REG_CHIP_EN );
	rtk_writel( CMD_RESET, REG_CMD1 );
	rtk_writel( (0x80 | 0x00), REG_CTL );
	while( rtk_readl(REG_CTL) & 0x80 );
	
	rtk_writel(0x00, REG_MULTICHNL_MODE);
		
	rtk_writel( 0x00, REG_T1 );
	rtk_writel( 0x00, REG_T2 );
	rtk_writel( 0x00, REG_T3 );
	rtk_writel( 0x09, REG_DELAY_CNT );  
     
	rtk_writel( 0x00, REG_TABLE_CTL);

}


static void rtk_nand_resume (struct mtd_info *mtd)
{
	resume_to_reset_reg();
	if ( rtk_readl(REG_DMA_CTL3) & 0x02 ){
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
		udelay(20+60);
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
	}else{
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
	}
}


static void read_oob_from_PP(struct mtd_info *mtd, __u8 *r_oobbuf, int section)
{
	unsigned int reg_oob, reg_num;
	int i;

	if ( mtd->ecctype == MTD_ECC_NONE ){
		if ( section==0 ){
			reg_num = REG_BASE_ADDR;
			reg_oob = rtk_readl(reg_num);	
			r_oobbuf[0] = reg_oob & 0xff;
			r_oobbuf[1] = (reg_oob >> 8) & 0xff;
			r_oobbuf[2] = (reg_oob >> 16) & 0xff;
			r_oobbuf[3] = (reg_oob >> 24) & 0xff;	
			
			reg_num = REG_BASE_ADDR+4;
			reg_oob = rtk_readl(reg_num);	
			r_oobbuf[4] = reg_oob & 0xff;
			
			reg_num = REG_BASE_ADDR+4*4;
			reg_oob = rtk_readl(reg_num);	
			r_oobbuf[5] = reg_oob & 0xff;
			r_oobbuf[6] = (reg_oob >> 8) & 0xff;
			r_oobbuf[7] = (reg_oob >> 16) & 0xff;
			r_oobbuf[8] = (reg_oob >> 24) & 0xff;	
		}else{
			reg_num = REG_BASE_ADDR;
			reg_oob = rtk_readl(reg_num);	
			r_oobbuf[9] = reg_oob & 0xff;
			r_oobbuf[10] = (reg_oob >> 8) & 0xff;
			r_oobbuf[11] = (reg_oob >> 16) & 0xff;
			r_oobbuf[12] = (reg_oob >> 24) & 0xff;		
	
			reg_num = REG_BASE_ADDR+4*4;
			reg_oob = rtk_readl(reg_num);	
			r_oobbuf[13] = reg_oob & 0xff;
			r_oobbuf[14] = (reg_oob >> 8) & 0xff;
			r_oobbuf[15] = (reg_oob >> 16) & 0xff;
			r_oobbuf[16] = (reg_oob >> 24) & 0xff;	
		}
	}else{
		for ( i=0; i < (32/4); i++){
			reg_num = REG_BASE_ADDR + i*4;
			reg_oob = rtk_readl(reg_num);
			r_oobbuf[32*section + i*4+0] = reg_oob & 0xff;
			r_oobbuf[32*section + i*4+1] = (reg_oob >> 8) & 0xff;
			r_oobbuf[32*section + i*4+2] = (reg_oob >> 16) & 0xff;
			r_oobbuf[32*section + i*4+3] = (reg_oob >> 24) & 0xff;
		}
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


static void rtk_nand_read_id(struct mtd_info *mtd, u_char id[5])
{
	int id_chain;

     rtk_writel(0x05, REG_DATA_CNT1);
     rtk_writel(0x80, REG_DATA_CNT2);
     
	rtk_writel(0x0, REG_PP_RDY);
	rtk_writel(0x01, REG_PP_CTL0);
	rtk_writel(0x0, REG_PP_CTL1);
	
	rtk_writel(CMD_READ_ID, REG_CMD1);
	rtk_writel(0x80, REG_CTL);
	while ( rtk_readl(REG_CTL) & 0x80 );
	
	rtk_writel(0, REG_PAGE_ADR0);
	rtk_writel(0x7<<5, REG_PAGE_ADR2);
	rtk_writel(0x81, REG_CTL);
	while ( rtk_readl(REG_CTL) & 0x80 );

	rtk_writel(0x84, REG_CTL);
	while ( rtk_readl(REG_CTL) & 0x80 );

	rtk_writel(0x2, REG_PP_CTL0);
	
	rtk_writel(0x30, REG_SRAM_CTL);
	
	id_chain = rtk_readl(REG_PAGE_ADR0);
	id[0] = id_chain & 0xff;
	id[1] = (id_chain >> 8) & 0xff;
	id[2] = (id_chain >> 16) & 0xff;
	id[3] = (id_chain >> 24) & 0xff;

	id_chain = rtk_readl(REG_PAGE_ADR1);
	id[4] = id_chain & 0xff;

	rtk_writel(0x0, REG_SRAM_CTL);	//# no omitted
}


static int rtk_erase_block(struct mtd_info *mtd, u16 chipnr, int page)
{	
	page_size = mtd->oobblock;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->oobblock;
	
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	
	if ( page & (ppb-1) ){
		printk("%s: page %d is not block alignment !!\n", __FUNCTION__, page);
		return -1;
	}

	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}
	
	rtk_writel( 0x20, REG_MULTICHNL_MODE);

	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (0x4<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );
	rtk_writel( ((page>> 21)&0x7)<<5, REG_PAGE_ADR3 );
	
	rtk_writel( CMD_BLK_ERASE_C1, REG_CMD1 );
	rtk_writel( CMD_BLK_ERASE_C2, REG_CMD2 );
	rtk_writel( CMD_BLK_ERASE_C3, REG_CMD3 );	

	rtk_writel( (1 << 7)|(0x01 << 3)|0x02, REG_AUTO_TRIG );
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );		

	rtk_writel(0x0d, REG_POLL_STATUS );
	while ( rtk_readl(REG_POLL_STATUS) & 0x01 );

	if ( rtk_readl(REG_DATA) & 0x01 ){
		up (&sem);
		printk("[%s] erasure is not completed at block %d\n", __FUNCTION__, page/ppb);
		if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size )
			return 0;
		else
			return -1;
	}

	up (&sem);
	
	unsigned int chip_section = (chipnr * this->page_num) >> 5;
	unsigned int section = page >> 5;
	memset ( (__u32 *)(this->erase_page_flag)+ chip_section + section, 0xff, ppb>>3);

	return 0;
}


static int rtk_read_oob (struct mtd_info *mtd, u16 chipnr, int page, int len, u_char *oob_buf)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	int dram_sa, dma_len;
	int blank_all_one=0;
	int dma_counter = page_size >> 10;
	int buf_pos=0;
	uint8_t	auto_trigger_mode = 2;
	uint8_t	addr_mode = 1;
	int page_len;
	
	if ( len>oob_size || !oob_buf ){
		printk("[%s] error: len>oob_size OR oob_buf is NULL\n", __FUNCTION__);
		return -1;
	}

	RTK_FLUSH_CACHE((unsigned long) this->g_databuf, page_size);
	if ( oob_buf ) 
		RTK_FLUSH_CACHE((unsigned long) oob_buf, oob_size);
			
	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}
	
	rtk_writel(0x01, REG_BLANK_CHECK);
	
	rtk_writel(0x00, REG_DATA_CNT1);
	rtk_writel(0x82, REG_DATA_CNT2);

	page_len = 1024 >> 9;
	rtk_writel(page_len, REG_PAGE_LEN);
	
	rtk_writel(0x80, REG_PP_RDY);
	rtk_writel(0x00, REG_PP_CTL1);
	rtk_writel(0x01, REG_PP_CTL0);

	rtk_writel(CMD_PG_READ_C1, REG_CMD1);
	rtk_writel(CMD_PG_READ_C2, REG_CMD2);
	rtk_writel(CMD_PG_READ_C3, REG_CMD3);

	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);

	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (addr_mode<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  
	rtk_writel( ((page>>21)&0x7)<<5, REG_PAGE_ADR3 );

	rtk_writel( 0x20, REG_MULTICHNL_MODE);
	rtk_writel( 0x80, REG_ECC_STOP);

	dma_len = 1024 >> 9;	
	rtk_writel(dma_len, REG_DMA_CTL2);
	
	dram_sa = ( (uint32_t) this->g_databuf >> 3);
	rtk_writel(dram_sa, REG_DMA_CTL1);	

	rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
	
	rtk_writel(0x03, REG_DMA_CTL3);
	while ( rtk_readl(REG_DMA_CTL3) & 0x01 );

	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );		
		
	rtk_writel(0x00, REG_PP_RDY);
	rtk_writel(0x30 | 0x02, REG_SRAM_CTL);	
	read_oob_from_PP(mtd, oob_buf, 0);
	rtk_writel(0x00, REG_SRAM_CTL); 
		
	dma_counter--;
	buf_pos++;				

	/*============================2nd-1K page==============================*/
	while(dma_counter>0){
		rtk_writel(0x80, REG_PP_RDY);	

		dram_sa = ( (uint32_t)(this->g_databuf + buf_pos*1024) >> 3);
		rtk_writel(dram_sa, REG_DMA_CTL1);	

		rtk_writel( (0x01 << 7)|(0x00 << 3)|0x04, REG_AUTO_TRIG );

		rtk_writel(0x03, REG_DMA_CTL3);	//DMA_DIR_READ

		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );

		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );

		rtk_writel(0x00, REG_PP_RDY);
		rtk_writel(0x30 | 0x02, REG_SRAM_CTL);	
		read_oob_from_PP(mtd, oob_buf, 1);

		rtk_writel(0x00, REG_SRAM_CTL); 							
		
		dma_counter--;
		buf_pos++;
	}

	up (&sem);
	
	unsigned int chip_section = (chipnr * this->page_num) >> 5;
	unsigned int section = page >> 5;
	unsigned int index = page & (32-1);	
	blank_all_one = (rtk_readl(REG_BLANK_CHECK)>>1) & 0x01;
	if (blank_all_one)
		this->erase_page_flag[chip_section+section] =  (1<< index);
		
	if (rtk_readl(REG_ECC_STATE) & 0x0C){
		if (this->erase_page_flag[chip_section+section] & (1<< index) ){
			;
		}else{
			if (rtk_readl(REG_ECC_STATE) & 0x08){		
				if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size )
					return 0;
				else
					return -1;
			}
			if (rtk_readl(REG_ECC_STATE) & 0x04)		
				printk("[%s] Correctable HW ECC Error at page=%d\n", __FUNCTION__, page);
		}
	}

	return rc;
}


static int rtk_read_ecc_page (struct mtd_info *mtd, u16 chipnr, unsigned int page, 
			u_char *data_buf, u_char *oob_buf)
{

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
	
	if ( !oob_buf )
		memset(this->g_oobbuf, 0xff, oob_size);

	RTK_FLUSH_CACHE((unsigned long) data_buf, page_size);
	if ( oob_buf )
		RTK_FLUSH_CACHE((unsigned long) oob_buf, oob_size);
	else
		RTK_FLUSH_CACHE((unsigned long) this->g_oobbuf, oob_size);
					
	uint8_t	auto_trigger_mode = 2;	
	uint8_t	addr_mode = 1;	

	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}

	rtk_writel(0x01, REG_BLANK_CHECK);
	
	rtk_writel( 0x00, REG_DATA_CNT1);
	rtk_writel( 0x82, REG_DATA_CNT2);

	page_len = 1024 >> 9;
	rtk_writel( page_len, REG_PAGE_LEN);
	
	rtk_writel(0x80, REG_PP_RDY);
	rtk_writel(0x00, REG_PP_CTL1);
	rtk_writel(0x01, REG_PP_CTL0);

	rtk_writel(CMD_PG_READ_C1, REG_CMD1);
	rtk_writel(CMD_PG_READ_C2, REG_CMD2);
	rtk_writel(CMD_PG_READ_C3, REG_CMD3);

	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (addr_mode<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  
	rtk_writel( ((page>>21)&0x7)<<5, REG_PAGE_ADR3 );

	rtk_writel( 0x20, REG_MULTICHNL_MODE);
	rtk_writel( 0x80, REG_ECC_STOP);

	dma_len = 1024 >> 9;
	rtk_writel(dma_len, REG_DMA_CTL2);

	dram_sa = ( (uint32_t)data_buf >> 3);
	rtk_writel(dram_sa, REG_DMA_CTL1);	
	rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
	rtk_writel(0x03, REG_DMA_CTL3);
	while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );

	rtk_writel(0x00, REG_PP_RDY);
	rtk_writel(0x30 | 0x02, REG_SRAM_CTL);	
	if ( oob_buf ){
		read_oob_from_PP(mtd, oob_buf, 0);
	}else{
		read_oob_from_PP(mtd, this->g_oobbuf, 0);
	}
	rtk_writel(0x00, REG_SRAM_CTL); 

	dma_counter--;
	buf_pos++;

	/*============================2nd-1K page==============================*/
	while(dma_counter>0){
		rtk_writel(0x80, REG_PP_RDY);			

		dram_sa = ( (uint32_t)(data_buf+buf_pos*1024) >> 3);
		rtk_writel(dram_sa, REG_DMA_CTL1);	

		rtk_writel( (0x01 << 7)|(0x00 << 3)|0x04, REG_AUTO_TRIG );
		rtk_writel(0x03, REG_DMA_CTL3);
		while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
		while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );		
		
		rtk_writel(0x00, REG_PP_RDY);
		rtk_writel(0x30 | 0x02, REG_SRAM_CTL);	
		if ( oob_buf ){
			read_oob_from_PP(mtd, oob_buf, 1);
		}else{
			read_oob_from_PP(mtd, this->g_oobbuf , 1);
		}
		rtk_writel(0x00, REG_SRAM_CTL); 							
			
		dma_counter--;
		buf_pos++;		
	}

	up (&sem);

#ifdef CONFIG_REALTEK_MCP
	if ( this->mcp==MCP_AES_ECB ){
		MCP_AES_ECB_Decryption(NULL, data_buf, data_buf, page_size);
	}else{
		;
	}
#endif


	unsigned int chip_section = (chipnr * this->page_num) >> 5;
	unsigned int section = page >> 5;
	unsigned int index = page & (32-1);	
	
	blank_all_one = (rtk_readl(REG_BLANK_CHECK)>>1) & 0x01;
	if (blank_all_one)
		this->erase_page_flag[chip_section+section] =  (1<< index);

	if (rtk_readl(REG_ECC_STATE) & 0x0C){
		if (this->erase_page_flag[chip_section+section] & (1<< index) ){
			;
		}else{
			if (rtk_readl(REG_ECC_STATE) & 0x08){
				if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size )
					return 0;
				else
					return -1;
			}
			if (rtk_readl(REG_ECC_STATE) & 0x04)		
				printk("[%s] Correctable HW ECC Error at page=%d\n", __FUNCTION__, page);
		}
	}
	
	return rc;
}


static int rtk_write_oob(struct mtd_info *mtd, u16 chipnr, int page, int len, const u_char *oob_buf)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	unsigned int page_len, dram_sa, dma_len;
	unsigned char oob_1stB;
	int rc = 0;
	uint8_t	auto_trigger_mode = 1;
	uint8_t	addr_mode = 1;
	
	memset(this->g_databuf, 0xff, page_size);
	
	page_size = mtd->oobblock;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->oobblock;
	
	if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size ){
		printk("[%s] You have no permission to write this page %d\n", __FUNCTION__, page);
		return -2;
	}

	if ( len>oob_size || !oob_buf ){
		printk("[%s] error: len>oob_size OR oob_buf is NULL\n", __FUNCTION__);
		return -1;
	}

	RTK_FLUSH_CACHE((unsigned long) this->g_databuf, page_size);
	if ( oob_buf ) 
		RTK_FLUSH_CACHE((unsigned long) oob_buf, oob_size);
		
	if ( page == 0 || page == ppb )
		oob_1stB = BBT_TAG;
	else		
		oob_1stB = 0xFF;

	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}
	
	rtk_writel(0x3e, REG_SRAM_CTL);
	write_oob_to_TableSram((__u8 *)oob_buf, 0xFF, 0);
	rtk_writel(0x00, REG_SRAM_CTL); 		

	rtk_writel( 0x00, REG_DATA_CNT1);
 	rtk_writel( 0x42, REG_DATA_CNT2);
 	
	page_len = page_size >> 9;	
	rtk_writel( page_len, REG_PAGE_LEN);
 		
	rtk_writel(0x00, REG_PP_RDY);
	rtk_writel(0x00, REG_PP_CTL1);
	rtk_writel(0x00, REG_PP_CTL0);
 
	rtk_writel(CMD_PG_WRITE_C1, REG_CMD1);
	rtk_writel(CMD_PG_WRITE_C2, REG_CMD2);
	rtk_writel(CMD_PG_WRITE_C3, REG_CMD3);
 	
	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (addr_mode<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  
	rtk_writel( ((page>>21)&0x7)<<5, REG_PAGE_ADR3 );
 		
	rtk_writel( 0x20, REG_MULTICHNL_MODE);
	rtk_writel( 0x80, REG_ECC_STOP);
	
	dma_len = page_size >> 9;
	rtk_writel(dma_len, REG_DMA_CTL2);

	dram_sa = ( (uint32_t)this->g_databuf >> 3);
	rtk_writel(dram_sa, REG_DMA_CTL1);	
	rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
	rtk_writel(0x01, REG_DMA_CTL3);
 	while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
	
	rtk_writel(0x0d, REG_POLL_STATUS );
	while ( rtk_readl(REG_POLL_STATUS) & 0x01 );

	if ( rtk_readl(REG_DATA) & 0x01 ){
		up (&sem);
		printk("[%s] write oob is not completed at page %d\n", __FUNCTION__, page);
		return -1;
	}

	up (&sem);

	unsigned int chip_section = (chipnr * this->page_num) >> 5;
	unsigned int section = page >> 5;
	unsigned int index = page & (32-1);
	this->erase_page_flag[chip_section+section] &= ~(1 << index);

	return rc;
}


int rtk_write_ecc_page (struct mtd_info *mtd, u16 chipnr, unsigned int page, 
			const u_char *data_buf, const u_char *oob_buf, int isBBT)	
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	uint8_t	auto_trigger_mode = 1;
	uint8_t	addr_mode = 1;
	unsigned int page_len, dram_sa, dma_len;
	unsigned char oob_1stB;

	if ( !oob_buf )
		memset(this->g_oobbuf, 0xff, oob_size);

#ifdef CONFIG_REALTEK_MCP
	if ( this->mcp==MCP_AES_ECB ){
		MCP_AES_ECB_Encryption(NULL, data_buf, data_buf, page_size);
	}else{
	}
#endif

	RTK_FLUSH_CACHE((unsigned long) data_buf, page_size);
	if ( oob_buf )
		RTK_FLUSH_CACHE((unsigned long) oob_buf, oob_size);
	else
		RTK_FLUSH_CACHE((unsigned long) this->g_oobbuf, oob_size);
			
	if ( chipnr == 0 && page >= 0 && page < BOOTCODE/page_size ){
		if ( isBBT && page<2*ppb ){
			printk("[%s] Updating BBT %d page=%d\n", __FUNCTION__, page/ppb, page%ppb);
		}else{
			printk("[%s] You have no permission to write page %d\n", __FUNCTION__, page);
			return -2;
		}
	}

	if ( page == 0 || page == 1 || page == ppb || page == ppb+1 )
		oob_1stB = BBT_TAG;
	else		
		oob_1stB = 0xFF;

	if (down_interruptible (&sem)) {
		printk("%s : user breaking\n",__FUNCTION__);
		return -ERESTARTSYS;
	}
	
	rtk_writel(0x3e, REG_SRAM_CTL);
	if ( !oob_buf  && this->g_oobbuf ) {
		write_oob_to_TableSram(this->g_oobbuf, 0xFF, 0);
	}else{
		write_oob_to_TableSram((__u8 *)oob_buf, oob_1stB, 1);
	}
	rtk_writel(0x00, REG_SRAM_CTL); 		
	
	rtk_writel( 0x00, REG_DATA_CNT1);
 	rtk_writel( 0x42, REG_DATA_CNT2);
 	
	page_len = page_size >> 9;
	rtk_writel( page_len, REG_PAGE_LEN);
 		
	rtk_writel(0x00, REG_PP_RDY);
	rtk_writel(0x00, REG_PP_CTL1);
	rtk_writel(0x00, REG_PP_CTL0);
 
	rtk_writel(CMD_PG_WRITE_C1, REG_CMD1);
	rtk_writel(CMD_PG_WRITE_C2, REG_CMD2);
	rtk_writel(CMD_PG_WRITE_C3, REG_CMD3);
 	
	rtk_writel(0x00, REG_COL_ADR0);
	rtk_writel(0x00, REG_COL_ADR1);
	rtk_writel( page&0xff, REG_PAGE_ADR0 );
	rtk_writel( page>>8, REG_PAGE_ADR1 );
	rtk_writel( (addr_mode<<5)|((page>>16)&0x1f), REG_PAGE_ADR2 );  
	rtk_writel( ((page>>21)&0x7)<<5, REG_PAGE_ADR3 );
 		
	rtk_writel( 0x20, REG_MULTICHNL_MODE);
	rtk_writel( 0x80, REG_ECC_STOP);
	
	dma_len = page_size >> 9;
	rtk_writel(dma_len, REG_DMA_CTL2);

	dram_sa = ( (uint32_t)data_buf >> 3);
	rtk_writel(dram_sa, REG_DMA_CTL1);	
	rtk_writel( (0x01 << 7)|(0x00 << 3)|auto_trigger_mode, REG_AUTO_TRIG );
	rtk_writel(0x01, REG_DMA_CTL3);
 	while ( rtk_readl(REG_DMA_CTL3) & 0x01 );
	while ( rtk_readl(REG_AUTO_TRIG) & 0x80 );
	
	rtk_writel(0x0d, REG_POLL_STATUS );
	while ( rtk_readl(REG_POLL_STATUS) & 0x01 );

	if ( rtk_readl(REG_DATA) & 0x01 ){
		up (&sem);
		printk("[%s] write is not completed at page %d\n", __FUNCTION__, page);
		return -1;
	}

	up (&sem);

	unsigned int chip_section = (chipnr * this->page_num) >> 5;
	unsigned int section = page >> 5;
	unsigned int index = page & (32-1);
	this->erase_page_flag[chip_section+section] &= ~(1 << index);

	return rc;
}


const char *ptypes[] = {"cmdlinepart", NULL};

static int rtk_nand_profile (void)
{
	int maxchips = 4;	
	struct nand_chip *this = (struct nand_chip *)rtk_mtd->priv;
	this->RBA_PERCENT = RBA_PERCENT;
	if (rtk_nand_scan (rtk_mtd, maxchips) < 0 || rtk_mtd->size == 0){
		printk("%s: Error, cannot do nand_scan(on-board)\n", __FUNCTION__);
		return -ENODEV;
	}

	if ( !(rtk_mtd->oobblock&(0x200-1)) )
		rtk_writel( rtk_mtd->oobblock >> 9, REG_PAGE_LEN);
	else{ 
		printk("Error: pagesize is not 512B Multiple");
		return -1;
	}

	char *ptype;
	int pnum = 0;
	struct mtd_partition *mtd_parts;

#ifdef CONFIG_MTD_CMDLINE_PARTS
	ptype = (char *)ptypes[0];
	pnum = parse_mtd_partitions (rtk_mtd, ptypes, &mtd_parts, 0);
#endif

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

	return 0;
}


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
	strsep(&running, " ");
	strsep(&running, " ");
	revision = strsep(&running, " ");
	date = strsep(&running, " ");
	printk(BANNER " Rev:%s (%s)\n", revision, date);
}


static int __init rtk_nand_init (void)
{
	if ( is_venus_cpu() || is_neptune_cpu() )
		return -1;

	if ( rtk_readl(0xb800000c) & 0x00800000 ){
		display_version();
	}else{ 
		printk(KERN_ERR "Nand Flash Clock is NOT Open. Installing fails.\n");
		return -1;	
	}

	rtk_writel( 0x103, 0xb800036c );
	
	rtk_writel(rtk_readl(0xb800000c)& (~0x00800000), 0xb800000c);
	rtk_writel( 0x02, 0xb8000034 );
	rtk_writel(rtk_readl(0xb800000c)| (0x00800000), 0xb800000c);

	struct nand_chip *this;
	int rc = 0;
	
	rtk_mtd = kmalloc (MTDSIZE, GFP_KERNEL);
	if ( !rtk_mtd ){
		printk("%s: Error, no enough memory for rtk_mtd\n",__FUNCTION__);
		rc = -ENOMEM;
		goto EXIT;
	}
	memset ( (char *)rtk_mtd, 0, MTDSIZE);
	rtk_mtd->priv = this = (struct nand_chip *)(rtk_mtd+1);
	
	rtk_writel( ~(0x1 << 0), REG_CHIP_EN );
	rtk_writel( CMD_RESET, REG_CMD1 );
	rtk_writel( (0x80 | 0x00), REG_CTL );
	while( rtk_readl(REG_CTL) & 0x80 );
	
	rtk_writel(0x00, REG_MULTICHNL_MODE);
		
	rtk_writel( 0x06, REG_T1 );
	rtk_writel( 0x06, REG_T2 );
	rtk_writel( 0x06, REG_T3 );
	rtk_writel( 0x09, REG_DELAY_CNT );

	rtk_writel( 0x00, REG_TABLE_CTL);
	
	this->read_id		= rtk_nand_read_id;
	this->read_ecc_page 	= rtk_read_ecc_page;
	this->read_oob 		= rtk_read_oob;
	this->write_ecc_page	= rtk_write_ecc_page;
	this->write_oob		= rtk_write_oob;
	this->erase_block 	= rtk_erase_block;
	this->suspend		= rtk_nand_suspend;
	this->resume		= rtk_nand_resume;
	this->sync		= NULL;

	if( rtk_nand_profile() < 0 ){
		rc = -1;
		goto EXIT;
	}
	
	page_size = rtk_mtd->oobblock;
	oob_size = rtk_mtd->oobsize;
	ppb = (rtk_mtd->erasesize)/(rtk_mtd->oobblock);
	
	create_proc_read_entry("nandinfo", 0, NULL, rtk_read_proc_nandinfo, NULL);
	

EXIT:
	if (rc < 0) {
		if (rtk_mtd){
			del_mtd_partitions (rtk_mtd);
			if (this->g_databuf)
				kfree(this->g_databuf);
			if(this->g_oobbuf)
				kfree(this->g_oobbuf);
			if (this->erase_page_flag){
				unsigned int flag_size =  (this->numchips * this->page_num) >> 3;
				unsigned int mempage_order = get_order(flag_size);
				free_pages((unsigned long)this->erase_page_flag, mempage_order);
			}
			kfree(rtk_mtd);
		}
		remove_proc_entry("nandinfo", NULL);
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
			unsigned int flag_size =  (this->numchips * this->page_num) >> 3;
			unsigned int mempage_order = get_order(flag_size);
			free_pages((unsigned long)this->erase_page_flag, mempage_order);
		}
		kfree(rtk_mtd);
	}
	remove_proc_entry("nandinfo", NULL);
}

module_init(rtk_nand_init);
module_exit(rtk_nand_exit);
MODULE_AUTHOR("Ken Yu<Ken.Yu@realtek.com>");
MODULE_DESCRIPTION("Realtek NAND Flash Controller Driver");
