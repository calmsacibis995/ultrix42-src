##############################################################################
#    Instructions to Make, for compilation of MH modules
#
#	This file is automatically generated.  Do not edit!
# @(#)$Id: config,v 1.2 90/11/25 18:56:37 sharpe Exp $
##############################################################################

OPTIONS	=	-DISI -DMHRC -DWHATNOW -DMHE -DBSD42 -DBERK -DNETWORK -DBIND -DRPATHS -DDPOP -DRPOP -DFOLDPROT='"0700"' -DMSGPROT='"0600"' -DTYPESIG='void' -DPOPSERVICE='"pop"' -DSPRINTF='int' -DSENDMTS -DSMTP -DPOP -DX400 -O
LDOPTIONS=	-O

CC	=	cc
CFLAGS	=	$(OPTIONS)
LDFLAGS	=	$(LDOPTIONS)
LIBES	=
LINT	=	lint
LFLAGS	=	-bhu $(OPTIONS)
LLIBS	=


##############################################################################
#	Generate object file for MH configuration
##############################################################################

CFILES	=	config.c version.c
OFILES	=	config.o version.o

all:		$(OFILES)

lint:;		$(LINT) $(LFLAGS) $(CFILES) $(LLIBS)

version version.c:	true
		@/bin/sh version.sh 6.7

true:;

##############################################################################
#	Miscellaneous tasks
##############################################################################

unconfig:	distribution
		-rm -f Makefile

distribution:   clean
		-rm -f config.c version.c version

clean:		unclean
		-rm -f *.o

unclean:;	-rm -f _* :* core eddep makedep


##############################################################################
#	Dependencies
##############################################################################

MODULES	=	$(CFILES)

depend:;	for m in $(MODULES); do ( \
		    i=`basename $$m .c`; \
		    echo $$i.o: $$i.c >> makedep; \
		    grep '^#[ 	]*include' $$i.c | \
			sed -e 's,[^"]*"/\([^"]*\)".*,'$$i'.o: /\1,' \
			    -e 's,[^"]*"\([^"]*\)".*,'$$i'.o: \1,' \
			    -e 's,[^<]*<\(.*\)>.*,#'$$i'.o: /usr/include/\1,' \
			>> makedep \
		); done
		echo '/^# DO NOT DELETE THIS LINE/+2,$$d' > eddep
		echo '$$r makedep' >> eddep
		echo 'w' >> eddep
		cp Makefile _Makefile
		ed - Makefile < eddep
		rm eddep makedep
		echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
		echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile

# DO NOT DELETE THIS LINE
# DEPENDENCIES START HERE
config.o: config.c
config.o: ../h/mh.h
#config.o: /usr/include/pwd.h
#config.o: /usr/include/stdio.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
