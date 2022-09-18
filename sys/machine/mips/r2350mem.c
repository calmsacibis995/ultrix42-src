/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/gnode.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/msgbuf.h"
#ifdef QUOTA
#include "../h/quota.h"
#endif QUOTA
#include "../h/exec.h"
#include "../h/syslog.h"

#include "../machine/cpu.h"
#include "../machine/r2350mem.h"
#include "../mipsvme/vmereg.h"

char *memipls[] = {
	"NONE",
	"0x1",
	"0x3",
	"0x7"
};

/*
 * total number of local memory boards in system
 */
int lmem_units;

lmem_probe(unit)
{
	register volatile struct memdevice *k1md;
	extern autoconf_cvec, autoconf_ipl;
	extern u_int autoconf_csr;
	extern (*autoconf_intr)();
	extern lmem_intr();

	k1md = (struct memdevice *)vme_to_k1(VME_A16SAMOD,
	    LMEM_ADR_BASE + (unit * LMEM_IO_SIZE));
	if (badaddr(k1md, sizeof(short)))
		return(0);
	autoconf_cvec = k1md->mem_iv & 0xff;
	autoconf_csr = (unsigned)k1md;
	autoconf_intr = lmem_intr;
	return(LMEM_IO_SIZE);
}

lmem_attach(unit)
{
	register volatile struct memdevice *k1md;
	unsigned md;
	unsigned vec;
	u_char dramtype;
	unsigned membase;
	unsigned memsize;

	md = LMEM_ADR_BASE + (unit * LMEM_IO_SIZE);
	k1md = (struct memdevice *)vme_to_k1(VME_A16SAMOD, md);
	vec = k1md->mem_iv & 0xff;
	printf("MIPS R2350 Local Memory, csr 0x%x, am 0x%x, vec 0x%x, ipl %s\n",
	    (unsigned)md, VME_A16SAMOD, vec,
	    memipls[(k1md->mem_cntrl&INT_LEVMASK)>>INT_LEVELSHIFT]);
	/*
	 * Show base address, size, and indicate if interleaved
	 */
	dramtype = k1md->mem_promid[ID_DRAMTYPE] & 0xff;
	membase = (k1md->mem_addr & MEMBASE_MASK) << 22;
	memsize = dramtype == 'M' ? 0x1000000 :
	    (dramtype == 'm' ? 0x800000 : 0x400000);
	printf("\tBase Address = 0x%x, %d MB",
	    membase, memsize>>20);
	if (k1md->mem_addr & INTLV) {
		if (k1md->mem_addr & INTLV_BRD1)
			printf(", Interleaved: board 1");
		else
			printf(", Interleaved: board 0");
	}
	printf("\n");
	/*
	 * clear errors and insure inhibits off
	 */
	k1md->mem_cntrl &= ~(INHIB_ECC|INHIB_CHKDATA|INHIB_WRTDATA
	    |EN_PRIVERR|EN_SYNERR|ENINT_SGLERR|ENINT_TERR|ENINT_DBLERR);
	k1md->mem_cntrl |= (PRIV_ENAB|SYS_ENAB|EN_PRIVERR|EN_SYNERR|
	    ENINT_SGLERR|ENINT_TERR|ENINT_DBLERR);
	if (unit > lmem_units)
		lmem_units = unit;
}

/*
 * syndrome to failing dram map for R2350 local memory
 */
struct syn_tbl {
	unsigned syn;
	char *pos;
};

struct syn_tbl evsyn_tbl[] = {
	/* low bank */
	{ 0x0a,	"H22C (D31)" }, { 0x0f,	"H22B (D30)" }, { 0x12,	"H22A (D29)" },
	{ 0x14,	"H21C (D28)" }, { 0x17,	"H21B (D27)" }, { 0x18,	"H21A (D26)" },
	{ 0x1b,	"H20C (D25)" }, { 0x1d,	"H20B (D24)" }, { 0x22,	"J21B (D7)" },
	{ 0x24,	"J21A (D6)" }, { 0x27,	"J20C (D5)" }, { 0x28,	"J20B (D4)" },
	{ 0x2b,	"J20A (D3)" }, { 0x2d,	"J19C (D2)" }, { 0x30,	"J19A (D0)" },
	{ 0x35,	"J19B (D1)" }, { 0x3f,	"H25A (C6)" }, { 0x4b,	"J24A (D15)" },
	{ 0x4e,	"J23C (D14)" }, { 0x53,	"J23B (D13)" }, { 0x55,	"J23A (D12)" },
	{ 0x56,	"J22C (D11)" }, { 0x59,	"J22B (D10)" }, { 0x5a,	"J22A (D9)" },
	{ 0x5c,	"J21C (D8)" }, { 0x5f,	"H24C (C5)" }, { 0x63,	"H20A (D23)" },
	{ 0x65,	"H19B (D22)" }, { 0x66,	"H19C (D21)" }, { 0x69,	"H19A (D20)" },
	{ 0x6a,	"J25B (D19)" }, { 0x6c,	"J25A (D18)" }, { 0x6f,	"H24B (C4)" },
	{ 0x71,	"J24B (D16)" }, { 0x74,	"J24C (D17)" }, { 0x77,	"H24A (C3)" },
	{ 0x7b,	"H23C (C2)" }, { 0x7d,	"H23B (C1)" }, { 0x7e,	"H23A (C0)" },
	/* high bank */
	{ 0x8a,	"F22C (D31)" }, { 0x8f,	"F22B (D30)" }, { 0x92,	"F22A (D29)" },
	{ 0x94,	"F21C (D28)" }, { 0x97,	"F21B (D27)" }, { 0x98,	"F21A (D26)" },
	{ 0x9b,	"F20C (D25)" }, { 0x9d,	"F20B (D24)" }, { 0xa2,	"G21B (D7)" },
	{ 0xa4,	"G21A (D6)" }, { 0xa7,	"G20C (D5)" }, { 0xa8,	"G20B (D4)" },
	{ 0xab,	"G20A (D3)" }, { 0xad,	"G19C (D2)" }, { 0xb0,	"G19A (D0)" },
	{ 0xb5,	"G19B (D1)" }, { 0xbf,	"F25A (C6)" }, { 0xcb,	"G24A (D15)" },
	{ 0xce,	"G23C (D14)" }, { 0xd3,	"G23B (D13)" }, { 0xd5,	"G23A (D12)" },
	{ 0xd6,	"G22C (D11)" }, { 0xd9,	"G22B (D10)" }, { 0xda,	"G22A (D9)" },
	{ 0xdc,	"G21C (D8)" }, { 0xdf,	"F24C (C5)" }, { 0xe3,	"F20A (D23)" },
	{ 0xe5,	"F19B (D22)" }, { 0xe6,	"F19C (D21)" }, { 0xe9,	"F19A (D20)" },
	{ 0xea,	"G25B (D19)" }, { 0xec,	"G25A (D18)" }, { 0xef,	"F24B (C4)" },
	{ 0xf1,	"G24B (D16)" }, { 0xf4,	"G24C (D17)" }, { 0xf7,	"F24A (C3)" },
	{ 0xfb,	"F23C (C2)" }, { 0xfd,	"F23B (C1)" }, { 0xfe,	"F23A (C0)" },
	{ 0, NULL }
};

struct syn_tbl odsyn_tbl[] = {
	/* low bank */
	{ 0x0a,	"H4A (D31)" }, { 0x0f,	"H4B (D30)" }, { 0x12,	"H4C (D29)" },
	{ 0x14,	"H5A (D28)" }, { 0x17,	"H5B (D27)" }, { 0x18,	"H6A (D26)" },
	{ 0x1b,	"H6B (D25)" }, { 0x1d,	"H6C (D24)" }, { 0x22,	"J5B (D7)" },
	{ 0x24,	"J6A (D6)" }, { 0x27,	"J6B (D5)" }, { 0x28,	"J6C (D4)" },
	{ 0x2b,	"J7A (D3)" }, { 0x2d,	"J7B (D2)" }, { 0x30,	"J8B (D0)" },
	{ 0x35,	"J8A (D1)" }, { 0x3f,	"H1C (C6)" }, { 0x4b,	"J2C (D15)" },
	{ 0x4e,	"J3A (D14)" }, { 0x53,	"J3B (D13)" }, { 0x55,	"J3C (D12)" },
	{ 0x56,	"J4A (D11)" }, { 0x59,	"J4B (D10)" }, { 0x5a,	"J4C (D9)" },
	{ 0x5c,	"J5A (D8)" }, { 0x5f,	"H2A (C5)" }, { 0x63,	"H7A (D23)" },
	{ 0x65,	"H7B (D22)" }, { 0x66,	"H8A (D21)" }, { 0x69,	"H8B (D20)" },
	{ 0x6a,	"J1B (D19)" }, { 0x6c,	"J1C (D18)" }, { 0x6f,	"H2B (C4)" },
	{ 0x71,	"J2B (D16)" }, { 0x74,	"J2A (D17)" }, { 0x77,	"H2C (C3)" },
	{ 0x7b,	"H2A (C2)" }, { 0x7d,	"H3B (C1)" }, { 0x7e,	"H3C (C0)" },
	/* high bank */
	{ 0x8a,	"F4A (D31)" }, { 0x8f,	"F4B (D30)" }, { 0x92,	"F4C (D29)" },
	{ 0x94,	"F5A (D28)" }, { 0x97,	"F5B (D27)" }, { 0x98,	"F6A (D26)" },
	{ 0x9b,	"F6B (D25)" }, { 0x9d,	"F6C (D24)" }, { 0xa2,	"G5B (D7)" },
	{ 0xa4,	"G6A (D6)" }, { 0xa7,	"G6B (D5)" }, { 0xa8,	"G6C (D4)" },
	{ 0xab,	"G7A (D3)" }, { 0xad,	"G7B (D2)" }, { 0xb0,	"G8B (D0)" },
	{ 0xb5,	"G8A (D1)" }, { 0xbf,	"F1C (C6)" }, { 0xcb,	"G2C (D15)" },
	{ 0xce,	"G3A (D14)" }, { 0xd3,	"G3B (D13)" }, { 0xd5,	"G3C (D12)" },
	{ 0xd6,	"G4A (D11)" }, { 0xd9,	"G4B (D10)" }, { 0xda,	"G4C (D9)" },
	{ 0xdc,	"G5A (D8)" }, { 0xdf,	"F2A (C5)" }, { 0xe3,	"F7A (D23)" },
	{ 0xe5,	"F7B (D22)" }, { 0xe6,	"F8A (D21)" }, { 0xe9,	"F8B (D20)" },
	{ 0xea,	"G1B (D19)" }, { 0xec,	"G1C (D18)" }, { 0xef,	"F2B (C4)" },
	{ 0xf1,	"G2B (D16)" }, { 0xf4,	"G2A (D17)" }, { 0xf7,	"F2C (C3)" },
	{ 0xfb,	"F2A (C2)" }, { 0xfd,	"F3B (C1)" }, { 0xfe,	"F3C (C0)" },
	{ 0, NULL }
};

struct reg_desc lmemstatus_desc[] = {
	/* mask	     	shift 	name   		format  values */
	{  SYS_DBLERR,	0,	"VME_DBLERR",	NULL,	NULL },
	{  NSYS_ALIGN,	0,	"VME_ALIGN",	NULL,	NULL },
	{  NPRIV_DBLERR,0,	"PRIV_DBLERR",	NULL,	NULL },
	{  PRIV_ALIGN,	0,	"PRIV_ALIGN",	NULL,	NULL },
	{  NEVEN_DBLERR,0,	"EVEN_DBLERR",	NULL,	NULL },
	{  NEVEN_ERR,	0,	"EVEN_SGLERR",	NULL,	NULL },
	{  NODD_DBLERR,	0,	"ODD_DBLERR",	NULL,	NULL },
	{  NODD_ERR,	0,	"ODD_SGLERR",	NULL,	NULL },
	{  0,		0,	NULL,		NULL,	NULL }
};

static int lmem_to_pending[32];
int eccdelay = 60;		/* 60 secs till between ecc err reports */

/*
 * lmem_scan -- scan all active local memory cards for errors
 * called on bus errors to correct and log error condition
 */
lmem_scan()
{
	unsigned unit;
	int s;
	extern int lmem_units;

	for (unit = 0; unit < lmem_units; unit++) {
		/*
		 * bus errors cause lmem_intr to be called both here
		 * and from an interrupt.  spl here so that one or
		 * the other handles the error entirely and the
		 * other sees no error condition.  this must be this
		 * way (at least for the vme side) so we see device
		 * initiated double bit ecc's which are only reported
		 * via interrupts.
		 */
		s = splbio();
		lmem_intr(unit);
		splx(s);
	}
}

/*
 * lmem_intr -- called on local memory interrupts
 */
lmem_intr(unit)
{
	register volatile struct memdevice *k1md;
	unsigned status;
	unsigned memaddr;
	unsigned control;
	unsigned odsyn, evsyn;
	unsigned ledbits;
	unsigned baseaddr;
	extern lmem_reenable();

	k1md = (struct memdevice *)
	    vme_to_k1(VME_A16SAMOD, LMEM_ADR_BASE + (LMEM_IO_SIZE * unit));
	if (badaddr(k1md, sizeof(short)))
		panic("Local memory interrupted but unaccessable");
	/*
	 * Some of the bits in status are active low, xor them to make
	 * them active high
	 */
	status = (k1md->mem_stat & STAT_MASK)
	 ^(NSYS_ALIGN|NPRIV_DBLERR|NEVEN_DBLERR|NEVEN_ERR|NODD_DBLERR|NODD_ERR);
	if (status == 0)	/* no errors pending */
		return;
	odsyn = k1md->mem_odsyn & 0xff;
	evsyn = k1md->mem_evsyn & 0xff;
	memaddr = k1md->mem_addr;
	baseaddr = (memaddr & MEMBASE_MASK) << 22;
	ledbits = (baseaddr >> 20) & 0x3c;
	log(LOG_NOTICE, "Private memory %d error, base 0x%x, status %R\n",
	    unit, baseaddr, status, lmemstatus_desc);
	if (memaddr & INTLV) {
		ledbits |= 0x80;
		if (memaddr & INTLV_BRD1) {
			log(LOG_NOTICE, "Interleaved odd double word\n");
			ledbits |= 0x40;
		} else
			log(LOG_NOTICE, "Interleaved even double word\n");
	}
	if (status & (NODD_DBLERR|NODD_ERR)) {
		log(LOG_NOTICE, "odd bank syndrome: 0x%x\n", odsyn);
		lookup_dram(odsyn_tbl, odsyn);
	}
	if (status & (NEVEN_DBLERR|NEVEN_ERR)) {
		log(LOG_NOTICE, "even bank syndrome: 0x%x\n", odsyn);
		lookup_dram(evsyn_tbl, evsyn);
	}
	/*
	 * if this was a single bit error, disable reports
	 * temporarily, so a hard single bit error doesn't swamp console
	 */
	if ((status & (NEVEN_ERR|NODD_ERR)) && !lmem_to_pending[unit]
	    && eccdelay) {
		k1md->mem_cntrl &= ~ENINT_SGLERR;
		lmem_to_pending[unit] = 1;
		timeout(lmem_reenable, unit, eccdelay*hz);
	}

	/*
	 * reprogram leds to show failing board
	 */
	if (status & (NEVEN_ERR|NODD_ERR))
		ledbits |= 1;	/* bit 0 => single bit error on this board */
	if (status & (NEVEN_DBLERR|NODD_DBLERR))
		ledbits |= 2;	/* bit 1 => double bit error on this board */
	k1md->mem_ledreg = ~ledbits;
	/*
	 * clear error
	 */
	control = k1md->mem_cntrl;
	k1md->mem_cntrl = control &~ (EN_PRIVERR|EN_SYNERR|ENINT_TERR
	    |ENINT_SGLERR|ENINT_DBLERR);
	k1md->mem_cntrl = control;
}

lmem_reenable(unit)
{
	register volatile struct memdevice *k1md;

	k1md = (struct memdevice *)
	    vme_to_k1(VME_A16SAMOD, LMEM_ADR_BASE + (LMEM_IO_SIZE * unit));
	if (badaddr(k1md, sizeof(short)))
		panic("Local memory reenable");
	k1md->mem_cntrl |= ENINT_SGLERR;
	lmem_to_pending[unit] = 0;
}

lookup_dram(tbl, syn)
struct syn_tbl *tbl;
unsigned syn;
{
	for(; tbl->pos; tbl++)
		if (tbl->syn == syn) {
			log(LOG_NOTICE, "Failing dram at position %s\n",
			    tbl->pos);
			return;
		}
	log(LOG_NOTICE, "Ambiguous syndrome\n");
}

volatile int eccloc[4];		/* handy place to cause an ecc error */
int causeecc;

/*
 * doecc -- fake an ecc error
 * trying this from C is pretty adventurous
 */
doecc()
{
	register volatile struct memdevice *k1md;
	register volatile int *eccp;
	int tmp;

	k1md = (struct memdevice *)
	    vme_to_k1(VME_A16SAMOD, LMEM_ADR_BASE);
	printf("control reg = 0x%x\n", k1md->mem_cntrl);
	eccp = (int *)((unsigned)&eccloc[3] & ~(sizeof(eccloc)-1));
	*eccp = 0xffffffff;
	switch (causeecc) {
	case 1:
		splhigh();
		wbflush();
		k1md->mem_cntrl &= ~(EN_PRIVERR|EN_SYNERR);
		wbflush();
		k1md->mem_cntrl |= (INHIB_CHKDATA|INHIB_ECC);
		wbflush();
		*eccp = 0xfffffffe;
		wbflush();
		k1md->mem_cntrl &= ~(INHIB_CHKDATA|INHIB_ECC);
		wbflush();
		k1md->mem_cntrl |= (EN_PRIVERR|EN_SYNERR);
		wbflush();
		spl0();
		if ((tmp = *eccp) != 0xffffffff)
			printf("ecc correction failed, data = 0x%x\n",
			    tmp);
		break;
	case 2:
		splhigh();
		wbflush();
		k1md->mem_cntrl &= ~(EN_PRIVERR|EN_SYNERR);
		wbflush();
		k1md->mem_cntrl |= (INHIB_CHKDATA|INHIB_ECC);
		wbflush();
		*eccp = 0xfffffffc;
		wbflush();
		k1md->mem_cntrl &= ~(INHIB_CHKDATA|INHIB_ECC);
		wbflush();
		k1md->mem_cntrl |= (EN_PRIVERR|EN_SYNERR);
		wbflush();
		spl0();
		tmp = *eccp;	/* should get an interrupt */
		DELAY(1000);
		printf("ecc detection failed, data = 0x%x\n", tmp);
		break;
	default:
		printf("doecc, huh?\n");
		break;
	}
}
