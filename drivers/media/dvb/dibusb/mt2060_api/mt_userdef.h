/*****************************************************************************
**
**  Name: mt_userdef.h
**
**  Description:    User-defined data types needed by MicroTuner source code.
**
**                  Customers must provide the code for these functions
**                  in the file "mt_userdef.c".
**
**                  Customers must verify that the typedef's in the 
**                  "Data Types" section are correct for their platform.
**
**  Functions
**  Requiring
**  Implementation: MT_WriteSub
**                  MT_ReadSub
**                  MT_Sleep
**
**  References:     None
**
**  Exports:        None
**
**  CVS ID:         $Id: mt_userdef.h,v 1.5 2005/11/02 16:24:56 software Exp $
**  CVS Source:     $Source: /export/home/cvsroot/web05/html/software/tuners/MT2060/mt_userdef.h,v $
**	               
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   03-25-2004    DAD    Original
**   082   12-06-2004    JWS    Multi-tuner support - requires MTxxxx_CNT 
**                              declarations
**
*****************************************************************************/
#if !defined( __MT_USERDEF_H )
#define __MT_USERDEF_H

#include "mt_errordef.h"

#if defined( __cplusplus )     
extern "C"                     /* Use "C" external linkage                  */
{
#endif

/*
**  Data Types
*/
typedef unsigned char   U8Data;         /*  type corresponds to 8 bits      */
typedef unsigned int    UData_t;        /*  type must be at least 32 bits   */
typedef int             SData_t;        /*  type must be at least 32 bits   */
typedef void *          Handle_t;       /*  memory pointer type             */
typedef double          FData_t;        /*  floating point data type        */

/*
** Define an MTxxxx_CNT macro for each type of tuner that will be built
** into your application (e.g., MT2121, MT2060). MT_TUNER_CNT
** must be set to the SUM of all of the MTxxxx_CNT macros.
**
** #define MT2050_CNT  (1)
*/
#define MT2060_CNT  (1)
/*
** #define MT2111_CNT  (1)
** #define MT2121_CNT  (3)
*/

#define MT_TUNER_CNT               (1)  /*  total num of MicroTuner tuners  */
#define MAX_UDATA         (4294967295)  /*  max value storable in UData_t   */

/*
**  Optional user-defined Error/Info Codes  (examples below)
**
**  This is the area where you can define user-specific error/info return
**  codes to be returned by any of the functions you are responsible for
**  writing such as MT_WriteSub() and MT_ReadSub.  There are four bits
**  available in the status field for your use.  When set, these
**  bits will be returned in the status word returned by any tuner driver
**  call.  If you OR in the MT_ERROR bit as well, the tuner driver code
**  will treat the code as an error.
**
**  The following are a few examples of errors you can provide.
**
**  Example 1:
**  You might check to see the hUserData handle is correct and issue 
**  MY_USERDATA_INVALID which would be defined like this:
**
**  #define MY_USERDATA_INVALID  (MT_USER_ERROR | MT_USER_DEFINED1)
**
**
**  Example 2:
**  You might be able to provide more descriptive two-wire bus errors:
**
**  #define NO_ACK   (MT_USER_ERROR | MT_USER_DEFINED1)
**  #define NO_NACK  (MT_USER_ERROR | MT_USER_DEFINED2)
**  #define BUS_BUSY (MT_USER_ERROR | MT_USER_DEFINED3)
**
**
**  Example 3:
**  You can also provide information (non-error) feedback:
**
**  #define MY_INFO_1   (MT_USER_DEFINED1)
**
**
**  Example 4:
**  You can combine the fields together to make a multi-bit field.
**  This one can provide the tuner number based off of the addr
**  passed to MT_WriteSub or MT_ReadSub.  It assumes that
**  MT_USER_DEFINED4 through MT_USER_DEFINED1 are contiguously. If
**  TUNER_NUM were OR'ed into the status word on an error, you could
**  use this to identify which tuner had the problem (and whether it
**  was during a read or write operation).
**
**  #define TUNER_NUM  ((addr & 0x07) >> 1) << MT_USER_SHIFT
**
*/

/*****************************************************************************
**
**  Name: MT_WriteSub
**
**  Description:    Write values to device using a two-wire serial bus.
**
**  Parameters:     hUserData  - User-specific I/O parameter that was
**                               passed to tuner's Open function.
**                  addr       - device serial bus address  (value passed
**                               as parameter to MTxxxx_Open)
**                  subAddress - serial bus sub-address (Register Address)
**                  pData      - pointer to the Data to be written to the 
**                               device 
**                  cnt        - number of bytes/registers to be written
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      user-defined
**
**  Notes:          This is a callback function that is called from the
**                  the tuning algorithm.  You MUST provide code for this
**                  function to write data using the tuner's 2-wire serial 
**                  bus.
**
**                  The hUserData parameter is a user-specific argument.
**                  If additional arguments are needed for the user's
**                  serial bus read/write functions, this argument can be
**                  used to supply the necessary information.
**                  The hUserData parameter is initialized in the tuner's Open
**                  function.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   03-25-2004    DAD    Original
**
*****************************************************************************/
UData_t MT_WriteSub(Handle_t hUserData, 
                    UData_t addr, 
                    U8Data subAddress, 
                    U8Data *pData, 
                    UData_t cnt);


/*****************************************************************************
**
**  Name: MT_ReadSub
**
**  Description:    Read values from device using a two-wire serial bus.
**
**  Parameters:     hUserData  - User-specific I/O parameter that was
**                               passed to tuner's Open function.
**                  addr       - device serial bus address  (value passed
**                               as parameter to MTxxxx_Open)
**                  subAddress - serial bus sub-address (Register Address)
**                  pData      - pointer to the Data to be written to the 
**                               device 
**                  cnt        - number of bytes/registers to be written
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      user-defined
**
**  Notes:          This is a callback function that is called from the
**                  the tuning algorithm.  You MUST provide code for this
**                  function to read data using the tuner's 2-wire serial 
**                  bus.
**
**                  The hUserData parameter is a user-specific argument.
**                  If additional arguments are needed for the user's
**                  serial bus read/write functions, this argument can be
**                  used to supply the necessary information.
**                  The hUserData parameter is initialized in the tuner's Open
**                  function.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   03-25-2004    DAD    Original
**
*****************************************************************************/
UData_t MT_ReadSub(Handle_t hUserData, 
                   UData_t addr, 
                   U8Data subAddress, 
                   U8Data *pData, 
                   UData_t cnt);


/*****************************************************************************
**
**  Name: MT_Sleep
**
**  Description:    Delay execution for "nMinDelayTime" milliseconds
**
**  Parameters:     hUserData     - User-specific I/O parameter that was
**                                  passed to tuner's Open function.
**                  nMinDelayTime - Delay time in milliseconds
**
**  Returns:        None.
**
**  Notes:          This is a callback function that is called from the
**                  the tuning algorithm.  You MUST provide code that
**                  blocks execution for the specified period of time. 
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   03-25-2004    DAD    Original
**
*****************************************************************************/
void MT_Sleep(Handle_t hUserData,
              UData_t nMinDelayTime);


#if defined(MT2060_CNT)
#if MT2060_CNT > 0
/*****************************************************************************
**
**  Name: MT_TunerGain  (for MT2060 only)
**
**  Description:    Measure the relative tuner gain using the demodulator
**
**  Parameters:     hUserData  - User-specific I/O parameter that was
**                               passed to tuner's Open function.
**                  pMeas      - Tuner gain (1/100 of dB scale).
**                               ie. 1234 = 12.34 (dB)
**
**  Returns:        status:
**                      MT_OK  - No errors
**                      user-defined errors could be set
**
**  Notes:          This is a callback function that is called from the
**                  the 1st IF location routine.  You MUST provide
**                  code that measures the relative tuner gain in a dB
**                  (not linear) scale.  The return value is an integer
**                  value scaled to 1/100 of a dB.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-16-2004    DAD    Original
**   N/A   11-30-2004    DAD    Renamed from MT_DemodInputPower.  This name
**                              better describes what this function does.
**
*****************************************************************************/
UData_t MT_TunerGain(Handle_t hUserData,
                     SData_t* pMeas);
                     
                     
                     
#endif
#endif


#define MT2060_DBG      printk

#if defined( __cplusplus )     
}
#endif

#endif
