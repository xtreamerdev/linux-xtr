#ifndef  __MARS_PCMCIA_H__
#define  __MARS_PCMCIA_H__



/*======================== Type Defines =======================*/ 
typedef unsigned int      UINT32;
typedef int               INT32;
typedef unsigned char     UINT8;
typedef int               ErrCode;

/*======================== Constant ===========================*/ 
#define R_ERR_SUCCESS                0
#define R_ERR_FAILED                -1
#define R_ERR_IRQ_NOT_AVALIABLE     -2


// runing state
#define PCMCIA_IDEL            0
#define PCMCIA_RUNING          1
#define PCMCIA_ACCESS_OK       0x80
#define PCMCIA_ACCESS_FAIL     0x81

// timming (unit jiffies = 10ms)
#define RESET_TIME_OUT         300
#define ACCESS_TIME_OUT        10


typedef struct {
    void (*ci_camchange_irq)(int slot, int change_type);
    void (*ci_frda_irq)(int slot);    			
}ci_int_handler;


/*==================== Function  Prototypes ==================*/
extern ErrCode rtdpc_registerCiIntHandler(ci_int_handler* p_isr);
extern ErrCode rtdpc_unregisterCiIntHandler(ci_int_handler* p_isr);
extern void    rtdpc_hw_reset(UINT32 slot);
extern ErrCode rtdpc_writeMem(UINT32 addr, UINT8 data, UINT32 mode, UINT32 slot);
extern UINT8   rtdpc_readMem (UINT32 addr, UINT32 mode, UINT32 slot);

#define rtdpc_writeAttrMem(addr, data, slot)      rtdpc_writeMem(addr, data, 1, slot) 
#define rtdpc_writeIO_DRFR(addr, data, slot)      rtdpc_writeMem(addr, data, 0, slot) 
#define rtdpc_writeIO(addr, data, slot)           rtdpc_writeMem(addr, data, 0, slot) 

#define rtdpc_readAttrMem(addr, slot)             rtdpc_readMem(addr, 1, slot)
#define rtdpc_readIO_DRFR(addr, slot)             rtdpc_readMem(addr, 0, slot)
#define rtdpc_readIO(addr, slot)                  rtdpc_readMem(addr, 0, slot)

#endif // __MARS_PCMCIA_H__
