#  @(#)Makelocal_mips.mk	4.1  ULTRIX  7/2/90

all:	libmV.a

install: 
	$(INSTALL) -c libmV.a $(DESTLIB)
	ranlib $(DESTLIB)/libmV.a

