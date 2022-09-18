#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
# xti library
include $(GMAKEVARS)
CFILES=	xti.c xti_lib.c 
OBJECTS= xti.o xti_lib.o

all: libxti

libxti: ${OBJECTS}
	@echo building xti library libxti
	@ar cr libxti ${OBJECTS}

install:
	install -c -m 644 libxti ${DESTROOT}/usr/lib/libxti.a
	ranlib ${DESTROOT}/usr/lib/libxti.a

include $(GMAKERULES)
include ../Makelocal_$(MACHINE).mk

xti.o: xti.c
xti_lib.o: xti_lib.c


