# @(#)Makelocal.mk	4.2      ULTRIX 	10/16/90
#
#
# AUTHOR:	Adrian Thoms
# DATE:		28th February 1989
# Modification History:
#
# 02-Oct-90 - Adrian Thoms (thoms@wessex)
#	Added layup examples files

include $(GMAKEVARS)

XLDIR=usr/lib/lpdfilters
EXDIR=usr/examples/print/layup

DESTLIST= \
	$(DESTROOT)/$(XLDIR) \
	$(DESTROOT)/usr/examples \
	$(DESTROOT)/usr/examples/print \
	$(DESTROOT)/$(EXDIR)

AOUT	=layup

HFILES =	lup_def.h  lup_errs.h
OBJS =		lup_ultrix_main.o lup_main.o lup_io.o lup_trans.o lup_error.o
CFILES =	lup_ultrix_main.c lup_main.c lup_io.c lup_trans.c lup_error.c

LAYUP_EXAMPLES =\
	lpsdoubleholes.lup \
	lpsholes.lup \
	lpsnup.lup \
	lpssingleholes.lup


lup_ultrix_main.o: lup_ultrix_main.c
lup_main.o: lup_main.c
lup_io.o: lup_io.c
lup_trans.o: lup_trans.c
lup_error.o: lup_error.c

install:
	install -c -s layup ${DESTROOT}/${XLDIR}/layup
	@for i in ${LAYUP_EXAMPLES}; do \
		echo "Installing ${DESTROOT}/${EXDIR}/$$i" ; \
		install -c -m 644 ../$$i ${DESTROOT}/${EXDIR}/$$i; \
	done

include $(GMAKERULES)
