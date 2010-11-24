	


#ifndef __RTL8711_INTCTRL_BITDEF_H__
#define __RTL8711_INTCTRL_BITDEF_H__

#define	SETVOFWOWN	BIT(12)
#define	SETVIFWOWN	BIT(13)
#define	SETBEFWOWN	BIT(14)
#define	SETBKFWOWN	BIT(15)
#define	SETTSFWOWN	BIT(16)
#define	SETBMFWOWN	BIT(18)

#define SETFWOWN		(SETVOFWOWN | SETVIFWOWN | SETBEFWOWN | \
				SETBKFWOWN | SETTSFWOWN | SETBMFWOWN)

#define SETFWOWN_SHIFT	12


#define	VOCMDCL			BIT(3)
#define	VICMDCL			BIT(4)
#define	BECMDCL			BIT(5)
#define	BKCMDCL			BIT(6)
#define	TSCMDCL			BIT(7)
#define	MGTCMDCL		BIT(8)
#define	BMCMDCL			BIT(9)

#define TXCMDCL			(VOCMDCL | VICMDCL | BECMDCL | \
				/*BKCMDCL | TSCMDCL | MGTCMDCL | BMCMDCL )*/\
				BKCMDCL | MGTCMDCL | BMCMDCL )
										
#define TXCMDCL_SHIFT	3										



#define RXCMD0CL		BIT(0)
#define RXCMD1CL		BIT(1)
#define RXCMDCL			(RXCMD0CL | RXCMD1CL)
#define RXERR			BIT(2)

#define BCNINT			BIT(10)


//HISR
#if 0
#define _SCSIR_FULL		BIT(19)
#define _SCSIW_EMPTY	BIT(18)
#define _CPWM			BIT(17)
#define _WATCHDOG		BIT(16)
#define _C2HSET			BIT(15)
#define _H2CCLR			BIT(14)
#endif

// HISR
#define	_RXDONE			BIT(0)
#define	_RXERR				BIT(1)
#define	_VOACINT			BIT(2)
#define	_VIACINT			BIT(3)
#define	_BEACINT			BIT(4)
#define	_BKACINT			BIT(5)
#define	_TSSAINT			BIT(6)

#define	_BMACINT			BIT(8)
#define	_TXFFERR			BIT(9)
#define	_TS_START			BIT(10)
#define	_TS_FAILED			BIT(11)
#define	_TS_END_INT		BIT(12)
#define	_FREETXINT			BIT(13)
#define	_H2CCLR			BIT(14)
#define	_C2HSET			BIT(15)
#define	_WATCHDOG			BIT(16)
#define	_CPWM				BIT(17)
#define	_SCSIW_EMPTY		BIT(18)
#define	_SCSIR_FULL		BIT(19)
#define	_RX0CMDERR		BIT(20)
#if 0
//SYSISR
#define _GPIOINT		BIT(14)
#define _C2HCLR			BIT(13)
#define _H2CSET			BIT(12)
#define _SCSIR_EMPTY	BIT(11)
#define _SCSIW_FULL		BIT(10)
#define _RPWM			BIT(9)
#define _DMACHGR1OK		BIT(4)
#define _DMACHGR0OK		BIT(3)
#define _GTMINT			BIT(2)
#endif

/*---------------------------------------------------------------------------
									Below is for the SYSISR/SYSIMR bit definition
---------------------------------------------------------------------------*/


#define WLANRX0INT_SHT	15

#define GPIOINT_SHT	14
#define GPIOINT	BIT(GPIOINT_SHT)

#define C2HCLR_SHT	13
#define C2HCLR  BIT(C2HCLR_SHT)

#define H2CSET_SHT	12
#define H2CSET  BIT(H2CSET_SHT)

#define SCSIR_EMPTY_SHT	11
#define SCSIR_EMPTY BIT(SCSIR_EMPTY_SHT)

#define SCSIW_FULL_SHT	10
#define SCSIW_FULL BIT(SCSIW_FULL_SHT)

#define RPWM_SHT    9
#define RPWM BIT(RPWM_SHT)

#define UTINT_SHT   8
#define UTINT BIT(UTINT_SHT)

#define FCINT_SHT	7
#define FCINT BIT(FCINT_SHT)

//5 and 6 is reserved for WLAN_TXDSR/WLAN_RXDSR

#define WLANTXINT_SHT	6
#define WLANRX1INT_SHT	5


#define DMACHGR1OK_SHT	4
#define DMACHGR1OK BIT(DMACHGR1OK_SHT)

#define DMACHGR0OK_SHT 3
#define DMACHGR0OK BIT(DMACHGR0OK_SHT)

#define GTMINT_SHT		2
#define GTMINT BIT(GTMINT_SHT)

#define LBS1ERR_SHT 1
#define LBS1ERR BIT(LBS1ERR_SHT)

#define LBS0ERR_SHT	0
#define LBS0ERR BIT(LBS0ERR_SHT)

#if 0
#define SYSIMRON		(GTMINT | DMACHGR0OK | DMACHGR1OK | 				\
										FCINT | RPWMINT | SCSIWINT | SCSIRINT | H2CSET | C2HCLR)
#endif

#define SYSIMRON		(GTMINT | H2CSET | C2HCLR )


/*---------------------------------------------------------------------------
									Below is for the RTISR/RTIMR bit definition
---------------------------------------------------------------------------*/

#define VORETXINT		BIT(0)
#define VIRETXINT		BIT(1)
#define BERETXINT		BIT(2)
#define BKRETXINT		BIT(3)
#define TSRETXINT		BIT(4)
#define MGTRETXINT	BIT(5)
#define FWMAC				BIT(6)
#define FWMACOK			BIT(7)
#define FWMACERR		BIT(8)




#endif // __RTL8711_INTCTRL_BITDEF_H__

