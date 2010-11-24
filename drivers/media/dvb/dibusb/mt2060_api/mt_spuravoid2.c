/*****************************************************************************
**
**  Name: mt_spuravoid2.c
**
**  Description:    Microtune single tuner spur avoidance software module.
**                  Supports Microtune tuner drivers.
**
**  CVS ID:         $Id: mt_spuravoid2.c,v 1.3 2005/02/11 16:08:04 software Exp $
**  CVS Source:     $Source: /export/home/cvsroot/wwwdev/html/software/tuners/MT2060/mt_spuravoid2.c,v $
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   083   02-08-2005    JWS    Ver 1.11: Added separate version number for
**                              expected version of MT_SpurAvoid.h
**   086   02-08-2005    JWS    Ver 1.11: Added call to MT_RegisterTuner().
**
*****************************************************************************/
#include "mt_spuravoid.h"
//#include <stdlib.h>
#include "../dvb-dibusb.h"  // Added for NULL
//#include <assert.h>  /*  debug purposes  */

#define assert(x)           // disable assert check


/*  Version of this module                         */
#define VERSION 10101             /*  Version 01.11 */


/*  Implement ceiling, floor functions.  */
#define ceil(n, d) (((n) < 0) ? (-((-(n))/(d))) : (n)/(d) + ((n)%(d) != 0))
#define floor(n, d) (((n) < 0) ? (-((-(n))/(d))) - ((n)%(d) != 0) : (n)/(d))


struct MT_FIFZone_t;

struct MT_FIFZone_t
{
    SData_t         min_;
    SData_t         max_;
};



UData_t MT_RegisterTuner(MT_AvoidSpursData_t* pAS_Info)
{
    pAS_Info->nAS_Algorithm = 1;

    return MT_OK;
}


/* Does not provide any function in this configuration */
void MT_UnRegisterTuner(MT_AvoidSpursData_t* pAS_Info)
{
    pAS_Info;
}


//
//  Reset all exclusion zones.
//  Add zones to protect the PLL FracN regions near zero
//
void MT_ResetExclZones(MT_AvoidSpursData_t* pAS_Info)
{
    UData_t center;

    pAS_Info->nZones = 0;                           //  this clears the used list
    pAS_Info->usedZones = NULL;                     //  reset ptr
    pAS_Info->freeZones = NULL;                     //  reset ptr

    center = pAS_Info->f_ref * ((pAS_Info->f_if1_Center - pAS_Info->f_if1_bw/2 + pAS_Info->f_in) / pAS_Info->f_ref) - pAS_Info->f_in;
    while (center < pAS_Info->f_if1_Center + pAS_Info->f_if1_bw/2 + pAS_Info->f_LO1_FracN_Avoid)
    {
        //  Exclude LO1 FracN
        MT_AddExclZone(pAS_Info, center-pAS_Info->f_LO1_FracN_Avoid, center-1);
        MT_AddExclZone(pAS_Info, center+1, center+pAS_Info->f_LO1_FracN_Avoid);
        center += pAS_Info->f_ref;
    }

    center = pAS_Info->f_ref * ((pAS_Info->f_if1_Center - pAS_Info->f_if1_bw/2 - pAS_Info->f_out) / pAS_Info->f_ref) + pAS_Info->f_out;
    while (center < pAS_Info->f_if1_Center + pAS_Info->f_if1_bw/2 + pAS_Info->f_LO2_FracN_Avoid)
    {
        //  Exclude LO2 FracN
        MT_AddExclZone(pAS_Info, center-pAS_Info->f_LO2_FracN_Avoid, center-1);
        MT_AddExclZone(pAS_Info, center+1, center+pAS_Info->f_LO2_FracN_Avoid);
        center += pAS_Info->f_ref;
    }
}


static struct MT_ExclZone_t* InsertNode(MT_AvoidSpursData_t* pAS_Info,
                                  struct MT_ExclZone_t* pPrevNode)
{
    struct MT_ExclZone_t* pNode;
    /*  Check for a node in the free list  */
    if (pAS_Info->freeZones != NULL)
    {
        /*  Use one from the free list  */
        pNode = pAS_Info->freeZones;
        pAS_Info->freeZones = pNode->next_;
    }
    else
    {
        /*  Grab a node from the array  */
        pNode = &pAS_Info->MT_ExclZones[pAS_Info->nZones];
    }

    if (pPrevNode != NULL)
    {
        pNode->next_ = pPrevNode->next_;
        pPrevNode->next_ = pNode;
    }
    else    /*  insert at the beginning of the list  */
    {
        pNode->next_ = pAS_Info->usedZones;
        pAS_Info->usedZones = pNode;
    }

    pAS_Info->nZones++;
    return pNode;
}


static struct MT_ExclZone_t* RemoveNode(MT_AvoidSpursData_t* pAS_Info,
                                  struct MT_ExclZone_t* pPrevNode,
                                  struct MT_ExclZone_t* pNodeToRemove)
{
    struct MT_ExclZone_t* pNext = pNodeToRemove->next_;

    /*  Make previous node point to the subsequent node  */
    if (pPrevNode != NULL)
        pPrevNode->next_ = pNext;

    /*  Add pNodeToRemove to the beginning of the freeZones  */
    pNodeToRemove->next_ = pAS_Info->freeZones;
    pAS_Info->freeZones = pNodeToRemove;

    /*  Decrement node count  */
    pAS_Info->nZones--;

    return pNext;
}


/*
**  Add (and merge) an exclusion zone into the list.
**  If the range (f_min, f_max) is totally outside the 1st IF BW,
**  ignore the entry.
**
*/
void MT_AddExclZone(MT_AvoidSpursData_t* pAS_Info,
                    UData_t f_min,
                    UData_t f_max)
{
    /*  Check to see if this overlaps the 1st IF filter  */
    if ((f_max > (pAS_Info->f_if1_Center - (pAS_Info->f_if1_bw / 2)))
        && (f_min < (pAS_Info->f_if1_Center + (pAS_Info->f_if1_bw / 2))))
    {
        /*
        **                1           2          3        4         5          6
        **
        **   New entry:  |---|    |--|        |--|       |-|      |---|         |--|
        **                     or          or        or       or        or
        **   Existing:  |--|          |--|      |--|    |---|      |-|      |--|
        */
        struct MT_ExclZone_t* pNode = pAS_Info->usedZones;
        struct MT_ExclZone_t* pPrev = NULL;
        struct MT_ExclZone_t* pNext = NULL;

        /*  Check for our place in the list  */
        while ((pNode != NULL) && (pNode->max_ < f_min))
        {
            pPrev = pNode;
            pNode = pNode->next_;
        }

        if ((pNode != NULL) && (pNode->min_ < f_max))
        {
            /*  Combine me with pNode  */
            if (f_min < pNode->min_)
                pNode->min_ = f_min;
            if (f_max > pNode->max_)
                pNode->max_ = f_max;
        }
        else
        {
            pNode = InsertNode(pAS_Info, pPrev);
            pNode->min_ = f_min;
            pNode->max_ = f_max;
        }

        /*  Look for merging possibilities  */
        pNext = pNode->next_;
        while ((pNext != NULL) && (pNext->min_ < pNode->max_))
        {
            if (pNext->max_ > pNode->max_)
                pNode->max_ = pNext->max_;
            pNext = RemoveNode(pAS_Info, pNode, pNext);  /*  Remove pNext, return ptr to pNext->next  */
        }
    }
}


//
//  Return f_Desired if it is not excluded.
//  Otherwise, return the value closest to f_Center that is not excluded
//
UData_t MT_ChooseFirstIF(MT_AvoidSpursData_t* pAS_Info)
{
    const UData_t f_Desired = pAS_Info->f_if1_Request;
    const UData_t f_Step = (pAS_Info->f_LO1_Step > pAS_Info->f_LO2_Step) ? pAS_Info->f_LO1_Step : pAS_Info->f_LO2_Step;
    UData_t f_Center;
    
    SData_t i;
    SData_t j = 0;
    UData_t bDesiredExcluded = 0;
    UData_t bZeroExcluded = 0;
    SData_t tmpMin, tmpMax;
    SData_t bestDiff;
    struct MT_ExclZone_t* pNode = pAS_Info->usedZones;
    struct MT_FIFZone_t zones[MAX_ZONES];

    if (pAS_Info->nZones == 0)
        return f_Desired;

    //  f_Center needs to be an integer multiple of f_Step away from f_Desired
    if ((pAS_Info->f_if1_Center + f_Step/2) > f_Desired)
        f_Center = f_Desired + f_Step * (((pAS_Info->f_if1_Center + f_Step/2) - f_Desired) / f_Step);
    else
        f_Center = f_Desired - f_Step * ((f_Desired - (pAS_Info->f_if1_Center - f_Step/2)) / f_Step);

    //  Take MT_ExclZones, center around f_Center and change the resolution to f_Step
    while (pNode != NULL)
    {
            //  floor function
            tmpMin = floor((SData_t) (pNode->min_ - f_Center), (SData_t) f_Step);

            //  ceil function
            tmpMax = ceil((SData_t) (pNode->max_ - f_Center), (SData_t) f_Step);

        if ((pNode->min_ < f_Desired) && (pNode->max_ > f_Desired))
            bDesiredExcluded = 1;

        if ((tmpMin < 0) && (tmpMax > 0))
            bZeroExcluded = 1;

        //  See if this zone overlaps the previous
        if ((j>0) && (tmpMin < zones[j-1].max_))
            zones[j-1].max_ = tmpMax;
        else
        {
            //  Add new zone
            assert(j<MAX_ZONES);
            zones[j].min_ = tmpMin;
            zones[j].max_ = tmpMax;
            j++;
        }
        pNode = pNode->next_;
    }

    /*
    **  If the desired is okay, return with it
    */
    if (bDesiredExcluded == 0)
        return f_Desired;

    /*
    **  If the desired is excluded and the center is okay, return with it
    */
    if (bZeroExcluded == 0)
        return f_Center;

    //  Find the value closest to 0 (f_Center)
    bestDiff = zones[j-1].max_;
    for (i=j-1; i>=0; i--)
    {
        if (abs(zones[i].max_) > abs(bestDiff))
            break;
        bestDiff = zones[i].max_;

        if (abs(zones[i].min_) > abs(bestDiff))
            break;
        bestDiff = zones[i].min_;
    }

    if (bestDiff < 0)
        return f_Center - ((UData_t) (-bestDiff) * f_Step);

    return f_Center + (bestDiff * f_Step);
}


/****************************************************************************
**
**  Name: gcd
**
**  Description:    Uses Euclid's algorithm
**
**  Parameters:     u, v     - unsigned values whose GCD is desired.
**
**  Global:         None
**
**  Returns:        greatest common divisor of u and v, if either value
**                  is 0, the other value is returned as the result.
**
**  Dependencies:   None.  
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-01-2004    JWS    Original
**   N/A   08-03-2004    DAD    Changed to Euclid's since it can handle
**                              unsigned numbers.
**
****************************************************************************/
static UData_t gcd(UData_t u, UData_t v)
{
    UData_t r;

    while (v != 0)
    {
        r = u % v;
        u = v;
        v = r;
    }

    return u;
}

/****************************************************************************
**
**  Name: umax
**
**  Description:    Implements a simple maximum function for unsigned numbers.
**                  Implemented as a function rather than a macro to avoid
**                  multiple evaluation of the calling parameters.
**
**  Parameters:     a, b     - Values to be compared
**
**  Global:         None
**
**  Returns:        larger of the input values.
**
**  Dependencies:   None.  
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-02-2004    JWS    Original
**
****************************************************************************/
static UData_t umax(UData_t a, UData_t b)
{
    return (a >= b) ? a : b;
}

/****************************************************************************
**
**  Name: IsSpurInBand
**
**  Description:    Checks to see if a spur will be present within the IF's
**                  bandwidth. (fIFOut +/- fIFBW, -fIFOut +/- fIFBW)
**
**                    ma   mb                                     mc   md
**                  <--+-+-+-------------------+-------------------+-+-+-->
**                     |   ^                   0                   ^   |
**                     ^   b=-fIFOut+fIFBW/2      -b=+fIFOut-fIFBW/2   ^
**                     a=-fIFOut-fIFBW/2              -a=+fIFOut+fIFBW/2
**
**                  Note that some equations are doubled to prevent round-off
**                  problems when calculating fIFBW/2
**
**  Parameters:     pAS_Info - Avoid Spurs information block
**                  fm       - If spur, amount f_IF1 has to move negative
**                  fp       - If spur, amount f_IF1 has to move positive
**
**  Global:         None
**
**  Returns:        1 if an LO spur would be present, otherwise 0.
**
**  Dependencies:   None.  
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   11-28-2002    DAD    Implemented algorithm from applied patent 
**
****************************************************************************/
static UData_t IsSpurInBand(MT_AvoidSpursData_t* pAS_Info, 
                            UData_t* fm, 
                            UData_t* fp)
{
    /*
    **  Calculate LO frequency settings.
    */
    UData_t n, n0;
    const UData_t f_LO1 = pAS_Info->f_LO1;
    const UData_t f_LO2 = pAS_Info->f_LO2;
    const UData_t d = pAS_Info->f_out + pAS_Info->f_out_bw/2;
    const UData_t c = d - pAS_Info->f_out_bw;
    const UData_t f = pAS_Info->f_zif_bw/2;
    const UData_t f_Scale = (f_LO1 / (MAX_UDATA/2 / pAS_Info->maxH1)) + 1;
    SData_t f_nsLO1, f_nsLO2;
    SData_t f_Spur;
    UData_t ma, mb, mc, md, me, mf;
    UData_t lo_gcd, gd_Scale, gc_Scale, gf_Scale;

    *fm = 0;

    /*
    ** For each edge (d, c & f), calculate a scale, based on the gcd
    ** of f_LO1, f_LO2 and the edge value.  Use the larger of this
    ** gcd-based scale factor or f_Scale.
    */
    lo_gcd = gcd(f_LO1, f_LO2);
    gd_Scale = umax((UData_t) gcd(lo_gcd, d), f_Scale);
    gc_Scale = umax((UData_t) gcd(lo_gcd, c), f_Scale);
    gf_Scale = umax((UData_t) gcd(lo_gcd, f), f_Scale);

    n0 = ceil(f_LO2 - d, f_LO1 - f_LO2);

    //  Check out all multiples of LO1 from n0 to m_maxLOSpurHarmonic
    for (n=n0; n<=pAS_Info->maxH1; ++n)
    {
        md = (n*(f_LO1/gd_Scale) - (d/gd_Scale)) / (f_LO2/gd_Scale);

        //  If # fLO2 harmonics > m_maxLOSpurHarmonic, then no spurs present
        if (md >= pAS_Info->maxH1)
            break;

        ma = (n*(f_LO1/gd_Scale) + (d/gd_Scale)) / (f_LO2/gd_Scale);

        //  If no spurs between +/- (f_out + f_IFBW/2), then try next harmonic
        if (md == ma)
            continue;

        mc = (n*(f_LO1/gc_Scale) - (c/gc_Scale)) / (f_LO2/gc_Scale);
        if (mc != md)
        {
            f_nsLO1 = (SData_t) (n*(f_LO1/gc_Scale));
            f_nsLO2 = (SData_t) (mc*(f_LO2/gc_Scale));
            f_Spur = (gc_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gc_Scale) - mc*(f_LO2 % gc_Scale);

            *fp = ((f_Spur - (SData_t) c) / (mc - n)) + 1;
            *fm = (((SData_t) d - f_Spur) / (mc - n)) + 1;
            return 1;
        }

        //  Location of Zero-IF-spur to be checked
        me = (n*(f_LO1/gf_Scale) + (f/gf_Scale)) / (f_LO2/gf_Scale);
        mf = (n*(f_LO1/gf_Scale) - (f/gf_Scale)) / (f_LO2/gf_Scale);
        if (me != mf)
        {
            f_nsLO1 = n*(f_LO1/gf_Scale);
            f_nsLO2 = me*(f_LO2/gf_Scale);
            f_Spur = (gf_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gf_Scale) - me*(f_LO2 % gf_Scale);

            *fp = ((f_Spur + (SData_t) f) / (me - n)) + 1;
            *fm = (((SData_t) f - f_Spur) / (me - n)) + 1;
            return 1;
        }

        mb = (n*(f_LO1/gc_Scale) + (c/gc_Scale)) / (f_LO2/gc_Scale);
        if (ma != mb)
        {
            f_nsLO1 = n*(f_LO1/gc_Scale);
            f_nsLO2 = ma*(f_LO2/gc_Scale);
            f_Spur = (gc_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gc_Scale) - ma*(f_LO2 % gc_Scale);

            *fp = (((SData_t) d + f_Spur) / (ma - n)) + 1;
            *fm = (-(f_Spur + (SData_t) c) / (ma - n)) + 1;
            return 1;
        }
    }

    // No spurs found
    return 0;
}


UData_t MT_AvoidSpurs(Handle_t h,
                      MT_AvoidSpursData_t* pAS_Info)
{
    UData_t status = MT_OK;
    UData_t fm, fp;                     /*  restricted range on LO's        */
    pAS_Info->bSpurAvoided = 0;
    pAS_Info->nSpursFound = 0;
    h;

    /*
    **  Avoid LO Generated Spurs
    **
    **  Make sure that have no LO-related spurs within the IF output
    **  bandwidth.
    **
    **  If there is an LO spur in this band, start at the current IF1 frequency
    **  and work out until we find a spur-free frequency or run up against the
    **  1st IF SAW band edge.  Use temporary copies of fLO1 and fLO2 so that they
    **  will be unchanged if a spur-free setting is not found.
    */
    pAS_Info->bSpurPresent = IsSpurInBand(pAS_Info, &fm, &fp);
    if (pAS_Info->bSpurPresent)
    {
        UData_t zfIF1 = pAS_Info->f_LO1 - pAS_Info->f_in; //  current attempt at a 1st IF
        UData_t zfLO1 = pAS_Info->f_LO1;         //  current attempt at an LO1 freq
        UData_t zfLO2 = pAS_Info->f_LO2;         //  current attempt at an LO2 freq
        UData_t delta_IF1;
        UData_t new_IF1;

        //
        //  Spur was found, attempt to find a spur-free 1st IF
        //
        do
        {
            pAS_Info->nSpursFound++;

            //  Raise f_IF1_upper, if needed
            MT_AddExclZone(pAS_Info, zfIF1 - fm, zfIF1 + fp);

            /*  Choose next IF1 that is closest to f_IF1_CENTER              */
            new_IF1 = MT_ChooseFirstIF(pAS_Info);

            if (new_IF1 > zfIF1)
            {
                pAS_Info->f_LO1 += (new_IF1 - zfIF1);
                pAS_Info->f_LO2 += (new_IF1 - zfIF1);
            }
            else
            {
                pAS_Info->f_LO1 -= (zfIF1 - new_IF1);
                pAS_Info->f_LO2 -= (zfIF1 - new_IF1);
            }
            zfIF1 = new_IF1;

            if (zfIF1 > pAS_Info->f_if1_Center)
                delta_IF1 = zfIF1 - pAS_Info->f_if1_Center;
            else
                delta_IF1 = pAS_Info->f_if1_Center - zfIF1;
        }
        //  Continue while the new 1st IF is still within the 1st IF bandwidth
        //  and there is a spur in the band (again)
        while ((2*delta_IF1 + pAS_Info->f_out_bw <= pAS_Info->f_if1_bw) && 
              (pAS_Info->bSpurPresent = IsSpurInBand(pAS_Info, &fm, &fp)));

        /*
        ** Use the LO-spur free values found.  If the search went all the way to
        ** the 1st IF band edge and always found spurs, just leave the original
        ** choice.  It's as "good" as any other.
        */
        if (pAS_Info->bSpurPresent == 1)
        {
            status |= MT_SPUR_PRESENT;
            pAS_Info->f_LO1 = zfLO1;
            pAS_Info->f_LO2 = zfLO2;
        }
        else
            pAS_Info->bSpurAvoided = 1;
    }

    status |= ((pAS_Info->nSpursFound << MT_SPUR_SHIFT) & MT_SPUR_CNT_MASK);
    
    return (status);
}


UData_t MT_AvoidSpursVersion(void)
{
    return (VERSION);
}
