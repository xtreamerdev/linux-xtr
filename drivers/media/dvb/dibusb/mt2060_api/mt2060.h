/*****************************************************************************
**
**  Name: mt2060.h
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
**  Enumerations
**  Defined:        MT2060_GPO_Mask
**                  MT2060_Temperature
**                  MT2060_VGA_Gain_Range
**                  MT2060_Ext_SRO
**                  MT2060_Param
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
**  CVS ID:         $Id: mt2060.h,v 1.7 2005/11/02 16:24:10 software Exp $
**  CVS Source:     $Source: /export/home/cvsroot/web05/html/software/tuners/MT2060/mt2060.h,v $
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
**                              expected version of MT_SpurAvoid.h
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
#if !defined( __MT2060_H )
#define __MT2060_H

#include "mt_userdef.h"

#if defined( __cplusplus )     
extern "C"                     /* Use "C" external linkage                  */
{
#endif

/*
**  Parameter for function MT2060_SetGPO that specifies the GPO pin
**  to be read/written.
*/
typedef enum
{
    MT2060_GPO_0 = 0x02,                /*  bitmask for GPO_0               */
    MT2060_GPO_1 = 0x04,                /*  bitmask for GPO_1               */
    MT2060_GPO   = 0x06,                /*  bitmask for GPO_0 | GPO_1       */
} MT2060_GPO_Mask;


/*
**  Values returned by the MT2060's on-chip temperature sensor
**  to be read/written.  Note the 40C resolution.
*/
typedef enum 
{
    MT2060_T_0C = 0,                    /*  Temperature < 0C                */
    MT2060_0C_T_40C,                    /*  0C < Temperature < 40C          */
    MT2060_40C_T_80C,                   /*  40C < Temperature < 80C         */
    MT2060_80C_T,                       /*  80C < Temperature               */
} MT2060_Temperature;


/*
**  Parameter for function MT2060_SetGainRange.
**  Note that the delta between min/max is the same, but the max
**  attainable gain is increased with non-default settings
**
**  MT2060_VGAG_0DB is the power-on default, but MT2060_VGAG_5DB is the
**  default value after initialization.
*/
typedef enum 
{
    MT2060_VGAG_0DB = 0,                /*  VGA gain range                  */
    MT2060_VGAG_5DB = 1,                /*  VGA gain range + 5 dB           */
    MT2060_VGAG_11DB = 3,               /*  VGA gain range + 11 dB          */
} MT2060_VGA_Gain_Range;


/*
**  Parameter for function MT2060_SetExtSRO that specifies the external
**  SRO drive frequency.
**
**  MT2060_EXT_SRO_OFF is the power-up default value.
*/
typedef enum
{
    MT2060_EXT_SRO_OFF,                 /*  External SRO drive off          */
    MT2060_EXT_SRO_BY_4,                /*  External SRO drive divide by 4  */
    MT2060_EXT_SRO_BY_2,                /*  External SRO drive divide by 2  */
    MT2060_EXT_SRO_BY_1                 /*  External SRO drive divide by 1  */
} MT2060_Ext_SRO;

/*
**  Parameter for function MT2060_GetParam & MT2060_SetParam that 
**  specifies the tuning algorithm parameter to be read/written.
*/
typedef enum
{
    /*  tuner address                                  set by MT2060_Open() */
    MT2060_IC_ADDR,

    /*  max number of MT2060 tuners       set by MT2060_CNT in mt_userdef.h */
    MT2060_MAX_OPEN,

    /*  current number of open MT2060 tuners           set by MT2060_Open() */
    MT2060_NUM_OPEN,
    
    /*  crystal frequency                            (default: 16000000 Hz) */
    MT2060_SRO_FREQ,

    /*  min tuning step size                            (default: 50000 Hz) */
    MT2060_STEPSIZE,

    /*  input center frequency                   set by MT2060_ChangeFreq() */
    MT2060_INPUT_FREQ,

    /*  LO1 Frequency                            set by MT2060_ChangeFreq() */
    MT2060_LO1_FREQ,
              
    /*  LO1 minimum step size                          (default: 250000 Hz) */
    MT2060_LO1_STEPSIZE,
    
    /*  LO1 FracN keep-out region                     (default: 2999999 Hz) */
    MT2060_LO1_FRACN_AVOID,

    /*  Current 1st IF in use                    set by MT2060_ChangeFreq() */
    MT2060_IF1_ACTUAL,
          
    /*  Requested 1st IF                         set by MT2060_ChangeFreq() */
    MT2060_IF1_REQUEST,
                
    /*  Center of 1st IF SAW filter                     located by customer */
    MT2060_IF1_CENTER,
                    
    /*  Bandwidth of 1st IF SAW filter               (default: 22000000 Hz) */
    MT2060_IF1_BW,
                    
    /*  zero-IF bandwidth                             (default: 3000000 Hz) */
    MT2060_ZIF_BW,

    /*  LO2 Frequency                            set by MT2060_ChangeFreq() */
    MT2060_LO2_FREQ,

    /*  LO2 minimum step size                           (default: 50000 Hz) */
    MT2060_LO2_STEPSIZE,

    /*  LO2 FracN keep-out region                       (default: 99999 Hz) */
    MT2060_LO2_FRACN_AVOID,

    /*  output center frequency                  set by MT2060_ChangeFreq() */
    MT2060_OUTPUT_FREQ,

    /*  output bandwidth                         set by MT2060_ChangeFreq() */
    MT2060_OUTPUT_BW,

    /*  min inter-tuner LO separation                 (default: 1000000 Hz) */
    MT2060_LO_SEPARATION,

    /*  ID of avoid-spurs algorithm in use            compile-time constant */
    MT2060_AS_ALG,

    /*  max # of intra-tuner harmonics                        (default: 5)  */
    MT2060_MAX_HARM1,

    /*  max # of inter-tuner harmonics                        (default: 5)  */
    MT2060_MAX_HARM2,

    /*  # of 1st IF exclusion zones used         set by MT2060_ChangeFreq() */
    MT2060_EXCL_ZONES,
                 
    /*  # of spurs found/avoided                 set by MT2060_ChangeFreq() */
    MT2060_NUM_SPURS,
          
    /*  >0 spurs avoided                         set by MT2060_ChangeFreq() */
    MT2060_SPUR_AVOIDED,
              
    /*  >0 spurs in output (mathematically)      set by MT2060_ChangeFreq() */
    MT2060_SPUR_PRESENT,

    MT2060_EOP                    /*  last entry in enumerated list         */
} MT2060_Param;

/* ====== Functions which are declared in MT2060.c File ======= */

/******************************************************************************
**
**  Name: MT2060_Open
**
**  Description:    Initialize the tuner's register values.
**
**  Usage:          status = MT2060_Open(0xC0, &hMT2060, NULL);
**                  if (MT_IS_ERROR(status))
**                      //  Check error codes for reason
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
**
******************************************************************************/
UData_t MT2060_Open(UData_t MT2060_Addr,
                    Handle_t* hMT2060,
                    Handle_t hUserData);


/******************************************************************************
**
**  Name: MT2060_Close
**
**  Description:    Release the handle to the tuner.
**
**  Parameters:     hMT2060      - Handle to the MT2060 tuner
**
**  Usage:          status = MT2060_Close(hMT2060);
**
**  Returns:        status:
**                      MT_OK         - No errors
**                      MT_INV_HANDLE - Invalid tuner handle
**
**  Dependencies:   MT_ErrorDef.h - definition of error codes
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   09-13-2004    DAD    Original.
**   086   02-08-2005    JWS    Ver 1.11: Added call to MT_UnRegisterTuner().
**
******************************************************************************/
UData_t MT2060_Close(Handle_t hMT2060);


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
**  Usage:          status = MT2060_Open(0xC0, &hMT2060, NULL);
**                  if (MT_NO_ERROR(status))
**                      status |= MT2060_LocateIF1(hMT2060,
**                                                 375000000UL,
**                                                 43750000UL,
**                                                 &f_if1_Center,
**                                                 &P_max);
**
**                  if (MT_NO_ERROR(status)) {
**                      //  Check value of P_max
**                      //  If OK, store f_if1_Center in non-volatile memory.
**                  }
**
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
**
******************************************************************************/
UData_t MT2060_LocateIF1(Handle_t h, 
                         UData_t f_in, 
                         UData_t f_out, 
                         UData_t* f_IF1, 
                         SData_t* P_max);


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
**  Usage:          status = MT2060_ChangeFreq(hMT2060,
**                                             f_in,
**                                             f_if1_Center,
**                                             43750000UL,
**                                             6750000UL);
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
                          UData_t f_in, 
                          UData_t f_IF1, 
                          UData_t f_out, 
                          UData_t f_IFBW);


/******************************************************************************
**
**  Name: MT2060_GetFMF
**
**  Description:    Get a new FMF register value.
**                  Uses The single step mode of the MT2060's FMF function.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**
**  Usage:          status = MT2060_GetFMF(hMT2060);
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
UData_t MT2060_GetFMF(Handle_t h);


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
**  Usage:          status = MT2060_GetGPO(hMT2060, MT2060_GPO, &value);
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
UData_t MT2060_GetGPO(Handle_t h, UData_t gpo, UData_t* value);


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
**  Usage:          status = MT2060_GetIF1Center(hMT2060, &f_if1_Center);
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
UData_t MT2060_GetIF1Center(Handle_t h, UData_t* value);


/****************************************************************************
**
**  Name: MT2060_GetLocked
**
**  Description:    Checks to see if LO1 and LO2 are locked.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**
**  Usage:          status = MT2060_GetLocked(hMT2060);
**                  if (status & (MT_UPC_UNLOCK | MT_DNC_UNLOCK))
**                      //  error!, one or more PLL's unlocked
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
UData_t MT2060_GetLocked(Handle_t h);


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
                        UData_t*     pValue);


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
                      U8Data*  val);


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
**  Usage:          status = MT2060_GetTemp(hMT2060, &temperature);
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
**
******************************************************************************/
UData_t MT2060_GetTemp(Handle_t h, MT2060_Temperature* value);


/****************************************************************************
**
**  Name: MT2060_GetUserData
**
**  Description:    Gets the user-defined data item.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**
**  Usage:          status = MT2060_GetUserData(hMT2060, &hUserData);
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
                           Handle_t* hUserData);


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
**
******************************************************************************/
UData_t MT2060_ReInit(Handle_t h);


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
                         MT2060_Ext_SRO Ext_SRO_Setting);


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
**  Usage:          status = MT2060_SetGainRange(hMT2060, MT2060_VGAG_5DB);
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
**
******************************************************************************/
UData_t MT2060_SetGainRange(Handle_t h, UData_t range);


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
**  Usage:          status = MT2060_SetGPO(hMT2060, MT2060_GPO, 3);
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
UData_t MT2060_SetGPO(Handle_t h, UData_t gpo, UData_t value);


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
**  Usage:          status = MT2060_SetIF1Center(hMT2060, f_if1_Center);
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
UData_t MT2060_SetIF1Center(Handle_t h, UData_t value);


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
                        UData_t      nValue);


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
                      U8Data   val);


/****************************************************************************
**
**  Name: MT2060_SetUserData
**
**  Description:    Sets the user-defined data item.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  hUserData   - ptr to user-defined data
**
**  Usage:          status = MT2060_SetUserData(hMT2060, hUserData);
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
                           Handle_t hUserData);


#if defined( __cplusplus )
}
#endif

#endif

