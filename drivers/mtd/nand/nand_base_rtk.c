/******************************************************************************
 * $Id: nand_base_rtk.c 257683 2009-08-03 02:47:34Z ken.yu $
 * drivers/mtd/nand/nand_base_rtk.c
 * Overview: Realtek MTD NAND Driver 
 * Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2008-06-10 Ken-Yu   create file
 *    #001 2008-09-10 Ken-Yu   add BBT and BB management
 *    #002 2008-09-28 Ken-Yu   change r/w from single to multiple pages
 *    #003 2008-10-09 Ken-Yu   support single nand with multiple dies
 *    #004 2008-10-23 Ken-Yu   support multiple nands
 *
 *******************************************************************************/
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/bitops.h>

// Ken-Yu 
#include <mtd/mtd-abi.h>
#include <linux/mtd/rtk_nand_reg.h>
#include <linux/mtd/rtk_nand.h>
#include <asm/r4kcache.h>
#include <asm/page.h>
#include <linux/jiffies.h>
#include <asm/mach-venus/platform.h>

#define RTK_DEBUG 0
#if RTK_DEBUG
      #define debug_nand(fmt, arg...)  printk(fmt, ##arg);
#else
      #define debug_nand(fmt, arg...)
#endif

#define RTK_TEST_ERASE_ALL 0
#define RTK_TEST_ERASE_BBT 0
#define RTK_TEST_MARK_BB 0
#define RTK_TEST_WRITE_OOB 0

/* Slow Version will check two pages of block. 
	It will be safe but slow */
#define Nand_Block_Isbad_Slow_Version 0

#define NOTALIGNED(mtd, x) (x & (mtd->oobblock-1)) != 0

#define check_end(mtd, addr, len)					\
do {									\
	if (mtd->size == 0) 						\
		return -ENODEV;						\
	else								\
	if ((addr + len) > mtd->size) {					\
		printk (				\
			"%s: attempt access past end of device\n",	\
			__FUNCTION__);					\
		return -EINVAL;						\
	}								\
} while(0)

#define check_page_align(mtd, addr)					\
do {									\
	if (addr & (mtd->oobblock - 1)) {				\
		printk (				\
			"%s: attempt access non-page-aligned data\n",	\
			__FUNCTION__);					\
		printk (				\
			"%s: mtd->oobblock = 0x%x\n",			\
			__FUNCTION__,mtd->oobblock);			\
		return -EINVAL;						\
	}								\
} while (0)

#define check_block_align(mtd, addr)					\
do {									\
	if (addr & (mtd->erasesize - 1)) {				\
		printk (				\
			"%s: attempt access non-block-aligned data\n",	\
			__FUNCTION__);					\
		return -EINVAL;						\
	}								\
} while (0)

#define check_len_align(mtd,len)					\
do {									\
	if (len & (512 - 1)) {          	 			\
		printk (				\
               	      "%s: attempt access non-512bytes-aligned mem\n",	\
			__FUNCTION__);					\
		return -EINVAL;						\
	}								\
} while (0)


/* Realtek supports nand chip types */
/* STMicorn */
#define NAND01GW3B2B	0x20F1801D	//SLC, 128 MB, 1 dies
#define NAND01GW3B2C	0x20F1001D	//SLC, 128 MB, 1 dies, son of NAND01GW3B2B
#define NAND02GW3B2D	0x20DA1095	//SLC, 256 MB, 1 dies
#define NAND04GW3B2D	0x20DC1095	//SLC, 512 MB, 1 dies
#define NAND08GW3C2B	0x20D314A5	//MLC, 1GB, 1 dies

/* Hynix Nand */
#define HY27UF081G2A	0xADF1801D	//SLC, 128 MB, 1 dies
#define HY27UF082G2A	0xADDA801D	//SLC, 256 MB, 1 dies
#define HY27UF082G2B	0xADDA1095	//SLC, 256 MB, 1 dies
#define HY27UF084G2M	0xADDC8095	//SLC, 512 MB, 1 dies
	/* HY27UT084G2M speed is slower, we have to decrease T1, T2 and T3 */
#define HY27UT084G2M	0xADDC8425	//MLC, 512 MB, 1 dies, BB check at last page, SLOW nand
#define HY27UT084G2A	0xADDC14A5	//MLC, 512 MB, 1 dies
#define HY27UT088G2A	0xADD314A5	//MLC, 1GB, 1 dies, BB check at last page
#define HY27UG088G5M	0xADDC8095	//SLC, 1GB, 2 dies
#define HY27UG088G5B	0xADDC1095	//SLC, 1GB, 2 dies

/* Samsung Nand */
#define K9F1G08U0B	0xECF10095	//SLC, 128 MB, 1 dies
#define K9F2G08U0B	0xECDA1095	//SLC, 256 MB, 1 dies
#define K9G4G08U0A	0xECDC1425	//MLC, 512 MB, 1 dies, BB check at last page
#define K9G8G08U0A	0xECD314A5	//MLC, 1GB, 1 dies, BB check at last page
#define K9G8G08U0M	0xECD31425	//MLC, 1GB, 1 dies, BB check at last page
#define K9K8G08U0A	0xECD35195	//SLC, 1GB, 1 dies
#define K9F8G08U0M	0xECD301A6	//SLC, 1GB, 1 dies, 4K page

/* Toshiba */
#define TC58NVG0S3C	0x98F19095	//128 MB, 1 dies
#define TC58NVG0S3E	0x98D19015	//128 MB, 1 dies
#define TC58NVG1S3C	0x98DA9095	//256 MB, 1 dies
#define TC58NVG1S3E	0x98DA9015	//256 MB, 1 dies


/* RTK Nand Chip ID list */
static device_type_t nand_device[] = 
{
	{"NAND01GW3B2B", NAND01GW3B2B, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0x00, 0x00, 0x00} ,
	{"NAND01GW3B2C", NAND01GW3B2C, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0x00, 0x00, 0x00} ,
	{"NAND02GW3B2D", NAND02GW3B2D, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0x00, 0x00, 0x00} ,
	{"NAND04GW3B2D", NAND04GW3B2D, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x54, 0x00, 0x00, 0x00} ,
	{"NAND08GW3C2B", NAND08GW3C2B, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 0, 0x34, 0x00, 0x00, 0x00} ,
	{"HY27UF081G2A", HY27UF081G2A, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0x00, 0x00, 0x00} ,
	{"HY27UF082G2A", HY27UF082G2A, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x00, 0x00, 0x00, 0x00} ,
	{"HY27UF082G2B", HY27UF082G2B, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0x00, 0x00, 0x00} ,
	{"HY27UT084G2A", HY27UT084G2A, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x24, 0x00, 0x00, 0x00} ,
	{"HY27UT088G2A", HY27UT088G2A, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x34, 0x00, 0x00, 0x00} ,
	{"HY27UF084G2M", HY27UF084G2M, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0xff, 0x00, 0x00, 0x00} ,
	{"HY27UT084G2M", HY27UT084G2M, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0xff, 0x04, 0x04, 0x04} ,
	{"HY27UG088G5M", HY27UG088G5M, 0x40000000, 0x20000000, 2048, 64*2048, 64, 2, 0, 0xff, 0x00, 0x00, 0x00} ,
	{"HY27UG088G5B", HY27UG088G5B, 0x40000000, 0x20000000, 2048, 64*2048, 64, 2, 0, 0x54, 0x00, 0x00, 0x00} ,
	{"K9F1G08U0B", K9F1G08U0B, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0x40, 0x00, 0x00, 0x00} ,
	{"K9F2G08U0B", K9F2G08U0B, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0x00, 0x00, 0x00} ,
	{"K9G4G08U0A", K9G4G08U0A, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x54, 0x00, 0x00, 0x00} ,
	{"K9G8G08U0A", K9G8G08U0A, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x64, 0x00, 0x00, 0x00} ,
	{"K9G8G08U0M", K9G8G08U0M, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x64, 0x00, 0x00, 0x00} ,
	{"K9K8G08U0A", K9K8G08U0A, 0x40000000, 0x40000000, 2048, 64*2048, 64, 1, 0, 0x58, 0x00, 0x00, 0x00} ,
	{"K9F8G08U0M", K9F8G08U0M, 0x40000000, 0x40000000, 4096, 64*4096, 128, 1, 0, 0x64, 0x00, 0x00, 0x00} ,
	{"TC58NVG0S3C", TC58NVG0S3C, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0x00, 0x00, 0x00} ,
	{"TC58NVG0S3E", TC58NVG0S3E, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0x76, 0x00, 0x00, 0x00} ,
	{"TC58NVG1S3C", TC58NVG1S3C, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xff, 0x00, 0x00, 0x00} ,
	{"TC58NVG1S3E", TC58NVG1S3E, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x76, 0x00, 0x00, 0x00} ,	
	{NULL, }
};

/* NAND low-level MTD interface functions */
static int nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
static int nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf, 
			u_char *oob_buf, struct nand_oobinfo *oobsel);
static int nand_read_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *oob_buf);
static int nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);
static int nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char * buf, 
			const u_char *oob_buf, struct nand_oobinfo *oobsel);
static int nand_write_oob (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);
static int nand_erase (struct mtd_info *mtd, struct erase_info *instr);
static void nand_sync (struct mtd_info *mtd);
static int nand_suspend (struct mtd_info *mtd);
static void nand_resume (struct mtd_info *mtd);

/* YAFFS2 */
static int nand_block_isbad (struct mtd_info *mtd, loff_t ofs);
static int nand_block_markbad (struct mtd_info *mtd, loff_t ofs);

/* Global Variables */
int RBA;
static int page_size, oob_size, ppb, isLastPage;
//CMYu:, 20090415
extern platform_info_t platform_info;
char *mp_erase_nand;
int mp_erase_flag = 0;
//CMYu:, 20090512
char *mp_time_para;
int mp_time_para_value = 0;
char *nf_clock;
int nf_clock_value = 0;
//CMYu:, 20090720
char *mcp;


//===========================================================================
/* 
* NF_CKSEL: setup nand flash clock
*/
static void NF_CKSEL(char *PartNum, unsigned int value)
{
	// disable NF clock
	rtk_writel(rtk_readl(0xb800000c)& (~0x00800000), 0xb800000c);
	//nf clock=> 011:54MHZ, 010:72MHZ
	rtk_writel( value, 0xb8000034 );
	// enable NF clock
	rtk_writel(rtk_readl(0xb800000c)| (0x00800000), 0xb800000c);
	printk("[%s] %s is set to nf clock: 0x%x\n", __FUNCTION__, PartNum, value);
}


/*
 * To dump BBT Content
 */
static void dump_BBT(struct mtd_info *mtd)
{
	printk("[%s] Nand BBT Content\n", __FUNCTION__);
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int i;
	int BBs=0;
	for ( i=0; i<RBA; i++){
		if ( i==0 && this->bbt[i].BB_die == BB_DIE_INIT && this->bbt[i].bad_block == BB_INIT ){
			printk("Congratulation!! No BBs in this Nand.\n");
			break;
		}
		if ( this->bbt[i].bad_block != BB_INIT ){
			printk("[%d] (%d, %u, %d, %u)\n", i, this->bbt[i].BB_die, this->bbt[i].bad_block, 
				this->bbt[i].RB_die, this->bbt[i].remap_block);
			BBs++;
		}
	}
	this->BBs = BBs;
}


static void reverse_to_Yaffs2Tags(__u8 *r_oobbuf)
{
	int k;
	for ( k=0; k<4; k++ ){
		r_oobbuf[k]  = r_oobbuf[1+k];
	}

	for ( k=0; k<4; k++ ){
		r_oobbuf[4+k] = r_oobbuf[16+k];
		r_oobbuf[8+k] = r_oobbuf[32+k];
		r_oobbuf[12+k] = r_oobbuf[48+k];
	}
}


/*
rtk_block_isbad: 
This funciton checks BBs according to realtek nand flash controller data layout, 
not normal data layout of nand flash datasheet.
It uesd to check BB status of realtek, not shipping.
*/
 static int rtk_block_isbad(struct mtd_info *mtd, u16 chipnr, loff_t ofs)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned int page, block, page_offset;
	unsigned char block_status_p1;
#if Nand_Block_Isbad_Slow_Version
	unsigned char block_status_p2;
#endif

	unsigned char buf[oob_size] __attribute__((__aligned__(4)));
	
	/* Shift to get page */
	page = ((int) ofs) >> this->page_shift;
	page_offset = page & (ppb-1);
	block = page/ppb;

	//debug_nand("rtk_block_isbad: nand_type_id=0x%x, block=%d, page=%d\n",
		//		nand_type_id, block, page);

	if ( isLastPage ){
		//last page of this block
		page = block*ppb + (ppb-1);	
		if ( this->read_oob (mtd, chipnr, page, oob_size, buf) ){
			printk ("%s: read_oob page=%d failed\n", __FUNCTION__, page);
			return 1;
		}
		block_status_p1 = buf[0];
#if Nand_Block_Isbad_Slow_Version
		//(Last-2)th page of this block
		page = block*ppb + (ppb-2);	
		//if ( this->read_oob (mtd, page, 0, mtd->oobsize, buf) ){
		if ( this->read_oob (mtd, chipnr, page, oob_size, buf) ){
			printk ("%s: read_oob page=%d failed\n", __FUNCTION__, page);
			return 1;
		}
		block_status_p2 = buf[0];
		debug_nand("[1]block_status_p1=0x%x, block_status_p2=0x%x\n", block_status_p1, block_status_p2);
#endif		
	}else{	
		//first page of this block
		if ( this->read_oob (mtd, chipnr, page, oob_size, buf) ){
			printk ("%s: read_oob page=%d failed\n", __FUNCTION__, page);
			return 1;
		}
		block_status_p1 = buf[0];
#if Nand_Block_Isbad_Slow_Version
		//2nd page of this block
		if ( this->read_oob (mtd, chipnr, page+1, oob_size, buf) ){
			printk ("%s: read_oob page+1=%d failed\n", __FUNCTION__, page+1);
			return 1;
		}
		block_status_p2 = buf[0];
		debug_nand("[2]block_status_p1=0x%x, block_status_p2=0x%x\n", block_status_p1, block_status_p2);
#endif
	}
#if Nand_Block_Isbad_Slow_Version
	if ( (block_status_p1 != 0xff) && (block_status_p2 != 0xff) ){
#else
	if ( block_status_p1 != 0xff){
#endif		
		printk ("WARNING: Die %d: block=%d is bad, block_status_p1=0x%x\n", chipnr, block, block_status_p1);
		return -1;
	}
	
	return 0;
}


/*
 * For YAFFS2: I treat the nand flash that does not have BB from Yaffs view
 */
 static int nand_block_isbad (struct mtd_info *mtd, loff_t ofs)
{
	//printk("===[%s][%d]===\n", __FUNCTION__, __LINE__);
	return 0;
}


static int nand_block_markbad (struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned int page, block, page_offset;
	int i;
	int rc = 0;
	unsigned char buf[oob_size] __attribute__((__aligned__(4)));
	int chipnr, chipnr_remap;
	
	/* Shift to get page */
	page = ((int) ofs) >> this->page_shift;
	this->active_chip = chipnr = chipnr_remap = (int)(ofs >> this->chip_shift);
	page_offset = page & (ppb-1);
	block = page/ppb;

	/* Select the NAND device */
	this->select_chip(mtd, chipnr);
	
	//Ken(20080928): search BBT to check if the block is bad
	//if the block is bad, remapping the block and page
	for ( i=0; i<RBA; i++){
		if ( this->bbt[i].bad_block != BB_INIT ){
			if ( chipnr == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
				block = this->bbt[i].remap_block;
				if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
					this->active_chip = chipnr_remap = this->bbt[i].RB_die;
					this->select_chip(mtd, chipnr_remap);
				}					
			}
		}else
			break;
	}
	page = block*ppb + page_offset;
				
	debug_nand("[%s] write_oob(%s,%d,%d,%d,buf)\n",
						__FUNCTION__, mtd->name, page, 0, mtd->oobsize);

	buf[0]=0x00;
	rc = this->write_oob (mtd, this->active_chip, page, oob_size, buf);
	if (rc) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: write_oob failed\n", __FUNCTION__);
			return -1;
	}
	return 0;
}


/**
 * nand_read_oob - [MTD Interface] NAND read out-of-band
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of oob bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @oob_buf:	the databuffer to put oob, YAFFS pass the buffer size = mtd->oobsize
 *
 * NAND read out-of-band data from the spare area
 */
static int nand_read_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, 
			u_char *oob_buf)
{
	struct nand_chip *this = mtd->priv;
	unsigned int page, realpage;
	int oob_len = 0, thislen;
	int rc=0;
	int i, old_page, page_offset, block;
	int chipnr, chipnr_remap;
	
	//debug_nand("nand_read_oob: from = 0x%08x, len = %d, retlen=%d\n", 
			//			(unsigned int) from, (int) len, (int )*retlen);

	/* Do not allow read past end of device */
	if ((from + len) > mtd->size) {
		printk ("nand_read_oob: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* reject reads, which are not page aligned */	
	if (NOTALIGNED (mtd, from)) {
		printk (KERN_NOTICE "nand_read_oob: Attempt to read not page aligned data\n");
		return -EINVAL;
	}

	/* Shift to get page */
	realpage = (int)(from >> this->page_shift);
	this->active_chip = chipnr = chipnr_remap = (int)(from >> this->chip_shift);
	old_page = page = realpage & this->pagemask;	//page offset in each Die
	page_offset = page & (ppb-1);
	block = page/ppb;	//block offset in each Die
		
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Grab the lock and see if the device is available */
	//nand_get_device (this, mtd , FL_READING);
	
	/* Initialize return length value */
	if ( retlen ) 
		*retlen = 0;
	thislen = oob_size;

	/* Loop until all len has read */
	while (oob_len < len) {
		/* length to read in this page */
		if (thislen > (len - oob_len)) 
			thislen = (len - oob_len);

		/* Read OOB */
		//debug_nand ("%s: mtd->name=%s, page=%d, thislen=%d, &oob_buf[oob_len]=%p)\n", 
			//__FUNCTION__, mtd->name, page, thislen,&oob_buf[oob_len]);
		//Ken(20080928): search BBT to check if the block is bad
		//if the block is bad, remapping the block and page
		for ( i=0; i<RBA; i++){
			if ( this->bbt[i].bad_block != BB_INIT ){
				if ( this->active_chip == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
					block = this->bbt[i].remap_block;
					if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
						this->active_chip = chipnr_remap = this->bbt[i].RB_die;
						this->select_chip(mtd, chipnr_remap);
					}					
				}
			}else
				break;
		}
		page = block*ppb + page_offset;
		
		//printk("[%s]block=%d, page=%d, oob_buf=%p\n", __FUNCTION__, block, page, oob_buf);	
		
		rc = this->read_oob (mtd, this->active_chip, page, thislen, &oob_buf[oob_len]);
		/* begin: handle BB occurence when reading page */
		if (rc < 0) {
			if (rc == -1){
				printk ("%s: read_oob: Un-correctable HW ECC\n", __FUNCTION__);
				//update BBT
				for( i=0; i<RBA; i++){
					if ( this->bbt[i].bad_block == BB_INIT && this->bbt[i].remap_block != RB_INIT){
						if ( chipnr != chipnr_remap)	//remap block is bad
							this->bbt[i].BB_die = chipnr_remap;
						else
							this->bbt[i].BB_die = chipnr;
						this->bbt[i].bad_block = page/ppb;
						break;
					}
				}

				dump_BBT(mtd);
				
				//update BBT B0 and BBT B1
				if ( rtk_update_bbt (mtd, this->g_databuf, this->g_oobbuf, this->bbt) ){
					printk("[%s] rtk_update_bbt() fails\n", __FUNCTION__);
					return -1;
				}
				
				//mark the block as BB
				this->g_oobbuf[0] = 0x00;	//mark the page is bad
				if ( isLastPage ){
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-1, oob_size, this->g_oobbuf);
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-2, oob_size, this->g_oobbuf);
				}else{
					this->write_oob(mtd, this->active_chip, block*ppb, oob_size, this->g_oobbuf);
					this->write_oob(mtd, this->active_chip, block*ppb+1, oob_size, this->g_oobbuf);
				}
				printk("rtk_read_oob: Un-correctable HW ECC Error at page=%d\n", page);				
			}else{
				printk ("%s: rtk_read_oob: semphore failed\n", __FUNCTION__);
				return -1;
			}
		}
		/* end: handle BB occurence when writing page */
		
		/* Update read bytes count */
		oob_len += thislen;

		/* Increment page address */
		old_page++;
		page_offset = old_page & (ppb-1);
		 /* Check, if we cross a chip boundary */
		if ( oob_len<len && !(old_page & this->pagemask)) {
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
		}
		block = old_page/ppb;
	}

	/* Deselect and wake up anyone waiting on the device */
	//nand_release_device(mtd);

	/* Return happy */
	//read len must equal to return len, otherwise mtd will pop "end_request: I/O error"
	if ( retlen ){
		if ( oob_len == len )
			*retlen = oob_len;
		else{
			printk("[%s] error: oob_len %d != len %d\n", __FUNCTION__, oob_len, len);
			return -1;
		}	
	}

	return 0;
}


/**
 * nand_write_oob - [MTD Interface] NAND write out-of-band
 * @mtd:	MTD device structure
 * @to:	offset to read to
 * @len:	number of oob bytes to write
 * @retlen:	pointer to variable to store the number of read bytes
 * @oob_buf:	the databuffer to put oob
 *
 * NAND write out-of-band data to the spare area
 */
static int nand_write_oob (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, 
			const u_char * oob_buf)
{
	struct nand_chip *this = mtd->priv;
	unsigned int page, realpage;
	int oob_len=0, thislen;
	int rc=0; 
	int i, old_page, page_offset, block;
	int chipnr, chipnr_remap, err_chipnr = 0, err_chipnr_remap = 1;
	
	//debug_nand ("nand_write_oob: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	//printk("[%s]: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) len);
	
	/* Do not allow writes past end of device */
	if ((to + len) > mtd->size) {
		printk ("nand_write_oob: Attempt write beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* reject writes, which are not page aligned */	
	if (NOTALIGNED (mtd, to)) {
		printk (KERN_NOTICE "nand_write_oob: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	/* Shift to get page */
	realpage = (int)(to >> this->page_shift);
	this->active_chip = chipnr = chipnr_remap = (int)(to >> this->chip_shift);
	old_page = page = realpage & this->pagemask;	//page offset in each Die
	page_offset = page & (ppb-1);
	block = page/ppb;	//block offset in each Die
		
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);
		
	/* Grab the lock and see if the device is available */
	//nand_get_device (this, mtd , FL_WRITING);

	/* Initialize return length value */
	if ( retlen )
		*retlen = 0;
	thislen = oob_size;
	
	/* Loop until all len has written */
	while (oob_len < len) {
		/* length to write in this page */
		if (thislen> (len - oob_len)) 
			thislen = (len - oob_len);
		//printk("[%s](1)block=%d, page=%d, oob_buf=%p\n", __FUNCTION__, block, page, oob_buf);
		//Ken(20080928): search BBT to check if the block is bad
		//if the block is bad, remapping the block and page
		for ( i=0; i<RBA; i++){
			if ( this->bbt[i].bad_block != BB_INIT ){
				if ( this->active_chip == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
					block = this->bbt[i].remap_block;
					if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
						this->active_chip = chipnr_remap = this->bbt[i].RB_die;
						this->select_chip(mtd, chipnr_remap);
					}					
				}
			}else
				break;
		}
		page = block*ppb + page_offset;
		//printk("[%s](1)block=%d, page=%d, oob_buf=%p\n", __FUNCTION__, block, page, oob_buf);
	
		rc = this->write_oob (mtd, this->active_chip, page, thislen, &oob_buf[oob_len]);
		/* begin: handle BB occurence when writing page */
		if (rc < 0) {
			if ( rc == -1){
				printk ("%s: write_ecc_page:  write failed\n", __FUNCTION__);
				int block_remap = 0x12345678;
				//update BBT
				for( i=0; i<RBA; i++){
					if ( this->bbt[i].bad_block == BB_INIT && this->bbt[i].remap_block != RB_INIT){
						if ( chipnr != chipnr_remap)	//remap block is bad
							err_chipnr = chipnr_remap;
						else
							err_chipnr = chipnr;
						this->bbt[i].BB_die = err_chipnr;
						this->bbt[i].bad_block = page/ppb;
						err_chipnr_remap = this->bbt[i].RB_die;
						block_remap = this->bbt[i].remap_block;
						break;
					}
				}
			
				//check if we 	have free remap block
				if ( block_remap == 0x12345678 ){
					printk("[%s] RBA do not have free remap block\n", __FUNCTION__);
					return -1;
				}
			
				dump_BBT(mtd);
				
				//update BBT B0 and BBT B1
				if ( rtk_update_bbt (mtd, this->g_databuf, this->g_oobbuf, this->bbt) ){
					printk("[%s] rtk_update_bbt() fails\n", __FUNCTION__);
					return -1;
				}
				
				//backup the page data before the write failure page
				int backup_offset = page&(ppb-1);
				this->select_chip(mtd, err_chipnr_remap);
				this->erase_block (mtd, err_chipnr_remap, block_remap*ppb);
				printk("[%s] Start to Backup old_page from %d to %d\n", __FUNCTION__, block*ppb, block*ppb+backup_offset-1);
				for ( i=0; i<backup_offset; i++){
					//printk("[%s]old_page = %d, new_page=%d\n", __FUNCTION__, block*ppb+i, block_remap*ppb+i);
					//change to where bad page of the Die is
					if ( err_chipnr != err_chipnr_remap ){
						this->active_chip = err_chipnr;
						this->select_chip(mtd, err_chipnr);
					}
					this->read_ecc_page(mtd, this->active_chip, block*ppb+i, this->g_databuf, this->g_oobbuf);
					//shift to be original yaffs tags
					if ( this->g_oobbuf )
						reverse_to_Yaffs2Tags(this->g_oobbuf);
					//change to where remap page of the Die is
					if ( err_chipnr != err_chipnr_remap ){
						this->active_chip = err_chipnr_remap;
						this->select_chip(mtd, err_chipnr_remap);
					}
					this->write_ecc_page(mtd, this->active_chip, block_remap*ppb+i, this->g_databuf, this->g_oobbuf, 0);
				}
				//write the failure page to remap block
				this->write_oob (mtd, this->active_chip, block_remap*ppb+backup_offset, oob_size, &oob_buf[oob_len]);
				printk("[%s] write failure page = %d to %d\n", __FUNCTION__, page, block_remap*ppb+backup_offset);
				
				//mark the block as BB
				if ( err_chipnr != err_chipnr_remap ){
					this->active_chip = err_chipnr;
					this->select_chip(mtd, err_chipnr);
				}

				this->g_oobbuf[0] = 0x00;
				if ( isLastPage ){	//mark the block as bad
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-1, oob_size, this->g_oobbuf);	//the last page
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-2, oob_size, this->g_oobbuf);	//the (last-1) page
				}else{		//other nand: mark the block as bad
					this->write_oob(mtd, this->active_chip, block*ppb, oob_size, this->g_oobbuf);	//first page
					this->write_oob(mtd, this->active_chip, block*ppb+1, oob_size, this->g_oobbuf);	//2nd page
				}
			}else{
				printk ("%s: write_ecc_page:  semphore failed\n", __FUNCTION__);
				return -1;
			}	
		}
		/* end: handle BB occurence when writing page */
		
		/* Update write bytes count */
		oob_len += thislen;

		/* Increment page address */
		old_page++;
		page_offset = old_page & (ppb-1);
		 /* Check, if we cross a chip boundary */
		if ( oob_len<len && !(old_page & this->pagemask)) {
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
		}
		block = old_page/ppb;
	}

	/* Deselect and wake up anyone waiting on the device */
	//nand_release_device(mtd);

	/* Return happy */
	//write len must equal to return len, otherwise mtd will pop "end_request: I/O error"
	if ( retlen ){
		if ( oob_len == len )
			*retlen = oob_len;
		else{
			printk("[%s] error: oob_len %d != len %d\n", __FUNCTION__, oob_len, len);
			return -1;
		}	
	}
	
	return rc;
}


/**
 * nand_read - [MTD Interface] MTD compability function for nand_read_ecc
 * @mtd:        MTD device structure
 * @from:       offset to read from
 * @len:        number of bytes to read
 * @retlen:     pointer to variable to store the number of read bytes
 * @buf:        the databuffer to put data
 *
* This function simply calls nand_read_ecc with oob buffer and oobsel = NULL
*/
static int nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	return nand_read_ecc (mtd, from, len, retlen, buf, NULL, NULL);
}                          


/**
 * nand_read_ecc - [MTD Interface] Read data with ECC
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of data bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 * @oob_buf:	filesystem supplied oob data buffer
 * @oobsel:	oob selection structure
 *
 * NAND read with ECC
 */
static int nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, u_char *buf, u_char *oob_buf, struct nand_oobinfo *oobsel)
{
	struct nand_chip *this = mtd->priv;
	unsigned int page, realpage;
	int data_len, oob_len;
	int rc;
	int i, old_page, page_offset, block;
	int chipnr, chipnr_remap;

	//debug_nand ("nand_read_ecc: from = 0x%08x, len = %i, buf=%p\n",
		//				 (unsigned int) from, (int) len, buf);

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		printk ("nand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* reject writes, which are not page aligned */	
	if (NOTALIGNED (mtd, from) || NOTALIGNED(mtd, len)) {
		printk (KERN_NOTICE "nand_read_ecc: Attempt to read not page aligned data\n");
		return -EINVAL;
	}

	/* Shift to get page */
	realpage = (int)(from >> this->page_shift);
	this->active_chip = chipnr = chipnr_remap = (int)(from >> this->chip_shift);
	old_page = page = realpage & this->pagemask;	//page offset in each Die
	page_offset = page & (ppb-1);
	block = page/ppb;	//block offset in each Die

#if 0
	printk("[%s]: from = 0x%08x, realpage=%d, old_page=%d, org_block=%d, block=%d, chipnr=%d\n", 
			__FUNCTION__, (unsigned int) from, realpage, old_page, realpage/ppb, block, chipnr);
#endif
		
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);
	
	/* Grab the lock and see if the device is available */
	//nand_get_device (this, mtd ,FL_READING);

	/* Initialize return length value */
	if ( retlen )
		*retlen = 0;
	
	data_len = oob_len = 0;
	
	/* Loop until all len has read */
	while (data_len < len) {
		//printk("[%s](1)block=%d, page=%d, buf=%p, oob_buf=%p\n", __FUNCTION__, block, page, buf, oob_buf);
		//Ken(20080908): search BBT to check if the block is bad
		//if the block is bad, remapping the block and page
		for ( i=0; i<RBA; i++){
			if ( this->bbt[i].bad_block != BB_INIT ){
				if ( this->active_chip == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
					block = this->bbt[i].remap_block;
					if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
						this->active_chip = chipnr_remap = this->bbt[i].RB_die;
						this->select_chip(mtd, chipnr_remap);
					}
					//printk("(%d, %u, %d, %u)\n", this->bbt[i].BB_die, this->bbt[i].bad_block, 
						//	this->bbt[i].RB_die, this->bbt[i].remap_block);	
				}
			}else
				break;
		}
		page = block*ppb + page_offset;
		//printk("[%s](2)block=%d, page=%d, buf=%p, oob_buf=%p\n", __FUNCTION__, block, page, buf, oob_buf);
		
		rc = this->read_ecc_page (mtd, this->active_chip, page, &buf[data_len], &oob_buf[oob_len]);
		/* begin: handle BB occurence when reading page */
		if (rc < 0) {
			if (rc == -1){
				printk ("%s: read_ecc_page: Un-correctable HW ECC\n", __FUNCTION__);
				//update BBT
				for( i=0; i<RBA; i++){
					if ( this->bbt[i].bad_block == BB_INIT && this->bbt[i].remap_block != RB_INIT){
						if ( chipnr != chipnr_remap)	//remap block is bad
							this->bbt[i].BB_die = chipnr_remap;
						else
							this->bbt[i].BB_die = chipnr;
						this->bbt[i].bad_block = page/ppb;
						break;
					}
				}
				
				dump_BBT(mtd);
				
				//update BBT B0 and BBT B1
				if ( rtk_update_bbt (mtd, this->g_databuf, this->g_oobbuf, this->bbt) ){
					printk("[%s] rtk_update_bbt() fails\n", __FUNCTION__);
					return -1;
				}
				
				//mark the block as BB
				this->g_oobbuf[0] = 0x00;	//mark the page is bad
				if ( isLastPage){
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-1, oob_size, this->g_oobbuf);
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-2, oob_size, this->g_oobbuf);
				}else{
					this->write_oob(mtd, this->active_chip, block*ppb, oob_size, this->g_oobbuf);
					this->write_oob(mtd, this->active_chip, block*ppb+1, oob_size, this->g_oobbuf);
				}
				printk("rtk_read_ecc_page: Un-correctable HW ECC Error at page=%d\n", page);
			}else{
				printk ("%s: read_ecc_page:  semphore failed\n", __FUNCTION__);
				return -1;
			}
		}
		/* end: handle BB occurence when writing page */
		
		/* Update write bytes count */
		data_len += page_size;
		oob_len += oob_size;
		
		/* Increment page address */
		old_page++;
		page_offset = old_page & (ppb-1);
		 /* Check, if we cross a chip boundary */
		if ( data_len<len && !(old_page & this->pagemask)) {
			//printk("[%s] old_page=%d, chipnr=%d\n", __FUNCTION__, old_page, chipnr);
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
			//printk("[%s] old_page=%d, chipnr=%d\n", __FUNCTION__, old_page, chipnr);
		}
		block = old_page/ppb;
	}
	//read len must equal to return len, otherwise mtd will pop "end_request: I/O error"
	if ( retlen ){
		if ( data_len == len )
			*retlen = data_len;
		else{
			printk("[%s] error: data_len %d != len %d\n", __FUNCTION__, data_len, len);
			return -1;
		}	
	}

	return 0;
}


/**
 * nand_write - [MTD Interface] MTD compability function for nand_write_ecc
 * @mtd:        MTD device structure
 * @to:       offset to write from
 * @len:        number of data bytes to write
 * @retlen:     pointer to variable to store the number of write bytes
 * @buf:        the databuffer to put data
 *
* This function simply calls nand_write_ecc with oob buffer and oobsel = NULL
*/
static int nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	return (nand_write_ecc (mtd, to, len, retlen, buf, NULL, NULL));
}

/**
 * nand_write_ecc - [MTD Interface] NAND write with ECC
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of data bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
 * @oob_buf:	filesystem supplied oob data buffer
 * @oobsel:	oob selection structure
 *
 * NAND write with ECC
 */
static int nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, 
			const u_char * buf, const u_char *oob_buf, struct nand_oobinfo *oobsel)
{
	struct nand_chip *this = mtd->priv;
	unsigned int page, realpage;
	int data_len, oob_len;
	int rc;
	int i, old_page, page_offset, block;
	int chipnr, chipnr_remap, err_chipnr = 0, err_chipnr_remap = 1;
	
	//printk("[%s]: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) len);
	
	/* Do not allow write past end of device */
	if ((to + len) > mtd->size) {
		printk ("nand_write_ecc: Attempt write beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* reject writes, which are not page aligned */	
	if (NOTALIGNED (mtd, to) || NOTALIGNED(mtd, len)) {
		printk (KERN_NOTICE "nand_write_ecc: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	/* Shift to get page */
	realpage = (int)(to >> this->page_shift);
	this->active_chip = chipnr = chipnr_remap = (int)(to >> this->chip_shift);
	old_page = page = realpage & this->pagemask;	//page offset in each Die
	page_offset = page & (ppb-1);
	block = page/ppb;	//block offset in each Die

#if 0
	printk("[%s]: to = 0x%08x, realpage=%d, old_page=%d, org_block=%d, block=%d, chipnr=%d\n", 
			__FUNCTION__, (unsigned int) to, realpage, old_page, realpage/ppb, block, chipnr);
#endif
		
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);
	
	/* Grab the lock and see if the device is available */
	//nand_get_device (this, mtd ,FL_READING);

	/* Initialize return length value */
	if ( retlen )
		*retlen = 0;
	
	data_len = oob_len = 0;

	/* Loop until all len has written */
	while ( data_len < len) {
		//printk("[%s](1)block=%d, page=%d, buf=%p, oob_buf=%p\n", __FUNCTION__, block, page, buf, oob_buf);
		//Ken(20080908): search BBT to check if the block is bad
		//if the block is bad, remapping the block and page
		for ( i=0; i<RBA; i++){
			if ( this->bbt[i].bad_block != BB_INIT ){
				if ( this->active_chip == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
					block = this->bbt[i].remap_block;
					if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
						this->active_chip = chipnr_remap = this->bbt[i].RB_die;
						this->select_chip(mtd, chipnr_remap);
					}
					//printk("(%d, %u, %d, %u)\n", this->bbt[i].BB_die, this->bbt[i].bad_block, 
						//	this->bbt[i].RB_die, this->bbt[i].remap_block);
				}
			}else
				break;
		}
		page = block*ppb + page_offset;
		//printk("[%s](2)block=%d, page=%d, buf=%p, oob_buf=%p\n", __FUNCTION__, block, page, buf, oob_buf);
		
		rc = this->write_ecc_page (mtd, this->active_chip, page, &buf[data_len], &oob_buf[oob_len], 0);
		/* begin: handle BB occurence when writing page */
		if (rc < 0) {
			if ( rc == -1){
				printk ("%s: write_ecc_page:  write failed\n", __FUNCTION__);
				int block_remap = 0x12345678;
				/* update BBT */
				for( i=0; i<RBA; i++){
					if ( this->bbt[i].bad_block == BB_INIT && this->bbt[i].remap_block != RB_INIT){
						if ( chipnr != chipnr_remap)	//remap block is bad
							err_chipnr = chipnr_remap;
						else
							err_chipnr = chipnr;
						this->bbt[i].BB_die = err_chipnr;
						this->bbt[i].bad_block = page/ppb;
						err_chipnr_remap = this->bbt[i].RB_die;
						block_remap = this->bbt[i].remap_block;
						break;
					}
				}
			
				//check if we 	have free remap block
				if ( block_remap == 0x12345678 ){
					printk("[%s] RBA do not have free remap block\n", __FUNCTION__);
					return -1;
				}
			
				dump_BBT(mtd);
				
				//update BBT B0 and BBT B1
				if ( rtk_update_bbt (mtd, this->g_databuf, this->g_oobbuf, this->bbt) ){
					printk("[%s] rtk_update_bbt() fails\n", __FUNCTION__);
					return -1;
				}
//=====================================================
#if 0		//for BBT test
				if ( page == 8200 ){	//at block=64
					int j;
					__u8 *data_buf1 = kmalloc( page_size, GFP_KERNEL );
					__u8 *temp_buf1 = kmalloc( oob_size, GFP_KERNEL );							
					this->read_ecc_page(mtd, 0, data_buf1, temp_buf1);
					for ( j=0; j < oob_size; j++){
						if ( !(j % 8) )
							printk("[%p]:", &temp_buf1[j]);
							printk("[0x%x]", temp_buf1[j]);
						if ( (j % 8) == 7  )
							printk("\n");
					}
					kfree(data_buf1);
					kfree(temp_buf1);			
				}
#endif			
//=====================================================
				/* backup the page data before the write failure page */
				int backup_offset = page&(ppb-1);
				this->select_chip(mtd, err_chipnr_remap);
				this->erase_block (mtd, err_chipnr_remap, block_remap*ppb);
				printk("[%s] Start to Backup old_page from %d to %d\n", __FUNCTION__, block*ppb, block*ppb+backup_offset-1);
				for ( i=0; i<backup_offset; i++){
					//printk("[%s]old_page = %d, new_page=%d\n", __FUNCTION__, block*ppb+i, block_remap*ppb+i);
					//change to where bad page of the Die is
					if ( err_chipnr != err_chipnr_remap ){
						this->active_chip = err_chipnr;
						this->select_chip(mtd, err_chipnr);
					}
					this->read_ecc_page(mtd, this->active_chip, block*ppb+i, this->g_databuf, this->g_oobbuf);
					//shift to be original yaffs tags
					if ( this->g_oobbuf )
						reverse_to_Yaffs2Tags(this->g_oobbuf);
					//change to where remap page of the Die is
					if ( err_chipnr != err_chipnr_remap ){
						this->active_chip = err_chipnr_remap;
						this->select_chip(mtd, err_chipnr_remap);
					}
					this->write_ecc_page(mtd, this->active_chip, block_remap*ppb+i, this->g_databuf, this->g_oobbuf, 0);
				}
				//write the failure page to remap block
				this->write_ecc_page (mtd, this->active_chip, block_remap*ppb+backup_offset, &buf[data_len], &oob_buf[oob_len], 0);
				printk("[%s] write failure page = %d to %d\n", __FUNCTION__, page, block_remap*ppb+backup_offset);
			
				/* mark the block as BB */
				if ( err_chipnr != err_chipnr_remap ){
					this->active_chip = err_chipnr;
					this->select_chip(mtd, err_chipnr);
				}

				this->g_oobbuf[0] = 0x00;
				if ( isLastPage ){	//mark the block as bad
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-1, oob_size, this->g_oobbuf);	//the last page
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-2, oob_size, this->g_oobbuf);	//the (last-1) page
				}else{		//other nand: mark the block as bad
					//printk("mark block=%d, page=%d\n", block, block*ppb);
					this->write_oob(mtd, this->active_chip, block*ppb, oob_size, this->g_oobbuf);	//first page
					this->write_oob(mtd, this->active_chip, block*ppb+1, oob_size, this->g_oobbuf);	//2nd page
				}
			}else{
				printk ("%s: write_ecc_page:  rc=%d\n", __FUNCTION__, rc);
				return -1;
			}	
		}
		/* end: handle BB occurence when writing page */
		
		/* Update write bytes count */
		data_len += page_size;
		oob_len += oob_size;
		
		/* Increment page address */
		old_page++;
		page_offset = old_page & (ppb-1);
		 /* Check, if we cross a die boundary */
		if ( data_len<len && !(old_page & this->pagemask)) {
			//printk("[%s] old_page=%d, chipnr=%d\n", __FUNCTION__, old_page, chipnr);
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
			//printk("[%s] old_page=%d, chipnr=%d\n", __FUNCTION__, old_page, chipnr);
		}
		block = old_page/ppb;
	}
	//write len must equal to return len, otherwise mtd will pop "end_request: I/O error"
	if ( retlen ){
		if ( data_len == len )
			*retlen = data_len;
		else{
			printk("[%s] error: data_len %d != len %d\n", __FUNCTION__, data_len, len);
			return -1;
		}	
	}

	return 0;
}


/**
 * nand_erase - [MTD Interface] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 *
 * Erase one ore more blocks
 */
static int nand_erase (struct mtd_info *mtd, struct erase_info *instr)
{
	return nand_erase_nand (mtd, instr, 0);
}


/**
 * nand_erase - [MTD Interface] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 * @allowbbt:	allow erasing the bbt area
 *
 * Erase one ore more blocks
 */
int nand_erase_nand (struct mtd_info *mtd, struct erase_info *instr, int allowbbt)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	u_int32_t addr = instr->addr;
	u_int32_t len = instr->len;
	int page, chipnr;
	int i, old_page, block;
	int elen = 0;
	int rc = 0;
	int realpage, chipnr_remap;
	//printk ("%s: start addr= 0x%08x, len = %i\n", __FUNCTION__, addr, len);

	/* sanity check */
	check_end (mtd, addr, len);
	check_block_align (mtd, addr);

	 instr->fail_addr = 0xffffffff;
	 /* Grab the lock and see if the device is available */
	//nand_get_device (this, mtd, FL_ERASING);

	/* Shift to get first page */
	realpage = ((int) addr) >> this->page_shift;
	this->active_chip = chipnr = chipnr_remap = ((int) addr) >> this->chip_shift;
	old_page = page = realpage & this->pagemask;	//page offset in each Die
	block = page/ppb;	//block offset in each Die
	
#if 0
	printk("[%s]: realpage=%d, old_page=%d, org_block=%d, block=%d, chipnr=%d\n", 
			__FUNCTION__, realpage, old_page, realpage/ppb, block, chipnr);
#endif
	
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);
	
	/* Loop until blocks are erased */
	instr->state = MTD_ERASING;
	while (elen < len) {
		//printk("[%s]elen=%d, len=%d, old_page=%d, block=%d\n", 
				//__FUNCTION__, elen, len, old_page, block);
		//if the block is bad in BBT, we erase the remap block !
		for ( i=0; i<RBA; i++){
			if ( this->bbt[i].bad_block != BB_INIT ){
				if ( this->active_chip == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
					block = this->bbt[i].remap_block;
					if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
						this->active_chip = chipnr_remap = this->bbt[i].RB_die;
						this->select_chip(mtd, chipnr_remap);
					}
					//printk("(%d, %u, %d, %u)\n", this->bbt[i].BB_die, this->bbt[i].bad_block, 
							//this->bbt[i].RB_die, this->bbt[i].remap_block);
				}
			}else
				break;
		}
		page = block*ppb;
		
		//printk("%s: block=%d, old_page=%d, page=%d, elen=%d, len=%d\n", 
				//__FUNCTION__, block, old_page, page, elen, len);

		/*		
		if ( page == old_page )
			rc = this->erase_block (mtd, chipnr, old_page);	//normal erase block
		else
			rc = this->erase_block (mtd, chipnr_remap, page);	//erase remap block
		*/
		
		rc = this->erase_block (mtd, this->active_chip, page);
			
		if (rc) {
			printk ("%s: block erase failed at page address=0x%08x\n", __FUNCTION__, addr);
			instr->fail_addr = (page << this->page_shift);	
			/* update BBT */
			int block_remap = 0x12345678;
			for( i=0; i<RBA; i++){
				if ( this->bbt[i].bad_block == BB_INIT && this->bbt[i].remap_block != RB_INIT){
						if ( chipnr != chipnr_remap)	//remap block is bad
							this->bbt[i].BB_die = chipnr_remap;
						else
							this->bbt[i].BB_die = chipnr;
					//this->bbt[i].BB_die = this->active_chip;
					this->bbt[i].bad_block = page/ppb;
					block_remap = this->bbt[i].remap_block;
					break;
				}
			}

			//check if we 	have free remap block
			if ( block_remap == 0x12345678 ){
				printk("[%s] RBA do not have free remap block\n", __FUNCTION__);
				return -1;
			}

			dump_BBT(mtd);

			//update BBT B0 and BBT B1
			if ( rtk_update_bbt (mtd, this->g_databuf, this->g_oobbuf, this->bbt) ){
				printk("[%s] rtk_update_bbt() fails\n", __FUNCTION__);
				return -1;
			}
			
			/* mark the block as BB */
			this->g_oobbuf[0] = 0x00;
			if ( isLastPage ){	//mark the block as bad
				this->write_oob(mtd, chipnr, page+ppb-1, oob_size, this->g_oobbuf);	//the last page
				this->write_oob(mtd, chipnr, page+ppb-2, oob_size, this->g_oobbuf);	//the (last-1) page
			}else{		//other nand: mark the block as bad
				this->write_oob(mtd, chipnr, page, oob_size, this->g_oobbuf);	//first page
				this->write_oob(mtd, chipnr, page+1, oob_size, this->g_oobbuf);	//2nd page
			}
		
			//erase the remap block
			this->erase_block(mtd, chipnr_remap, block_remap*ppb);
		}
		
		if ( chipnr != chipnr_remap )
			this->select_chip(mtd, chipnr);
			
		//printk("[%s]block=%d, page=%d\n", __FUNCTION__, block, page);
		
		/* Update erase bytes count */
		elen += mtd->erasesize;

		/* Increment page address */
		old_page += ppb;
		
		 /* Check, if we cross a chip boundary */
		if ( elen<len && !(old_page & this->pagemask)) {
			//printk("[%s] old_page=%d, chipnr=%d\n", __FUNCTION__, old_page, chipnr);
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
			//printk("[%s] old_page=%d, chipnr=%d\n", __FUNCTION__, old_page, chipnr);
		}

		block = old_page/ppb;
	}
	instr->state = MTD_ERASE_DONE;

	/* Deselect and wake up anyone waiting on the device */
	//nand_release_device(mtd);

	return rc;
}


/*
 * NAND sync: no special job need to do
 */
static void nand_sync (struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	this->state = FL_READY;
}

/*
 * NAND suspend:
 */
static int nand_suspend (struct mtd_info *mtd)
{
//	printk("[%s] is implemented .\n", __FUNCTION__);
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	this->suspend(mtd);
	return 0;
}

/*
 * NAND resume:
 */
static void nand_resume (struct mtd_info *mtd)
{
//	printk("[%s] is implemented.\n", __FUNCTION__);
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	this->resume(mtd);
}


/**
 * nand_select_chip - [DEFAULT] control CE line
 * @mtd:	MTD device structure
 * @chip:	chipnumber to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
static void nand_select_chip(struct mtd_info *mtd, int chip)
{
	switch(chip) {
		case -1:
			rtk_writel(0xff, REG_CHIP_EN);  //disable chip	select
			break;			
		case 0:
		case 1:
		case 2:
		case 3:
			rtk_writel( ~(1 << chip), REG_CHIP_EN );  //select chip 0~4
			break;
		default:
			rtk_writel( ~(1 << 0), REG_CHIP_EN);  //select chip 0
	}
}


/**
 * scan_last_die_BB -  scan last die BB and create BBT
 * @mtd: MTD device structure
 */
static int scan_last_die_BB(struct mtd_info *mtd)
{
	debug_nand("---------[%s]----------\n", __FUNCTION__);
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	__u32 start_page;
	__u32 addr;
	int block_num = this->block_num;
	int block_size = 1 << this->phys_erase_shift;
	int table_index=0;
	int remap_block[RBA];
	int remap_count = 0;
	int i, j;
	int numchips = this->numchips;
	int chip_size = this->chipsize;
	int rc = 0;
	//int bb_count = 0;
	
	if ( numchips>1 ){	//multi-dies, start_page is at last die
		start_page = 0x00000000;
	}else{						//single die, start_page is 16MB
		start_page = 0x01000000;	//16MB is used for bootcode
	}		
	
	//printk("[%s] page_size=%d, block_size=%d, numchips=%d\n", 
		//	__FUNCTION__, page_size, block_size, numchips);
	
	this->active_chip = numchips-1;
	this->select_chip(mtd, numchips-1);
	
	__u8 *block_status = kmalloc( block_num, GFP_KERNEL );	
	if ( !block_status ){
		printk("%s: Error, no enough memory for block_status\n",__FUNCTION__);
		rc = -ENOMEM;
		goto EXIT;
	}
	memset ( (__u32 *)block_status, 0, block_num );
	
	//scan all block in nand to label the block status
	for( addr=start_page; addr<chip_size; addr+=block_size ){
		if ( rtk_block_isbad(mtd, numchips-1, addr) ){
			int bb = addr >> this->phys_erase_shift;
			block_status[bb] = 0xff;		//mark the flags to 0xff when the block is bad
			//bb_count++;
			//printk("addr=0x%x, bb=%d, bb_count=%d\n", addr, bb, bb_count);
		}
	}

	//record bottom-up good blocks in RBA for re-mapping usage	
	for ( i=0; i<RBA; i++){
		if ( block_status[(block_num-1)-i] == 0x00){
			remap_block[remap_count] = (block_num-1)-i;
			remap_count++;
			//printk("remap_block[%d] = %d, remap_count=%d\n", i, remap_block[i], remap_count);
		}
	}

	//fill the rest remap_block values in remap_block[]
	if (remap_count<RBA+1){
		for (j=remap_count+1; j<RBA+1; j++)
			remap_block[j-1] = RB_INIT;
	}
		
	//fill bad blcok and re-map block in BBT
	for ( i=0; i<(block_num-RBA); i++){
		if (block_status[i] == 0xff){
			this->bbt[table_index].bad_block = i;
			this->bbt[table_index].BB_die = numchips-1;
			this->bbt[table_index].remap_block = remap_block[table_index];
			this->bbt[table_index].RB_die = numchips-1;
			table_index++;
		}
	}
	
	 //fill the rest bad_block values in BBT entry
	for( i=table_index; table_index<RBA; table_index++){
		this->bbt[table_index].bad_block = BB_INIT;
		this->bbt[table_index].BB_die = BB_DIE_INIT;
		this->bbt[table_index].remap_block = remap_block[table_index];
		this->bbt[table_index].RB_die = numchips-1;
	}
	
	kfree(block_status);

	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);
	
EXIT:
	if (rc){
		if (block_status)
			kfree(block_status);	
	}
				
	return 0;
}


/**
 * scan_other_die_BB -  scan other die BB 
 * @mtd: MTD device structure
 */
static int scan_other_die_BB(struct mtd_info *mtd)
{
	debug_nand("---------[%s]----------\n", __FUNCTION__);
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	__u32 start_page;
	__u32 addr;
	int block_size = 1 << this->phys_erase_shift;
	int j, k;
	int numchips = this->numchips;
	int chip_size = this->chipsize;

	for( k=0; k<numchips-1; k++ ){
		this->active_chip = k;
		this->select_chip(mtd, k);
		if( k==0 ){
			start_page = 0x01000000;	//16MB is used for bootcode
		}else{
			start_page = 0x00000000;
		}
		
		//scan all block in die to label the block status
		for( addr=start_page; addr<chip_size; addr+=block_size ){
			if ( rtk_block_isbad(mtd, k, addr) ){
				//fill other die bad blcok into BBT
				for( j=0; j<RBA; j++){
					if ( this->bbt[j].bad_block == BB_INIT && this->bbt[j].remap_block != RB_INIT){
						this->bbt[j].bad_block = addr >> this->phys_erase_shift;
						this->bbt[j].BB_die = k;
						this->bbt[j].RB_die = numchips-1;
						break;
					}
				}
			}
		}
	}
	
	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);
	
#if 0
	//int j;
	int print_col = 16;			
	for ( j=0; j < sizeof(BB_t)*RBA; j++){
		if ( !(j % print_col) )
			printk("[%p]:", &this->bbt[j]);
		printk(" %02x", this->bbt[j]);
		if ( (j % print_col) == print_col-1 )
			printk("\n");
		if ( j == 2047 )	
			printk("=======================================================\n");
	}
	printk("\n");
#endif
		
	return 0;
}


/**
 * rtk_create_bbt -  mars nand driver creates  block management table
 * @mtd:	MTD device structure
 * @page:	page address to put BBT
 * This function support for the device and calls the nand_scan_bbt function
 */
static int rtk_create_bbt(struct mtd_info *mtd, int page)
{
	printk("[%s] nand driver creates B%d !!\n", __FUNCTION__, page ==0?0:1);
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	u8 *temp_BBT = 0;
	u8 mem_page_num, page_counter=0;
	
	//scan last Die in nand
	if ( scan_last_die_BB(mtd) ){
		printk("[%s] scan_last_die_BB() error !!\n", __FUNCTION__);
		return -1;
	}

	//scan Die 0~(last-1) in nand
	if ( this->numchips >1 ){	
		if ( scan_other_die_BB(mtd) ){
			printk("[%s] scan_last_die() error !!\n", __FUNCTION__);
			return -1;
		}
	}

	mem_page_num = (sizeof(BB_t)*RBA + page_size-1 )/page_size;
	//printk("[%s] %d page_size memory to load BBT !!\n", __FUNCTION__, mem_page_num);
	temp_BBT = kmalloc( mem_page_num*page_size, GFP_KERNEL );
	if ( !temp_BBT ){
		printk("%s: Error, no enough memory for temp_BBT\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset( temp_BBT, 0xff, mem_page_num*page_size);
			
	//go back Die 0 to create BBT
	this->select_chip(mtd, 0);
	
	if ( this->erase_block(mtd, 0, page) ){
		printk("[%s]erase block %d failure !!\n", __FUNCTION__, page/ppb);
		rc =  -1;
		goto EXIT;
	}
		
	/* write bbt */
	this->g_oobbuf[0] = BBT_TAG;

	memcpy( temp_BBT, this->bbt, sizeof(BB_t)*RBA );
	while( mem_page_num>0 ){
		if ( this->write_ecc_page(mtd, 0, page+page_counter, temp_BBT+page_counter*page_size, 
			this->g_oobbuf, 1) ){
				printk("[%s] write BBT B%d page %d failure!!\n", __FUNCTION__, 
					page ==0?0:1, page+page_counter);
				rc =  -1;
				goto EXIT;
		}
		page_counter++;
		mem_page_num--;		
	}

EXIT:
	if (temp_BBT)
		kfree(temp_BBT);
		
	return rc;		
}


/**
 * rtk_update_bbt -  mars nand driver updates  block management table
 * @mtd: MTD device structure
 * @data_buf: 	BBT data
 * @oob_buf:	BBT tags
 * @bbt:	BBT
 */
int rtk_update_bbt (struct mtd_info *mtd, __u8 *data_buf, __u8 *oob_buf, BB_t *bbt)
{
	debug_nand("---------[%s]----------\n", __FUNCTION__);
	int rc = 0;
	struct nand_chip *this = mtd->priv;
	unsigned char active_chip = this->active_chip;
	u8 *temp_BBT = 0;
	
	oob_buf[0] = BBT_TAG;

	//change to die 0 to update BBT
	this->select_chip(mtd, 0);	
		
	if ( sizeof(BB_t)*RBA <= page_size){
		memcpy( data_buf, bbt, sizeof(BB_t)*RBA );
	}else{
		temp_BBT = kmalloc( 2*page_size, GFP_KERNEL );
		if ( !(temp_BBT) ){
			printk("%s: Error, no enough memory for temp_BBT\n",__FUNCTION__);
			return -ENOMEM;
		}
		memset(temp_BBT, 0xff, 2*page_size);
		memcpy(temp_BBT, bbt, sizeof(BB_t)*RBA );
		memcpy(data_buf, temp_BBT, page_size);
	}
		
	/* update BBT B0 */
	if ( this->erase_block(mtd, 0, 0) ){
		printk("[%s]error: erase block 0 failure\n", __FUNCTION__);
	}

	if ( this->write_ecc_page(mtd, 0, 0, data_buf, oob_buf, 1) ){
		printk("[%s]update BBT B0 page 0 failure\n", __FUNCTION__);
	}else{		//if RBA>page_size, update BBT 0 page 1
		if ( sizeof(BB_t)*RBA > page_size){
			memset(data_buf, 0xff, page_size);
			memcpy( data_buf, temp_BBT+page_size, sizeof(BB_t)*RBA - page_size );
			if ( this->write_ecc_page(mtd, 0, 1, data_buf, oob_buf, 1) ){
				printk("[%s]update BBT B0 page 1 failure\n", __FUNCTION__);
			}
		}	
	}

	/* update BBT B1 */
	if ( this->erase_block(mtd, 1, ppb) ){
		printk("[%s]error: erase block 1 failure\n", __FUNCTION__);
		return -1;
	}
	
	if ( this->write_ecc_page(mtd, 0, ppb, data_buf, oob_buf, 1) ){
		printk("[%s]update BBT B1 failure\n", __FUNCTION__);
		return -1;
	}else{		//if RBA>page_size, update BBT 1 page ppb+1
		if ( sizeof(BB_t)*RBA > page_size){
			memset(data_buf, 0xff, page_size);
			memcpy( data_buf, temp_BBT+page_size, sizeof(BB_t)*RBA - page_size );
			if ( this->write_ecc_page(mtd, 0, ppb+1, data_buf, oob_buf, 1) ){
				printk("[%s]error: erase block 0 failure\n", __FUNCTION__);
				return -1;
			}
		}		
	}
	
	//printk("active_chip=%d\n", active_chip );
	//change to previos die status after update BBT	
	this->select_chip(mtd, active_chip);

	if (temp_BBT)
		kfree(temp_BBT);
			
	return rc;
}


/**
 * rtk_nand_scan_bbt - [DEFAULT] scan if bad block management table exists
 * @mtd:	MTD device structure
 */
static int rtk_nand_scan_bbt(struct mtd_info *mtd)
{
	debug_nand("---------[%s]----------\n", __FUNCTION__);
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	__u8 isbbt_b0, isbbt_b1;
	u8 *temp_BBT=0;
	u8 mem_page_num, page_counter=0;

	//for reload bbt usage
	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);
	
	mem_page_num = (sizeof(BB_t)*RBA + page_size-1 )/page_size;
	//printk("[%s] %d page_size memory to load BBT !!\n", __FUNCTION__, mem_page_num);
	temp_BBT = kmalloc( mem_page_num*page_size, GFP_KERNEL );
	if ( !temp_BBT ){
		printk("%s: Error, no enough memory for temp_BBT\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset( temp_BBT, 0xff, mem_page_num*page_size);
				
	//check to see if the BBT is created
	rc = this->read_ecc_page(mtd, 0, 0, this->g_databuf, this->g_oobbuf);
	
	isbbt_b0 = this->g_oobbuf[0];
	//printk("isbbt_b0=0x%x\n", isbbt_b0);

	if ( !rc ){
		if ( isbbt_b0 == BBT_TAG ){
			printk("[%s] have created bbt B0, just loads it !!\n", __FUNCTION__);
			memcpy( temp_BBT, this->g_databuf, page_size );
			page_counter++;
			mem_page_num--;

			while( mem_page_num>0 ){
				if ( this->read_ecc_page(mtd, 0, page_counter, this->g_databuf, this->g_oobbuf) ){
					printk("[%s] load bbt B0 error!!\n", __FUNCTION__);
					kfree(temp_BBT);
					return -1;
				}
				memcpy( temp_BBT+page_counter*page_size, this->g_databuf, page_size );
				//printk("mem_page_num=%d,  page_counter=%d\n", mem_page_num, page_counter);
				page_counter++;
				mem_page_num--;
			}
			memcpy( this->bbt, temp_BBT, sizeof(BB_t)*RBA );
		}else{
			printk("[%s] read BBT B0 tags fails, try to load BBT B1\n", __FUNCTION__);
			rc = this->read_ecc_page(mtd, 0, ppb, this->g_databuf, this->g_oobbuf);
			isbbt_b1 = this->g_oobbuf[0];	
			if ( !rc ){
				if ( isbbt_b1 == BBT_TAG ){
					printk("[%s] have created bbt B1, just loads it !!\n", __FUNCTION__);
					memcpy( temp_BBT, this->g_databuf, page_size );
					page_counter++;
					mem_page_num--;

					while( mem_page_num>0 ){
						if ( this->read_ecc_page(mtd, 0, ppb+page_counter, this->g_databuf, this->g_oobbuf) ){
							printk("[%s] load bbt B1 error!!\n", __FUNCTION__);
							kfree(temp_BBT);
							return -1;
						}
						memcpy( temp_BBT+page_counter*page_size, this->g_databuf, page_size );
						//printk("mem_page_num=%d,  page_counter=%d\n", mem_page_num, page_counter);
						page_counter++;
						mem_page_num--;
					}
					memcpy( this->bbt, temp_BBT, sizeof(BB_t)*RBA );
				}else{
					printk("[%s] read BBT B1 tags fails, nand driver will creat BBT B0 and B1\n", __FUNCTION__);
					rtk_create_bbt(mtd, 0);
					rtk_create_bbt(mtd, ppb);
				}
			}else{
				printk("[%s] read BBT B1 with HW ECC fails, nand driver will creat BBT B0\n", __FUNCTION__);
				rtk_create_bbt(mtd, 0);
			}
		}
	}else{
		printk("[%s] read BBT B0 with HW ECC error, try to load BBT B1\n", __FUNCTION__);
		rc = this->read_ecc_page(mtd, 0, ppb, this->g_databuf, this->g_oobbuf);
		isbbt_b1 = this->g_oobbuf[0];	
		if ( !rc ){
			if ( isbbt_b1 == BBT_TAG ){
				printk("[%s] have created bbt B1, just loads it !!\n", __FUNCTION__);
				memcpy( temp_BBT, this->g_databuf, page_size );
				page_counter++;
				mem_page_num--;

				while( mem_page_num>0 ){
					if ( this->read_ecc_page(mtd, 0, ppb+page_counter, this->g_databuf, this->g_oobbuf) ){
						printk("[%s] load bbt B1 error!!\n", __FUNCTION__);
						kfree(temp_BBT);
						return -1;
					}
					memcpy( temp_BBT+page_counter*page_size, this->g_databuf, page_size );
					//printk("mem_page_num=%d,  page_counter=%d\n", mem_page_num, page_counter);
					page_counter++;
					mem_page_num--;
				}
				memcpy( this->bbt, temp_BBT, sizeof(BB_t)*RBA );
			}else{
				printk("[%s] read BBT B1 tags fails, nand driver will creat BBT B1\n", __FUNCTION__);
				rtk_create_bbt(mtd, ppb);
			}
		}else{
			printk("[%s] read BBT B0 and B1 with HW ECC fails\n", __FUNCTION__);
			kfree(temp_BBT);
			return -1;
		}
	}

	dump_BBT(mtd);

#if RTK_TEST_ERASE_BBT
	//TEST_ERASE_BBT(mtd, 0, 4096);
	//TEST_ERASE_BBT(mtd, 8192, 4032);		//16MB, nand boot, 1024MB
	TEST_ERASE_BBT(mtd, 8192, 8192-128);		//HY27UG088G5B: 16MB, nand boot
#endif	
	
	if (temp_BBT)
		kfree(temp_BBT);
	
	return rc;
}


/**
 * rtk_nand_scan - [NAND Interface] Scan for the NAND device
 * @mtd:        MTD device structure
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values. Buffers are allocated if
 * they are not provided by the board driver
 */
int rtk_nand_scan(struct mtd_info *mtd, int maxchips)
{
	debug_nand("---------[%s]----------\n", __FUNCTION__);
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned char id[5];
	unsigned int device_size=0;
	unsigned int i;
	unsigned int nand_type_id ;
	int rtk_lookup_table_flag=0;
	unsigned char maker_code;	
	unsigned char device_code; 
	unsigned char B5th;
	unsigned int block_size;
	unsigned int num_chips = 1;
	unsigned int chip_size=0;
	unsigned int num_chips_probe = 1;
	
	if ( !this->select_chip )
		this->select_chip = nand_select_chip;
	if ( !this->scan_bbt )
		this->scan_bbt = rtk_nand_scan_bbt;

	/* Select the device */
	this->active_chip = 0;
	this->select_chip(mtd, 0);	//chip_nr=0

	//Cuz rescure has to set up a fixed mtd name
	mtd->name = "rtk_nand";

	while (1) {
		/* Read manufacturer id and device id */
		this->read_id(mtd, id);

		debug_nand ("%s: Manufacture ID=0x%02x, Chip ID=0x%02x, "
					"3thID=0x%02x, 4thID=0x%02x, 5thID=0x%02x\n",
					mtd->name, id[0], id[1], id[2], id[3], id[4]);

		//get manufacture & device ID, ex: Samsung: 5 bytes value : EC D3 10 A6 64
		this->maker_code = maker_code = id[0];
		this->device_code = device_code = id[1];
		nand_type_id = maker_code<<24 | device_code<<16 | id[2]<<8 | id[3];
		B5th = id[4];
		
		/* handle multiple dies in one Chip */
		for (i=1;  i<maxchips; i++) {
			this->select_chip(mtd, i);
			/* Send the command for reading device ID */
			this->read_id(mtd, id);
			/* Read manufacturer and device IDs */
			if ( maker_code !=  id[0] ||device_code != id[1] )
				break;
		}

		if (i > 1){
			num_chips_probe = i;
			printk(KERN_INFO "NAND Flash Controller detects %d dies\n", num_chips_probe);
		}
			
		for (i = 0; nand_device[i].name; i++){
			if ( nand_device[i].id==nand_type_id && 
				((nand_device[i].CycleID5th==0xff)?1:(nand_device[i].CycleID5th==B5th)) ){
				/* reset the optimal speed */
				rtk_writel( nand_device[i].T1, REG_T1 );
				rtk_writel( nand_device[i].T2, REG_T2 );
				rtk_writel( nand_device[i].T3, REG_T3 );
				if ( nand_type_id != HY27UT084G2M ){
					rtk_writel( 0x20, REG_MULTICHNL_MODE);
					//rtk_writel( 0x00, REG_DELAY_CNT );  // set T_WHR_ADL delay clock #
				}
				if (nand_device[i].size == num_chips_probe * nand_device[i].chipsize){
					if ( num_chips_probe == nand_device[i].num_chips ){
						printk("One %s chip has %d die(s) on board\n", 
							nand_device[i].name, nand_device[i].num_chips);
						mtd->PartNum = nand_device[i].name;
						device_size = nand_device[i].size;
						chip_size = nand_device[i].chipsize;	
						page_size = nand_device[i].PageSize;
						block_size = nand_device[i].BlockSize;
						oob_size = nand_device[i].OobSize;
						num_chips = nand_device[i].num_chips;
						isLastPage = nand_device[i].isLastPage;
						rtk_lookup_table_flag = 1;
						printk("nand part=%s, id=%x, device_size=%d, chip_size=%d, num_chips=%d, isLastPage=%d\n", 
							nand_device[i].name, nand_device[i].id, nand_device[i].size, nand_device[i].chipsize, 
							nand_device[i].num_chips, nand_device[i].isLastPage);
						break;
					}				
				}else{
					if ( !strcmp(nand_device[i].name, "HY27UF084G2M" ) )
						continue;
					else{	
						printk("We have %d the same %s chips on board\n", 
							num_chips_probe/nand_device[i].num_chips, nand_device[i].name);
						mtd->PartNum = nand_device[i].name;
						device_size = nand_device[i].size;
						chip_size = nand_device[i].chipsize;	
						page_size = nand_device[i].PageSize;
						block_size = nand_device[i].BlockSize;
						oob_size = nand_device[i].OobSize;
						num_chips = nand_device[i].num_chips;
						isLastPage = nand_device[i].isLastPage;
						rtk_lookup_table_flag = 1;
						printk("nand part=%s, id=%x, device_size=%d, chip_size=%d, num_chips=%d, isLastPage=%d\n", 
							nand_device[i].name, nand_device[i].id, nand_device[i].size, nand_device[i].chipsize, 
							nand_device[i].num_chips, nand_device[i].isLastPage);
						break;
					}
				}
			}
		}
		
		if ( !rtk_lookup_table_flag ){
			printk("Warning: Lookup Table do not have this nand flash !!\n");
			return -1;
		}
		
		this->page_shift = generic_ffs(page_size)-1; 
		this->phys_erase_shift = generic_ffs(block_size)-1;
		this->oob_shift = generic_ffs(oob_size)-1;
		ppb = this->ppb = block_size >> this->page_shift;
		
		if (chip_size){
			this->block_num = chip_size >> this->phys_erase_shift;
			this->page_num = chip_size >> this->page_shift;
			this->chipsize = chip_size;
			this->device_size = device_size;
			this->chip_shift =  generic_ffs(this->chipsize )-1;
			//printk("[%s][%d] this->page_num=%d \n", __FUNCTION__, __LINE__, this->page_num);
		}

		/* Convert chipsize to number of pages per chip -1. */
		this->pagemask = (this->chipsize >> this->page_shift) - 1;
	
		/* Calc oobsize: Spare area size, the number of bytes per page used for OOB */
		mtd->oobsize = this->oob_size = oob_size;

		/* Calc pagesize */
		mtd->oobblock = page_size;
			
		/* Calc blocksize. * Blocksize is multiples of 64KiB */
		mtd->erasesize = block_size;
	
		/* data bytes under ecc protection */
		mtd->eccsize = 512; 	
	
		/* set ecc to RTK HW ECC  */
		this->eccmode = MTD_ECC_RTK_HW;

		break;

	} //end while (1)

	debug_nand( "%s:\n" "nand_type_id=%x\n"
						"chip size = %d MB,\n" "blk size = %d KB,\n" 	
						"page size = %d B,\n"	"oob size = %d B\n" 
						"chip shift = %d\n"
						"page shift = %d,\n" "oob_shift = %d\n",
						mtd->PartNum, nand_type_id, 
						this->chipsize >> 20, mtd->erasesize >> 10, 
						mtd->oobblock , mtd->oobsize, 
						this->chip_shift , this->page_shift, 
						this->oob_shift);

	/* Set back to the default device */
	this->select_chip(mtd, 0);	//chip_nr=0
			
	/* Store the number of chips and calc total size for mtd */
	// more than one nand that has single die respectively 
	if ( num_chips != num_chips_probe )
		this->numchips = num_chips_probe;
	else
		this->numchips = num_chips;
	
	mtd->size = this->numchips * this->chipsize;
	//printk("mtd->size=%u, num_chips=%d, num_chips_probe=%d\n", mtd->size, num_chips, num_chips_probe);

	//set up RBA size
	RBA = this->RBA = (mtd->size/block_size) * this->RBA_PERCENT/100;
	//printk("RBA=%d, page_size=%d, sizeof(BB_t)=%d\n", RBA, page_size, sizeof(BB_t));		
	
	//allocate memory for BBT
	this->bbt = kmalloc( sizeof(BB_t)*RBA, GFP_KERNEL );
	if ( !this->bbt ){
		printk("%s: Error, no enough memory for BBT\n",__FUNCTION__);
		return -1;
	}
	memset(this->bbt, 0,  sizeof(BB_t)*RBA);
	
	/* allocate memory for global buffers: *g_databuf, *g_oobbuf and *erase_page_flag
	in struct nand_chip */
	this->g_databuf = kmalloc( page_size, GFP_KERNEL );
	if ( !this->g_databuf ){
		printk("%s: Error, no enough memory for g_databuf\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset(this->g_databuf, 0xff, page_size);

	this->g_oobbuf = kmalloc( oob_size, GFP_KERNEL );
	if ( !this->g_oobbuf ){
		printk("%s: Error, no enough memory for g_oobbuf\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset(this->g_oobbuf, 0xff, oob_size);

	//Ken uses bitmap to record page erase status
	unsigned int flag_size =  (this->numchips * this->page_num) >> 3;		//Kynix 1GB: 4096block, 128 page/block
	unsigned int mempage_order = get_order(flag_size);
	//printk("total pages=%u, mempage_order=%d\n", this->numchips*this->page_num, mempage_order);
	/* erase_page_flag is for that ECC don't pull high when first read */
	this->erase_page_flag = (void *)__get_free_pages(GFP_KERNEL, mempage_order);	
	if ( !this->erase_page_flag ){
		printk("%s: Error, no enough memory for erase_page_flag\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset ( (__u32 *)this->erase_page_flag, 0, flag_size);

	/* Fill in remaining MTD driver data */
	mtd->type				= MTD_NANDFLASH;
	mtd->flags			= MTD_CAP_NANDFLASH;
	mtd->ecctype		= MTD_ECC_RTK_HW;
	mtd->erase			= nand_erase;
	mtd->point			= NULL;
	mtd->unpoint		= NULL;
	mtd->read				= nand_read;
	mtd->write			= nand_write;
	mtd->read_ecc		= nand_read_ecc;
	mtd->write_ecc		= nand_write_ecc;
	mtd->read_oob		= nand_read_oob;
	mtd->write_oob	= nand_write_oob;
	mtd->readv			= NULL;
	mtd->writev			= NULL;
	mtd->readv_ecc	= NULL;
	mtd->writev_ecc	= NULL;
	mtd->sync			= nand_sync;
	mtd->lock				= NULL;
	mtd->unlock			= NULL;
	mtd->suspend		= nand_suspend;
	mtd->resume		= nand_resume;
	mtd->owner			= THIS_MODULE;

	mtd->block_isbad			= nand_block_isbad;
	mtd->block_markbad	= nand_block_markbad;
	
	/* Ken: 20090210 */
	mtd->reload_bbt = rtk_nand_scan_bbt;
	
	mtd->owner = THIS_MODULE;

	/* =========================== for MP usage =======================*/
	/* get mp_time_para from /sys/realtek_boards/system_parameters */
	mp_time_para = (char *) parse_token(platform_info.system_parameters, "mp_time_para");
	if ( mp_time_para && strlen(mp_time_para) )
		mp_time_para_value = simple_strtoul(mp_time_para, &mp_time_para, 10);

	if ( mp_time_para_value ){
		/* reset the optimal speed */
		rtk_writel( mp_time_para_value, REG_T1 );
		rtk_writel( mp_time_para_value, REG_T2 );
		rtk_writel( mp_time_para_value, REG_T3 );
	}

	/* get nf_clock from /sys/realtek_boards/system_parameters */
	nf_clock = (char *) parse_token(platform_info.system_parameters, "nf_clock");
	if ( nf_clock && strlen(nf_clock) )
		nf_clock_value = simple_strtoul(nf_clock, &nf_clock, 10);

	if ( nf_clock_value )
		NF_CKSEL(mtd->PartNum, nf_clock_value);

	/* get mp_erase_nand from /sys/realtek_boards/system_parameters */
	mp_erase_nand = (char *) parse_token(platform_info.system_parameters, "mp_erase_nand");
	//printk("mp_erase_nand=%s\n", mp_erase_nand);
	if ( mp_erase_nand && strlen(mp_erase_nand) )
		mp_erase_flag = simple_strtoul(mp_erase_nand, &mp_erase_nand, 10);
	//printk("mp_erase_flag=%d\n", mp_erase_flag);
	
	if ( mp_erase_flag ){
		int start_pos = 0;
		int start_page = (start_pos/mtd->erasesize)*ppb;
		int block_num = (mtd->size - start_pos)/mtd->erasesize;
		//printk("start_page=%d, block_num=%d\n", start_page, block_num);
		printk("Starting erasure all contents of nand for MP.\n");
		TEST_ERASE_ALL(mtd, start_page, block_num);
		this->select_chip(mtd, 0);
		return 0;
	}

	/* CMYu, 20090720, get mcp from /sys/realtek_boards/system_parameters */
	mcp = (char *) parse_token(platform_info.system_parameters, "mcp");
	//printk("mcp=%s\n", mcp);
	if (mcp){
		if ( strstr(mcp, "ecb") )
			this->mcp = MCP_AES_ECB;
		else if ( strstr(mcp, "cbc") )
			this->mcp = MCP_AES_CBC;
		else if ( strstr(mcp, "ctr") )
			this->mcp = MCP_AES_CTR;
		else
			this->mcp = MCP_NONE;			
	}
	
	//for MCP test
	//this->mcp = MCP_AES_ECB;
	//printk("[%s] this->mcp=%d\n", __FUNCTION__, this->mcp);
			
	/* =========== WE Over spec: Underclocking lists: ========== */
	switch(nand_type_id){
		case HY27UT084G2M:	//WE: 40 ns
			//NF_CKSEL(mtd->PartNum, 0x04);	//43.2 MHz
			break;
		case HY27UF081G2A:	//WE: 15 ns
		case HY27UF082G2A:
		case K9G4G08U0A:
		case K9G8G08U0M:
		case TC58NVG0S3C:
		case TC58NVG1S3C:
			NF_CKSEL(mtd->PartNum, 0x03);	//54 MHz
			break;
		default:
			;
	}	
			
#if RTK_TEST_ERASE_ALL
	//TEST_ERASE_ALL(mtd, 7680, 1024);
	//TEST_ERASE_ALL(mtd, 138752, 512);	//128MB
	//TEST_ERASE_ALL(mtd, 138752, 256);		//64MB
	//TEST_ERASE_ALL(mtd, 384000, 256);		//64MB
	//TEST_ERASE_ALL(mtd, 0, 256);		//64MB
	//TEST_ERASE_ALL(mtd, 0, 512);		//128MB
	//TEST_ERASE_ALL(mtd, 0, 768);		//192MB
	//TEST_ERASE_ALL(mtd, 0, 1024);		//256MB
	//TEST_ERASE_ALL(mtd, 0, 2048);		//512MB
	//TEST_ERASE_ALL(mtd, 8192, 448);		//nand boot, 128MB
	//TEST_ERASE_ALL(mtd, 8192, 704);		//nand boot, 192MB
	//TEST_ERASE_ALL(mtd, 8192, 4032-200);		//nand boot, 1024MB-200BLOCK
	//TEST_ERASE_ALL(mtd, 8192, 4032);		//nand boot, 1024MB
#if 1
	int start_pos = 0;
#else	
	int start_pos = 16*1024*1024;
#endif	
	int start_page = (start_pos/mtd->erasesize)*ppb;
	int block_num = (mtd->size - start_pos)/mtd->erasesize;
	printk("start_page=%d, block_num=%d\n", start_page, block_num);
	TEST_ERASE_ALL(mtd, start_page, block_num);
	this->select_chip(mtd, 0);
#endif	

#if RTK_TEST_MARK_BB
	TEST_MARK_BB(mtd);
#endif

	/* Scan bad block table */
	return this->scan_bbt (mtd);
	
#if RTK_TEST_WRITE_OOB
	//First, U have to mark "return this->scan_bbt (mtd)" to use this test
	this->scan_bbt(mtd);
	TEST_WRITE_OOB(mtd);
	return 0;
#endif	

}


int TEST_ERASE_ALL(struct mtd_info *mtd, int page, int bc)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int i;
	//unsigned char oob_buf[oob_size] __attribute__((__aligned__(8)));
	int chip_block_num = this->block_num;
	int start_block = page/ppb;
	int block_in_die; 
	int rc = 0;
	int chipnr =0, block;
	
	if ( page & (ppb-1) ){
		page = (page/ppb)*ppb;
	}
	//printk("page=%d, bc=%d, ppb=%d, chip_block_num=%d\n", page, bc, ppb, chip_block_num);
	
	//printk("**********RTK_TEST_ERASE_ALL NOW **********\n");		
	for ( i=0; i<bc; i++){
#if 0	//0: force to erase
		//check the last page
		this->read_oob(mtd, page+ i*ppb + (ppb-1), oob_size, oob_buf);
		if ( oob_buf[0] != 0xFF )
			continue;
#endif
		
		block_in_die = start_block + i;
		chipnr = block_in_die/chip_block_num;
		block = block_in_die%chip_block_num;
		this->select_chip(mtd, block_in_die/chip_block_num);
		rc = this->erase_block(mtd, chipnr, block*ppb);
		//printk("i=%d, Die=%d, block=%d, start page=%d\n", i, chipnr, block, block*ppb);
		/* mark the block as BB */
		if ( rc<0 ){
			this->g_oobbuf[0] = 0x00;
			if ( isLastPage ){	//mark the block as bad
				this->write_oob(mtd, chipnr, block*ppb+ppb-1, oob_size, this->g_oobbuf);	//the last page
				this->write_oob(mtd, chipnr, block*ppb+ppb-2, oob_size, this->g_oobbuf);	//the (last-1) page
			}else{		//other nand: mark the block as bad
				this->write_oob(mtd, chipnr, block*ppb, oob_size, this->g_oobbuf);	//first page
				this->write_oob(mtd, chipnr, block*ppb+1, oob_size, this->g_oobbuf);	//2nd page
			}
		}
	}
	
#if 0
	int j;
	unsigned char data_buf1[mtd->oobblock] __attribute__((__aligned__(8)));
	printk("***********after erase: read data from NAND**********\n");
	for ( i=0; i<bc; i++){	
		//only read the first page of each block
		this->read_ecc_page(mtd, page+i*ppb, data_buf1, NULL);
		//only dump 128B of  the each first page
		for ( j=0; j < 64; j++){
			if ( !(j % 8)  ){
				printk("\n");
				printk("[%p]:", &data_buf1[j]);
			}
			printk("[0x%x]", data_buf1[j]);
		}	
	}
#endif	
	return 0;
}


//==========================Ken: Self Test=========================
#if RTK_TEST_ERASE_BBT
int TEST_ERASE_BBT(struct mtd_info *mtd, int page, int bc)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	struct erase_info instr;
	instr.addr = page <<  this->page_shift;	
	instr.len = bc * mtd->erasesize;
		
	printk("**********RTK_TEST_ERASE_BBT NOW **********\n");		

 	nand_erase_nand (mtd, &instr, 0);
 	
	return 0;
}
#endif


#if RTK_TEST_MARK_BB
int TEST_MARK_BB(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned char oob_buf[oob_size] __attribute__((__aligned__(8)));
	unsigned char active_chip = this->active_chip;
	unsigned int nand_type_id = this->maker_code << 8 | this->device_code;
	int pos;
	oob_buf[0] = 0x00;
	//bootcode: 16MB = 64 block

	if ( isLastPage ){	//mark the block as bad, mark the last page
		pos = ppb-1;
	}else{			//mark the first page
		pos = 0;
	}

	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	
	this->select_chip(mtd, 0);
 
#if 0
	this->write_oob(mtd, 0,  7*ppb+ pos, oob_size, oob_buf);	//block 7
	this->write_oob(mtd, 0, 13*ppb+ pos, oob_size, oob_buf);	//block 13
	this->write_oob(mtd, 0, 34*ppb+ pos, oob_size, oob_buf);	//block 34
#endif

	if ( isLastPage ){
		this->write_oob(mtd, 0, 68*ppb+ pos, oob_size, oob_buf);	//block 68
		this->write_oob(mtd, 0, 74*ppb+ pos, oob_size, oob_buf);	//block 74
		this->write_oob(mtd, 0, 78*ppb+ pos, oob_size, oob_buf);	//block 78
		this->write_oob(mtd, 0, 82*ppb+ pos, oob_size, oob_buf);	//block 82
		this->write_oob(mtd, 0, 101*ppb+ pos, oob_size, oob_buf);	//block 101
		this->write_oob(mtd, 0, 114*ppb+ pos, oob_size, oob_buf);	//block 114
	}

	this->write_oob(mtd, 0, 135*ppb+ pos, oob_size, oob_buf);	//block 135
	this->write_oob(mtd, 0, 144*ppb+ pos, oob_size, oob_buf);	//block 144
	this->write_oob(mtd, 0, 166*ppb+ pos, oob_size, oob_buf);	//block 166
	this->write_oob(mtd, 0, 180*ppb+ pos, oob_size, oob_buf);	//block 180
	this->write_oob(mtd, 0, 255*ppb+ pos, oob_size, oob_buf);	//block 255
	this->write_oob(mtd, 0, 284*ppb+ pos, oob_size, oob_buf);	//block 284
	this->write_oob(mtd, 0, 368*ppb+ pos, oob_size, oob_buf);	//block 368
	this->write_oob(mtd, 0, 471*ppb+ pos, oob_size, oob_buf);	//block 471
	this->write_oob(mtd, 0, 592*ppb+ pos, oob_size, oob_buf);	//block 592
	this->write_oob(mtd, 0, 681*ppb+ pos, oob_size, oob_buf);	//block 681
	this->write_oob(mtd, 0, 724*ppb+ pos, oob_size, oob_buf);	//block 724
	this->write_oob(mtd, 0, 833*ppb+ pos, oob_size, oob_buf);	//block 833
	this->write_oob(mtd, 0, 924*ppb+ pos, oob_size, oob_buf);	//block 924
	this->write_oob(mtd, 0, 1037*ppb+ pos, oob_size, oob_buf);	//block 1037
	this->write_oob(mtd, 0, 4066*ppb+ pos, oob_size, oob_buf);	//block 4066
	this->write_oob(mtd, 0, 4076*ppb+ pos, oob_size, oob_buf);	//block 4076
	this->write_oob(mtd, 0, 4086*ppb+ pos, oob_size, oob_buf);	//block 4086

	if ( (nand_type_id == HY27UG088G5B) || (nand_type_id == HY27UT088G2A && this->numchips > 1) ){
		this->select_chip(mtd, 1);
		this->write_oob(mtd, 1, 833*ppb+ pos, oob_size, oob_buf);	//block 833
		this->write_oob(mtd, 1, 924*ppb+ pos, oob_size, oob_buf);	//block 924
		this->write_oob(mtd, 1, 3800*ppb+ pos, oob_size, oob_buf);	//block 3800
		this->write_oob(mtd, 1, 3900*ppb+ pos, oob_size, oob_buf);	//block 3900
		this->write_oob(mtd, 1, 4076*ppb+ pos, oob_size, oob_buf);	//block 4076
		this->write_oob(mtd, 1, 4086*ppb+ pos, oob_size, oob_buf);	//block 4086
	}

	this->select_chip(mtd, active_chip);
	return 0;
}
#endif 


#if RTK_TEST_WRITE_OOB
int TEST_WRITE_OOB(struct mtd_info *mtd)
{
	unsigned char oob_buf[oob_size] __attribute__((__aligned__(8)));
	oob_buf[0] = 0xAA;
	//bootcode: 16MB = 64 block
	int to = 8350 * page_size; 	//page 8350 at block 65+ 30 pages
	int len = oob_size;
	int *retlen = 0;

	nand_write_oob(mtd, to, len, retlen, oob_buf);

	return 0;
}
#endif 
