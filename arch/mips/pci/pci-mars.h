/*
 *  pci-mars.h, Include file for PCI Control Unit of the Realtek Mars series.
 *
 *  Copyright (C) 2007  Realtek Semiconductor Corp.
 *    Author: Frank Ting 
 *  Copyright (C)  Frank Ting <mjting@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __PCI_MARS_H
#define __PCI_MARS_H
// CRT for PCI
#define PCICK_SEL               0xb8000338
#define PFUNC_PCI5              0xb80003AC

// SB2 for PCI
#define PCI_BASE0               0xb801A030
#define PCI_BASE1               0xb801A034
#define PCI_BASE2               0xb801A038
#define PCI_BASE3               0xb801A03C
#define PCI_MASK0               0xb801A040
#define PCI_MASK1               0xb801A044
#define PCI_MASK2               0xb801A048
#define PCI_MASK3               0xb801A04C
#define PCI_TRANS0              0xb801A050
#define PCI_TRANS1              0xb801A054
#define PCI_TRANS2              0xb801A058
#define PCI_TRANS3              0xb801A05C

#define PCI_CTRL                0xb801A060    
    


// PCI-DVR Bridge
#define DVR_GNR_MODE            0xb8017000U
#define DVR_GNR_EN              0xb8017004U

#define DVR_GNR_INT             0xb8017008U
  #define S_INTP_RDIR           7
  #define S_INTP_WDIR           6
  #define S_INTP_MST            5
  #define S_INTP_SLV            4
  #define S_INTP_PCI            3
  #define S_INTP_CFG            2
  #define S_INTP_MIO            1
  #define S_INTP_DMA            0
    #define INTP_RDIR           1 << S_INTP_RDIR
    #define INTP_WDIR           1 << S_INTP_WDIR
    #define INTP_MST            1 << S_INTP_MST
    #define INTP_SLV            1 << S_INTP_SLV
    #define INTP_PCI            1 << S_INTP_PCI
    #define INTP_CFG            1 << S_INTP_CFG
    #define INTP_MIO            1 << S_INTP_MIO
    #define INTP_DMA            1 << S_INTP_DMA

#define DVR_CFG_CT              0xb801700cU
#define DVR_CFG_EN              0xb8017010U
#define DVR_CFG_ST              0xb8017014U
  #define S_ERROR_ST            1
  #define S_DONE_ST             0
    #define ERROR_ST            1 << S_ERROR_ST
    #define DONE_ST             1 << S_DONE_ST
  
#define DVR_CFG_ADDR            0xb8017018U
#define DVR_CFG_WDATA           0xb801701cU
#define DVR_CFG_RDATA           0xb8017020U

#define DVR_MIO_CT              0xb8017024U
#define DVR_MIO_ST              0xb8017028U
#define DVR_MIO_EN              0xb801702cU
#define DVR_MIO_ADDR            0xb8017030U
#define DVR_MIO_WDATA           0xb8017034U
#define DVR_MIO_RDATA           0xb8017038U
#define DVR_DMA_CT              0xb801703cU
#define DVR_DMA_ST              0xb8017040U
  #define S_SIN_DMA_DONE_ST     4
  #define S_DESC_DMA_DONE_ST    3
  #define S_DESC_EMPTY_ST       2
  #define S_PCI_ERROR_ST        1
  #define S_DESC_ERROR_ST       0
    #define SIN_DMA_DONE_ST     1 << S_SIN_DMA_DONE_ST
    #define DESC_DMA_DONE_ST    1 << S_DESC_DMA_DONE_ST
    #define DESC_EMPTY_ST       1 << S_DESC_EMPTY_ST
    #define PCI_ERROR_ST        1 << S_PCI_ERROR_ST
    #define DESC_ERROR_ST       1 << S_DESC_ERROR_ST
  
#define DVR_DMA_EN              0xb8017044U
#define DVR_DMA_PCI             0xb8017048U
#define DVR_DMA_DDR             0xb801704cU
#define DVR_DMA_CNT             0xb8017050U
#define DVR_DMA_DES0            0xb8017054U
#define DVR_DMA_DES1            0xb8017058U
#define DVR_DMA_DES2            0xb801705cU
#define DVR_DMA_DES3            0xb8017060U
#define DVR_SOFT_RST            0xb8017064U
#define DVR_CLK_ST              0xb8017068U
#define DVR_SPC_CT              0xb801706cU
#define DVR_SPC_ST              0xb8017070U
#define DVR_DBG                 0xb8017074U

#define DVR_DIR_ST              0xb8017078U
  #define S_WERROR_ST           0
  #define S_RERROR_ST           1
    #define WERROR_ST               1 << S_WERROR_ST
    #define RERROR_ST               1 << S_RERROR_ST

#define DVR_DIR_EN              0xb801707cU
#define DVR_DEV_ST              0xb8017080U
#define DVR_DEV_CT              0xb8017084U
#define DVR_DEV_DDR             0xb8017088U
#define DVR_DEV_PCI             0xb801708CU
#define DVR_DEV_CNT             0xb8017090U
#define DVR_TMP0_REG            0xb8017094U
#define DVR_TMP1_REG            0xb80170A0U

#define PCI_GNR_EN2             0xb8017108U
#define PCI_GNR_ST              0xb801710cU
  #define S_MASTER_ERROR        0
    #define MASTER_ERROR        1 << S_MASTER_ERROR
    
#define PCI_GNR_INT             0xb8017110U
           
    #define PCI_INTP_SPEC_CYC   1 << 6
    #define PCI_INTP_RETRY      1 << 5
    #define PCI_INTP_MON        1 << 4
    #define PCI_INTP_PME        1 << 3
    #define PCI_INTP_GENR       1 << 2
    #define PCI_INTP_SERR       1 << 1
    #define PCI_INTP_PERR       1 << 0

#define CFG(addr)               0xb8017200 + addr 
#define BAR1                    CFG(0x10)
#define BAR2                    CFG(0x14)
#define BAR3                    CFG(0x18)
#define BAR4                    CFG(0x1C)

#define S_DETEC_PAR_ERROR       31
#define S_SIGNAL_SYS_ERROR      30
#define S_REC_MASTER_ABORT      29
#define S_REC_TARGET_ABORT      28
#define S_SIG_TAR_ABORT         27

#define DETEC_PAR_ERROR         1 << S_DETEC_PAR_ERROR
#define SIGNAL_SYS_ERROR        1 << S_SIGNAL_SYS_ERROR
#define REC_MASTER_ABORT        1 << S_REC_MASTER_ABORT
#define REC_TARGET_ABORT        1 << S_REC_TARGET_ABORT
#define SIG_TAR_ABORT           1 << S_SIG_TAR_ABORT






#endif  /* __PCI_MARS_H */
