# @(#)Makelocal.mk	4.1	(ULTRIX)	7/3/90

include $(GMAKEVARS)

SUBDIRS=adb as c2 cpp dbx ld libF77 libI77 libU77 libm lint mip \
	pcc prof

include $(GMAKERULES)
