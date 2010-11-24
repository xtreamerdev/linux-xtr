/*****************************************************************************
**
**  Name: mt2060.c
**
**  Description:    Microtune MT2060 Tuner software interface.
**                  Supports tuners with Part/Rev code: 0x63.
**
**  Functions
**  Implemented:    UData_t  MT2060_Open
**                  UData_t  MT2060_Close
**                  UData_t  MT2060_ChangeFreq
**                  UData_t  MT2060_GetFMF
**                  UData_t  MT2060_GetGPO
**                  UData_t  MT2060_GetIF1Center (use MT2060_GetParam instead)
**                  UData_t  MT2060_GetLocked
**                  UData_t  MT2060_GetParam
**                  UData_t  MT2060_GetReg
**                  UData_t  MT2060_GetTemp
**                  UData_t  MT2060_GetUserData
**                  UData_t  MT2060_LocateIF1
**                  UData_t  MT2060_ReInit
**                  UData_t  MT2060_SetExtSRO
**                  UData_t  MT2060_SetGPO
**                  UData_t  MT2060_SetGainRange
**                  UData_t  MT2060_SetIF1Center (use MT2060_SetParam instead)
**                  UData_t  MT2060_SetParam
**                  UData_t  MT2060_SetReg
**                  UData_t  MT2060_SetUserData
**
**  References:     AN-00084: MT2060 Programming Procedures Application Note
**                  MicroTune, Inc.
**                  AN-00010: MicroTuner Serial Interface Application Note
**                  MicroTune, Inc.
**
**  Exports:        None
**
**  Dependencies:   MT_ReadSub(hUserData, IC_Addr, subAddress, *pData, cnt);
**                  - Read byte(s) of data from the two-wire bus.
**
**                  MT_WriteSub(hUserData, IC_Addr, subAddress, *pData, cnt);
**                  - Write byte(s) of data to the two-wire bus.
**
**                  MT_Sleep(hUserData, nMinDelayTime);
**                  - Delay execution for x milliseconds
**
**  CVS ID:         $Id: mt2060.c,v 1.8 2005/11/02 16:23:41 software Exp $
**  CVS Source:     $Source: /export/home/cvsroot/web05/html/software/tuners/MT2060/mt2060.c,v $
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   03-02-2004    DAD    Adapted from mt2060.c source file.
**   N/A   04-20-2004    JWS    Modified to support handle-based calls
**   N/A   05-04-2004    JWS    Upgrades for A2/A3.
**   N/A   07-22-2004    DAD    Added function MT2060_SetGainRange.
**   N/A   09-03-2004    DAD    Add mean Offset of FMF vs. 1220 MHz over 
**                              many devices when choosing the spline center
**                              point.
**   N/A   09-03-2004    DAD    Default VGAG to 1 (was 0).
**   N/A   09-10-2004    DAD    Changed LO1 FracN avoid region from 999,999
**                              to 2,999,999 Hz
**   078   01-26-2005    DAD    Ver 1.11: Force FM1CA bit to 1
**   079   01-20-2005    DAD    Ver 1.11: Fix setting of wrong bits in wrong
**                              register.
**   080   01-21-2005    DAD    Ver 1.11: Removed registers 0x0E through 0x11
**                              from the initialization routine.  All are
**                              reserved and not used.
**   081   01-21-2005    DAD    Ver 1.11: Changed LNA Band names to RF Band
**                              to match description in the data sheet.
**   082   02-01-2005    JWS    Ver 1.11: Support multiple tuners if present.
**   083   02-08-2005    JWS    Ver 1.11: Added separate version number for
**                              expected version of mt_spuravoid.h
**   084   02-08-2005    JWS    Ver 1.11: Reduced LO-related harmonics search
**                              from 11 to 5.
**   085   02-08-2005    DAD    Ver 1.11: Removed support for all but MT2060A3.
**   086   02-08-2005    JWS    Ver 1.11: Added call to MT_RegisterTuner().
**   088   01-26-2005    DAD    Added MT_TUNER_INIT_ERR.
**   089   02-17-2005    DAD    Ver 1.12: Fix bug in MT2060_SetReg() where
**                              the register was not getting set properly.
**   093   04-05-2005    DAD    Ver 1.13: MT2060_SetGainRange() is missing a
**                              break statement before the default handler.
**   095   04-05-2005    JWS    Ver 1.13: Clean up better if registration
**                              of the tuner fails in MT2060_Open.
**   099   06-21-2005    DAD    Ver 1.14: Increment i when waiting for MT2060
**                              to finish initializing.
**   102   08-16-2005    DAD    Ver 1.15: Fixed-point version added.  Location
**                              of 1st IF filter center requires 9-11 samples
**                              instead of 7 for floating-point version.
**   100   09-15-2005    DAD    Ver 1.15: Fixed register where temperature
**                              field is located (was MISC_CTRL_1).
**   101   09-15-2005    DAD    Ver 1.15: Re-wrote max- and zero-finding
**                              routines to take advantage of known cubic
**                              polynomial coefficients.
**
*****************************************************************************/
#include "../dvb-dibusb.h"  // Added for NULL
//#include <stdlib.h>       // for NULL
#include "mt2060.h"
#include "mt_spuravoid.h"

#define fabs(y_err)     ((y_err<0) ? -1* y_err : y_err)


double sqrt(double num)
{
    double x,x0;
    x0 = num;
    
    for(;;) {
        x = (x0 + num/x0) /2;
        if (x >= x0)
            break;
        x = x0;            
    }    
    return x0;
}
 

/*  Version of this module                         */
#define VERSION 10105             /*  Version 01.15 */

/*
**  By default, the floating-point FIFF location algorithm is used.
**  It provides faster FIFF location using fewer measurements, but
**  requires the use of a floating-point processor.
**
**
**       Interpolation | Processor  | Number of
**          Method     | Capability | Measurements
**      ---------------+------------+--------------
**       Cubic-spline  | floating   |      7
**       Linear        | fixed      |    9-11
**
**
**  See AN-00102 (Rev 1.1 or higher) for a detailed description of the 
**  differences between the fixed-point and floating-point algorithm.
**
**  To use the fixed-point version of the FIFF center frequency location,
**  comment out the next source line.
*/
#define __FLOATING_POINT__

/*
**  The expected version of MT_AvoidSpursData_t
**  If the version is different, an updated file is needed from Microtune
*/
/* Expecting version 1.20 of the Spur Avoidance API */
#define EXPECTED_MT_AVOID_SPURS_INFO_VERSION  010200

#if MT_AVOID_SPURS_INFO_VERSION < EXPECTED_MT_AVOID_SPURS_INFO_VERSION
#error Contact Microtune for a newer version of MT_SpurAvoid.c
#elif MT_AVOID_SPURS_INFO_VERSION > EXPECTED_MT_AVOID_SPURS_INFO_VERSION
#error Contact Microtune for a newer version of mt2060.c
#endif

#ifndef MT2060_CNT
#error You must define MT2060_CNT in the "mt_userdef.h" file
#endif


/*
**  Two-wire serial bus subaddresses of the tuner registers.
**  Also known as the tuner's register addresses.
*/
static enum MT2060_Register_Offsets
{
    PART_REV = 0,   /*  0x00: Part/Rev Code        */
    LO1C_1,         /*  0x01: LO1C Byte 1          */
    LO1C_2,         /*  0x02: LO1C Byte 2          */
    LO2C_1,         /*  0x03: LO2C Byte 1          */
    LO2C_2,         /*  0x04: LO2C Byte 2          */
    LO2C_3,         /*  0x05: LO2C Byte 3          */
    LO_STATUS,      /*  0x06: LO Status Byte       */
    FM_FREQ,        /*  0x07: FM FREQ Byte         */
    MISC_STATUS,    /*  0x08: Misc Status Byte     */
    MISC_CTRL_1,    /*  0x09: Misc Ctrl Byte 1     */
    MISC_CTRL_2,    /*  0x0A: Misc Ctrl Byte 2     */
    MISC_CTRL_3,    /*  0x0B: Misc Ctrl Byte 3     */
    LO1_1,          /*  0x0C: LO1 Byte 1           */
    LO1_2,          /*  0x0D: LO1 Byte 2           */
    END_REGS
};


/*
**  RF Band cross-over frequencies
*/
static UData_t MT2060_RF_Bands[] = 
{
     95000000L,     /*    0 ..  95 MHz: b1011      */
    180000000L,     /*   95 .. 180 MHz: b1010      */
    260000000L,     /*  180 .. 260 MHz: b1001      */
    335000000L,     /*  260 .. 335 MHz: b1000      */
    425000000L,     /*  335 .. 425 MHz: b0111      */
    490000000L,     /*  425 .. 490 MHz: b0110      */
    570000000L,     /*  490 .. 570 MHz: b0101      */
    645000000L,     /*  570 .. 645 MHz: b0100      */
    730000000L,     /*  645 .. 730 MHz: b0011      */
    810000000L      /*  730 .. 810 MHz: b0010      */
                    /*  810 ..     MHz: b0001      */
};


/*
** Constants used by the tuning algorithm
*/
#define REF_FREQ          (16000000UL)  /* Reference oscillator Frequency (in Hz) */
#define IF1_FREQ        (1220000000UL)  /* Default IF1 Frequency (in Hz) */
#define IF1_CENTER      (1220000000UL)  /* Center of the IF1 filter (in Hz) */
#define IF1_BW            (22000000UL)  /* The IF1 filter bandwidth (in Hz) */
#define TUNE_STEP_SIZE       (50000UL)  /* Tune in steps of 50 kHz */
#define SPUR_STEP_HZ        (250000UL)  /* Step size (in Hz) to move IF1 when avoiding spurs */
#define LO1_STEP_SIZE       (250000UL)  /* Step size (in Hz) of PLL1 */
#define ZIF_BW             (3000000UL)  /* Zero-IF spur-free bandwidth (in Hz) */
#define MAX_HARMONICS_1          (5UL)  /* Highest intra-tuner LO Spur Harmonic to be avoided */ 
#define MAX_HARMONICS_2          (5UL)  /* Highest inter-tuner LO Spur Harmonic to be avoided */ 
#define MIN_LO_SEP         (1000000UL)  /* Minimum inter-tuner LO frequency separation */
#define LO1_FRACN_AVOID    (2999999UL)  /* LO1 FracN numerator avoid region (in Hz) */
#define LO2_FRACN_AVOID      (99999UL)  /* LO2 FracN numerator avoid region (in Hz) */
#define MIN_FIN_FREQ      (48000000UL)  /* Minimum input frequency (in Hz) */
#define MAX_FIN_FREQ     (860000000UL)  /* Maximum input frequency (in Hz) */
#define MIN_FOUT_FREQ     (30000000UL)  /* Minimum output frequency (in Hz) */
#define MAX_FOUT_FREQ     (60000000UL)  /* Maximum output frequency (in Hz) */
#define MIN_DNC_FREQ    (1041000000UL)  /* Minimum LO2 frequency (in Hz) */
#define MAX_DNC_FREQ    (1310000000UL)  /* Maximum LO2 frequency (in Hz) */
#define MIN_UPC_FREQ    (1088000000UL)  /* Minimum LO1 frequency (in Hz) */
#define MAX_UPC_FREQ    (2214000000UL)  /* Maximum LO1 frequency (in Hz) */


/*
**  The number of Tuner Registers
*/
static const UData_t Num_Registers = END_REGS;

typedef struct
{
    Handle_t    handle;
    Handle_t    hUserData;
    UData_t     address;
    UData_t     version;
    UData_t     tuner_id;
    MT_AvoidSpursData_t AS_Data;
    UData_t     f_IF1_actual;
    UData_t     num_regs;
    U8Data      reg[END_REGS];
    UData_t     RF_Bands[10];
}  MT2060_Info_t;

static UData_t nMaxTuners = MT2060_CNT;
static MT2060_Info_t MT2060_Info[MT2060_CNT];
static MT2060_Info_t *Avail[MT2060_CNT];
static UData_t nOpenTuners = 0;


/******************************************************************************
**
**  Name: MT2060_Open
**
**  Description:    Initialize the tuner's register values.
**
**  Parameters:     MT2060_Addr  - Serial bus address of the tuner.
**                  hMT2060      - Tuner handle passed back.
**                  hUserData    - User-defined data, if needed for the
**                                 MT_ReadSub() & MT_WriteSub functions.
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_TUNER_ID_ERR   - Tuner Part/Rev code mismatch
**                      MT_TUNER_INIT_ERR - Tuner initialization failed
**                      MT_COMM_ERR       - Serial bus communications error
**                      MT_ARG_NULL       - Null pointer argument passed
**                      MT_TUNER_CNT_ERR  - Too many tuners open
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   05-04-2004    JWS    Upgrades for A2/A3.
**   N/A   09-03-2004    DAD    Default VGAG to 1 (was 0).
**   N/A   09-09-2004    DAD    Changed function to pass back status.
**   N/A   09-09-2004    DAD    Removed on-chip location step.
**                              Manual location is required (once)
**                              The 1st IF center must be set using the
**                              MT2060_SetIF1Center() function.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   086   02-08-2005    JWS    Ver 1.11: Added call to MT_RegisterTuner().
**   095   04-05-2005    JWS    Ver 1.13: Clean up better if registration
**                              of the tuner fails in MT2060_Open.
**
******************************************************************************/
UData_t MT2060_Open(UData_t MT2060_Addr,
                    Handle_t*   hMT2060,
                    Handle_t    hUserData)
{
    UData_t status = MT_OK;             /*  Status to be returned.  */
    SData_t i;
    MT2060_Info_t* pInfo = NULL;

    /*  Check the argument before using  */
    if (hMT2060 == NULL)
        return MT_ARG_NULL;
        
    /*
    **  If this is our first tuner, initialize the address fields and
    **  the list of available control blocks.
    */
    if (nOpenTuners == 0)
    {
        for (i=MT2060_CNT-1; i>=0; i--)
        {
            MT2060_Info[i].handle = NULL;
            MT2060_Info[i].address = MAX_UDATA;
            MT2060_Info[i].hUserData = NULL;
            Avail[i] = &MT2060_Info[i];
        }
    }

    /*
    **  Look for an existing MT2060_State_t entry with this address.
    */
    for (i=MT2060_CNT-1; i>=0; i--)
    {
        /*
        **  If an open'ed handle provided, we'll re-initialize that structure.
        **
        **  We recognize an open tuner because the address and hUserData are
        **  the same as one that has already been opened
        */
        if (/*(MT2060_Info[i].handle == hMT2060) && */
            (MT2060_Info[i].address == MT2060_Addr) &&
            (MT2060_Info[i].hUserData == hUserData))
        {
            pInfo = &MT2060_Info[i];
            break;
        }
    }

    /*  If not found, choose an empty spot.  */
    if (pInfo == NULL)
    {
        /*  Check to see that we're not over-allocating.  */
        if (nOpenTuners == MT2060_CNT)
            return MT_TUNER_CNT_ERR;

        /* Use the next available block from the list */
        pInfo = Avail[nOpenTuners];
        nOpenTuners++;
    }

    if (MT_NO_ERROR(status))
        status |= MT_RegisterTuner(&pInfo->AS_Data);

    if (MT_NO_ERROR(status))
    {        
        pInfo->handle = (Handle_t) pInfo;
        pInfo->hUserData = hUserData;
        pInfo->address = MT2060_Addr;
        status |= MT2060_ReInit((Handle_t) pInfo);
    }
        
    if (MT_IS_ERROR(status)){
        MT2060_DBG("ReInit MT2060 Fail\n");
        /*  MT2060_Close handles the un-registration of the tuner  */
        MT2060_Close((Handle_t) pInfo);
    }
    else
        *hMT2060 = pInfo->handle;

    return (status);
}


static UData_t IsValidHandle(MT2060_Info_t* handle)
{
#if 1    
    if ((handle != NULL) && (handle->handle == handle))
        return 1;    
    else{
        MT2060_DBG("[ERROR][FETAL] : MT_INV_HANDLE\n");
        return 0;
    }
#else        
    return ((handle != NULL) && (handle->handle == handle)) ? 1 : 0;
#endif    
}


/******************************************************************************
**
**  Name: MT2060_Close
**
**  Description:    Release the handle to the tuner.
**
**  Parameters:     hMT2060      - Handle to the MT2060 tuner
**
**  Returns:        status:
**                      MT_OK         - No errors
**                      MT_INV_HANDLE - Invalid tuner handle
**
**  Dependencies:   mt_errordef.h - definition of error codes
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   09-13-2004    DAD    Original.
**   086   02-08-2005    JWS    Ver 1.11: Added call to MT_UnRegisterTuner().
**
******************************************************************************/
UData_t MT2060_Close(Handle_t hMT2060)
{
    MT2060_Info_t* pInfo = (MT2060_Info_t*) hMT2060;

    if (!IsValidHandle(pInfo))
        return MT_INV_HANDLE;

    /* Unregister tuner with SpurAvoidance routines (if needed) */
    MT_UnRegisterTuner(&pInfo->AS_Data);

    /* Now remove the tuner from our own list of tuners */
    pInfo->handle = NULL;
    pInfo->address = MAX_UDATA;
    pInfo->hUserData = NULL;
    nOpenTuners--;
    Avail[nOpenTuners] = pInfo; /* Return control block to available list */

    return MT_OK;
}


/******************************************************************************
**
**  Name: MT2060_GetFMF
**
**  Description:    Get a new FMF register value.
**                  Uses The single step mode of the MT2060's FMF function.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire-bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire-bus
**                  MT_Sleep    - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   05-04-2004    JWS    Upgrades for A2/A3.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
UData_t MT2060_GetFMF(Handle_t h)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    UData_t idx;                             /* loop index variable          */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    /*  Set the FM1 Single-Step bit  */
    pInfo->reg[LO2C_1] |= 0x40;
    status = MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);

    if (MT_NO_ERROR(status))
    {
        /*  Set the FM1CA bit to start the FMF location  */
        pInfo->reg[LO2C_1] |= 0x80;
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);
        pInfo->reg[LO2C_1] &= 0x7F;    /*  FM1CA bit automatically goes back to 0  */

        /*  Toggle the single-step 8 times  */
        for (idx=0; (idx<8) && (MT_NO_ERROR(status)); ++idx)
        {
            MT_Sleep(pInfo->hUserData, 20);
            /*  Clear the FM1 Single-Step bit  */
            pInfo->reg[LO2C_1] &= 0xBF;
            status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);

            MT_Sleep(pInfo->hUserData, 20);
            /*  Set the FM1 Single-Step bit  */
            pInfo->reg[LO2C_1] |= 0x40;
            status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);
        }
    }

    /*  Clear the FM1 Single-Step bit  */
    pInfo->reg[LO2C_1] &= 0xBF;
    status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);

    if (MT_NO_ERROR(status))
    {
        idx = 25;
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);

        /*  Poll FMCAL until it is set or we time out  */
        while ((MT_NO_ERROR(status)) && (--idx) && (pInfo->reg[MISC_STATUS] & 0x40) == 0)
        {
            MT_Sleep(pInfo->hUserData, 20);
            status = MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);
        }

        if ((MT_NO_ERROR(status)) && ((pInfo->reg[MISC_STATUS] & 0x40) != 0))
        {
            /*  Read the 1st IF center offset register  */
            status = MT_ReadSub(pInfo->hUserData, pInfo->address, FM_FREQ, &pInfo->reg[FM_FREQ], 1);
            if (MT_NO_ERROR(status))
            {
                pInfo->AS_Data.f_if1_Center = 1000000L * (1084 + pInfo->reg[FM_FREQ]);
            }
        }
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_GetGPO
**
**  Description:    Get the current MT2060 GPO setting(s).
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  gpo          - indicates which GPO pin(s) are read
**                                 MT2060_GPO_0 - GPO0 (MT2060 A1 pin 29)
**                                 MT2060_GPO_1 - GPO1 (MT2060 A1 pin 45)
**                                 MT2060_GPO   - GPO1 (msb) & GPO0 (lsb)
**                  *value       - value read from the GPO pin(s)
**
**                                 GPO1      GPO0
**                  gpo           pin 45    pin 29      *value
**                  ---------------------------------------------
**                  MT2060_GPO_0    X         0            0
**                  MT2060_GPO_0    X         1            1
**                  MT2060_GPO_1    0         X            0
**                  MT2060_GPO_1    1         X            1
**                  MT2060_GPO      0         0            0
**                  MT2060_GPO      0         1            1
**                  MT2060_GPO      1         0            2
**                  MT2060_GPO      1         1            3
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire-bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
UData_t MT2060_GetGPO(Handle_t h, UData_t gpo, UData_t *value)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    if (value == NULL)
        return MT_ARG_NULL;

    /*  We'll read the register just in case the write didn't work last time */
    status = MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_CTRL_1, &pInfo->reg[MISC_CTRL_1], 1);

    switch (gpo)
    {
    case MT2060_GPO_0:
    case MT2060_GPO_1:
        *value = pInfo->reg[MISC_CTRL_1] & gpo ? 1 : 0;
        break;

    case MT2060_GPO:
        *value = pInfo->reg[MISC_CTRL_1] & gpo >> 1;
        break;

    default:
        status |= MT_ARG_RANGE;
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_GetIF1Center
**
**  Description:    Gets the MT2060 1st IF frequency.
**
**                  This function will be removed in future versions of this
**                  driver.  Instead, use the following function call:
**
**                  status = MT2060_GetParam(hMT2060, 
**                                           MT2060_IF1_CENTER, 
**                                           &f_if1_Center);
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  *value       - MT2060 1st IF frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**
**  Dependencies:   None
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-15-2004    DAD    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
UData_t MT2060_GetIF1Center(Handle_t h, UData_t* value)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    if (value == NULL)
        return MT_ARG_NULL;

    *value = pInfo->AS_Data.f_if1_Center;

    return status;
}


/****************************************************************************
**
**  Name: MT2060_GetLocked
**
**  Description:    Checks to see if LO1 and LO2 are locked.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_UPC_UNLOCK    - Upconverter PLL unlocked
**                      MT_DNC_UNLOCK    - Downconverter PLL unlocked
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   MT_ReadSub    - Read byte(s) of data from the serial bus
**                  MT_Sleep      - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
****************************************************************************/
UData_t MT2060_GetLocked(Handle_t h)
{
    const UData_t nMaxWait = 200;            /*  wait a maximum of 200 msec   */
    const UData_t nPollRate = 2;             /*  poll status bits every 2 ms */
    const UData_t nMaxLoops = nMaxWait / nPollRate;
    UData_t status = MT_OK;                  /* Status to be returned        */
    UData_t nDelays = 0;
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    do
    {
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, LO_STATUS, &pInfo->reg[LO_STATUS], 1);

        if ((MT_IS_ERROR(status)) || ((pInfo->reg[LO_STATUS] & 0x88) == 0x88))
            return (status);

        MT_Sleep(pInfo->hUserData, nPollRate);       /*  Wait between retries  */
    }
    while (++nDelays < nMaxLoops);

    if ((pInfo->reg[LO_STATUS] & 0x80) == 0x00)
        status |= MT_UPC_UNLOCK;
    if ((pInfo->reg[LO_STATUS] & 0x08) == 0x00)
        status |= MT_DNC_UNLOCK;

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_GetParam
**
**  Description:    Gets a tuning algorithm parameter.
**
**                  This function provides access to the internals of the
**                  tuning algorithm - mostly for testing purposes.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  param       - Tuning algorithm parameter 
**                                (see enum MT2060_Param)
**                  pValue      - ptr to returned value
**
**                  param                   Description
**                  ----------------------  --------------------------------
**                  MT2060_IC_ADDR          Serial Bus address of this tuner
**                  MT2060_MAX_OPEN         Max # of MT2060's allowed open
**                  MT2060_NUM_OPEN         # of MT2060's open                  
**                  MT2060_SRO_FREQ         crystal frequency                   
**                  MT2060_STEPSIZE         minimum tuning step size            
**                  MT2060_INPUT_FREQ       input center frequency              
**                  MT2060_LO1_FREQ         LO1 Frequency                       
**                  MT2060_LO1_STEPSIZE     LO1 minimum step size               
**                  MT2060_LO1_FRACN_AVOID  LO1 FracN keep-out region           
**                  MT2060_IF1_ACTUAL       Current 1st IF in use               
**                  MT2060_IF1_REQUEST      Requested 1st IF                    
**                  MT2060_IF1_CENTER       Center of 1st IF SAW filter         
**                  MT2060_IF1_BW           Bandwidth of 1st IF SAW filter      
**                  MT2060_ZIF_BW           zero-IF bandwidth                   
**                  MT2060_LO2_FREQ         LO2 Frequency                       
**                  MT2060_LO2_STEPSIZE     LO2 minimum step size               
**                  MT2060_LO2_FRACN_AVOID  LO2 FracN keep-out region           
**                  MT2060_OUTPUT_FREQ      output center frequency             
**                  MT2060_OUTPUT_BW        output bandwidth                    
**                  MT2060_LO_SEPARATION    min inter-tuner LO separation       
**                  MT2060_AS_ALG           ID of avoid-spurs algorithm in use  
**                  MT2060_MAX_HARM1        max # of intra-tuner harmonics      
**                  MT2060_MAX_HARM2        max # of inter-tuner harmonics      
**                  MT2060_EXCL_ZONES       # of 1st IF exclusion zones         
**                  MT2060_NUM_SPURS        # of spurs found/avoided            
**                  MT2060_SPUR_AVOIDED     >0 spurs avoided                    
**                  MT2060_SPUR_PRESENT     >0 spurs in output (mathematically) 
**
**  Usage:          status |= MT2060_GetParam(hMT2060, 
**                                            MT2060_IF1_ACTUAL,
**                                            &f_IF1_Actual);
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Invalid parameter requested
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**  See Also:       MT2060_SetParam, MT2060_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-02-2004    DAD    Original.
**
****************************************************************************/
UData_t MT2060_GetParam(Handle_t     h,
                        MT2060_Param param,
                        UData_t*     pValue)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (pValue == NULL)
        status |= MT_ARG_NULL;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status |= MT_INV_HANDLE;

    if (MT_NO_ERROR(status))
    {
        switch (param)
        {
        /*  Serial Bus address of this tuner      */
        case MT2060_IC_ADDR:
            *pValue = pInfo->address;
            break;

        /*  Max # of MT2060's allowed to be open  */
        case MT2060_MAX_OPEN:
            *pValue = nMaxTuners;
            break;

        /*  # of MT2060's open                    */
        case MT2060_NUM_OPEN:
            *pValue = nOpenTuners;
            break;

        /*  crystal frequency                     */
        case MT2060_SRO_FREQ:
            *pValue = pInfo->AS_Data.f_ref;
            break;

        /*  minimum tuning step size              */
        case MT2060_STEPSIZE:
            *pValue = pInfo->AS_Data.f_LO2_Step;
            break;

        /*  input center frequency                */
        case MT2060_INPUT_FREQ:
            *pValue = pInfo->AS_Data.f_in;
            break;

        /*  LO1 Frequency                         */
        case MT2060_LO1_FREQ:
            *pValue = pInfo->AS_Data.f_LO1;
            break;

        /*  LO1 minimum step size                 */
        case MT2060_LO1_STEPSIZE:
            *pValue = pInfo->AS_Data.f_LO1_Step;
            break;

        /*  LO1 FracN keep-out region             */
        case MT2060_LO1_FRACN_AVOID:
            *pValue = pInfo->AS_Data.f_LO1_FracN_Avoid;
            break;

        /*  Current 1st IF in use                 */
        case MT2060_IF1_ACTUAL:
            *pValue = pInfo->f_IF1_actual;
            break;

        /*  Requested 1st IF                      */
        case MT2060_IF1_REQUEST:
            *pValue = pInfo->AS_Data.f_if1_Request;
            break;

        /*  Center of 1st IF SAW filter           */
        case MT2060_IF1_CENTER:
            *pValue = pInfo->AS_Data.f_if1_Center;
            break;

        /*  Bandwidth of 1st IF SAW filter        */
        case MT2060_IF1_BW:
            *pValue = pInfo->AS_Data.f_if1_bw;
            break;

        /*  zero-IF bandwidth                     */
        case MT2060_ZIF_BW:
            *pValue = pInfo->AS_Data.f_zif_bw;
            break;

        /*  LO2 Frequency                         */
        case MT2060_LO2_FREQ:
            *pValue = pInfo->AS_Data.f_LO2;
            break;

        /*  LO2 minimum step size                 */
        case MT2060_LO2_STEPSIZE:
            *pValue = pInfo->AS_Data.f_LO2_Step;
            break;

        /*  LO2 FracN keep-out region             */
        case MT2060_LO2_FRACN_AVOID:
            *pValue = pInfo->AS_Data.f_LO2_FracN_Avoid;
            break;

        /*  output center frequency               */
        case MT2060_OUTPUT_FREQ:
            *pValue = pInfo->AS_Data.f_out;
            break;

        /*  output bandwidth                      */
        case MT2060_OUTPUT_BW:
            *pValue = pInfo->AS_Data.f_out_bw;
            break;

        /*  min inter-tuner LO separation         */
        case MT2060_LO_SEPARATION:
            *pValue = pInfo->AS_Data.f_min_LO_Separation;
            break;

        /*  ID of avoid-spurs algorithm in use    */
        case MT2060_AS_ALG:
            *pValue = pInfo->AS_Data.nAS_Algorithm;
            break;

        /*  max # of intra-tuner harmonics        */
        case MT2060_MAX_HARM1:
            *pValue = pInfo->AS_Data.maxH1;
            break;

        /*  max # of inter-tuner harmonics        */
        case MT2060_MAX_HARM2:
            *pValue = pInfo->AS_Data.maxH2;
            break;

        /*  # of 1st IF exclusion zones           */
        case MT2060_EXCL_ZONES:
            *pValue = pInfo->AS_Data.nZones;
            break;

        /*  # of spurs found/avoided              */
        case MT2060_NUM_SPURS:
            *pValue = pInfo->AS_Data.nSpursFound;
            break;

        /*  >0 spurs avoided                      */
        case MT2060_SPUR_AVOIDED:
            *pValue = pInfo->AS_Data.bSpurAvoided;
            break;

        /*  >0 spurs in output (mathematically)   */
        case MT2060_SPUR_PRESENT:
            *pValue = pInfo->AS_Data.bSpurPresent;
            break;

        case MT2060_EOP:
        default:
            status |= MT_ARG_RANGE;
        }
    }
    return (status);
}


/****************************************************************************
**
**  Name: MT2060_GetReg
**
**  Description:    Gets an MT2060 register.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  reg         - MT2060 register/subaddress location
**                  *val        - MT2060 register/subaddress value
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  Use this function if you need to read a register from
**                  the MT2060.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-28-2004    DAD    Original.
**
****************************************************************************/
UData_t MT2060_GetReg(Handle_t h,
                      U8Data   reg,
                      U8Data*  val)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status |= MT_INV_HANDLE;
    
    if (val == NULL)
        status |= MT_ARG_NULL;

    if (reg >= END_REGS)
        status |= MT_ARG_RANGE;

    if (MT_NO_ERROR(status))
    {
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, reg, &pInfo->reg[reg], 1);
        if (MT_NO_ERROR(status))
            *val = pInfo->reg[reg];
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_GetTemp
**
**  Description:    Get the MT2060 Temperature register.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  *value       - value read from the register
**
**                                    Binary
**                  Value Returned    Value    Meaning
**                  ---------------------------------------------
**                  MT2060_T_0C       00       Temperature < 0C
**                  MT2060_0C_T_40C   01       0C < Temperature < 40C
**                  MT2060_40C_T_80C  10       40C < Temperature < 80C
**                  MT2060_80C_T      11       80C < Temperature
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-04-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   100   09-15-2005    DAD    Ver 1.15: Fixed register where temperature
**                              field is located (was MISC_CTRL_1).
**
******************************************************************************/
UData_t MT2060_GetTemp(Handle_t h, MT2060_Temperature* value)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    if (value == NULL)
        return MT_ARG_NULL;

    /* The Temp register bits have to be written before they can be read */
    status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);

    if (MT_NO_ERROR(status))
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);

    if (MT_NO_ERROR(status))
    {
        switch (pInfo->reg[MISC_STATUS] & 0x03)
        {
        case 0:
            *value = MT2060_T_0C;
            break;

        case 1:
            *value = MT2060_0C_T_40C;
            break;

        case 2:
            *value = MT2060_40C_T_80C;
            break;

        case 3:
            *value = MT2060_80C_T;
            break;
        }
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_GetUserData
**
**  Description:    Gets the user-defined data item.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  The hUserData parameter is a user-specific argument
**                  that is stored internally with the other tuner-
**                  specific information.
**
**                  For example, if additional arguments are needed
**                  for the user to identify the device communicating
**                  with the tuner, this argument can be used to supply
**                  the necessary information.
**
**                  The hUserData parameter is initialized in the tuner's
**                  Open function to NULL.
**
**  See Also:       MT2060_SetUserData, MT2060_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-25-2004    DAD    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
****************************************************************************/
UData_t MT2060_GetUserData(Handle_t h,
                           Handle_t* hUserData)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status = MT_INV_HANDLE;

    if (hUserData == NULL)
        status |= MT_ARG_NULL;

    if (MT_NO_ERROR(status))
        *hUserData = pInfo->hUserData;

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_ReInit
**
**  Description:    Initialize the tuner's register values.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_TUNER_ID_ERR   - Tuner Part/Rev code mismatch
**                      MT_TUNER_INIT_ERR - Tuner initialization failed
**                      MT_INV_HANDLE     - Invalid tuner handle
**                      MT_COMM_ERR       - Serial bus communications error
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2004    DAD    Original, extracted from MT2060_Open.
**   078   01-26-2005    DAD    Ver 1.11: Force FM1CA bit to 1
**   085   02-08-2005    DAD    Ver 1.11: Removed support for all but MT2060A3.
**   081   01-21-2005    DAD    Ver 1.11: Changed LNA Band names to RF Band
**                              to match description in the data sheet.
**   088   01-26-2005    DAD    Added MT_TUNER_INIT_ERR.
**   099   06-21-2005    DAD    Ver 1.14: Increment i when waiting for MT2060
**                              to finish initializing.
**
******************************************************************************/
UData_t MT2060_ReInit(Handle_t h)
{
    unsigned char defaults[] =
    {
        0x3F,            /* reg[0x01] <- 0x3F */
        0x74,            /* reg[0x02] <- 0x74 */
        0x80,            /* reg[0x03] <- 0x80 (FM1CA = 1) */
        0x08,            /* reg[0x04] <- 0x08 */
        0x93,            /* reg[0x05] <- 0x93 */
        0x88,            /* reg[0x06] <- 0x88 (RO) */
        0x80,            /* reg[0x07] <- 0x80 (RO) */
        0x60,            /* reg[0x08] <- 0x60 (RO) */
        0x20,            /* reg[0x09] <- 0x20 */
        0x1E,            /* reg[0x0A] <- 0x1E */
        0x31,            /* reg[0x0B] <- 0x31 (VGAG = 1) */
        0xFF,            /* reg[0x0C] <- 0xFF */
        0x80,            /* reg[0x0D] <- 0x80 */
        0xFF,            /* reg[0x0E] <- 0xFF */
        0x00,            /* reg[0x0F] <- 0x00 */
        0x2C,            /* reg[0x10] <- 0x2C (HW def = 3C) */
        0x42,            /* reg[0x11] <- 0x42 */
    };

    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;
    SData_t i;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status |= MT_INV_HANDLE;
            
    /*  Read the Part/Rev code from the tuner */
    if (MT_NO_ERROR(status))
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, PART_REV, pInfo->reg, 1);
    
    if (MT_NO_ERROR(status) && (pInfo->reg[0] != 0x63)){  /*  MT2060 A3  */
        MT2060_DBG("MT2060_ReInit Fail : MT_TUNER_ID_ERR != 0x63 (0x%02x)\n",pInfo->reg[0]);    
        status |= MT_TUNER_ID_ERR;      /*  Wrong tuner Part/Rev code   */
    }

    if (MT_NO_ERROR(status))
    {
        MT2060_DBG("MT2060_ReInit(2) : MT2060 Initialize\n");
        /*  Initialize the tuner state.  */
        pInfo->version       = VERSION;
        pInfo->tuner_id      = pInfo->reg[0];
        pInfo->AS_Data.f_ref = REF_FREQ;
        pInfo->AS_Data.f_in  = 651000000UL;
        pInfo->AS_Data.f_if1_Center = IF1_CENTER;
        pInfo->AS_Data.f_if1_bw = IF1_BW;
        pInfo->AS_Data.f_out = 43750000UL;
        pInfo->AS_Data.f_out_bw = 6750000UL;
        pInfo->AS_Data.f_zif_bw = ZIF_BW;
        pInfo->AS_Data.f_LO1_Step = LO1_STEP_SIZE;
        pInfo->AS_Data.f_LO2_Step = TUNE_STEP_SIZE;
        pInfo->AS_Data.maxH1 = MAX_HARMONICS_1;
        pInfo->AS_Data.maxH2 = MAX_HARMONICS_2;
        pInfo->AS_Data.f_min_LO_Separation = MIN_LO_SEP;
        pInfo->AS_Data.f_if1_Request = IF1_CENTER;
        pInfo->AS_Data.f_LO1 = 1871000000UL;
        pInfo->AS_Data.f_LO2 = 1176250000UL;
        pInfo->f_IF1_actual = pInfo->AS_Data.f_LO1 - pInfo->AS_Data.f_in;
        pInfo->AS_Data.f_LO1_FracN_Avoid = LO1_FRACN_AVOID;
        pInfo->AS_Data.f_LO2_FracN_Avoid = LO2_FRACN_AVOID;
        pInfo->num_regs = END_REGS;
        for ( i = 0; i < 10; i++ )
        {
            pInfo->RF_Bands[i] = MT2060_RF_Bands[i];
        }

        /*  Write the default values to each of the tuner registers.  */
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, defaults, sizeof(defaults));
    }
        
    /*  Read back all the registers from the tuner */
    if (MT_NO_ERROR(status))        
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, PART_REV, pInfo->reg, END_REGS);    
    
    /*  Wait for the MT2060 to finish initializing  */
    i = 0;
    MT2060_DBG("MT2060_ReInit(5) : Wait Ready\n");
    while (MT_NO_ERROR(status) && ((pInfo->reg[MISC_STATUS] & 0x40) == 0x00))
    {        
        MT_Sleep(pInfo->hUserData, 20);
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);
        if (i++ > 25)
        {
            MT2060_DBG("Wait MT2060 Initialize Fail, Initial Time Out\n");
            /*  Timed out waiting for MT2060 to finish initializing  */
            status |= MT_TUNER_INIT_ERR;
            break;
        }
    }
    MT2060_DBG("MT2060_ReInit(6) : Initial Complete\n");
    return (status);
}


/****************************************************************************
**
**  Name: MT2060_SetExtSRO
**
**  Description:    Sets the external SRO driver.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  Ext_SRO_Setting - external SRO drive setting
**
**       (default)    MT2060_EXT_SRO_OFF  - ext driver off
**                    MT2060_EXT_SRO_BY_1 - ext driver = SRO frequency
**                    MT2060_EXT_SRO_BY_2 - ext driver = SRO/2 frequency
**                    MT2060_EXT_SRO_BY_4 - ext driver = SRO/4 frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  The Ext_SRO_Setting settings default to OFF
**                  Use this function if you need to override the default
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-01-2004    DAD    Original.
**   079   01-20-2005    DAD    Ver 1.11: Fix setting of wrong bits in wrong
**                              register.
**
****************************************************************************/
UData_t MT2060_SetExtSRO(Handle_t h,
                         MT2060_Ext_SRO Ext_SRO_Setting)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status = MT_INV_HANDLE;
    else
    {
        pInfo->reg[MISC_CTRL_3] = (U8Data) ((pInfo->reg[MISC_CTRL_3] & 0x3F) | (Ext_SRO_Setting << 6));
        status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_CTRL_3, &pInfo->reg[MISC_CTRL_3], 1);   /* 0x0B */
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_SetGainRange
**
**  Description:    Modify the MT2060 VGA Gain range.
**                  The MT2060's VGA has three separate gain ranges that affect
**                  both the minimum and maximum gain provided by the VGA.
**                  This setting moves the VGA's gain range up/down in
**                  approximately 5.5 dB steps.  See figure below.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  value        - Gain range to be set
**
**                  range                       Comment         
**                  ---------------------------------------------------------
**                    0    MT2060_VGAG_0DB      VGA gain range offset 0 dB
**       (default)    1    MT2060_VGAG_5DB      VGA gain range offset +5 dB
**                    3    MT2060_VGAG_11DB     VGA gain range offset +11 dB
**
**
**                                     IF AGC Gain Range
**
**                 min     min+5dB   min+11dB        max-11dB  max-6dB      max
**                  v         v         v               v         v          v
**                  |---------+---------+---------------+---------+----------|
**
**                  <--------- MT2060_VGAG_0DB --------->
**
**                            <--------- MT2060_VGAG_5DB --------->
**
**                                      <--------- MT2060_VGAG_11DB --------->
**
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   MT_WRITESUB - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   07-22-2004    DAD    Original.
**   N/A   09-03-2004    DAD    Default VGAG to 1 (was 0).
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   093   04-05-2005    DAD    Ver 1.13: MT2060_SetGainRange() is missing a
**                              break statement before the default handler.
**
******************************************************************************/
UData_t MT2060_SetGainRange(Handle_t h, UData_t range)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    switch (range)
    {
    case MT2060_VGAG_0DB:
    case MT2060_VGAG_5DB:
    case MT2060_VGAG_11DB:
        pInfo->reg[MISC_CTRL_3] = ((pInfo->reg[MISC_CTRL_3] & 0xFC) | (U8Data) range);
        status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_CTRL_3, &pInfo->reg[MISC_CTRL_3], 1);
        break;
    default:
        MT2060_DBG("MT2060_SetGainRange Fail : MT_ARG_RANGE");
        status = MT_ARG_RANGE;
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_SetGPO
**
**  Description:    Modify the MT2060 GPO value(s).
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  gpo          - indicates which GPO pin(s) are modified
**                                 MT2060_GPO_0 - GPO0 (MT2060 A1 pin 29)
**                                 MT2060_GPO_1 - GPO1 (MT2060 A1 pin 45)
**                                 MT2060_GPO   - GPO1 (msb) & GPO0 (lsb)
**                  value        - value to be written to the GPO pin(s)
**
**                                 GPO1      GPO0
**                  gpo           pin 45    pin 29       value
**                  ---------------------------------------------
**                  MT2060_GPO_0    X         0            0
**                  MT2060_GPO_0    X         1            1
**                  MT2060_GPO_1    0         X            0
**                  MT2060_GPO_1    1         X            1
**       (default)  MT2060_GPO      0         0            0
**                  MT2060_GPO      0         1            1
**                  MT2060_GPO      1         0            2
**                  MT2060_GPO      1         1            3
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   MT_WriteSub - Write byte(s) of data to the two-wire-bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
UData_t MT2060_SetGPO(Handle_t h, UData_t gpo, UData_t value)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    switch (gpo)
    {
    case MT2060_GPO_0:
    case MT2060_GPO_1:
        pInfo->reg[MISC_CTRL_1] = (U8Data) ((pInfo->reg[MISC_CTRL_1] & (~gpo)) | (value ? gpo : 0));
        status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_CTRL_1, &pInfo->reg[MISC_CTRL_1], 1);
        break;

    case MT2060_GPO:
        pInfo->reg[MISC_CTRL_1] = (U8Data) ((pInfo->reg[MISC_CTRL_1] & (~gpo)) | ((value & gpo) << 1));
        status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_CTRL_1, &pInfo->reg[MISC_CTRL_1], 1);
        break;

    default:
        status = MT_ARG_RANGE;
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_SetIF1Center (obsolete)
**
**  Description:    Sets the MT2060 1st IF frequency.
**
**                  This function will be removed in future versions of this
**                  driver.  Instead, use the following function call:
**
**                  status = MT2060_SetParam(hMT2060, 
**                                           MT2060_IF1_CENTER, 
**                                           f_if1_Center);
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  value        - MT2060 1st IF frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   None
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-15-2004    DAD    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
UData_t MT2060_SetIF1Center(Handle_t h, UData_t value)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    pInfo->AS_Data.f_if1_Center = value;
    pInfo->AS_Data.f_if1_Request = value;

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_SetParam
**
**  Description:    Sets a tuning algorithm parameter.
**
**                  This function provides access to the internals of the
**                  tuning algorithm.  You can override many of the tuning
**                  algorithm defaults using this function.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  param       - Tuning algorithm parameter 
**                                (see enum MT2060_Param)
**                  nValue      - value to be set
**
**                  param                   Description
**                  ----------------------  --------------------------------
**                  MT2060_SRO_FREQ         crystal frequency                   
**                  MT2060_STEPSIZE         minimum tuning step size            
**                  MT2060_INPUT_FREQ       Center of input channel             
**                  MT2060_LO1_STEPSIZE     LO1 minimum step size               
**                  MT2060_LO1_FRACN_AVOID  LO1 FracN keep-out region           
**                  MT2060_IF1_REQUEST      Requested 1st IF                    
**                  MT2060_IF1_CENTER       Center of 1st IF SAW filter         
**                  MT2060_IF1_BW           Bandwidth of 1st IF SAW filter      
**                  MT2060_ZIF_BW           zero-IF bandwidth                   
**                  MT2060_LO2_STEPSIZE     LO2 minimum step size               
**                  MT2060_LO2_FRACN_AVOID  LO2 FracN keep-out region           
**                  MT2060_OUTPUT_FREQ      output center frequency             
**                  MT2060_OUTPUT_BW        output bandwidth                    
**                  MT2060_LO_SEPARATION    min inter-tuner LO separation       
**                  MT2060_MAX_HARM1        max # of intra-tuner harmonics      
**                  MT2060_MAX_HARM2        max # of inter-tuner harmonics      
**
**  Usage:          status |= MT2060_SetParam(hMT2060, 
**                                            MT2060_STEPSIZE,
**                                            50000);
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Invalid parameter requested
**                                         or set value out of range
**                                         or non-writable parameter
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**  See Also:       MT2060_GetParam, MT2060_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-02-2004    DAD    Original.
**
****************************************************************************/
UData_t MT2060_SetParam(Handle_t     h,
                        MT2060_Param param,
                        UData_t      nValue)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status |= MT_INV_HANDLE;    

    if (MT_NO_ERROR(status))
    {
        switch (param)
        {
        /*  crystal frequency                     */
        case MT2060_SRO_FREQ:
            pInfo->AS_Data.f_ref = nValue;
            pInfo->AS_Data.f_LO1_FracN_Avoid = 3 * nValue / 16 - 1;
            pInfo->AS_Data.f_LO2_FracN_Avoid = nValue / 160 - 1;
            pInfo->AS_Data.f_LO1_Step = nValue / 64;
            break;

        /*  minimum tuning step size              */
        case MT2060_STEPSIZE:
            pInfo->AS_Data.f_LO2_Step = nValue;
            break;

        /*  Center of input channel               */
        case MT2060_INPUT_FREQ:
            pInfo->AS_Data.f_in = nValue;
            break;

        /*  LO1 minimum step size                 */
        case MT2060_LO1_STEPSIZE:
            pInfo->AS_Data.f_LO1_Step = nValue;
            break;

        /*  LO1 FracN keep-out region             */
        case MT2060_LO1_FRACN_AVOID:
            pInfo->AS_Data.f_LO1_FracN_Avoid = nValue;
            break;

        /*  Requested 1st IF                      */
        case MT2060_IF1_REQUEST:
            pInfo->AS_Data.f_if1_Request = nValue;
            break;

        /*  Center of 1st IF SAW filter           */
        case MT2060_IF1_CENTER:
            pInfo->AS_Data.f_if1_Center = nValue;
            break;

        /*  Bandwidth of 1st IF SAW filter        */
        case MT2060_IF1_BW:
            pInfo->AS_Data.f_if1_bw = nValue;
            break;

        /*  zero-IF bandwidth                     */
        case MT2060_ZIF_BW:
            pInfo->AS_Data.f_zif_bw = nValue;
            break;

        /*  LO2 minimum step size                 */
        case MT2060_LO2_STEPSIZE:
            pInfo->AS_Data.f_LO2_Step = nValue;
            break;

        /*  LO2 FracN keep-out region             */
        case MT2060_LO2_FRACN_AVOID:
            pInfo->AS_Data.f_LO2_FracN_Avoid = nValue;
            break;

        /*  output center frequency               */
        case MT2060_OUTPUT_FREQ:
            pInfo->AS_Data.f_out = nValue;
            break;

        /*  output bandwidth                      */
        case MT2060_OUTPUT_BW:
            pInfo->AS_Data.f_out_bw = nValue;
            break;

        /*  min inter-tuner LO separation         */
        case MT2060_LO_SEPARATION:
            pInfo->AS_Data.f_min_LO_Separation = nValue;
            break;

        /*  max # of intra-tuner harmonics        */
        case MT2060_MAX_HARM1:
            pInfo->AS_Data.maxH1 = nValue;
            break;

        /*  max # of inter-tuner harmonics        */
        case MT2060_MAX_HARM2:
            pInfo->AS_Data.maxH2 = nValue;
            break;

        /*  These parameters are read-only  */
        case MT2060_IC_ADDR:
        case MT2060_MAX_OPEN:
        case MT2060_NUM_OPEN:
        case MT2060_LO1_FREQ:
        case MT2060_IF1_ACTUAL:
        case MT2060_LO2_FREQ:
        case MT2060_AS_ALG:
        case MT2060_EXCL_ZONES:
        case MT2060_SPUR_AVOIDED:
        case MT2060_NUM_SPURS:
        case MT2060_SPUR_PRESENT:
        case MT2060_EOP:
        default:
            status |= MT_ARG_RANGE;
        }
    }
    return (status);
}


/****************************************************************************
**
**  Name: MT2060_SetReg
**
**  Description:    Sets an MT2060 register.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  reg         - MT2060 register/subaddress location
**                  val         - MT2060 register/subaddress value
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  Use this function if you need to override a default
**                  register value
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-28-2004    DAD    Original.
**   089   02-17-2005    DAD    Ver 1.12: Fix bug in MT2060_SetReg() where
**                              the register was not getting set properly.
**
****************************************************************************/
UData_t MT2060_SetReg(Handle_t h,
                      U8Data   reg,
                      U8Data   val)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)        
        status |= MT_INV_HANDLE;

    if (reg >= END_REGS)
        status |= MT_ARG_RANGE;

    if (MT_NO_ERROR(status))
    {
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, reg, &val, 1);
        if (MT_NO_ERROR(status))
            pInfo->reg[reg] = val;
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_SetUserData
**
**  Description:    Sets the user-defined data item.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  hUserData   - ptr to user-defined data
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  The hUserData parameter is a user-specific argument
**                  that is stored internally with the other tuner-
**                  specific information.
**
**                  For example, if additional arguments are needed
**                  for the user to identify the device communicating
**                  with the tuner, this argument can be used to supply
**                  the necessary information.
**
**                  The hUserData parameter is initialized in the tuner's
**                  Open function to NULL.
**
**  See Also:       MT2060_GetUserData, MT2060_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-25-2004    DAD    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
****************************************************************************/
UData_t MT2060_SetUserData(Handle_t h,
                           Handle_t hUserData)
{
    UData_t status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status = MT_INV_HANDLE;    
    else
        pInfo->hUserData = hUserData;

    return (status);
}


static UData_t Round_fLO(UData_t f_LO, UData_t f_LO_Step, UData_t f_ref)
{
    return f_ref * (f_LO / f_ref)
        + f_LO_Step * (((f_LO % f_ref) + (f_LO_Step / 2)) / f_LO_Step);
}


/****************************************************************************
**
**  Name: CalcLO1Mult
**
**  Description:    Calculates Integer divider value and the numerator
**                  value for LO1's FracN PLL.
**
**                  This function assumes that the f_LO and f_Ref are
**                  evenly divisible by f_LO_Step.
**
**  Parameters:     Div       - OUTPUT: Whole number portion of the multiplier
**                  FracN     - OUTPUT: Fractional portion of the multiplier
**                  f_LO      - desired LO frequency.
**                  f_LO_Step - Minimum step size for the LO (in Hz).
**                  f_Ref     - SRO frequency.
**
**  Returns:        Recalculated LO frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-05-2004    JWS    Original
**
****************************************************************************/
static UData_t CalcLO1Mult(UData_t *Div,
                           UData_t *FracN,
                           UData_t  f_LO,
                           UData_t  f_LO_Step,
                           UData_t  f_Ref)
{
    /*  Calculate the whole number portion of the divider */
    *Div = f_LO / f_Ref;

    /*  Calculate the numerator value (round to nearest f_LO_Step) */
    *FracN = (64 * (((f_LO % f_Ref) + (f_LO_Step / 2)) / f_LO_Step)+ (f_Ref / f_LO_Step / 2)) / (f_Ref / f_LO_Step);

    return (f_Ref * (*Div)) + (f_LO_Step * (*FracN));
}


/****************************************************************************
**
**  Name: CalcLO2Mult
**
**  Description:    Calculates Integer divider value and the numerator
**                  value for LO2's FracN PLL.
**
**                  This function assumes that the f_LO and f_Ref are
**                  evenly divisible by f_LO_Step.
**
**  Parameters:     Div       - OUTPUT: Whole number portion of the multiplier
**                  FracN     - OUTPUT: Fractional portion of the multiplier
**                  f_LO      - desired LO frequency.
**                  f_LO_Step - Minimum step size for the LO (in Hz).
**                  f_Ref     - SRO frequency.
**
**  Returns:        Recalculated LO frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   11-04-2003    JWS    Original, derived from Mtuner version
**
****************************************************************************/
static UData_t CalcLO2Mult(UData_t *Div,
                           UData_t *FracN,
                           UData_t  f_LO,
                           UData_t  f_LO_Step,
                           UData_t  f_Ref)
{
    const UData_t FracN_Scale = (f_Ref / (MAX_UDATA >> 13)) + 1;

    /*  Calculate the whole number portion of the divider */
    *Div = f_LO / f_Ref;

    /*  Calculate the numerator value (round to nearest f_LO_Step) */
    *FracN = (8191 * (((f_LO % f_Ref) + (f_LO_Step / 2)) / f_LO_Step) + (f_Ref / f_LO_Step / 2)) / (f_Ref / f_LO_Step);

    return (f_Ref * (*Div))
         + FracN_Scale * (((f_Ref / FracN_Scale) * (*FracN)) / 8191);

}


/****************************************************************************
**
**  Name: LO1LockWorkAround
**
**  Description:    Correct for problem where LO1 does not lock at the
**                  transition point between VCO1 and VCO2.
**
**  Parameters:     pInfo       - Pointer to MT2060_Info Structure
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_UPC_UNLOCK    - Upconverter PLL unlocked
**                      MT_DNC_UNLOCK    - Downconverter PLL unlocked
**                      MT_COMM_ERR      - Serial bus communications error
**
**  Dependencies:   None
**
**                  MT_ReadSub      - Read byte(s) of data from the two-wire-bus
**                  MT_WriteSub     - Write byte(s) of data to the two-wire-bus
**                  MT_Sleep        - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
****************************************************************************/
static UData_t LO1LockWorkAround(MT2060_Info_t *pInfo)
{
    U8Data cs;
    U8Data tad;
    UData_t status = MT_OK;                  /* Status to be returned        */
    SData_t ChkCnt = 0;

    cs = 30;  /* Pick starting CapSel Value */

    pInfo->reg[LO1_1] &= 0x7F;  /* Auto CapSelect off */
    pInfo->reg[LO1_2] = (pInfo->reg[LO1_2] & 0xC0) | cs;
    status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_1, &pInfo->reg[LO1_1], 2);  /* 0x0C - 0x0D */

    while ((ChkCnt < 64) && (MT_NO_ERROR(status)))
    {
        MT_Sleep(pInfo->hUserData, 2);
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, LO_STATUS, &pInfo->reg[LO_STATUS], 1);
        tad = (pInfo->reg[LO_STATUS] & 0x70) >> 4;  /* LO1AD */
        if (MT_NO_ERROR(status))
        {
            if (tad == 0)   /* Found good spot -- quit looking */
                break;
            else if (tad & 0x04 )  /* either 4 or 5; decrement */
            {
                pInfo->reg[LO1_2] = (pInfo->reg[LO1_2] & 0xC0) | --cs;
                status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_2, &pInfo->reg[LO1_2], 1);   /* 0x0D */
            }
            else if (tad & 0x02 )  /* either 2 or 3; increment */
            {
                pInfo->reg[LO1_2] = (pInfo->reg[LO1_2] & 0xC0) | ++cs;
                status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_2, &pInfo->reg[LO1_2], 1);   /* 0x0D */
            }
            ChkCnt ++;  /* Count this attempt */
        }
    }
    if (MT_NO_ERROR(status))
    {
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, LO_STATUS, &pInfo->reg[LO_STATUS], 1);
    }

    if (MT_NO_ERROR(status))
    {
        if ((pInfo->reg[LO_STATUS] & 0x80) == 0x00)
            status |= MT_UPC_UNLOCK;

        if ((pInfo->reg[LO_STATUS] & 0x08) == 0x00)
            status |= MT_DNC_UNLOCK;
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_ChangeFreq
**
**  Description:    Change the tuner's tuned frequency to f_in.
**
**  Parameters:     h           - Open handle to the tuner (from MT2060_Open).
**                  f_in        - RF input center frequency (in Hz).
**                  f_IF1       - Desired IF1 center frequency (in Hz).
**                  f_out       - Output IF center frequency (in Hz)
**                  f_IFBW      - Output IF Bandwidth (in Hz)
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_UPC_UNLOCK    - Upconverter PLL unlocked
**                      MT_DNC_UNLOCK    - Downconverter PLL unlocked
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_SPUR_CNT_MASK - Count of avoided LO spurs
**                      MT_SPUR_PRESENT  - LO spur possible in output
**                      MT_FIN_RANGE     - Input freq out of range
**                      MT_FOUT_RANGE    - Output freq out of range
**                      MT_UPC_RANGE     - Upconverter freq out of range
**                      MT_DNC_RANGE     - Downconverter freq out of range
**
**  Dependencies:   MUST CALL MT2060_Open BEFORE MT2060_ChangeFreq!
**
**                  MT_ReadSub       - Read byte(s) of data from the two-wire-bus
**                  MT_WriteSub      - Write byte(s) of data to the two-wire-bus
**                  MT_Sleep         - Delay execution for x milliseconds
**                  MT2060_GetLocked - Checks to see if LO1 and LO2 are locked
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   081   01-21-2005    DAD    Ver 1.11: Changed LNA Band names to RF Band
**                              to match description in the data sheet.
**
****************************************************************************/
UData_t MT2060_ChangeFreq(Handle_t h,
                          UData_t f_in,     /* RF input center frequency   */
                          UData_t f_IF1,    /* desired 1st IF frequency    */
                          UData_t f_out,    /* IF output center frequency  */
                          UData_t f_IFBW)   /* IF output bandwidth + guard */
{
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    UData_t status = MT_OK;       /*  status of operation             */
    UData_t t_status;             /*  temporary status of operation   */
    UData_t LO1;                  /*  1st LO register value           */
    UData_t Num1;                 /*  Numerator for LO1 reg. value    */
    UData_t LO2;                  /*  2nd LO register value           */
    UData_t Num2;                 /*  Numerator for LO2 reg. value    */
    UData_t ofLO1, ofLO2;         /*  last time's LO frequencies      */
    UData_t ofin, ofout;          /*  last time's I/O frequencies     */
    UData_t center;
    UData_t RFBand;               /*  RF Band setting                 */
    UData_t idx;                  /*  index loop                      */

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    /*  Check the input and output frequency ranges                   */
    if ((f_in < MIN_FIN_FREQ) || (f_in > MAX_FIN_FREQ)){
        MT2060_DBG("Out of MT_FIN_RANGE(%u)",f_in);    
        status |= MT_FIN_RANGE;
    }

    if ((f_out < MIN_FOUT_FREQ) || (f_out > MAX_FOUT_FREQ)){
        MT2060_DBG("Out of MT_FOUT_RANGE(%u)",f_out);    
        status |= MT_FOUT_RANGE;
    }

    /*
    **  Save original LO1 and LO2 register values
    */
    ofLO1 = pInfo->AS_Data.f_LO1;
    ofLO2 = pInfo->AS_Data.f_LO2;
    ofin = pInfo->AS_Data.f_in;
    ofout = pInfo->AS_Data.f_out;

    /*
    **  Find RF Band setting
    */
    RFBand = 1;                        /*  def when f_in > all    */
    for (idx=0; idx<10; ++idx)
    {
        if (pInfo->RF_Bands[idx] >= f_in)
        {
            RFBand = 11 - idx;
            break;
        }
    }
//    VERBOSE_PRINT1("Using RF Band #%d.\n", RFBand);
    /*
    **  Assign in the requested values
    */
    pInfo->AS_Data.f_in = f_in;
    pInfo->AS_Data.f_out = f_out;
    pInfo->AS_Data.f_out_bw = f_IFBW;
    //  Request a 1st IF such that LO1 is on a step size
    pInfo->AS_Data.f_if1_Request = Round_fLO(f_IF1 + f_in, pInfo->AS_Data.f_LO1_Step, pInfo->AS_Data.f_ref) - f_in;

    /*
    **  Calculate frequency settings.  f_IF1_FREQ + f_in is the
    **  desired LO1 frequency
    */
    MT_ResetExclZones(&pInfo->AS_Data);

    if (((f_in % pInfo->AS_Data.f_ref) <= f_IFBW/2)
        || ((pInfo->AS_Data.f_ref - (f_in % pInfo->AS_Data.f_ref)) <= f_IFBW/2))
    {
        /*
        **  Exclude LO frequencies that allow SRO harmonics to pass through
        */
        center = pInfo->AS_Data.f_ref * ((pInfo->AS_Data.f_if1_Center - pInfo->AS_Data.f_if1_bw/2 + pInfo->AS_Data.f_in) / pInfo->AS_Data.f_ref) - pInfo->AS_Data.f_in;
        while (center < pInfo->AS_Data.f_if1_Center + pInfo->AS_Data.f_if1_bw/2 + pInfo->AS_Data.f_LO1_FracN_Avoid)
        {
            //  Exclude LO1 FracN
            MT_AddExclZone(&pInfo->AS_Data, center-pInfo->AS_Data.f_LO1_FracN_Avoid, center+pInfo->AS_Data.f_LO1_FracN_Avoid);
            center += pInfo->AS_Data.f_ref;
        }

        center = pInfo->AS_Data.f_ref * ((pInfo->AS_Data.f_if1_Center - pInfo->AS_Data.f_if1_bw/2 - pInfo->AS_Data.f_out) / pInfo->AS_Data.f_ref) + pInfo->AS_Data.f_out;
        while (center < pInfo->AS_Data.f_if1_Center + pInfo->AS_Data.f_if1_bw/2 + pInfo->AS_Data.f_LO2_FracN_Avoid)
        {
            //  Exclude LO2 FracN
            MT_AddExclZone(&pInfo->AS_Data, center-pInfo->AS_Data.f_LO2_FracN_Avoid, center+pInfo->AS_Data.f_LO2_FracN_Avoid);
            center += pInfo->AS_Data.f_ref;
        }
    }

    f_IF1 = MT_ChooseFirstIF(&pInfo->AS_Data);
    pInfo->AS_Data.f_LO1 = Round_fLO(f_IF1 + f_in, pInfo->AS_Data.f_LO1_Step, pInfo->AS_Data.f_ref);
    pInfo->AS_Data.f_LO2 = Round_fLO(pInfo->AS_Data.f_LO1 - f_out - f_in, pInfo->AS_Data.f_LO2_Step, pInfo->AS_Data.f_ref);

    /*
    ** Check for any LO spurs in the output bandwidth and adjust
    ** the LO settings to avoid them if needed
    */
    status |= MT_AvoidSpurs(h, &pInfo->AS_Data);

    /*
    ** MT_AvoidSpurs spurs may have changed the LO1 & LO2 values.
    ** Recalculate the LO frequencies and the values to be placed
    ** in the tuning registers.
    */
    pInfo->AS_Data.f_LO1 = CalcLO1Mult(&LO1, &Num1, pInfo->AS_Data.f_LO1, pInfo->AS_Data.f_LO1_Step, pInfo->AS_Data.f_ref);
    pInfo->AS_Data.f_LO2 = Round_fLO(pInfo->AS_Data.f_LO1 - f_out - f_in, pInfo->AS_Data.f_LO2_Step, pInfo->AS_Data.f_ref);
    pInfo->AS_Data.f_LO2 = CalcLO2Mult(&LO2, &Num2, pInfo->AS_Data.f_LO2, pInfo->AS_Data.f_LO2_Step, pInfo->AS_Data.f_ref);

    /*
    **  If we have the same LO frequencies and we're already locked,
    **  then just return without writing any registers.
    */
    if ((ofLO1 == pInfo->AS_Data.f_LO1) 
        && (ofLO2 == pInfo->AS_Data.f_LO2) 
        && ((pInfo->reg[LO_STATUS] & 0x88) == 0x88))
    {
        pInfo->f_IF1_actual = pInfo->AS_Data.f_LO1 - f_in;
        return (status);
    }

    /*
    **  Check the upconverter and downconverter frequency ranges
    */
    if ((pInfo->AS_Data.f_LO1 < MIN_UPC_FREQ) || (pInfo->AS_Data.f_LO1 > MAX_UPC_FREQ))
        status |= MT_UPC_RANGE;

    if ((pInfo->AS_Data.f_LO2 < MIN_DNC_FREQ) || (pInfo->AS_Data.f_LO2 > MAX_DNC_FREQ))
        status |= MT_DNC_RANGE;

    /*
    **  Make sure that the Auto CapSelect is turned on since it
    **  may have been turned off last time we tuned.
    */
    if (MT_NO_ERROR(status) && ((pInfo->reg[LO1_1] & 0x80) == 0x00))
    {
        pInfo->reg[LO1_1] |= 0x80;
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_1, &pInfo->reg[LO1_1], 1);   /* 0x0C */
    }

    /*
    **  Place all of the calculated values into the local tuner
    **  register fields.
    */
    if (MT_NO_ERROR(status))
    {
        pInfo->reg[LO1C_1] = (U8Data)((RFBand << 4) | (Num1 >> 2));
        pInfo->reg[LO1C_2] = (U8Data)(LO1 & 0xFF);
    
        pInfo->reg[LO2C_1] = (U8Data)((Num2 & 0x000F) | ((Num1 & 0x03) << 4));
        pInfo->reg[LO2C_2] = (U8Data)((Num2 & 0x0FF0) >> 4);
        pInfo->reg[LO2C_3] = (U8Data)(((LO2 << 1) & 0xFE) | ((Num2 & 0x1000) >> 12));
    
        /*
        ** Now write out the computed register values
        ** IMPORTANT: There is a required order for writing some of the
        **            registers.  R2 must follow R1 and  R5 must
        **            follow R3 & R4. Simple numerical order works, so...
        */
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, &pInfo->reg[LO1C_1], 5);   /* 0x01 - 0x05 */
    }

    /*
    **  Check for LO's locking
    */
    if (MT_NO_ERROR(status))
    {
        t_status = MT2060_GetLocked(h);
        if ((t_status & (MT_UPC_UNLOCK | MT_DNC_UNLOCK)) == MT_UPC_UNLOCK)
            /*  Special Case: LO1 not locked, LO2 locked  */
            status |= (t_status & ~MT_UPC_UNLOCK) | LO1LockWorkAround(pInfo);
        else
            /*  OR in the status from the call to MT2060_GetLocked()  */
            status |= t_status;
    }

    /*
    **  If we locked OK, assign calculated data to MT2060_Info_t structure
    */
    if (MT_NO_ERROR(status))
    {
        pInfo->f_IF1_actual = pInfo->AS_Data.f_LO1 - f_in;
    }

    return (status);
}


#if defined (__FLOATING_POINT__)
/****************************************************************************
**
**  Name: spline
**
**  Description:    Compute the 2nd derivatives of a set of x, y data for use
**                  in a cubic-spline interpolation.
**
**  Parameters:     x[9]        - array of independent variable (freqs)
**                  y[9]        - array of dependent variable (power)
**
**  Returns:        y2[9]       - 2nd derivative values at each x[]
**
**  Dependencies:   Requires floating point arithmetic.
**
**                  Assumes that the 1st derivative at each end point
**                  is zero (no slope).
**
**                  Assumes that there are exactly 9 data points.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-15-2004    DAD    From "Numerical Recipes in C", 2nd Ed.
**                              Modified for this particular application.
**   N/A   08-11-2004    DAD    Re-derived from cubic-spline equations
**                              using a specialized tri-diagonal matrix.
**
****************************************************************************/
static void spline(FData_t x[], FData_t y[], FData_t y2[])
{
    int i;
    FData_t dx[8],dy[8],w[8],d[8];
    
    dx[0] = x[1]-x[0];
    dy[0] = y[1]-y[0];
    d[0] = dx[0] * dx[0];       //  Assumes 2(x[2]-x[0]) is much greater than 1
    y2[0] = y2[8] = 0.0;
    w[0] = 0.0;

    /*  Notes for further optimization:
    **
    **  d[0] = 6889000000000000.0
    **  d[1] = 195999999.00000000
    **  d[2] = 58852040.810469598
    **  d[3] = 56176853.055536300
    **  d[4] = 55994791.666639537
    **  d[5] = 55981769.137752682
    **  d[6] = 55980834.413318574
    **  d[7] = 191980767.30441749
    **  dx[0]=dx[7] = 83000000
    **  dx[1..6] = 15000000  (15 MHz spacing from x[1] through x[7])
    **
    */
    for (i=1;i<8;i++)
    {
        dx[i] = (x[i+1] - x[i]);
        dy[i] = (y[i+1] - y[i]);
        w[i] = 6*((dy[i] / dx[i]) - (dy[i-1] / dx[i-1])) - (w[i-1] * dx[i-1] / d[i-1]);
        d[i] = 2 * (x[i+1] - x[i-1]) - dx[i-1] * dx[i-1] / d[i-1];
    }

    for (i=7;i>0;i--)
        y2[i] = (w[i] - dx[i]*y2[i+1]) / d[i];
}


//#include <math.h>
/****************************************************************************
**
**  Name: findMax
**
**  Description:    Find the maximum x,y of a cubic-spline interpolated
**                  function.
**
**  Parameters:     x[9]        - array of independent variable (freqs)
**                  y[9]        - array of dependent variable (power)
**                  y2[9]       - 2nd derivative values at each x[]
**
**  Returns:        xmax        - frequency of maximum power
**                  return val  - interpolated power value at xmax
**
**  Dependencies:   Requires floating point arithmetic.
**
**                  Assumes that spline() has been called to calculate the
**                  values of y2[].
**
**                  Assumes that there is a peak between x[0] and x[8].
**
**                  Assumes that there are exactly 9 data points.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   101   09-15-2005    DAD    Ver 1.15: Re-wrote max- and zero-finding
**                              routines to take advantage of known cubic
**                              polynomial coefficients.
**
****************************************************************************/
static FData_t findMax(FData_t x[], FData_t y[], FData_t y2[], FData_t *xmax)
{
    FData_t ymax = y[0];
    UData_t i;

    /*  Calculate the maximum of each cubic spline segment */
    for (i=0; i<8; i++)
    {
        /*
        **  Compute the cubic polynomial coefficients:
        **
        **  y = ax^3 + bx^2 + cx + d
        **
        */
        FData_t dx = x[i+1] - x[i];
        FData_t dy = y[i+1] - y[i];
        FData_t a = (y2[i+1] - y2[i]) / (6 * dx);
        FData_t b = (x[i+1] * y2[i] - x[i] * y2[i+1]) / (2 * dx);
        FData_t c = (dy + a*(x[i]*x[i]*x[i]-x[i+1]*x[i+1]*x[i+1]) + b*(x[i]*x[i]-x[i+1]*x[i+1])) / dx;
        FData_t d = y[i] - a*x[i]*x[i]*x[i] - b*x[i]*x[i] - c*x[i];
        FData_t t = b*b - 3*a*c;

        if (ymax < y[i+1])
        {
            *xmax = x[i+1];
            ymax = y[i+1];
        }

        /*
        **  Solve the cubic polynomial's 1st derivative for zero
        **  to find the maxima or minima
        **
        **  y' = 3ax^2 + 2bx + c = 0
        **
        **       -b +/- sqrt(b^2 - 3ac)
        **  x = ---------------------------
        **                3a
        */

        /*  Skip this segment if x will be imaginary  */
        if (t>=0.0)
        {
            FData_t x_t;

            t = sqrt(t);
            x_t = (-b + t) / (3*a);
            /*  Only check answers that are between x[i] & x[i+1]  */
            if ((x[i]<=x_t) && (x_t<=x[i+1]))
            {
                FData_t y_t = a*x_t*x_t*x_t + b*x_t*x_t + c*x_t + d;
                if (ymax < y_t)
                {
                    *xmax = x_t;
                    ymax = y_t;
                }
            }

            x_t = (-b - t) / (3*a);
            /*  Only check answers that are between x[i] & x[i+1]  */
            if ((x[i]<=x_t) && (x_t<=x[i+1]))
            {
                FData_t y_t = a*x_t*x_t*x_t + b*x_t*x_t + c*x_t + d;
                if (ymax < y_t)
                {
                    *xmax = x_t;
                    ymax = y_t;
                }
            }
        }
    }
    return (ymax);
}


/****************************************************************************
**
**  Name: findRoot
**
**  Description:    Find the root x,y of a cubic-spline interpolated
**                  function, given the x, y, 2nd derivatives and a
**                  set of x values that bound the particular root to
**                  be found.
**
**                  If more than one root is bounded, the root closest to
**                  x_at_ymax will be found.
**
**
**                               -*  x_at_ymax
**                              /  \
**                             X    X  
**                            /      \
**                  0 --------------------------
**                          /          \
**                         /            \
**                        X              X
**                       /                \
**                   X---                  ---X
**                   ^                        ^
**                   |                        |
**                   x_at_ymin    -- OR --    x_at_ymin
**
**
**  Parameters:     x[9]        - array of independent variable (freqs)
**                  y[9]        - array of dependent variable (power)
**                  y2[9]       - 2nd derivative values at each x[]
**                  x_at_ymax   - frequency where power > 0
**                  x_at_ymin   - frequency where power < 0
**
**  Returns:        xroot       - x-axis value of the root
**
**  Dependencies:   Requires floating point arithmetic.
**
**                  Assumes that spline() has been called to calculate the
**                  values of y2[].
**
**                  Assumes: 
**                      y(x_at_ymax) > 0
**                      y(x_at_ymin) < 0
**
**                  Assumes that there are exactly 9 data points.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   101   09-15-2005    DAD    Ver 1.15: Re-wrote max- and zero-finding
**                              routines to take advantage of known cubic
**                              polynomial coefficients.
**
****************************************************************************/
static FData_t findRoot(FData_t x[], 
                        FData_t y[], 
                        FData_t y2[], 
                        FData_t x_at_ymax,
                        FData_t x_at_ymin)
{
    FData_t x_upper, x_lower;           /*  limits for zero-crossing       */
    UData_t i;
    FData_t dx, dy;
    FData_t a, b, c, d;                 /*  cubic coefficients             */
    FData_t xg;                         /*  x-axis guesses                 */
    FData_t y_err;                      /*  y-axis error of x-axis guess   */

    /*  Find the index into x[] that is just below x_ymax  */
    i = 8;
    while (x[i] > x_at_ymax)
        --i;

    if (x_at_ymax > x_at_ymin)
    {
        /*  Decrement i until y[i] < 0
        **  We want to end up with this:
        **
        **             X  {x[i+1], y[i+1]}
        **            /
        **  0 ----------------------
        **          /
        **         /
        **        X  {x[i], y[i]}
        **
        */
        while (y[i] > 0)
            --i;
        x_upper = (x_at_ymax < x[i+1]) ? x_at_ymax : x[i+1];
        x_lower = (x[i] < x_at_ymin) ? x_at_ymin : x[i];
    }
    else
    {
        /*  Increment i until y[i] < 0
        **  We want to end up with this:
        **
        **        X  {x[i], y[i]}
        **         \
        **  0 ----------------------
        **           \
        **            \
        **             X  {x[i+1], y[i+1]}
        **
        */
        while (y[++i] > 0);
        --i;
        x_upper = (x_at_ymin < x[i+1]) ? x_at_ymin : x[i+1];
        x_lower = (x[i] < x_at_ymax) ? x_at_ymax : x[i];
    }

    /*
    **  Compute the cubic polynomial coefficients:
    **
    **  y = ax^3 + bx^2 + cx + d
    **
    */
    dx = x[i+1] - x[i];
    dy = y[i+1] - y[i];
    a = (y2[i+1] - y2[i]) / (6 * dx);
    b = (x[i+1] * y2[i] - x[i] * y2[i+1]) / (2 * dx);
    c = (dy + a*(x[i]*x[i]*x[i]-x[i+1]*x[i+1]*x[i+1]) + b*(x[i]*x[i]-x[i+1]*x[i+1])) / dx;
    d = y[i] - a*x[i]*x[i]*x[i] - b*x[i]*x[i] - c*x[i];

    /*
    **  Make an initial guess at the root (bisection)
    */
    xg = (x_lower + x_upper) / 2.0;

    while (1)
    {
        FData_t u = 3*a*xg*xg+2*b*xg+c;
        FData_t v = 6*a*xg+2*b;
        FData_t t;

        /*
        **  Compute the y-axis value of this guess (error)
        */
        y_err = a*xg*xg*xg + b*xg*xg + c*xg + d;

        /*  Halt search if we're within 0.01 dB  */
        if (fabs(y_err) < 0.01)
            break;

        /*  Update upper and/or lower bound  */
        if ((x_at_ymax > x_at_ymin) ^ (y_err < 0))
            x_upper = xg;
        else
            x_lower = xg;

        /*  Check to see if we can use the quadratic form of the root solution  */
        t = u*u - 2*y_err*v;
        if (t>=0)
        {
            UData_t bInBound2, bInBound3;
            FData_t xg2, xg3;

            /*  Compute new guesses  */
            t = sqrt(t);
            xg2 = xg - (u - t) / v;
            bInBound2 = (xg2 >= x_lower) && (xg2 <= x_upper);
            xg3 = xg - (u + t) / v;
            bInBound3 = (xg3 >= x_lower) && (xg3 <= x_upper);

            if (bInBound2 && bInBound3)
                /*  If both xg2 & xg3 are valid, choose the one closest to x_at_ymax  */
                xg = (fabs(xg2 - x_at_ymax) < fabs(xg3 - x_at_ymax)) ? xg2 : xg3;

            else if (bInBound2 != 0)
                xg = xg2;               /*  Keep this new guess  */

            else if (bInBound3 != 0)
                xg = xg3;               /*  Keep this new guess  */

            else
                /*  Quadratic form failed, use bisection  */
                xg = (x_lower + x_upper) / 2.0;
        }
        else
        {
            /*  Quadratic form failed, use bisection  */
            xg = (x_lower + x_upper) / 2.0;
        }
    }

    return (xg);
}
#endif


/******************************************************************************
**
**  Name: MT2060_Program_LOs
**
**  Description:    Computes and programs MT2060 LO frequencies
**
**  Parameters:     pinfo       - Pointer to MT2060_Info Structure
**                  f_in        - Input frequency to the MT2060
**                                If 0 Hz, use injected LO frequency
**                  f_out       - Output frequency from MT2060
**                  f_IF1       - Desired 1st IF filter center frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**
**  Dependencies:   None
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   102   08-16-2005    DAD    Ver 1.15: Fixed-point version added.  Location
**                              of 1st IF filter center requires 9-11 samples
**                              instead of 7 for floating-point version.
**
******************************************************************************/
static UData_t MT2060_Program_LOs(MT2060_Info_t* pInfo,
                                  UData_t f_in,
                                  UData_t f_IF1,
                                  UData_t f_out)
{
    UData_t status = MT_OK;
    UData_t LO1, Num1, LO2, Num2;
    UData_t idx;
    long RFBand = 1;                  /*  RF Band setting  */

    if (f_in == 0)
    {
        /*  Force LO1 to feed through mode  */
        pInfo->reg[LO1_2] |= 0x40;
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_2, &pInfo->reg[LO1_2], 1);   /* 0x0D */
    }
    else
    {
        /*
        **  Find RF Band setting
        */
        for (idx=0; idx<10; ++idx)
        {
            if (pInfo->RF_Bands[idx] >= f_in)
            {
                RFBand = 11 - idx;
                break;
            }
        }
    }

    /*
    **  Assign in the requested values
    */
    pInfo->AS_Data.f_in = f_in;
    pInfo->AS_Data.f_out = f_out;

    /*  Program LO1 and LO2 for a 1st IF frequency of x[i]  */
    pInfo->AS_Data.f_LO1 = CalcLO1Mult(&LO1, &Num1, (UData_t) f_IF1 + f_in, pInfo->AS_Data.f_LO1_Step, pInfo->AS_Data.f_ref);
    pInfo->AS_Data.f_LO2 = Round_fLO(pInfo->AS_Data.f_LO1 - f_out - f_in, pInfo->AS_Data.f_LO2_Step, pInfo->AS_Data.f_ref);
    pInfo->AS_Data.f_LO2 = CalcLO2Mult(&LO2, &Num2, pInfo->AS_Data.f_LO2, pInfo->AS_Data.f_LO2_Step, pInfo->AS_Data.f_ref);

    pInfo->reg[LO1C_1] = (U8Data)((RFBand << 4) | (Num1 >> 2));
    pInfo->reg[LO1C_2] = (U8Data)(LO1 & 0xFF);

    pInfo->reg[LO2C_1] = (U8Data)((Num2 & 0x000F) | ((Num1 & 0x03) << 4));
    pInfo->reg[LO2C_2] = (U8Data)((Num2 & 0x0FF0) >> 4);
    pInfo->reg[LO2C_3] = (U8Data)(((LO2 << 1) & 0xFE) | ((Num2 & 0x1000) >> 12));

    /*  Now write out the computed register values  */
    status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, &pInfo->reg[LO1C_1], 5);   /* 0x01 - 0x05 */

    if (MT_NO_ERROR(status))
        status |= MT2060_GetLocked((Handle_t) pInfo);

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_LocateIF1
**
**  Description:    Locates the MT2060 1st IF frequency with the help
**                  of a demodulator.
**
**                  The caller should examine the returned value P_max
**                  and verify that it is within the acceptable limits
**                  for the demodulator input levels.  If P_max is too
**                  large or small, the IF AGC voltage should be adjusted and
**                  this routine should be called again.  The gain slope
**                  of the IF AGC is approximately 20 dB/V.
**
**                  If P_max is acceptable, the user should then save
**                  f_IF1 in permanent storage (such as EEPROM).  After
**                  powering up and after calling MT2060_Open(), the
**                  user should call MT2060_SetIF1Center() with the saved
**                  f_IF1 to set the MT2060's 1st IF center frequency.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  f_in         - Input frequency to the MT2060
**                                 If 0 Hz, use injected LO frequency
**                  f_out        - Output frequency from MT2060
**                  *f_IF1       - 1st IF filter center frequency (output)
**                  *P_max       - maximum measured output power level (output)
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**
**  Dependencies:   None
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-15-2004    DAD    Original.
**   N/A   06-28-2004    DAD    Changed to use MT2060_GetFMF().
**   N/A   06-28-2004    DAD    Fix RF Band selection when f_in != 0.
**   N/A   07-01-2004    DAD    Place limits on FMF so that LO1 and LO2
**                              frequency ranges aren't breached.
**   N/A   09-03-2004    DAD    Add mean Offset of FMF vs. 1220 MHz over 
**                              many devices when choosing the spline center
**                              point.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   081   01-21-2005    DAD    Ver 1.11: Changed LNA Band names to RF Band
**                              to match description in the data sheet.
**   102   08-16-2005    DAD    Ver 1.15: Fixed-point version added.  Location
**                              of 1st IF filter center requires 9-11 samples
**                              instead of 7 for floating-point version.
**   101   09-15-2005    DAD    Ver 1.15: Re-wrote max- and zero-finding
**                              routines to take advantage of known cubic
**                              polynomial coefficients.
**
******************************************************************************/
UData_t MT2060_LocateIF1(Handle_t h, 
                         UData_t f_in, 
                         UData_t f_out, 
                         UData_t* f_IF1, 
                         SData_t* P_max)
{
    UData_t status = MT_OK;
    UData_t i;
    UData_t f_FMF;
    SData_t Pmeas;
    UData_t i_max = 1;

#if defined (__FLOATING_POINT__)
    FData_t x[9], y[9], y2[9];
    FData_t x_max, y_max;
    FData_t xm, xp;
#else
    UData_t x[9];
    UData_t x_lo, x_hi;
    SData_t y[9];
    SData_t y_lo, y_hi, y_meas;
    UData_t dx, xm, xp;
    UData_t n;
#endif
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;
        
    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    if ((f_IF1 == NULL) || (P_max == NULL))
        return MT_ARG_NULL;

    /*  Get a new FMF value from the MT2060  */
    status |= MT2060_GetFMF(h);

    if (MT_IS_ERROR(status))
        return (status);

    f_FMF = pInfo->reg[FM_FREQ];
    /*  Enforce limits on FMF so that the LO1 & LO2 limits are not breached  */
    /*  1088 <= f_LO1 <= 2214  */
    /*  1041 <= f_LO2 <= 1310  */
    if (f_FMF < 49)
        f_FMF = 49;
    if (f_FMF < ((f_out / 1000000) + 2))
        f_FMF = ((f_out / 1000000) + 2);
    if (f_FMF > ((f_out / 1000000) + 181))
        f_FMF = ((f_out / 1000000) + 181);

    /*  Set the frequencies to be used by the cubic spline  */
    x[0] = 1000000 * (pInfo->AS_Data.f_ref / 16000000) * (f_FMF + 956);
    x[8] = 1000000 * (pInfo->AS_Data.f_ref / 16000000) * (f_FMF + 1212);
    for (i=1; i<8; i++)
        x[i] = 1000000 * (pInfo->AS_Data.f_ref / 16000000) * (f_FMF + 1024 + (15 * i));

    /*  Measure the power levels for the cubic spline at the given frequencies */
    for (i=1; ((i<8) && (MT_NO_ERROR(status))); i++)
    {
        /*  Program LO1 and LO2 for a 1st IF frequency of x[i]  */
        status |= MT2060_Program_LOs(pInfo, f_in, (UData_t) x[i], f_out);

        if (MT_NO_ERROR(status))
        {
            /*  Ask the demodulator to measure the gain of the MT2060  */
            Pmeas = (SData_t) x[i];
            status |= MT_TunerGain(pInfo->hUserData, &Pmeas);
#if defined (__FLOATING_POINT__)
            y[i] = Pmeas / 100.0;
#else
            y[i] = Pmeas;
#endif

            /*  Keep track of the maximum power level measured  */
            if ((i == 1) || (Pmeas > *P_max))
            {
                i_max = i;
                *P_max = Pmeas;
            }
        }
    }
    /*  Force end-points to be -50 dBc  */
#if defined (__FLOATING_POINT__)
    y[0] = y[8] = (FData_t) *P_max / 100.0 - 50.0;
#else
    y[0] = y[8] = *P_max - 5000;
#endif

    if (f_in == 0)                      /*  want to try, even if there were errors  */
    {
        /*  Turn off LO1 feed through mode  */
        pInfo->reg[LO1_2] &= 0xBF;
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_2, &pInfo->reg[LO1_2], 1);   /* 0x0D */
    }

    if (MT_NO_ERROR(status))
    {
#if defined (__FLOATING_POINT__)
        /*  Calculate the 2nd derivatives for each data point  */
        spline(x, y, y2);

        /*  Find the maximum of the cubic-spline curve */
        y_max = findMax(x, y, y2, &x_max);

        /*  Move the entire curve such that the peak is at +1.5 dB  */
        for (i=0;i<9;i++)
            y[i]=y[i]-y_max+1.5;

        /*  Find the -1.5 dBc points to the left and right of the peak  */
        xm = findRoot(x, y, y2, x_max, x[0]);
        xp = findRoot(x, y, y2, x_max, x[8]);
#else
        /*  The peak of the filter is x[i_max], y[i_max]  */

        /*  To Do:  Change y[] measurements type UData_t  */

        /*  Move the entire curve such that the peak is at +1.5 dB  */
        for (i=0;i<9;i++)
            y[i] -= (*P_max-150);

        /*
        **  Look for the 1.5 dBc (low frequency side)
        **  We are guaranteed that x[0] & x[8] are 50 dB
        **  below the peak measurement.
        */
        i = i_max - 1;
        while (y[i] > 0)
            i--;

        /*  The 1.5 dBc point is now bounded by x[i] .. x[i+1]  */
        y_lo = y[i];
        x_lo = x[i];
        y_hi = y[i+1];
        x_hi = x[i+1];

        /*
        **
        **               (x_hi, y_hi)
        **  Peak               o--------------o------
        **                    /
        **                  /
        **                /
        **  1.5 dBc ..../............................
        **            /
        **          /
        **         o
        **   (x_lo, y_lo)
        **
        */
        n = 0;                          /*  # times through the loop         */
        dx = (-y_lo < y_hi) ? 10000000 : 5000000;
        while (((x_hi - x_lo) > 5000000) && (++n<3))
        {
            status |= MT2060_Program_LOs(pInfo, f_in, x_hi - dx, f_out);
            if (MT_NO_ERROR(status))
            {
                y_meas = (SData_t) (x_hi - dx);
                status |= MT_TunerGain(pInfo->hUserData, &y_meas);
                if (MT_NO_ERROR(status))
                {
                    y_meas -= (*P_max-150);
                    if (y_meas > 0)
                    {
                        y_hi = y_meas;
                        x_hi = x_hi - dx;
                    }
                    else
                    {
                        y_lo = y_meas;
                        x_lo = x_hi - dx;
                    }
                }
            }
            dx = 5000000;
        }
        /*  Use linear interpolation  */
        xm = x_lo + (-y_lo * (x_hi - x_lo)) / (y_hi - y_lo);

        /*
        **  Look for the 1.5 dBc (high frequency side)
        **  We are guaranteed that x[0] & x[8] are 50 dB
        **  below the peak measurement.
        */
        i = i_max + 1;
        while (y[i] > 0)
            i++;

        /*  The 1.5 dBc point is now bounded by x[i-1] .. x[i]  */
        y_lo = y[i-1];
        x_lo = x[i-1];
        y_hi = y[i];
        x_hi = x[i];

        n = 0;                          /*  # times through the loop         */
        dx = (-y_hi > y_lo) ? 5000000 : 10000000;
        while (((x_hi - x_lo) > 5000000) && (++n<3))
        {
            status |= MT2060_Program_LOs(pInfo, f_in, x_lo + dx, f_out);
            if (MT_NO_ERROR(status))
            {
                y_meas = (SData_t) (x_lo + dx);
                status |= MT_TunerGain(pInfo->hUserData, &y_meas);
                if (MT_NO_ERROR(status))
                {
                    y_meas -= (*P_max-150);
                    if (y_meas > 0)
                    {
                        y_lo = y_meas;
                        x_lo += dx;
                    }
                    else
                    {
                        y_hi = y_meas;
                        x_hi = x_lo + dx;
                    }
                }
            }
            dx = 5000000;
        }
        /*  Use linear interpolation  */
        xp = x_lo + (y_lo * (x_hi - x_lo)) / (y_lo - y_hi);
#endif
        *f_IF1 = (UData_t) ((xm + xp) / 2);
    }

    return (status);
}
