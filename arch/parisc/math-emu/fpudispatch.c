/*
 * Linux/PA-RISC Project (http://www.parisc-linux.org/)
 *
 * Floating-point emulation code
 *  Copyright (C) 2001 Hewlett-Packard (Paul Bame) <bame@debian.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * BEGIN_DESC
 *
 *  File:
 *	@(#)	pa/fp/fpudispatch.c		$Revision: 1.1 $
 *
 *  Purpose:
 *	<<please update with a synopsis of the functionality provided by this file>>
 *
 *  External Interfaces:
 *	<<the following list was autogenerated, please review>>
 *	emfpudispatch(ir, dummy1, dummy2, fpregs)
 *	fpudispatch(ir, excp_code, holder, fpregs)
 *
 *  Internal Interfaces:
 *	<<the following list was autogenerated, please review>>
 *	static u_int decode_06(u_int, u_int *)
 *	static u_int decode_0c(u_int, u_int, u_int, u_int *)
 *	static u_int decode_0e(u_int, u_int, u_int, u_int *)
 *	static u_int decode_26(u_int, u_int *)
 *	static u_int decode_2e(u_int, u_int *)
 *	static void update_status_cbit(u_int *, u_int, u_int, u_int)
 *
 *  Theory:
 *	<<please update with a overview of the operation of this file>>
 *
 * END_DESC
*/

#define FPUDEBUG 0

#include "float.h"
#include "types.h"
/* #include <sys/debug.h> */
/* #include <machine/sys/mdep_private.h> */

#define COPR_INST 0x30000000

/*
 * definition of extru macro.  If pos and len are constants, the compiler
 * will generate an extru instruction when optimized
 */
#define extru(r,pos,len)	(((r) >> (31-(pos))) & (( 1 << (len)) - 1))
/* definitions of bit field locations in the instruction */
#define fpmajorpos 5
#define fpr1pos	10
#define fpr2pos 15
#define fptpos	31
#define fpsubpos 18
#define fpclass1subpos 16
#define fpclasspos 22
#define fpfmtpos 20
#define fpdfpos 18
#define fpnulpos 26
/*
 * the following are the extra bits for the 0E major op
 */
#define fpxr1pos 24
#define fpxr2pos 19
#define fpxtpos 25
#define fpxpos 23
#define fp0efmtpos 20
/*
 * the following are for the multi-ops
 */
#define fprm1pos 10
#define fprm2pos 15
#define fptmpos 31
#define fprapos 25
#define fptapos 20
#define fpmultifmt 26
/*
 * the following are for the fused FP instructions
 */
     /* fprm1pos 10 */
     /* fprm2pos 15 */
#define fpraupos 18
#define fpxrm2pos 19
     /* fpfmtpos 20 */
#define fpralpos 23
#define fpxrm1pos 24
     /* fpxtpos 25 */
#define fpfusedsubop 26
     /* fptpos	31 */

/*
 * offset to constant zero in the FP emulation registers
 */
#define fpzeroreg (32*sizeof(double)/sizeof(u_int))

/*
 * extract the major opcode from the instruction
 */
#define get_major(op) extru(op,fpmajorpos,6)
/*
 * extract the two bit class field from the FP instruction. The class is at bit
 * positions 21-22
 */
#define get_class(op) extru(op,fpclasspos,2)
/*
 * extract the 3 bit subop field.  For all but class 1 instructions, it is
 * located at bit positions 16-18
 */
#define get_subop(op) extru(op,fpsubpos,3)
/*
 * extract the 2 or 3 bit subop field from class 1 instructions.  It is located
 * at bit positions 15-16 (PA1.1) or 14-16 (PA2.0)
 */
#define get_subop1_PA1_1(op) extru(op,fpclass1subpos,2)	/* PA89 (1.1) fmt */
#define get_subop1_PA2_0(op) extru(op,fpclass1subpos,3)	/* PA 2.0 fmt */

/* definitions of unimplemented exceptions */
#define MAJOR_0C_EXCP	0x09
#define MAJOR_0E_EXCP	0x0b
#define MAJOR_06_EXCP	0x03
#define MAJOR_26_EXCP	0x23
#define MAJOR_2E_EXCP	0x2b
#define PA83_UNIMP_EXCP	0x01

/*
 * Special Defines for TIMEX specific code
 */

#define FPU_TYPE_FLAG_POS (EM_FPU_TYPE_OFFSET>>2)
#define TIMEX_ROLEX_FPU_MASK (TIMEX_EXTEN_FLAG|ROLEX_EXTEN_FLAG)

/*
 * Static function definitions
 */
#define _PROTOTYPES
#if defined(_PROTOTYPES) || defined(_lint)
static u_int decode_0c(u_int, u_int, u_int, u_int *);
static u_int decode_0e(u_int, u_int, u_int, u_int *);
static u_int decode_06(u_int, u_int *);
static u_int decode_26(u_int, u_int *);
static u_int decode_2e(u_int, u_int *);
static void update_status_cbit(u_int *, u_int, u_int, u_int);
#else /* !_PROTOTYPES&&!_lint */
static u_int decode_0c();
static u_int decode_0e();
static u_int decode_06();
static u_int decode_26();
static u_int decode_2e();
static void update_status_cbit();
#endif /* _PROTOTYPES&&!_lint */

#define VASSERT(x)

/*
 * this routine will decode the excepting floating point instruction and
 * call the approiate emulation routine.
 * It is called by decode_fpu with the following parameters:
 * fpudispatch(current_ir, unimplemented_code, 0, &Fpu_register)
 * where current_ir is the instruction to be emulated,
 * unimplemented_code is the exception_code that the hardware generated
 * and &Fpu_register is the address of emulated FP reg 0.
 */
u_int
fpudispatch(u_int ir, u_int excp_code, u_int holder, u_int fpregs[])
{
	u_int class, subop;
	u_int fpu_type_flags;

	/* All FP emulation code assumes that ints are 4-bytes in length */
	VASSERT(sizeof(int) == 4);

	fpu_type_flags=fpregs[FPU_TYPE_FLAG_POS];  /* get fpu type flags */

	class = get_class(ir);
	if (class == 1) {
		if  (fpu_type_flags & PA2_0_FPU_FLAG)
			subop = get_subop1_PA2_0(ir);
		else
			subop = get_subop1_PA1_1(ir);
	}
	else
		subop = get_subop(ir);

	if (FPUDEBUG) printk("class %d subop %d\n", class, subop);

	switch (excp_code) {
		case MAJOR_0C_EXCP:
		case PA83_UNIMP_EXCP:
			return(decode_0c(ir,class,subop,fpregs));
		case MAJOR_0E_EXCP:
			return(decode_0e(ir,class,subop,fpregs));
		case MAJOR_06_EXCP:
			return(decode_06(ir,fpregs));
		case MAJOR_26_EXCP:
			return(decode_26(ir,fpregs));
		case MAJOR_2E_EXCP:
			return(decode_2e(ir,fpregs));
		default:
			/* "crashme Night Gallery painting nr 2. (asm_crash.s).
			 * This was fixed for multi-user kernels, but
			 * workstation kernels had a panic here.  This allowed
			 * any arbitrary user to panic the kernel by executing
			 * setting the FP exception registers to strange values
			 * and generating an emulation trap.  The emulation and
			 * exception code must never be able to panic the
			 * kernel.
			 */
			return(UNIMPLEMENTEDEXCEPTION);
	}
}

/*
 * this routine is called by $emulation_trap to emulate a coprocessor
 * instruction if one doesn't exist
 */
u_int
emfpudispatch(u_int ir, u_int dummy1, u_int dummy2, u_int fpregs[])
{
	u_int class, subop, major;
	u_int fpu_type_flags;

	/* All FP emulation code assumes that ints are 4-bytes in length */
	VASSERT(sizeof(int) == 4);

	fpu_type_flags=fpregs[FPU_TYPE_FLAG_POS];  /* get fpu type flags */

	major = get_major(ir);
	class = get_class(ir);
	if (class == 1) {
		if  (fpu_type_flags & PA2_0_FPU_FLAG)
			subop = get_subop1_PA2_0(ir);
		else
			subop = get_subop1_PA1_1(ir);
	}
	else
		subop = get_subop(ir);
	switch (major) {
		case 0x0C:
			return(decode_0c(ir,class,subop,fpregs));
		case 0x0E:
			return(decode_0e(ir,class,subop,fpregs));
		case 0x06:
			return(decode_06(ir,fpregs));
		case 0x26:
			return(decode_26(ir,fpregs));
		case 0x2E:
			return(decode_2e(ir,fpregs));
		default:
			return(PA83_UNIMP_EXCP);
	}
}
	

static u_int
decode_0c(u_int ir, u_int class, u_int subop, u_int fpregs[])
{
	u_int r1,r2,t;		/* operand register offsets */ 
	u_int fmt;		/* also sf for class 1 conversions */
	u_int  df;		/* for class 1 conversions */
	u_int *status;
	u_int retval, local_status;
	u_int fpu_type_flags;

	if (ir == COPR_INST) {
		fpregs[0] = EMULATION_VERSION << 11;
		return(NOEXCEPTION);
	}
	status = &fpregs[0];	/* fp status register */
	local_status = fpregs[0]; /* and local copy */
	r1 = extru(ir,fpr1pos,5) * sizeof(double)/sizeof(u_int);
	if (r1 == 0)		/* map fr0 source to constant zero */
		r1 = fpzeroreg;
	t = extru(ir,fptpos,5) * sizeof(double)/sizeof(u_int);
	if (t == 0 && class != 2)	/* don't allow fr0 as a dest */
		return(MAJOR_0C_EXCP);
	fmt = extru(ir,fpfmtpos,2);	/* get fmt completer */

	switch (class) {
	    case 0:
		switch (subop) {
			case 0:	/* COPR 0,0 emulated above*/
			case 1:
				return(MAJOR_0C_EXCP);
			case 2:	/* FCPY */
				switch (fmt) {
				    case 2: /* illegal */
					return(MAJOR_0C_EXCP);
				    case 3: /* quad */
					t &= ~3;  /* force to even reg #s */
					r1 &= ~3;
					fpregs[t+3] = fpregs[r1+3];
					fpregs[t+2] = fpregs[r1+2];
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					fpregs[t] = fpregs[r1];
					return(NOEXCEPTION);
				}
			case 3: /* FABS */
				switch (fmt) {
				    case 2: /* illegal */
					return(MAJOR_0C_EXCP);
				    case 3: /* quad */
					t &= ~3;  /* force to even reg #s */
					r1 &= ~3;
					fpregs[t+3] = fpregs[r1+3];
					fpregs[t+2] = fpregs[r1+2];
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					/* copy and clear sign bit */
					fpregs[t] = fpregs[r1] & 0x7fffffff;
					return(NOEXCEPTION);
				}
			case 6: /* FNEG */
				switch (fmt) {
				    case 2: /* illegal */
					return(MAJOR_0C_EXCP);
				    case 3: /* quad */
					t &= ~3;  /* force to even reg #s */
					r1 &= ~3;
					fpregs[t+3] = fpregs[r1+3];
					fpregs[t+2] = fpregs[r1+2];
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					/* copy and invert sign bit */
					fpregs[t] = fpregs[r1] ^ 0x80000000;
					return(NOEXCEPTION);
				}
			case 7: /* FNEGABS */
				switch (fmt) {
				    case 2: /* illegal */
					return(MAJOR_0C_EXCP);
				    case 3: /* quad */
					t &= ~3;  /* force to even reg #s */
					r1 &= ~3;
					fpregs[t+3] = fpregs[r1+3];
					fpregs[t+2] = fpregs[r1+2];
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					/* copy and set sign bit */
					fpregs[t] = fpregs[r1] | 0x80000000;
					return(NOEXCEPTION);
				}
			case 4: /* FSQRT */
				switch (fmt) {
				    case 0:
					return(sgl_fsqrt(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1:
					return(dbl_fsqrt(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2:
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 5: /* FRND */
				switch (fmt) {
				    case 0:
					return(sgl_frnd(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1:
					return(dbl_frnd(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2:
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
		} /* end of switch (subop) */

	case 1: /* class 1 */
		df = extru(ir,fpdfpos,2); /* get dest format */
		if ((df & 2) || (fmt & 2)) {
			/*
			 * fmt's 2 and 3 are illegal of not implemented
			 * quad conversions
			 */
			return(MAJOR_0C_EXCP);
		}
		/*
		 * encode source and dest formats into 2 bits.
		 * high bit is source, low bit is dest.
		 * bit = 1 --> double precision
		 */
		fmt = (fmt << 1) | df;
		switch (subop) {
			case 0: /* FCNVFF */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(MAJOR_0C_EXCP);
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvff(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvff(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(MAJOR_0C_EXCP);
				}
			case 1: /* FCNVXF */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvxf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvxf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvxf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvxf(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 2: /* FCNVFX */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfx(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfx(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfx(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfx(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 3: /* FCNVFXT */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfxt(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfxt(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfxt(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfxt(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 5: /* FCNVUF (PA2.0 only) */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvuf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvuf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvuf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvuf(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 6: /* FCNVFU (PA2.0 only) */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfu(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfu(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfu(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfu(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 7: /* FCNVFUT (PA2.0 only) */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfut(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfut(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfut(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfut(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 4: /* undefined */
				return(MAJOR_0C_EXCP);
		} /* end of switch subop */

	case 2: /* class 2 */
		fpu_type_flags=fpregs[FPU_TYPE_FLAG_POS];
		r2 = extru(ir, fpr2pos, 5) * sizeof(double)/sizeof(u_int);
		if (r2 == 0)
			r2 = fpzeroreg;
		if  (fpu_type_flags & PA2_0_FPU_FLAG) {
			/* FTEST if nullify bit set, otherwise FCMP */
			if (extru(ir, fpnulpos, 1)) {  /* FTEST */
				switch (fmt) {
				    case 0:
					/*
					 * arg0 is not used
					 * second param is the t field used for
					 * ftest,acc and ftest,rej
					 * third param is the subop (y-field)
					 */
					BUG();
					/* Unsupported
					 * return(ftest(0L,extru(ir,fptpos,5),
					 *	 &fpregs[0],subop));
					 */
				    case 1:
				    case 2:
				    case 3:
					return(MAJOR_0C_EXCP);
				}
			} else {  /* FCMP */
				switch (fmt) {
				    case 0:
					retval = sgl_fcmp(&fpregs[r1],
						&fpregs[r2],extru(ir,fptpos,5),
						&local_status);
					update_status_cbit(status,local_status,
						fpu_type_flags, subop);
					return(retval);
				    case 1:
					retval = dbl_fcmp(&fpregs[r1],
						&fpregs[r2],extru(ir,fptpos,5),
						&local_status);
					update_status_cbit(status,local_status,
						fpu_type_flags, subop);
					return(retval);
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			}
		}  /* end of if for PA2.0 */
		else {	/* PA1.0 & PA1.1 */
		    switch (subop) {
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				return(MAJOR_0C_EXCP);
			case 0: /* FCMP */
				switch (fmt) {
				    case 0:
					retval = sgl_fcmp(&fpregs[r1],
						&fpregs[r2],extru(ir,fptpos,5),
						&local_status);
					update_status_cbit(status,local_status,
						fpu_type_flags, subop);
					return(retval);
				    case 1:
					retval = dbl_fcmp(&fpregs[r1],
						&fpregs[r2],extru(ir,fptpos,5),
						&local_status);
					update_status_cbit(status,local_status,
						fpu_type_flags, subop);
					return(retval);
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 1: /* FTEST */
				switch (fmt) {
				    case 0:
					/*
					 * arg0 is not used
					 * second param is the t field used for
					 * ftest,acc and ftest,rej
					 * third param is the subop (y-field)
					 */
					BUG();
					/* unsupported
					 * return(ftest(0L,extru(ir,fptpos,5),
					 *     &fpregs[0],subop));
					 */
				    case 1:
				    case 2:
				    case 3:
					return(MAJOR_0C_EXCP);
				}
		    } /* end of switch subop */
		} /* end of else for PA1.0 & PA1.1 */
	case 3: /* class 3 */
		r2 = extru(ir,fpr2pos,5) * sizeof(double)/sizeof(u_int);
		if (r2 == 0)
			r2 = fpzeroreg;
		switch (subop) {
			case 5:
			case 6:
			case 7:
				return(MAJOR_0C_EXCP);
			
			case 0: /* FADD */
				switch (fmt) {
				    case 0:
					return(sgl_fadd(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fadd(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 1: /* FSUB */
				switch (fmt) {
				    case 0:
					return(sgl_fsub(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fsub(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 2: /* FMPY */
				switch (fmt) {
				    case 0:
					return(sgl_fmpy(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fmpy(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 3: /* FDIV */
				switch (fmt) {
				    case 0:
					return(sgl_fdiv(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fdiv(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 4: /* FREM */
				switch (fmt) {
				    case 0:
					return(sgl_frem(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_frem(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
		} /* end of class 3 switch */
	} /* end of switch(class) */

	/* If we get here, something is really wrong! */
	return(MAJOR_0C_EXCP);
}

static u_int
decode_0e(ir,class,subop,fpregs)
u_int ir,class,subop;
u_int fpregs[];
{
	u_int r1,r2,t;		/* operand register offsets */
	u_int fmt;		/* also sf for class 1 conversions */
	u_int df;		/* dest format for class 1 conversions */
	u_int *status;
	u_int retval, local_status;
	u_int fpu_type_flags;

	status = &fpregs[0];
	local_status = fpregs[0];
	r1 = ((extru(ir,fpr1pos,5)<<1)|(extru(ir,fpxr1pos,1)));
	if (r1 == 0)
		r1 = fpzeroreg;
	t = ((extru(ir,fptpos,5)<<1)|(extru(ir,fpxtpos,1)));
	if (t == 0 && class != 2)
		return(MAJOR_0E_EXCP);
	if (class < 2)		/* class 0 or 1 has 2 bit fmt */
		fmt = extru(ir,fpfmtpos,2);
	else 			/* class 2 and 3 have 1 bit fmt */
		fmt = extru(ir,fp0efmtpos,1);
	/*
	 * An undefined combination, double precision accessing the
	 * right half of a FPR, can get us into trouble.  
	 * Let's just force proper alignment on it.
	 */
	if (fmt == DBL) {
		r1 &= ~1;
		if (class != 1)
			t &= ~1;
	}

	switch (class) {
	    case 0:
		switch (subop) {
			case 0: /* unimplemented */
			case 1:
				return(MAJOR_0E_EXCP);
			case 2: /* FCPY */
				switch (fmt) {
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					fpregs[t] = fpregs[r1];
					return(NOEXCEPTION);
				}
			case 3: /* FABS */
				switch (fmt) {
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					fpregs[t] = fpregs[r1] & 0x7fffffff;
					return(NOEXCEPTION);
				}
			case 6: /* FNEG */
				switch (fmt) {
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					fpregs[t] = fpregs[r1] ^ 0x80000000;
					return(NOEXCEPTION);
				}
			case 7: /* FNEGABS */
				switch (fmt) {
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					fpregs[t] = fpregs[r1] | 0x80000000;
					return(NOEXCEPTION);
				}
			case 4: /* FSQRT */
				switch (fmt) {
				    case 0:
					return(sgl_fsqrt(&fpregs[r1],0,
						&fpregs[t], status));
				    case 1:
					return(dbl_fsqrt(&fpregs[r1],0,
						&fpregs[t], status));
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				}
			case 5: /* FRMD */
				switch (fmt) {
				    case 0:
					return(sgl_frnd(&fpregs[r1],0,
						&fpregs[t], status));
				    case 1:
					return(dbl_frnd(&fpregs[r1],0,
						&fpregs[t], status));
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				}
		} /* end of switch (subop */
	
	case 1: /* class 1 */
		df = extru(ir,fpdfpos,2); /* get dest format */
		/*
		 * Fix Crashme problem (writing to 31R in double precision)
		 * here too.
		 */
		if (df == DBL) {
			t &= ~1;
		}
		if ((df & 2) || (fmt & 2))
			return(MAJOR_0E_EXCP);
		
		fmt = (fmt << 1) | df;
		switch (subop) {
			case 0: /* FCNVFF */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(MAJOR_0E_EXCP);
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvff(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvff(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(MAJOR_0E_EXCP);
				}
			case 1: /* FCNVXF */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvxf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvxf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvxf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvxf(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 2: /* FCNVFX */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfx(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfx(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfx(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfx(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 3: /* FCNVFXT */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfxt(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfxt(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfxt(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfxt(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 5: /* FCNVUF (PA2.0 only) */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvuf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvuf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvuf(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvuf(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 6: /* FCNVFU (PA2.0 only) */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfu(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfu(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfu(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfu(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 7: /* FCNVFUT (PA2.0 only) */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfut(&fpregs[r1],0,
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfut(&fpregs[r1],0,
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfut(&fpregs[r1],0,
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfut(&fpregs[r1],0,
						&fpregs[t],status));
				}
			case 4: /* undefined */
				return(MAJOR_0C_EXCP);
		} /* end of switch subop */
	case 2: /* class 2 */
		/*
		 * Be careful out there.
		 * Crashme can generate cases where FR31R is specified
		 * as the source or target of a double precision operation.
		 * Since we just pass the address of the floating-point
		 * register to the emulation routines, this can cause
		 * corruption of fpzeroreg.
		 */
		if (fmt == DBL)
			r2 = (extru(ir,fpr2pos,5)<<1);
		else
			r2 = ((extru(ir,fpr2pos,5)<<1)|(extru(ir,fpxr2pos,1)));
		fpu_type_flags=fpregs[FPU_TYPE_FLAG_POS];
		if (r2 == 0)
			r2 = fpzeroreg;
		if  (fpu_type_flags & PA2_0_FPU_FLAG) {
			/* FTEST if nullify bit set, otherwise FCMP */
			if (extru(ir, fpnulpos, 1)) {  /* FTEST */
				/* not legal */
				return(MAJOR_0E_EXCP);
			} else {  /* FCMP */
			switch (fmt) {
				    /*
				     * fmt is only 1 bit long
				     */
				    case 0:
					retval = sgl_fcmp(&fpregs[r1],
						&fpregs[r2],extru(ir,fptpos,5),
						&local_status);
					update_status_cbit(status,local_status,
						fpu_type_flags, subop);
					return(retval);
				    case 1:
					retval = dbl_fcmp(&fpregs[r1],
						&fpregs[r2],extru(ir,fptpos,5),
						&local_status);
					update_status_cbit(status,local_status,
						fpu_type_flags, subop);
					return(retval);
				}
			}
		}  /* end of if for PA2.0 */
		else {  /* PA1.0 & PA1.1 */
		    switch (subop) {
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				return(MAJOR_0E_EXCP);
			case 0: /* FCMP */
				switch (fmt) {
				    /*
				     * fmt is only 1 bit long
				     */
				    case 0:
					retval = sgl_fcmp(&fpregs[r1],
						&fpregs[r2],extru(ir,fptpos,5),
						&local_status);
					update_status_cbit(status,local_status,
						fpu_type_flags, subop);
					return(retval);
				    case 1:
					retval = dbl_fcmp(&fpregs[r1],
						&fpregs[r2],extru(ir,fptpos,5),
						&local_status);
					update_status_cbit(status,local_status,
						fpu_type_flags, subop);
					return(retval);
				}
		    } /* end of switch subop */
		} /* end of else for PA1.0 & PA1.1 */
	case 3: /* class 3 */
		/*
		 * Be careful out there.
		 * Crashme can generate cases where FR31R is specified
		 * as the source or target of a double precision operation.
		 * Since we just pass the address of the floating-point
		 * register to the emulation routines, this can cause
		 * corruption of fpzeroreg.
		 */
		if (fmt == DBL)
			r2 = (extru(ir,fpr2pos,5)<<1);
		else
			r2 = ((extru(ir,fpr2pos,5)<<1)|(extru(ir,fpxr2pos,1)));
		if (r2 == 0)
			r2 = fpzeroreg;
		switch (subop) {
			case 5:
			case 6:
			case 7:
				return(MAJOR_0E_EXCP);
			
			/*
			 * Note that fmt is only 1 bit for class 3 */
			case 0: /* FADD */
				switch (fmt) {
				    case 0:
					return(sgl_fadd(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fadd(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				}
			case 1: /* FSUB */
				switch (fmt) {
				    case 0:
					return(sgl_fsub(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fsub(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				}
			case 2: /* FMPY or XMPYU */
				/*
				 * check for integer multiply (x bit set)
				 */
				if (extru(ir,fpxpos,1)) {
				    /*
				     * emulate XMPYU
				     */
				    switch (fmt) {
					case 0:
					    /*
					     * bad instruction if t specifies
					     * the right half of a register
					     */
					    if (t & 1)
						return(MAJOR_0E_EXCP);
					    BUG();
					    /* unsupported
					     * impyu(&fpregs[r1],&fpregs[r2],
						 * &fpregs[t]);
					     */
					    return(NOEXCEPTION);
					case 1:
						return(MAJOR_0E_EXCP);
				    }
				}
				else { /* FMPY */
				    switch (fmt) {
				        case 0:
					    return(sgl_fmpy(&fpregs[r1],
					       &fpregs[r2],&fpregs[t],status));
				        case 1:
					    return(dbl_fmpy(&fpregs[r1],
					       &fpregs[r2],&fpregs[t],status));
				    }
				}
			case 3: /* FDIV */
				switch (fmt) {
				    case 0:
					return(sgl_fdiv(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fdiv(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				}
			case 4: /* FREM */
				switch (fmt) {
				    case 0:
					return(sgl_frem(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_frem(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				}
		} /* end of class 3 switch */
	} /* end of switch(class) */

	/* If we get here, something is really wrong! */
	return(MAJOR_0E_EXCP);
}


/*
 * routine to decode the 06 (FMPYADD and FMPYCFXT) instruction
 */
static u_int
decode_06(ir,fpregs)
u_int ir;
u_int fpregs[];
{
	u_int rm1, rm2, tm, ra, ta; /* operands */
	u_int fmt;
	u_int error = 0;
	u_int status;
	u_int fpu_type_flags;
	union {
		double dbl;
		float flt;
		struct { u_int i1; u_int i2; } ints;
	} mtmp, atmp;


	status = fpregs[0];		/* use a local copy of status reg */
	fpu_type_flags=fpregs[FPU_TYPE_FLAG_POS];  /* get fpu type flags */
	fmt = extru(ir, fpmultifmt, 1);	/* get sgl/dbl flag */
	if (fmt == 0) { /* DBL */
		rm1 = extru(ir, fprm1pos, 5) * sizeof(double)/sizeof(u_int);
		if (rm1 == 0)
			rm1 = fpzeroreg;
		rm2 = extru(ir, fprm2pos, 5) * sizeof(double)/sizeof(u_int);
		if (rm2 == 0)
			rm2 = fpzeroreg;
		tm = extru(ir, fptmpos, 5) * sizeof(double)/sizeof(u_int);
		if (tm == 0)
			return(MAJOR_06_EXCP);
		ra = extru(ir, fprapos, 5) * sizeof(double)/sizeof(u_int);
		ta = extru(ir, fptapos, 5) * sizeof(double)/sizeof(u_int);
		if (ta == 0)
			return(MAJOR_06_EXCP);

		if  (fpu_type_flags & TIMEX_ROLEX_FPU_MASK)  {

			if (ra == 0) {
			 	/* special case FMPYCFXT, see sgl case below */
				if (dbl_fmpy(&fpregs[rm1],&fpregs[rm2],
					&mtmp.ints.i1,&status))
					error = 1;
				if (dbl_to_sgl_fcnvfxt(&fpregs[ta],
					&atmp.ints.i1,&atmp.ints.i1,&status))
					error = 1;
				}
			else {

			if (dbl_fmpy(&fpregs[rm1],&fpregs[rm2],&mtmp.ints.i1,
					&status))
				error = 1;
			if (dbl_fadd(&fpregs[ta], &fpregs[ra], &atmp.ints.i1,
					&status))
				error = 1;
				}
			}

		else

			{
			if (ra == 0)
				ra = fpzeroreg;

			if (dbl_fmpy(&fpregs[rm1],&fpregs[rm2],&mtmp.ints.i1,
					&status))
				error = 1;
			if (dbl_fadd(&fpregs[ta], &fpregs[ra], &atmp.ints.i1,
					&status))
				error = 1;

			}

		if (error)
			return(MAJOR_06_EXCP);
		else {
			/* copy results */
			fpregs[tm] = mtmp.ints.i1;
			fpregs[tm+1] = mtmp.ints.i2;
			fpregs[ta] = atmp.ints.i1;
			fpregs[ta+1] = atmp.ints.i2;
			fpregs[0] = status;
			return(NOEXCEPTION);
		}
	}
	else { /* SGL */
		/*
		 * calculate offsets for single precision numbers
		 * See table 6-14 in PA-89 architecture for mapping
		 */
		rm1 = (extru(ir,fprm1pos,4) | 0x10 ) << 1;	/* get offset */
		rm1 |= extru(ir,fprm1pos-4,1);	/* add right word offset */

		rm2 = (extru(ir,fprm2pos,4) | 0x10 ) << 1;	/* get offset */
		rm2 |= extru(ir,fprm2pos-4,1);	/* add right word offset */

		tm = (extru(ir,fptmpos,4) | 0x10 ) << 1;	/* get offset */
		tm |= extru(ir,fptmpos-4,1);	/* add right word offset */

		ra = (extru(ir,fprapos,4) | 0x10 ) << 1;	/* get offset */
		ra |= extru(ir,fprapos-4,1);	/* add right word offset */

		ta = (extru(ir,fptapos,4) | 0x10 ) << 1;	/* get offset */
		ta |= extru(ir,fptapos-4,1);	/* add right word offset */
		
		if (ra == 0x20 &&(fpu_type_flags & TIMEX_ROLEX_FPU_MASK)) {
			/* special case FMPYCFXT (really 0)
			  * This instruction is only present on the Timex and
			  * Rolex fpu's in so if it is the special case and
			  * one of these fpu's we run the FMPYCFXT instruction
			  */
			if (sgl_fmpy(&fpregs[rm1],&fpregs[rm2],&mtmp.ints.i1,
					&status))
				error = 1;
			if (sgl_to_sgl_fcnvfxt(&fpregs[ta],&atmp.ints.i1,
				&atmp.ints.i1,&status))
				error = 1;
		}
		else {
			if (sgl_fmpy(&fpregs[rm1],&fpregs[rm2],&mtmp.ints.i1,
					&status))
				error = 1;
			if (sgl_fadd(&fpregs[ta], &fpregs[ra], &atmp.ints.i1,
					&status))
				error = 1;
		}
		if (error)
			return(MAJOR_06_EXCP);
		else {
			/* copy results */
			fpregs[tm] = mtmp.ints.i1;
			fpregs[ta] = atmp.ints.i1;
			fpregs[0] = status;
			return(NOEXCEPTION);
		}
	}
}

/*
 * routine to decode the 26 (FMPYSUB) instruction
 */
static u_int
decode_26(ir,fpregs)
u_int ir;
u_int fpregs[];
{
	u_int rm1, rm2, tm, ra, ta; /* operands */
	u_int fmt;
	u_int error = 0;
	u_int status;
	union {
		double dbl;
		float flt;
		struct { u_int i1; u_int i2; } ints;
	} mtmp, atmp;


	status = fpregs[0];
	fmt = extru(ir, fpmultifmt, 1);	/* get sgl/dbl flag */
	if (fmt == 0) { /* DBL */
		rm1 = extru(ir, fprm1pos, 5) * sizeof(double)/sizeof(u_int);
		if (rm1 == 0)
			rm1 = fpzeroreg;
		rm2 = extru(ir, fprm2pos, 5) * sizeof(double)/sizeof(u_int);
		if (rm2 == 0)
			rm2 = fpzeroreg;
		tm = extru(ir, fptmpos, 5) * sizeof(double)/sizeof(u_int);
		if (tm == 0)
			return(MAJOR_26_EXCP);
		ra = extru(ir, fprapos, 5) * sizeof(double)/sizeof(u_int);
		if (ra == 0)
			return(MAJOR_26_EXCP);
		ta = extru(ir, fptapos, 5) * sizeof(double)/sizeof(u_int);
		if (ta == 0)
			return(MAJOR_26_EXCP);
		
		if (dbl_fmpy(&fpregs[rm1],&fpregs[rm2],&mtmp.ints.i1,&status))
			error = 1;
		if (dbl_fsub(&fpregs[ta], &fpregs[ra], &atmp.ints.i1,&status))
			error = 1;
		if (error)
			return(MAJOR_26_EXCP);
		else {
			/* copy results */
			fpregs[tm] = mtmp.ints.i1;
			fpregs[tm+1] = mtmp.ints.i2;
			fpregs[ta] = atmp.ints.i1;
			fpregs[ta+1] = atmp.ints.i2;
			fpregs[0] = status;
			return(NOEXCEPTION);
		}
	}
	else { /* SGL */
		/*
		 * calculate offsets for single precision numbers
		 * See table 6-14 in PA-89 architecture for mapping
		 */
		rm1 = (extru(ir,fprm1pos,4) | 0x10 ) << 1;	/* get offset */
		rm1 |= extru(ir,fprm1pos-4,1);	/* add right word offset */

		rm2 = (extru(ir,fprm2pos,4) | 0x10 ) << 1;	/* get offset */
		rm2 |= extru(ir,fprm2pos-4,1);	/* add right word offset */

		tm = (extru(ir,fptmpos,4) | 0x10 ) << 1;	/* get offset */
		tm |= extru(ir,fptmpos-4,1);	/* add right word offset */

		ra = (extru(ir,fprapos,4) | 0x10 ) << 1;	/* get offset */
		ra |= extru(ir,fprapos-4,1);	/* add right word offset */

		ta = (extru(ir,fptapos,4) | 0x10 ) << 1;	/* get offset */
		ta |= extru(ir,fptapos-4,1);	/* add right word offset */
		
		if (sgl_fmpy(&fpregs[rm1],&fpregs[rm2],&mtmp.ints.i1,&status))
			error = 1;
		if (sgl_fsub(&fpregs[ta], &fpregs[ra], &atmp.ints.i1,&status))
			error = 1;
		if (error)
			return(MAJOR_26_EXCP);
		else {
			/* copy results */
			fpregs[tm] = mtmp.ints.i1;
			fpregs[ta] = atmp.ints.i1;
			fpregs[0] = status;
			return(NOEXCEPTION);
		}
	}

}

/*
 * routine to decode the 2E (FMPYFADD,FMPYNFADD) instructions
 */
static u_int
decode_2e(ir,fpregs)
u_int ir;
u_int fpregs[];
{
	u_int rm1, rm2, ra, t; /* operands */
	u_int fmt;

	fmt = extru(ir,fpfmtpos,1);	/* get fmt completer */
	if (fmt == DBL) { /* DBL */
		rm1 = extru(ir,fprm1pos,5) * sizeof(double)/sizeof(u_int);
		if (rm1 == 0)
			rm1 = fpzeroreg;
		rm2 = extru(ir,fprm2pos,5) * sizeof(double)/sizeof(u_int);
		if (rm2 == 0)
			rm2 = fpzeroreg;
		ra = ((extru(ir,fpraupos,3)<<2)|(extru(ir,fpralpos,3)>>1)) *
		     sizeof(double)/sizeof(u_int);
		if (ra == 0)
			ra = fpzeroreg;
		t = extru(ir,fptpos,5) * sizeof(double)/sizeof(u_int);
		if (t == 0)
			return(MAJOR_2E_EXCP);

		if (extru(ir,fpfusedsubop,1)) { /* fmpyfadd or fmpynfadd? */
			return(dbl_fmpynfadd(&fpregs[rm1], &fpregs[rm2],
					&fpregs[ra], &fpregs[0], &fpregs[t]));
		} else {
			return(dbl_fmpyfadd(&fpregs[rm1], &fpregs[rm2],
					&fpregs[ra], &fpregs[0], &fpregs[t]));
		}
	} /* end DBL */
	else { /* SGL */
		rm1 = (extru(ir,fprm1pos,5)<<1)|(extru(ir,fpxrm1pos,1));
		if (rm1 == 0)
			rm1 = fpzeroreg;
		rm2 = (extru(ir,fprm2pos,5)<<1)|(extru(ir,fpxrm2pos,1));
		if (rm2 == 0)
			rm2 = fpzeroreg;
		ra = (extru(ir,fpraupos,3)<<3)|extru(ir,fpralpos,3);
		if (ra == 0)
			ra = fpzeroreg;
		t = ((extru(ir,fptpos,5)<<1)|(extru(ir,fpxtpos,1)));
		if (t == 0)
			return(MAJOR_2E_EXCP);

		if (extru(ir,fpfusedsubop,1)) { /* fmpyfadd or fmpynfadd? */
			return(sgl_fmpynfadd(&fpregs[rm1], &fpregs[rm2],
					&fpregs[ra], &fpregs[0], &fpregs[t]));
		} else {
			return(sgl_fmpyfadd(&fpregs[rm1], &fpregs[rm2],
					&fpregs[ra], &fpregs[0], &fpregs[t]));
		}
	} /* end SGL */
}

/*
 * update_status_cbit
 *
 *	This routine returns the correct FP status register value in
 *	*status, based on the C-bit & V-bit returned by the FCMP
 *	emulation routine in new_status.  The architecture type
 *	(PA83, PA89 or PA2.0) is available in fpu_type.  The y_field
 *	and the architecture type are used to determine what flavor
 *	of FCMP is being emulated.
 */
static void
update_status_cbit(status, new_status, fpu_type, y_field)
u_int *status, new_status;
u_int fpu_type;
u_int y_field;
{
	/*
	 * For PA89 FPU's which implement the Compare Queue and
	 * for PA2.0 FPU's, update the Compare Queue if the y-field = 0,
	 * otherwise update the specified bit in the Compare Array.
	 * Note that the y-field will always be 0 for non-PA2.0 FPU's.
	 */
	if ((fpu_type & TIMEX_EXTEN_FLAG) || 
	    (fpu_type & ROLEX_EXTEN_FLAG) ||
	    (fpu_type & PA2_0_FPU_FLAG)) {
		if (y_field == 0) {
			*status = ((*status & 0x04000000) >> 5) | /* old Cbit */
				  ((*status & 0x003ff000) >> 1) | /* old CQ   */
				  (new_status & 0xffc007ff); /* all other bits*/
		} else {
			*status = (*status & 0x04000000) |     /* old Cbit */
				  ((new_status & 0x04000000) >> (y_field+4)) |
				  (new_status & ~0x04000000 &  /* other bits */
				   ~(0x04000000 >> (y_field+4)));
		}
	}
	/* if PA83, just update the C-bit */
	else {
		*status = new_status;
	}
}
