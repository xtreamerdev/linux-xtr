/* IEEE754 floating point arithmetic
 * double precision: common utilities
 */
/*
 * MIPS floating point support
 * Copyright (C) 1994-2000 Algorithmics Ltd.  All rights reserved.
 * http://www.algor.co.uk
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 */


#include <limits.h>
#ifdef __KERNEL__
#define assert(expr) ((void)0)
#else
#include <assert.h>
#endif
#include <stdarg.h>
#include "ieee754dp.h"

int ieee754dp_class(ieee754dp x)
{
	COMPXDP;
	EXPLODEXDP;
	return xc;
}

int ieee754dp_isnan(ieee754dp x)
{
	return ieee754dp_class(x) >= IEEE754_CLASS_SNAN;
}

int ieee754dp_issnan(ieee754dp x)
{
	assert(ieee754dp_isnan(x));
	if (ieee754_csr.noq)
		return 1;
	return !(DPMANT(x) & DP_MBIT(DP_MBITS - 1));
}


ieee754dp ieee754dp_xcpt(ieee754dp r, const char *op, ...)
{
	struct ieee754xctx ax;
	if (!TSTX())
		return r;

	ax.op = op;
	ax.rt = IEEE754_RT_DP;
	ax.rv.dp = r;
	va_start(ax.ap, op);
	ieee754_xcpt(&ax);
	return ax.rv.dp;
}

ieee754dp ieee754dp_nanxcpt(ieee754dp r, const char *op, ...)
{
	struct ieee754xctx ax;

	assert(ieee754dp_isnan(r));

	if (!ieee754dp_issnan(r))	/* QNAN does not cause invalid op !! */
		return r;

	if (!SETCX(IEEE754_INVALID_OPERATION)) {
		/* not enabled convert to a quiet NaN */
		if (ieee754_csr.noq)
			return r;
		DPMANT(r) |= DP_MBIT(DP_MBITS - 1);
		return r;
	}

	ax.op = op;
	ax.rt = 0;
	ax.rv.dp = r;
	va_start(ax.ap, op);
	ieee754_xcpt(&ax);
	return ax.rv.dp;
}

ieee754dp ieee754dp_bestnan(ieee754dp x, ieee754dp y)
{
	assert(ieee754dp_isnan(x));
	assert(ieee754dp_isnan(y));

	if (DPMANT(x) > DPMANT(y))
		return x;
	else
		return y;
}


/* generate a normal/denormal number with over,under handeling
 * sn is sign
 * xe is an unbiased exponent
 * xm is 3bit extended precision value.
 */
ieee754dp ieee754dp_format(int sn, int xe, unsigned long long xm)
{
	assert(xm);		/* we dont gen exact zeros (probably should) */

	assert((xm >> (DP_MBITS + 1 + 3)) == 0);	/* no execess */
	assert(xm & (DP_HIDDEN_BIT << 3));

	if (xe < DP_EMIN) {
		/* strip lower bits */
		int es = DP_EMIN - xe;

		if (ieee754_csr.nod) {
			SETCX(IEEE754_UNDERFLOW);
			return ieee754dp_zero(sn);
		}

		/* sticky right shift es bits 
		 */
		xm = XDPSRS(xm, es);
		xe += es;

		assert((xm & (DP_HIDDEN_BIT << 3)) == 0);
		assert(xe == DP_EMIN);
	}
	if (xm & (DP_MBIT(3) - 1)) {
		SETCX(IEEE754_INEXACT);
		/* inexact must round of 3 bits 
		 */
		switch (ieee754_csr.rm) {
		case IEEE754_RZ:
			break;
		case IEEE754_RN:
			xm += 0x3 + ((xm >> 3) & 1);
			/* xm += (xm&0x8)?0x4:0x3 */
			break;
		case IEEE754_RU:	/* toward +Infinity */
			if (!sn)	/* ?? */
				xm += 0x8;
			break;
		case IEEE754_RD:	/* toward -Infinity */
			if (sn)	/* ?? */
				xm += 0x8;
			break;
		}
		/* adjust exponent for rounding add overflowing 
		 */
		if (xm >> (DP_MBITS + 3 + 1)) {	/* add causes mantissa overflow */
			xm >>= 1;
			xe++;
		}
	}
	/* strip grs bits */
	xm >>= 3;

	assert((xm >> (DP_MBITS + 1)) == 0);	/* no execess */
	assert(xe >= DP_EMIN);

	if (xe > DP_EMAX) {
		SETCX(IEEE754_OVERFLOW);
		/* -O can be table indexed by (rm,sn) */
		switch (ieee754_csr.rm) {
		case IEEE754_RN:
			return ieee754dp_inf(sn);
		case IEEE754_RZ:
			return ieee754dp_max(sn);
		case IEEE754_RU:	/* toward +Infinity */
			if (sn == 0)
				return ieee754dp_inf(0);
			else
				return ieee754dp_max(1);
		case IEEE754_RD:	/* toward -Infinity */
			if (sn == 0)
				return ieee754dp_max(0);
			else
				return ieee754dp_inf(1);
		}
	}
	/* gen norm/denorm/zero */

	if ((xm & DP_HIDDEN_BIT) == 0) {
		/* we underflow (tiny/zero) */
		assert(xe == DP_EMIN);
		SETCX(IEEE754_UNDERFLOW);
		return builddp(sn, DP_EMIN - 1 + DP_EBIAS, xm);
	} else {
		assert((xm >> (DP_MBITS + 1)) == 0);	/* no execess */
		assert(xm & DP_HIDDEN_BIT);

		return builddp(sn, xe + DP_EBIAS, xm & ~DP_HIDDEN_BIT);
	}
}
