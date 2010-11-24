#ifndef __RTL8711_SDIO_BITDEF_H__
#define __RTL8711_SDIO_BITDEF_H__

//------------------------------HCI Bit ( FA  means "formated address")
#define _HFFR_HWFF0_CLR			BIT(1) // When set, it indicates the Host Write FIFO0 is empty
#define _HFFR_HRFF0_READTY		BIT(2) // if 1: indicate data transfer from MAC is done
#define _HFFR_RRFF_READTY			BIT(8)// if 1: indicate data transfer from internal register is done
#define _RSTMSKOFF_RSTMSKOFF		BIT(0)// HCI Reset Mask Off. if 1: 8711 will ignore HCI BUS Reset command
												     //if 1: 8711 will be reset by HCI BUS Reset cmd

#define _SLEEPMSKOFF_SLEEPMSKOFF	BIT(0)// HCI Sleep Mask Off. if 1: 8711 will ignore HCI BUS Sleep command
															//if 0: LDO will turn off due to HCI BUS Sleep command



#define _HSYSCLK_HHSSEL			BIT(1) // High Speed Select
#define _HSYSCLK_HLXBUS1S			BIT(2) // 
#define _HSYSCLK_HVCODISABLE		BIT(4) // VCO Disable
#endif
