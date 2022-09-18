#ifndef lint
static char *sccsid = "@(#)fpi_sigfpe.c	4.1	(ULTRIX)	7/3/90";
#endif not lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: fpi_sigfpe.c,v 1.3 87/08/20 14:50:59 dce Exp $ */

/*
 * THIS FILE MUST BE COMPILED WITH THE -float COMPILER OPTION!!
 */

#include <sys/param.h>

#if defined(BSD) || defined(ultrix)
#include <mips/cpu.h>
#include <mips/fpu.h>
#include <mips/inst.h>
#else
#include <sys/sbd.h>
#include <sys/fpu.h>
#include <sys/inst.h>
#endif
#include <fpi.h>
#include <fp_class.h>
#include <signal.h>
#include <stdio.h>

extern char *sys_errlist[];
extern int errno;

char *fpi_list[] = {
	"source signaling NaN",
	"source quiet NaN",
	"source denormalized value",
	"move of zero",
	"negate of zero",
	"implemented only in software",
	"invalid operation",
	"divide by zero",
	"destination overflow",
	"destination underflow"
};

/* The counters */
int fpi_counts[FPI_SIZE];

/*
 * fpi() is called to setup the signal hander and do the system call.
 * fpi_() is the fortran wrapper.
 */
void
fpi_()
{
	fpi();
}

void
fpi()
{
    void fpi_sigfpe();

#ifdef BSD
	signal(SIGFPE, fpi_sigfpe);
#else
	sigset(SIGFPE, fpi_sigfpe);
#endif
	if(fp_sigintr(1) == -1){
	    fprintf(stderr,
		   "fpi(): Error: system call fp_sigintr(1) failed (%s)\n",
		   sys_errlist[errno]);
	    exit(1);
	}
}

/*
 * printfpi_counts() is used to print the counts.  printfpi_counts_() is the
 * fortran wrapper.
 */
void
printfpi_counts_()
{
	printfpi_counts();
}

void
printfpi_counts()
{
    int i;

	for(i = 0 ; i < FPI_SIZE; i++)
		fprintf(stderr, "%s = %d\n", fpi_list[i], fpi_counts[i]);
}

static int counted;
static void source_class();

static unsigned long instr;
static void help();

/* routines to execute single instructions (all in fp_instr.s) */
extern float sqrtf_instr(float x);
extern double sqrtd_instr(double x);
extern float absf_instr(float x);
extern double absd_instr(double x);
extern void cmpf_instr(float src1, float src2, long index);
extern void cmpd_instr(double src1, double src2, long index);
extern long cvtw_f_instr(float x);
extern long cvtw_d_instr(double x);

union value {
	struct {
	    unsigned long v0,v1,v2,v3;
	} v;		/* four words for the value */
	long w;		/* word */
	float f;	/* single floating point */
	double d;	/* double floating point */
};

/*
 * fpi_sigfpe() is used to handle the SIGFPE caused by the fp_sigintr()
 * system call and count the reasons why floating-point interrupts occured.
 */
void
fpi_sigfpe(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
    union fpc_irr fpc_irr;
    union fpu_instr fpu_instr;
    unsigned long reg, fmt, func;
    union value src1, src2, dest;
    int src1_class, src2_class, dest_class;
    int src1_fmt, src2_fmt, dest_fmt;
    union fpc_csr fpc_csr;
    int error;

	counted = 0;
	error = 0;

	if(fp_sigintr(0) == -1){
	    fprintf(stderr,
		   "sigfpe(): Error: system call fp_sigintr(0) failed (%s)\n",
		   sys_errlist[errno]);
	    exit(1);
	}

	/*
	 * Get the floating point instruction.  If the floating point unit
	 * is the board implementation it's in the fpc_eir.  Else it is at
	 * the exception program counter as modified by the branch delay
	 * bit in the cause register.
	 */
	fpc_irr.fi_word = get_fpc_irr();
	if(fpc_irr.fi_struct.implementation == IMPLEMENTATION_R2360)
	    fpu_instr.instr = scp->sc_fpc_eir;
	else
	    if(scp->sc_cause & CAUSE_BD)
		fpu_instr.instr = *(unsigned long *)(scp->sc_pc + 4);
	    else
		fpu_instr.instr = *(unsigned long *)(scp->sc_pc);

	instr = fpu_instr.instr;

	/*
	 * To get here and have the instruction not be a floating-point
	 * instruction is an error.
	 */
	if(fpu_instr.rtype.op != cop1_op){
	    fprintf(stderr, "sigfpe(): Error: reached with a non-floating-point instruction\n");
	    exit(1);
	}

	/*
	 * Get the format of the source operand(s) from the (fmt) field of
	 * the floating-point instruction.
	 */
	fmt = fpu_instr.rtype.fmt;
	src1_fmt = fmt;
	src2_fmt = fmt;

	/*
	 * Get src1's (rs) value from the value saved in the signal context.
	 */
	reg = fpu_instr.rtype.rs;
	switch(fmt){
	case FMT_WORD:
	    src1.v.v0 = scp->sc_fpregs[reg];
	    src1_class = -1;
	    break;
	case FMT_SINGLE:
	    src1.v.v0 = scp->sc_fpregs[reg];
	    src1_class = fp_class_f(src1.f);
	    break;
	case FMT_DOUBLE:
#ifdef MIPSEL
	    src1.v.v0 = scp->sc_fpregs[reg];
	    src1.v.v1 = scp->sc_fpregs[reg+1];
#endif MIPSEL
#ifdef MIPSEB
	    src1.v.v0 = scp->sc_fpregs[reg+1];
	    src1.v.v1 = scp->sc_fpregs[reg];
#endif MIPSEB
	    src1_class = fp_class_d(src1.d);
	    break;
	default:
	    fprintf(stderr, "sigfpe(): Error: reached with a floating-point instruction with an unsupported format\n");
	    error = 1;
	}

	/*
	 * If there is a src2 (rt) get it's value from the value saved in the
	 * signal context.
	 */
	func = fpu_instr.rtype.func;
	reg = fpu_instr.rtype.rt;
	switch(fmt){
	case FMT_WORD:
	    src2.v.v0 = scp->sc_fpregs[reg];
	    src2_class = -1;
	    break;
	case FMT_SINGLE:
	    src2.v.v0 = scp->sc_fpregs[reg];
	    src2_class = fp_class_f(src2.f);
	    break;
	case FMT_DOUBLE:
#ifdef MIPSEL
	    src2.v.v0 = scp->sc_fpregs[reg];
	    src2.v.v1 = scp->sc_fpregs[reg+1];
#endif MIPSEL
#ifdef MIPSEB
	    src2.v.v0 = scp->sc_fpregs[reg+1];
	    src2.v.v1 = scp->sc_fpregs[reg];
#endif MIPSEB
	    src2_class = fp_class_d(src2.d);
	    break;
	}

	/*
	 * Clear out the sticky exception bits so they can be used to tell
	 * what happened in creating the destination.
	 */
	fpc_csr.fc_word = get_fpc_csr();
	fpc_csr.fc_struct.se_invalid = 0;
	fpc_csr.fc_struct.se_divide0 = 0;
	fpc_csr.fc_struct.se_underflow = 0;
	fpc_csr.fc_struct.se_overflow = 0;
	set_fpc_csr(fpc_csr.fc_word);

	dest_fmt = fmt;
	switch(func){
	case FUNC_CVTS:
	    src2_class = -1;
	    src2_fmt = -1;
	    dest_fmt = FMT_SINGLE;
	    switch(fmt){
	    case FMT_WORD:
		dest.f = src1.w;
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.f = src1.d;
		dest_class = fp_class_f(dest.f);
		break;
	    default:
		error = 1;
		break;
	    }
	    break;
	case FUNC_CVTD:
	    src2_class = -1;
	    src2_fmt = -1;
	    dest_fmt = FMT_DOUBLE;
	    switch(fmt){
	    case FMT_WORD:
		dest.d = src1.w;
		dest_class = fp_class_d(dest.d);
		break;
	    case FMT_SINGLE:
		dest.d = src1.f;
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    break;
	case FUNC_CVTW:
	    src2_class = -1;
	    src2_fmt = -1;
	    dest_fmt = FMT_WORD;
	    dest_class = -1;
	    switch(fmt){
	    case FMT_SINGLE:
		dest.w = cvtw_f_instr(src1.f);
		break;
	    case FMT_DOUBLE:
		dest.w = cvtw_d_instr(src1.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    break;
	case FUNC_ADD:
	    switch(fmt){
	    case FMT_SINGLE:
		dest.f = src1.f + src2.f;
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.d = src1.d + src2.d;
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    break;
	case FUNC_SUB:
	    switch(fmt){
	    case FMT_SINGLE:
		dest.f = src1.f - src2.f;
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.d = src1.d - src2.d;
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    break;
	case FUNC_MUL:
	    switch(fmt){
	    case FMT_SINGLE:
		dest.f = src1.f * src2.f;
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.d = src1.d * src2.d;
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    break;
	case FUNC_DIV:
	    switch(fmt){
	    case FMT_SINGLE:
		dest.f = src1.f / src2.f;
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.d = src1.d / src2.d;
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    break;
	case FUNC_ABS:
	    src2_class = -1;
	    switch(fmt){
	    case FMT_SINGLE:
		dest.f = absf_instr(src1.f);
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.d = absd_instr(src1.d);
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    break;
	case FUNC_MOV:
	    src2_class = -1;
	    switch(fmt){
	    case FMT_SINGLE:
		dest.f = src1.f;
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.d = src1.d;
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    if(src1_class == FP_POS_ZERO || src1_class == FP_NEG_ZERO){
		fpi_counts[FPI_MOVEZERO]++;
		counted = 1;
	    }
	    break;
	case FUNC_NEG:
	    src2_class = -1;
	    switch(fmt){
	    case FMT_SINGLE:
		dest.f = -src1.f;
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.d = -src1.d;
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    if(src1_class == FP_POS_ZERO || src1_class == FP_NEG_ZERO){
		fpi_counts[FPI_NEGZERO]++;
		counted = 1;
	    }
	    break;
	case FUNC_SQRT:
	    src2_class = -1;
	    switch(fmt){
	    case FMT_SINGLE:
		dest.f = sqrtf_instr(src1.f);
		dest_class = fp_class_f(dest.f);
		break;
	    case FMT_DOUBLE:
		dest.d = sqrtd_instr(src1.d);
		dest_class = fp_class_d(dest.d);
		break;
	    default:
		error = 1;
		break;
	    }
	    fpi_counts[FPI_UNIMP]++;
	    counted = 1;
	    break;
	default:
	    if(func & FUNC_FC){
		dest_fmt = -1;
		dest_class = -1;
		fpc_csr.fc_word = scp->sc_fpc_csr;
		fpc_csr.fc_struct.condition = 0;
		scp->sc_fpc_csr = fpc_csr.fc_word;
		switch(fmt){
		case FMT_SINGLE:
		    cmpf_instr(src1.f, src2.f, (func & ~FUNC_FC) << 2);
		    break;
		case FMT_DOUBLE:
		    cmpd_instr(src1.d, src2.d, (func & ~FUNC_FC) << 2);
		    break;
		default:
		    error = 1;
		    break;
		}
		break;
	    }else
		error = 1;
	}

	source_class(src1_class);
	source_class(src2_class);
	if(!counted){
	    fpc_csr.fc_word = get_fpc_csr();
	    if(fpc_csr.fc_struct.se_invalid){
		fpi_counts[FPI_INVALID]++;
		counted = 1;
	    }
	    else
	    if(fpc_csr.fc_struct.se_divide0){
		fpi_counts[FPI_DIVIDE0]++;
		counted = 1;
	    }
	    else
	    if(fpc_csr.fc_struct.se_underflow ||
	       dest_class == FP_POS_DENORM || dest_class == FP_NEG_DENORM){
		fpi_counts[FPI_UNDERFLOW]++;
		counted = 1;
	    }
	    else
	    if(fpc_csr.fc_struct.se_overflow){
		fpi_counts[FPI_OVERFLOW]++;
		counted = 1;
	    }
	}

#ifdef DEBUG
	if(!counted){
	    fprintf(stderr, "Unknown reason for interrupt\n");
	    error = 1;
	}
	if(error){
	    help(fpc_csr.fc_word, src1_fmt, src2_fmt, dest_fmt,
		 &src1, &src2, &dest);
	}
#endif DEBUG

	/*
	 * Now if the floating point unit is the board implementation
	 * and the floating-point destination and fpc_csr must be stored. 
	 * The next floating-point interrupt is set to cause a SIGFPE
	 * since this one was emulated here and is not going to be re-excuted.
	 */
	if(fpc_irr.fi_struct.implementation == IMPLEMENTATION_R2360){
	    if(fp_sigintr(1) == -1){
		fprintf(stderr,
		   "sigfpe(): Error: system call fp_sigintr(1) failed (%s)\n",
		   sys_errlist[errno]);
		exit(1);
	    }
	    reg = fpu_instr.rtype.rd;
	    switch(dest_fmt){
	    case FMT_SINGLE:
	    case FMT_WORD:
		scp->sc_fpregs[reg] = dest.v.v0;
		break;
	    case FMT_DOUBLE:
#ifdef MIPSEL
		scp->sc_fpregs[reg]   = dest.v.v0;
		scp->sc_fpregs[reg+1] = dest.v.v1;
#endif MIPSEL
#ifdef MIPSEB
		scp->sc_fpregs[reg+1] = dest.v.v0;
		scp->sc_fpregs[reg]   = dest.v.v1;
#endif MIPSEB
		break;
	    default:
		break;
	    }
	    scp->sc_fpc_csr |= get_fpc_csr();
	    sigreturn(scp);
	}
	/*
	 * In the case the floating point unit is not the board implementation
	 * and the floating-point destination is not stored.  By returning the
	 * floating-point instruction will again be executed and the next
	 * floating-point interrupt is set to not cause a SIGFPE.
	 */
	else{
	    if(fp_sigintr(2) == -1){
		fprintf(stderr,
		   "sigfpe(): Error: system call fp_sigintr(2) failed (%s)\n",
		   sys_errlist[errno]);
		exit(1);
	    }
	}
}

static
void
source_class(class)
int class;
{
	if(counted)
	    return;

	if(class == FP_SNAN){
	    fpi_counts[FPI_SRCSNAN]++;
	    counted = 1;
	    return;
	}
	if(class == FP_QNAN){
	    fpi_counts[FPI_SRCQNAN]++;
	    counted = 1;
	    return;
	}
	if(class == FP_POS_DENORM || class == FP_NEG_DENORM){
	    fpi_counts[FPI_SRCDENORM]++;
	    counted = 1;
	    return;
	}
}
 
#ifdef DEBUG
static void print_header();
static long get_bytes();

static
void
help(fpc_csr, src1_fmt, src2_fmt, dest_fmt, src1, src2, dest)
int fpc_csr, src1_fmt, src2_fmt, dest_fmt;
union value *src1, *src2, *dest;
{
	printf("\tfpc_csr = ");
	print_csr(fpc_csr);
	printf("\tinstr = 0x%08x\n",instr);
	disassembler(0, 1, 0, 0, get_bytes, print_header);

	switch(src1_fmt){
	    case FMT_SINGLE:
		printf("\tsrc1 (single) = 0x%08x %g\n", src1->v.v0, src1->f);
		break;
	    case FMT_DOUBLE:
		printf("\tsrc1 (double) = 0x%08x %08x %g\n", src1->v.v0,
			src1->v.v1, src1->d);
		break;
	    case FMT_EXTENDED:
		printf("\tsrc1 (extended) = 0x%08x %08x %08x %08x\n",
			src1->v.v0, src1->v.v1,
			src1->v.v2, src1->v.v3);
		break;
	    case FMT_QUAD:
		printf("\tsrc1 (quad) = 0x%08x %08x %08x %08x\n",
			src1->v.v0, src1->v.v1,
			src1->v.v2, src1->v.v3);
		break;
	    case FMT_WORD:
		printf("\tsrc1 (word) = 0x%08x %d\n", src1->v.v0, src1->w);
		break;
	}
	switch(src2_fmt){
	    case FMT_SINGLE:
		printf("\tsrc2 (single) = 0x%08x %g\n", src2->v.v0, src2->f);
		break;
	    case FMT_DOUBLE:
		printf("\tsrc2 (double) = 0x%08x %08x %g\n", src2->v.v0,
			src2->v.v1, src2->d);
		break;
	    case FMT_EXTENDED:
		printf("\tsrc2 (extended) = 0x%08x %08x %08x %08x\n",
			src2->v.v0, src2->v.v1,
			src2->v.v2, src2->v.v3);
		break;
	    case FMT_QUAD:
		printf("\tsrc2 (quad) = 0x%08x %08x %08x %08x\n",
			src2->v.v0, src2->v.v1,
			src2->v.v2, src2->v.v3);
		break;
	    case FMT_WORD:
		printf("\tsrc2 (word) = 0x%08x %g\n", src2->v.v0, src2->d);
		break;
	}
	switch(dest_fmt){
	    case FMT_SINGLE:
		printf("\tdest (single) = 0x%08x %g\n", dest->v.v0, dest->f);
		break;
	    case FMT_DOUBLE:
		printf("\tdest (double) = 0x%08x %08x %g\n", dest->v.v0,
			dest->v.v1, dest->d);
		break;
	    case FMT_EXTENDED:
		printf("\tdest (extended) = 0x%08x %08x %08x %08x\n",
			dest->v.v0, dest->v.v1,
			dest->v.v2, dest->v.v3);
		break;
	    case FMT_QUAD:
		printf("\tdest (quad) = 0x%08x %08x %08x %08x\n",
			dest->v.v0, dest->v.v1,
			dest->v.v2, dest->v.v3);
		break;
	    case FMT_WORD:
		printf("\tdest (word) = 0x%08x %d\n", dest->v.v0, dest->w);
		break;
	}
}

print_csr(csr)
long csr;
{
	printf("0x%08x ", csr);
	if(csr & 1<<23) printf("T "); else printf("F ");
	if(csr & 1<<17) printf("E"); else printf("-");
	if(csr & 1<<16) printf("V"); else printf("-");
	if(csr & 1<<15) printf("Z"); else printf("-");
	if(csr & 1<<14) printf("O"); else printf("-");
	if(csr & 1<<13) printf("U"); else printf("-");
	if(csr & 1<<12) printf("I "); else printf("- ");
	if(csr & 1<<11) printf("V"); else printf("-");
	if(csr & 1<<10) printf("Z"); else printf("-");
	if(csr & 1<<9) printf("O"); else printf("-");
	if(csr & 1<<8) printf("U"); else printf("-");
	if(csr & 1<<7) printf("I "); else printf("- ");
	if(csr & 1<<6) printf("V"); else printf("-");
	if(csr & 1<<5) printf("Z"); else printf("-");
	if(csr & 1<<4) printf("O"); else printf("-");
	if(csr & 1<<3) printf("U"); else printf("-");
	if(csr & 1<<2) printf("I "); else printf("- ");

	switch(csr & 0x3){
		case 0:	printf("RN\n"); break;
		case 1:	printf("RZ\n"); break;
		case 2:	printf("RPI\n"); break;
		case 3:	printf("RMI\n"); break;
	}
}
static
long
get_bytes()
{
    return(instr);
}

static
void
print_header()
{
	printf("\t");
}
#endif DEBUG
