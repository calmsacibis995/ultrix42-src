#  @(#)Makelocal.mk	4.2      Print development 10/16/90


include $(GMAKEVARS)

DESTLIST=\
	${DESTROOT}/usr \
	${DESTROOT}/usr/bin

install:
	$(INSTALL) -c ../lp.sh $(DESTROOT)/usr/bin/lp
	$(INSTALL) -c ../lpstat.sh $(DESTROOT)/usr/bin/lpstat

include $(GMAKERULES)
