#include <drv_types.h>



struct _cmd_funcs {
	u32	cmd_code;
	u8 (*funcs)(_adapter  *padapter, u8 *pbuf);
};

extern unsigned char joinbss_hdl(_adapter *padapter,unsigned char *pbuf);	
extern unsigned char disconnect_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char createbss_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setopmode_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char sitesurvey_hdl(_adapter *padapter,unsigned char *pbuf);	
extern unsigned char setauth_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setkey_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char set_stakey_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char set_assocsta_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char del_assocsta_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setstapwrstate_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setbasicrate_hdl(_adapter *padapter,unsigned char *pbuf);	
extern unsigned char getbasicrate_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setdatarate_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char getdatarate_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setphyinfo_hdl(_adapter *padapter,unsigned char *pbuf);	
extern unsigned char getphyinfo_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setphy_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char getphy_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char readBB_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char writeBB_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char readRF_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char writeRF_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char readRssi_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char readGain_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setrfintfs_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char getrfintfs_hdl(_adapter *padapter,unsigned char *pbuf);


#ifdef CONFIG_H2CLBK
extern unsigned char seth2clbk_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char geth2clbk_hdl(_adapter *padapter,unsigned char *pbuf);
#endif

#ifdef CONFIG_POWERSAVING
extern unsigned char setatim_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setpwrmode_hdl(_adapter *padapter,unsigned char *pbuf);
#endif

/* sunghau@0329: for RA cmd */
extern unsigned char setratable_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char getratable_hdl(_adapter *padapter,unsigned char *pbuf);
extern unsigned char setusbsuspend_hdl(_adapter *padapter,unsigned char *pbuf);

struct _cmd_funcs 	cmd_funcs[] = {
	{_JoinBss_CMD_, &joinbss_hdl},//select_and_join_from_scanned_queue	0
	{_DisConnect_CMD_, &disconnect_hdl}, //MgntActSet_802_11_DISASSOCIATE
	{_CreateBss_CMD_, &createbss_hdl},
	{_SetOpMode_CMD_,&setopmode_hdl},
	{_SiteSurvey_CMD_, &sitesurvey_hdl}, //MgntActSet_802_11_BSSID_LIST_SCAN
	{_SetAuth_CMD_,&setauth_hdl},
	{_SetKey_CMD_,&setkey_hdl},
	{_SetStaKey_CMD_,&set_stakey_hdl},
	{_SetAssocSta_CMD_, &set_assocsta_hdl},
	{_DelAssocSta_CMD_,&del_assocsta_hdl},
	{_SetStaPwrState_CMD_,&setstapwrstate_hdl},	//10
	{_SetBasicRate_CMD_,&setbasicrate_hdl},
	{_GetBasicRate_CMD_,&getbasicrate_hdl},
	{_SetDataRate_CMD_,&setdatarate_hdl},
	{_GetDataRate_CMD_,&getdatarate_hdl},
	{_SetPhyInfo_CMD_,&setphyinfo_hdl},
	{_GetPhyInfo_CMD_,&getphyinfo_hdl},
	{_SetPhy_CMD_,&setphy_hdl},
	{_GetPhy_CMD_,&getphy_hdl},	
	{GEN_CMD_CODE(_GetBBReg),&readBB_hdl},
	{GEN_CMD_CODE(_SetBBReg),&writeBB_hdl},	//20
	{GEN_CMD_CODE(_GetRFReg),&readRF_hdl},
	{GEN_CMD_CODE(_SetRFReg),&writeRF_hdl},
	{GEN_CMD_CODE(_readRssi),&readRssi_hdl},
	{GEN_CMD_CODE(_readGain),&readGain_hdl},	
	{GEN_CMD_CODE(_SetRFIntFs),&setrfintfs_hdl},
	{GEN_CMD_CODE(_GetRFIntFs),&getrfintfs_hdl},
#ifdef CONFIG_PWRCTRL
	{_SetAtim_CMD_, &setatim_hdl},
	{_SetPwrMode_CMD_, &setpwrmode_hdl},	//28
#endif
	{_SetRaTable_CMD_, &setratable_hdl},
	{_GetRaTable_CMD_, &getratable_hdl},
	{_SetUsbSuspend_CMD_, &setusbsuspend_hdl}
};

/*
	GEN_CMD_CODE(_JoinBss) ,
	GEN_CMD_CODE(_DisConnect) ,
	GEN_CMD_CODE(_CreateBss) ,
	GEN_CMD_CODE(_SetOpMode) ,	
	GEN_CMD_CODE(_SiteSurvey) ,
	GEN_CMD_CODE(_SetAuth) ,
	GEN_CMD_CODE(_SetKey) ,
	GEN_CMD_CODE(_SetStaKey) ,
	GEN_CMD_CODE(_SetAssocSta) ,
	GEN_CMD_CODE(_DelAssocSta) ,
	GEN_CMD_CODE(_SetStaPwrState) ,
	GEN_CMD_CODE(_SetBasicRate) ,
	GEN_CMD_CODE(_GetBasicRate) ,
	GEN_CMD_CODE(_SetDataRate) ,
	GEN_CMD_CODE(_GetDataRate) ,
	GEN_CMD_CODE(_SetPhyInfo) ,
	GEN_CMD_CODE(_GetPhyInfo) ,
	GEN_CMD_CODE(_SetPhy) ,
	GEN_CMD_CODE(_GetPhy) ,
*/	



extern thread_return mlme_thread(thread_context	context);
