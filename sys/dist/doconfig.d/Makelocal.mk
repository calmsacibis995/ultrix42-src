#	Makelocal.mk -
#		sys/dist/etc.d/doconfig.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c -m 754 ../doconfig $(DESTROOT)/etc/doconfig

include $(GMAKERULES)
