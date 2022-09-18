#  sccsid  =  @(#)Makelocal.mk	4.5   (ULTRIX) 2/12/91 

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

LOADLIBES=-lerrlog

AOUT=	uerf

OBJS=	uerf.o getcmd.o summary.o errprn.o esget.o esopen.o select.o \
	btt.o msgwrt.o eritio.o eribld.o erisel.o erierr.o ulfile.o \
	eixfrm.o dsd_access.o \
	eribld_aq.o eribld_aqpcs.o qt_psreg.o qt_rsreg.o qt_hws_pf.o \
	$(MACHINE)_divq.o $(MACHINE)_subq.o

CINCLUDES= -I. -I.. -I$(SRCROOT)/usr/include -I$(SRCROOT)/usr/include/sys -I../../db \
	-I../../db/_$(MACHINE).b

eribld_aq.o: ../../db/_$(MACHINE).b/std_dsd.h ../../db/_$(MACHINE).b/os_dsd.h
eribld_aqpcs.o: ../../db/_$(MACHINE).b/std_dsd.h
eribld_aqpcs.o: ../../db/_$(MACHINE).b/os_dsd.h qtrans.h
eribld.o: ../../db/_$(MACHINE).b/std_dsd.h ../../db/_$(MACHINE).b/os_dsd.h
qt_rsreg.o:	qtrans.h
qt_psreg.o:	qtrans.h
qt_hws_pf.o:	qtrans.h

uerf.o:		uerf.c
getcmd.o:	getcmd.c
summary.o:	summary.c
errprn.o:	errprn.c
esget.o:	esget.c
esopen.o:	esopen.c
select.o:	select.c
btt.o:		btt.c
msgwrt.o:	msgwrt.c
eritio.o:	eritio.c
eribld.o:	eribld.c
erisel.o:	erisel.c
erierr.o:	erierr.c
ulfile.o:	ulfile.c
eixfrm.o:	eixfrm.c
dsd_access.o:	dsd_access.c
eribld_aq.o:	eribld_aq.c
eribld_aqpcs.o:	eribld_aqpcs.c
qt_rsreg.o:	qt_rsreg.c
qt_psreg.o:	qt_psreg.c
qt_hws_pf.o:	qt_hws_pf.c

vax_divq.o: vax_divq.s
vax_subq.o: vax_subq.s
mips_divq.o: mips_divq.c
mips_subq.o: mips_subq.c

install:
	$(INSTALL) -s -c uerf  $(DESTROOT)/usr/etc/uerf
	$(RM) $(DESTROOT)/etc/uerf
	$(LN) -s ../usr/etc/uerf $(DESTROOT)/etc/uerf
	$(INSTALL) -c ../uerf.err  $(DESTROOT)/usr/etc/uerf.err
	$(RM) $(DESTROOT)/etc/uerf.err
	$(LN) -s ../usr/etc/uerf.err $(DESTROOT)/etc/uerf.err
	$(INSTALL) -c ../uerf.hlp  $(DESTROOT)/usr/etc/uerf.hlp
	$(RM) $(DESTROOT)/etc/uerf.hlp
	$(LN) -s ../usr/etc/uerf.hlp $(DESTROOT)/etc/uerf.hlp
	$(INSTALL) -c -m 444 ../../db/_$(MACHINE).b/uerf.bin  $(DESTROOT)/usr/etc/uerf.bin
	$(RM) $(DESTROOT)/etc/uerf.bin
	$(LN) -s ../usr/etc/uerf.bin $(DESTROOT)/etc/uerf.bin

include $(GMAKERULES)

