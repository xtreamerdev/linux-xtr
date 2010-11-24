/*****************************************************************************
**
**  Name: MT_SpurAvoid.h
**
**  Description:    Implements spur avoidance for MicroTuners
**
**  References:     None
**
**  Exports:        None
**
**  CVS ID:         $Id: mt_spuravoid.h,v 1.3 2005/04/13 18:46:18 software Exp $
**  CVS Source:     $Source: /export/home/cvsroot/web05/html/software/tuners/MT2060/mt_spuravoid.h,v $
**	               
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   03-25-2004    DAD    Original
**   083   02-08-2005    JWS    Added separate version number for expected
**                              version of MT_SpurAvoid.h
**   083   02-08-2005    JWS    Added function to return the version of the
**                              MT_AvoidSpurs source file.
**   098   04-12-2005    DAD    Increased MAX_ZONES from 32 to 48.  32 is
**                              sufficient for the default avoidance params.
**
*****************************************************************************/
#if !defined(__MT_SPURAVOID_H)
#define __MT_SPURAVOID_H

#include "mt_userdef.h"

#if defined( __cplusplus )
extern "C"                     /* Use "C" external linkage                  */
{
#endif

/*
**  Constant defining the version of the following structure
**  and therefore the API for this code.
**
**  When compiling the tuner driver, the preprocessor will 
**  check against this version number to make sure that 
**  it matches the version that the tuner driver knows about.
*/
/* Version 010200 => 1.20 */
#define MT_AVOID_SPURS_INFO_VERSION 010200

#define MAX_ZONES 48

struct MT_ExclZone_t;

struct MT_ExclZone_t
{
    UData_t         min_;
    UData_t         max_;
    struct MT_ExclZone_t*  next_;
};

/*
**  Structure of data needed for Spur Avoidance
*/
typedef struct
{
    UData_t nAS_Algorithm;
    UData_t f_ref;
    UData_t f_in;
    UData_t f_LO1;
    UData_t f_if1_Center;
    UData_t f_if1_Request;
    UData_t f_if1_bw;
    UData_t f_LO2;
    UData_t f_out;
    UData_t f_out_bw;
    UData_t f_LO1_Step;
    UData_t f_LO2_Step;
    UData_t f_LO1_FracN_Avoid;
    UData_t f_LO2_FracN_Avoid;
    UData_t f_zif_bw;
    UData_t f_min_LO_Separation;
    UData_t maxH1;
    UData_t maxH2;
    UData_t bSpurPresent;
    UData_t bSpurAvoided;
    UData_t nSpursFound;
    UData_t nZones;
    struct MT_ExclZone_t* freeZones;
    struct MT_ExclZone_t* usedZones;
    struct MT_ExclZone_t MT_ExclZones[MAX_ZONES];
} MT_AvoidSpursData_t;

UData_t MT_RegisterTuner(MT_AvoidSpursData_t* pAS_Info);

void MT_UnRegisterTuner(MT_AvoidSpursData_t* pAS_Info);

void MT_ResetExclZones(MT_AvoidSpursData_t* pAS_Info);

void MT_AddExclZone(MT_AvoidSpursData_t* pAS_Info,
                    UData_t f_min,
                    UData_t f_max);

UData_t MT_ChooseFirstIF(MT_AvoidSpursData_t* pAS_Info);

UData_t MT_AvoidSpurs(Handle_t h,
                     MT_AvoidSpursData_t* pAS_Info);

UData_t MT_AvoidSpursVersion(void);

#if defined( __cplusplus )
}
#endif

#endif
