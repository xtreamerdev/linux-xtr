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

#define DVR_GNR_MODE				0xb8017000U
#define DVR_GNR_EN				0xb8017004U
#define DVR_GNR_INT				0xb8017008U
#define DVR_CFG_CT				0xb801700cU
#define DVR_CFG_EN				0xb8017010U
#define DVR_CFG_ST				0xb8017014U
#define DVR_CFG_ADDR				0xb8017018U
#define DVR_CFG_WDATA				0xb801701cU
#define DVR_CFG_RDATA				0xb8017020U

#define DVR_MIO_CT				0xb8017024U
#define DVR_MIO_ST				0xb8017028U
#define DVR_MIO_EN				0xb801702cU
#define DVR_MIO_ADDR				0xb8017030U
#define DVR_MIO_WDATA				0xb8017034U
#define DVR_MIO_RDATA				0xb8017038U
#define DVR_DMA_CT				0xb801703cU
#define DVR_DMA_ST				0xb8017040U
#define DVR_DMA_EN				0xb8017044U
#define DVR_DMA_PCI				0xb8017048U
#define DVR_DMA_DDR				0xb801704cU
#define DVR_DMA_CNT				0xb8017050U
#define DVR_DMA_DES0				0xb8017054U
#define DVR_DMA_DES1				0xb8017058U
#define DVR_DMA_DES2				0xb801705cU
#define DVR_DMA_DES3				0xb8017060U
#define DVR_SOFT_RST				0xb8017064U
#define DVR_CLK_ST				0xb8017068U
#define DVR_SPC_CT				0xb801706cU
#define DVR_SPC_ST				0xb8017070U
#define DVR_DBG					0xb8017074U
#define DVR_SCTCH				0xb8017078U
#define DVR_TMP_REG				0xb801707cU
#define DVR_DEV_ST				0xb8017080U
#define DVR_DEV_CT				0xb8017084U
#define DVR_DEV_ADR				0xb8017088U
#define DVR_DEV_CNT				0xb801708cU
#define DVR_TMP0_REG				0xb8017090U

#define CFG(addr)	0xb8017200 + addr 


#endif  /* __PCI_MARS_H */
