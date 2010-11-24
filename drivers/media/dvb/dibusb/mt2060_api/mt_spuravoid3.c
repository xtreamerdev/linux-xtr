/*****************************************************************************
**
**  Name: mt_spuravoid3.c
**
**  Description:    Microtune multi-tuner spur avoidance software module.
**                  Supports Microtune tuner drivers.
**
**  CVS ID:         $Id: mt_spuravoid3.c,v 1.1 2005/02/09 23:35:22 software Exp $
**  CVS Source:     $Source: /export/home/cvsroot/wwwdev/html/software/tuners/MT2060/mt_spuravoid3.c,v $
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   082   03-25-2004    JSW    Original multi-tuner support - requires
**                              MTxxxx_CNT declarations
**
*****************************************************************************/
#include "mt_spuravoid.h"
//#include <stdlib.h>
#include "../dvb-dibusb.h"  // Added for NULL
//#include <assert.h>
#define assert(x)           // disable assert check

/*  Version of this module                         */
#define VERSION 10100             /*  Version 01.10 */


/*  Implement ceiling, floor functions.  */
#define ceil(n, d) (((n) < 0) ? (-((-(n))/(d))) : (n)/(d) + ((n)%(d) != 0))
#define floor(n, d) (((n) < 0) ? (-((-(n))/(d))) - ((n)%(d) != 0) : (n)/(d))


struct MT_FIFZone_t;

struct MT_FIFZone_t
{
    SData_t         min_;
    SData_t         max_;
};


static MT_AvoidSpursData_t* TunerList[MT_TUNER_CNT];
static UData_t              TunerCount = 0;


UData_t MT_RegisterTuner(MT_AvoidSpursData_t* pAS_Info)
{
    UData_t index;

    pAS_Info->nAS_Algorithm = 2;

    /*
    **  Check to see if tuner is already registered
    */
    for (index = 0; index < TunerCount; index++)
    {
        if (TunerList[index] == pAS_Info)
        {
            return MT_OK;  // Already here - no problem
        }
    }

    /*
    ** Add tuner to list - if there is room.
    */
    if (TunerCount < MT_TUNER_CNT)
    {
        TunerList[TunerCount] = pAS_Info;
        TunerCount++;
        return MT_OK;
    }
    else
        return MT_TUNER_CNT_ERR;
}


void MT_UnRegisterTuner(MT_AvoidSpursData_t* pAS_Info)
{
    UData_t index;

    for (index = 0; index < TunerCount; index++)
    {
        if (TunerList[index] == pAS_Info)
        {
            TunerList[index] = TunerList[--TunerCount];
        }
    }
}


//
//  Reset all exclusion zones.
//  Add zones to protect the PLL FracN regions near zero
//
void MT_ResetExclZones(MT_AvoidSpursData_t* pAS_Info)
{
    UData_t center;
    UData_t index;
    MT_AvoidSpursData_t* adj;

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

	/*
    ** Iterate through all adjacent tuners and exclude frequencies related to them
    */
    for (index = 0; index < TunerCount; ++index)
    {
        adj = TunerList[index];
        if (pAS_Info == adj)    /* skip over our own data, don't process it */
            continue;

        /*
        **  Add 1st IF exclusion zone covering adjacent tuner's LO2
        **  at "adjfLO2 + f_out" +/- m_MinLOSpacing
        */
        if (adj->f_LO2 != 0)
            MT_AddExclZone(pAS_Info, 
                           adj->f_LO2 + pAS_Info->f_out - pAS_Info->f_min_LO_Separation,
                           adj->f_LO2 + pAS_Info->f_out + pAS_Info->f_min_LO_Separation );

        /*
        **  Add 1st IF exclusion zone covering adjacent tuner's LO1
        **  at "adjfLO1 - f_in" +/- m_MinLOSpacing
        */
        if (adj->f_LO1 != 0)
            MT_AddExclZone(pAS_Info, 
                           adj->f_LO1 - pAS_Info->f_in - pAS_Info->f_min_LO_Separation,
                           adj->f_LO1 - pAS_Info->f_in + pAS_Info->f_min_LO_Separation );
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

    assert(abs((SData_t) f_Center - (SData_t) pAS_Info->f_if1_Center) <= (SData_t) (f_Step/2));

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

static SData_t RoundAwayFromZero(SData_t n, SData_t d)
{
    return (n<0) ? floor(n, d) : ceil(n, d);
}

/****************************************************************************
**
**  Name: IsSpurInAdjTunerBand
**
**  Description:    Checks to see if a spur will be present within the IF's
**                  bandwidth or near the zero IF. 
**                  (fIFOut +/- fIFBW/2, -fIFOut +/- fIFBW/2)
**                                  and
**                  (0 +/- fZIFBW/2)
**
**                    ma   mb               me   mf               mc   md
**                  <--+-+-+-----------------+-+-+-----------------+-+-+-->
**                     |   ^                   0                   ^   |
**                     ^   b=-fIFOut+fIFBW/2      -b=+fIFOut-fIFBW/2   ^
**                     a=-fIFOut-fIFBW/2              -a=+fIFOut+fIFBW/2
**
**                  Note that some equations are doubled to prevent round-off
**                  problems when calculating fIFBW/2
**
**                  The spur frequencies are computed as:
**
**                     fSpur = n * f1 - m * f2 - fOffset
**
**  Parameters:     f1      - The 1st local oscillator (LO) frequency
**                            of the tuner whose output we are examining
**                  f2      - The 1st local oscillator (LO) frequency
**                            of the adjacent tuner
**                  fOffset - The 2nd local oscillator of the tuner whose
**                            output we are examining
**                  fIFOut  - Output IF center frequency
**                  fIFBW   - Output IF Bandwidth
**                  nMaxH   - max # of LO harmonics to search
**                  n       - If spur, # of harmonics of f1 (returned)
**                  m       - If spur, # of harmonics of f2 (returned)
**
**  Returns:        1 if an LO spur would be present, otherwise 0.
**
**  Dependencies:   None.  
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   01-21-2005    JWS    Original, adapted from MT_DoubleConversion.
**
****************************************************************************/
static UData_t IsSpurInAdjTunerBand(UData_t bIsMyOutput,
                                    UData_t f1,
                                    UData_t f2,
                                    UData_t fOffset,
                                    UData_t fIFOut,
                                    UData_t fIFBW,
                                    UData_t fZIFBW,
                                    UData_t nMaxH,
                                    UData_t *fp,
                                    UData_t *fm)
{
    UData_t bSpurFound = 0;

    const UData_t fHalf_IFBW = fIFBW / 2;
    const UData_t fHalf_ZIFBW = fZIFBW / 2;

    /* Calculate a scale factor for all frequencies, so that our
       calculations all stay within 31 bits */
    const SData_t f_Scale = ((f1 + (fOffset + fIFOut + fHalf_IFBW) / nMaxH) / (MAX_UDATA/2 / nMaxH)) + 1;

    const SData_t _f1 = (SData_t) f1 / f_Scale;
    const SData_t _f2 = (SData_t) f2 / f_Scale;
    const SData_t _f3 = (SData_t) fOffset / f_Scale;

    const SData_t c = (SData_t) (fIFOut - fHalf_IFBW) / f_Scale;
    const SData_t d = (SData_t) (fIFOut + fHalf_IFBW) / f_Scale;
    const SData_t f = (SData_t) fHalf_ZIFBW / f_Scale;

    SData_t  ma, mb, mc, md, me, mf;

    SData_t  fp_ = 0;
    SData_t  fm_ = 0;
    SData_t  n;


    /*
    **  If the other tuner does not have an LO frequency defined,
    **  assume that we cannot interfere with it
    */
    if (f2 == 0)
        return 0;


    /* Check out all multiples of f1 from -nMaxH to +nMaxH */
    for (n = -(SData_t)nMaxH; n <= (SData_t)nMaxH; ++n)
    {
        md = (_f3 + d - n*_f1) / _f2;

        /* If # f2 harmonics > nMaxH, then no spurs present */
        if (md <= -(SData_t) nMaxH )
            break;

        ma = (_f3 - d - n*_f1) / _f2;
        if ((ma == md) || (ma >= (SData_t) (nMaxH)))
            continue;

        mc = (_f3 + c - n*_f1) / _f2;
        if (mc != md)
        {
            const SData_t m = (n<0) ? md : mc;
            const SData_t fspur = (n*_f1 + m*_f2 - _f3);
            bSpurFound = 1;
            fp_ = RoundAwayFromZero((d - fspur)* f_Scale, (bIsMyOutput ? n - 1 : n));
            fm_ = RoundAwayFromZero((fspur - c)* f_Scale, (bIsMyOutput ? n - 1 : n));
            break;
        }

        /* Location of Zero-IF-spur to be checked */
        mf = (_f3 + f - n*_f1) / _f2;
        me = (_f3 - f - n*_f1) / _f2;
        if (me != mf)
        {
            const SData_t m = (n<0) ? mf : me;
            const SData_t fspur = (n*_f1 + m*_f2 - _f3);
            bSpurFound = 1;
            fp_ = (SData_t) RoundAwayFromZero((f - fspur)* f_Scale, (bIsMyOutput ? n - 1 : n));
            fm_ = (SData_t) RoundAwayFromZero((fspur + f)* f_Scale, (bIsMyOutput ? n - 1 : n));
            break;
        }

        mb = (_f3 - c - n*_f1) / _f2;
        if (ma != mb)
        {
            const SData_t m = (n<0) ? mb : ma;
            const SData_t fspur = (n*_f1 + m*_f2 - _f3);
            bSpurFound = 1;
            fp_ = (SData_t) RoundAwayFromZero((-c - fspur)* f_Scale, (bIsMyOutput ? n - 1 : n));
            fm_ = (SData_t) RoundAwayFromZero((fspur +d)* f_Scale, (bIsMyOutput ? n - 1 : n));
            break;
        }
    }

    //  Verify that fm & fp are both positive
    //  Add one to ensure next 1st IF choice is not right on the edge
    if (fp_ < 0)
    {
        *fp = -fm_ + 1;
        *fm = -fp_ + 1;
    }
    else if (fp_ > 0)
    {
        *fp = fp_ + 1;
        *fm = fm_ + 1;
    }
    else
    {
        *fp = 1;
        *fm = abs(fm_) + 1;
    }

    return bSpurFound;
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
    UData_t index;

    MT_AvoidSpursData_t *adj;

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

    //  If no spur found, see if there are more tuners on the same board
    for (index = 0; index < TunerCount; ++index)
    {
        adj = TunerList[index];
        if (pAS_Info == adj)    /* skip over our own data, don't process it */
            continue;

        //  Look for LO-related spurs from the adjacent tuner generated into my IF output
        if (IsSpurInAdjTunerBand(1,                   //  check my IF output
                                 pAS_Info->f_LO1,     //  my fLO1
                                 adj->f_LO1,          //  the other tuner's fLO1
                                 pAS_Info->f_LO2,     //  my fLO2
                                 pAS_Info->f_out,     //  my fOut
                                 pAS_Info->f_out_bw,  //  my output IF bandiwdth
                                 pAS_Info->f_zif_bw,  //  my Zero-IF bandwidth
                                 pAS_Info->maxH2,
                                 fp,                  //  minimum amount to move LO's positive
                                 fm))                 //  miminum amount to move LO's negative
            return 1;
        //  Look for LO-related spurs from my tuner generated into the adjacent tuner's IF output
        if (IsSpurInAdjTunerBand(0,             //  check his IF output
                                 pAS_Info->f_LO1,     //  my fLO1
                                 adj->f_LO1,          //  the other tuner's fLO1
                                 adj->f_LO2,          //  the other tuner's fLO2
                                 adj->f_out,          //  the other tuner's fOut
                                 adj->f_out_bw,       //  the other tuner's output IF bandiwdth
                                 pAS_Info->f_zif_bw,  //  the other tuner's Zero-IF bandwidth
                                 adj->maxH2,
                                 fp,                  //  minimum amount to move LO's positive
                                 fm))                 //  miminum amount to move LO's negative
            return 1;
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

