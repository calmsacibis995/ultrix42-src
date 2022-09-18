#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

all: doctor

doctor: 
	/usr/ucb/liszt -mrq -o doctor ../doctor.l

install:
	${INSTALL} -c -s doctor $(DESTROOT)/usr/games

include $(GMAKERULES)
