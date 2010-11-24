#ifndef __SCD_ATR_H__
#define __SCD_ATR_H__


//-------------------------------------------------------
// misc
//-------------------------------------------------------


typedef struct 
{
    unsigned char status;           // parse status
    unsigned char progress;         // parse progress
    
    unsigned char T1Len;
    unsigned char T2Len;
    unsigned char T3Len;
    unsigned char nHistoryBytes;
    
    //----------  info          
    unsigned char TS;
    unsigned char T0;
    unsigned char T1[4];    
    unsigned char T2[4];
    unsigned char T3[4];    
    unsigned char History[16];
    unsigned char TCK;                        
}scd_atr_info;


typedef struct 
{
    #define MAX_ATR_SIZE    32    
    unsigned char data[MAX_ATR_SIZE];
    unsigned char length;
}scd_atr;


#define ATR_COMPLETE        0x80


typedef enum {
    VALID_ATR,
    INVALID_ATR,
    INCOMPLETE_ATR,
}atr_status;


extern int  is_atr_complete(scd_atr* p_atr);
extern int  decompress_atr(scd_atr* p_atr, scd_atr_info* p_info);
extern void init_atr(scd_atr* p_atr);

#define SC_ATR_DBG          printk

#endif  //__SCD_ATR_H__

