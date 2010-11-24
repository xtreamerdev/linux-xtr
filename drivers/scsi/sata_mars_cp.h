#ifndef _MARS_CPREG
#define _MARS_CPREG

#define CP_DEBUG
#ifdef CP_DEBUG
	#define satacpinfo(fmt, args...) \
            printk(KERN_INFO "sata-cp info: " fmt, ## args)
#else
	#define satacpinfo(fmt, args...)
#endif

#define     MARS_ACP_BASE		0xb8015000
#define     MARS_MCP_BASE		0xb8015100

#define     ACP_CTRL			MARS_ACP_BASE+0x00	/*0xb8015000*/
#define     ACP_CP0_KEY0    	MARS_ACP_BASE+0x04	/*0xb8015004*/
#define     ACP_CP0_KEY1    	MARS_ACP_BASE+0x08	/*0xb8015008*/
#define     ACP_CP0_KEY2    	MARS_ACP_BASE+0x0C	/*0xb801500C*/
#define     ACP_CP0_KEY3    	MARS_ACP_BASE+0x10	/*0xb8015010*/

#define     ACP_CP1_KEY0    	MARS_ACP_BASE+0x14	/*0xb8015014*/
#define     ACP_CP1_KEY1    	MARS_ACP_BASE+0x18	/*0xb8015018*/
#define     ACP_CP1_KEY2    	MARS_ACP_BASE+0x1C	/*0xb801501C*/
#define     ACP_CP1_KEY3    	MARS_ACP_BASE+0x20	/*0xb8015020*/
#define     ACP_INI_KEY0    	MARS_ACP_BASE+0x24	/*0xb8015024*/
#define     ACP_INI_KEY1    	MARS_ACP_BASE+0x28	/*0xb8015028*/
#define     ACP_INI_KEY2    	MARS_ACP_BASE+0x2C	/*0xb801502C*/
#define     ACP_INI_KEY3    	MARS_ACP_BASE+0x30	/*0xb8015030*/
#define     ACP_REG_KEY0    	MARS_ACP_BASE+0x34	/*0xb8015034*/
#define     ACP_REG_KEY1    	MARS_ACP_BASE+0x38	/*0xb8015038*/
#define     ACP_REG_KEY2    	MARS_ACP_BASE+0x3C	/*0xb801503C*/
#define     ACP_REG_KEY3    	MARS_ACP_BASE+0x40	/*0xb8015040*/
#define     ACP_REG_DATAIN0  	MARS_ACP_BASE+0x44	/*0xb8015044*/
#define     ACP_REG_DATAIN1  	MARS_ACP_BASE+0x48	/*0xb8015048*/
#define     ACP_REG_DATAIN2  	MARS_ACP_BASE+0x4C	/*0xb801504C*/
#define     ACP_REG_DATAIN3  	MARS_ACP_BASE+0x50	/*0xb8015050*/
#define     ACP_REG_DATAOUT0 	MARS_ACP_BASE+0x54	/*0xb8015054*/
#define     ACP_REG_DATAOUT1 	MARS_ACP_BASE+0x58	/*0xb8015058*/
#define     ACP_REG_DATAOUT2 	MARS_ACP_BASE+0x5C	/*0xb801505C*/
#define     ACP_REG_DATAOUT3 	MARS_ACP_BASE+0x60	/*0xb8015060*/
#define     ACP_REG_SET   		MARS_ACP_BASE+0x64	/*0xb8015064*/
#define     ACP_SW_CLEAR    	MARS_ACP_BASE+0x68	/*0xb8015068*/
#define     ACP_DBG    			MARS_ACP_BASE+0x6C	/*0xb801506C*/
#define     ACP_INTE0    		MARS_ACP_BASE+0x70	/*0xb8015070*/
#define     ACP_INTS0    		MARS_ACP_BASE+0x74	/*0xb8015074*/
#define     ACP_INTE1    		MARS_ACP_BASE+0x78	/*0xb8015078*/
#define     ACP_INTS1    		MARS_ACP_BASE+0x7C	/*0xb801507C*/
#define     ACP_SCTCH    		MARS_ACP_BASE+0x80	/*0xb8015080*/
#define     ACP_LCNT    		MARS_ACP_BASE+0x84	/*0xb8015084*/
#define     ACP_ST    			MARS_ACP_BASE+0x90	/*0xb8015090*/
#define     MCP_ST    			MARS_ACP_BASE+0x94	/*0xb8015094*/

#define     MCP_CTRL			MARS_MCP_BASE+0x00	/*0xb8015100*/
#define     MCP_STATUS      	MARS_MCP_BASE+0x04	/*0xb8015104*/
#define     MCP_EN          	MARS_MCP_BASE+0x08	/*0xb8015108*/
#define     MCP_BASE        	MARS_MCP_BASE+0x0c	/*0xb801510c*/
#define     MCP_LIMIT       	MARS_MCP_BASE+0x10	/*0xb8015110*/
#define     MCP_RDPTR       	MARS_MCP_BASE+0x14	/*0xb8015114*/
#define     MCP_WRPTR       	MARS_MCP_BASE+0x18	/*0xb8015118*/
#define     MCP_DES_INI_KEY0	MARS_MCP_BASE+0x1c	/*0xb801511c*/
#define     MCP_DES_INI_KEY1	MARS_MCP_BASE+0x20	/*0xb8015120*/
#define     MCP_AES_INI_KEY0 	MARS_MCP_BASE+0x24	/*0xb8015124*/
#define     MCP_AES_INI_KEY1 	MARS_MCP_BASE+0x28	/*0xb8015128*/
#define     MCP_AES_INI_KEY2 	MARS_MCP_BASE+0x2C	/*0xb801512C*/
#define     MCP_AES_INI_KEY3 	MARS_MCP_BASE+0x30	/*0xb8015130*/
#define     MCP_DES_COUNT   	MARS_MCP_BASE+0x34	/*0xb8015134*/
#define     MCP_DES_COMPARE 	MARS_MCP_BASE+0x38	/*0xb8015138*/

#endif	/* _MARS_CPREG */
