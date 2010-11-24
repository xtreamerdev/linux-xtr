	


#ifndef __RTL8711_MACCTRL_BITDEF_H__
#define __RTL8711_MACCTRL_BITDEF_H__

/*---------------------------------------------------------------------------

		Below is for the TXCMD bit definition
									
---------------------------------------------------------------------------*/
#define OWN_FW_MSK		0x80000000
#define OWN_FW_SHT		31

#define MORE_FRAG_MSK		0x40000000
#define MORE_FRAG_SHT		30


#define INTERRUPT_MSK		0x20000000
#define INTERRUPT_SHT		29

#define SPLCP_MSK		0x10000000
#define SPLCP_SHT		28

#define NO_ECRYPT_MSK		0x08000000
#define NO_ENCRYPT_SHT		27

#define FRAME_LEN_MSK		0x07FF0000
#define FRAME_LEN_SHT		16

#define MACID_MSK		0x0000FC00
#define MACID_SHT		10
#define MACID_BIT		0x3F


#define TID_MSK			0x3E0
#define TID_SHT			5


#define QOS_MSK			0x00000020
#define QOS_SHT			5

#define HWEOSP_MSK		0x00000010
#define HWEOSP_SHT		4

#define CTRL_MSK		0x00000008
#define CTRL_SHT		3

#define MGT_MSK			0x00000004
#define MGT_SHT			2

#define BLOCK_MSK		0x00000002
#define BLOCK_SHT		1


#define BURST_MSK		0x00000001
#define BURST_SHT		0

#define TXRATE_SHT		4
#define VCRATE_SHT		0

//Below is for TXCMD offset8
#define OWN_MAC_MSK		0x80000000
#define OWN_MAC_SHT		31

#define TXOK_MSK		0x40000000
#define TXOK_SHT		30

#define PWRMGT_MSK		0x20000000
#define PWRMGT_SHT		29

#define RETXSTATUS_MSK		0x1F000000
#define	RETXSTATUS_SHT		24

//------------ Security Control Register  ---------
// WPA_CONFIG
#define RX_WPA_DUMMY			BIT(8)
#define DISABLE_RX_AES_MIC		BIT(3)
#define RX_DECRYPT			BIT(2)
#define TX_ENCRYPT			BIT(1)
#define USING_DEFAULT_KEY		BIT(0)


/*---------------------------------------------------------------------------

		Below is for the TX/RX HW Queue definition
									
---------------------------------------------------------------------------*/

#define 	TXQUEUES	7
#define 	RXQUEUES	2

#define VO_QUEUE_INX	0
#define VI_QUEUE_INX	1
#define BE_QUEUE_INX	2
#define BK_QUEUE_INX	3
#define TS_QUEUE_INX	4
#define MGT_QUEUE_INX	5
#define BMC_QUEUE_INX	6
#define BCN_QUEUE_INX	7

#define HW_QUEUE_ENTRY	8



#define APSDOFF		BIT(5)
#define RST				BIT(4)
#define RXEN			BIT(3)
#define TXEN			BIT(2)




#define	BCNQ_TXEN		BIT(23)
#define BMQ_TXEN		BIT(22)
#define MGTQ_TXEN		BIT(21)
#define TSQ_TXEN		BIT(20)
#define BKQ_TXEN		BIT(19)
#define BEQ_TXEN		BIT(18)
#define VIQ_TXEN		BIT(17)
#define VOQ_TXEN		BIT(16)

#define ENABLE_ALLQ	( BCNQ_TXEN | BMQ_TXEN | MGTQ_TXEN | TSQ_TXEN \
			| BKQ_TXEN |  BEQ_TXEN | VIQ_TXEN | VOQ_TXEN)

#define RXEN0			BIT(0)
#define RXEN1			BIT(1)


/*---------------------------------------------------------------------------

		Below is for the Encryp/Decryp CAM bit definition
									
---------------------------------------------------------------------------*/

#define 	CAMSIZE		64


#define 	NumChannels	26


#define		_1M_RATE_	0
#define		_2M_RATE_	1
#define		_5M_RATE_	2
#define		_11M_RATE_	3
#define		_6M_RATE_	4
#define		_9M_RATE_	5
#define		_12M_RATE_	6
#define		_18M_RATE_	7
#define		_24M_RATE_	8
#define		_36M_RATE_	9
#define		_48M_RATE_	10
#define		_54M_RATE_	11


#define		EN_1MRATE	(1 << (_1M_RATE_))
#define		EN_2MRATE	(1 << (_2M_RATE_))
#define		EN_5MRATE	(1 << (_5M_RATE_))
#define		EN_11MRATE	(1 << (_11M_RATE_))
#define		EN_6MRATE	(1 << (_6M_RATE_))
#define		EN_9MRATE	(1 << (_9M_RATE_))
#define		EN_12MRATE	(1 << (_12M_RATE_))
#define		EN_18MRATE	(1 << (_18M_RATE_))
#define		EN_24MRATE	(1 << (_24M_RATE_))
#define		EN_36MRATE	(1 << (_36M_RATE_))
#define		EN_48MRATE	(1 << (_48M_RATE_))
#define		EN_54MRATE	(1 << (_54M_RATE_))




#endif // __RTL8711_MACCTRL_BITDEF_H__

