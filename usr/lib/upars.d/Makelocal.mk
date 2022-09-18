#	@(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90
include $(GMAKEVARS)

CINCLUDES= -I. -I..
OBJECTS_LIB = 	parser.o tlex.o nlex.o lex_utils.o	
OBJECTS_UP = 	y.tab.o keywords.o tables.o

all: libup.a upars

tools2:	all
tools2 install:

libup.a:	$(OBJECTS_LIB)
		ar rv libup.a $(OBJECTS_LIB) 
		ranlib libup.a

upars:	$(OBJECTS_UP)
		$(LDCMD) $(OBJECTS_UP) -ll 

lex_utils.o:	upars.h keydefs.h keywords.h lex_utils.c
nlex.o:		upars.h keydefs.h nlex.c
tlex.o:		upars.h keydefs.h tlex.c
parser.o:	upars.h utables.h parser.c
keywords.o :	keywords.c keywords.h upars.h keydefs.h
tables.o :	tables.c utables.h upars.h

y.tab.o:	y.tab.c lex.yy.c upars.h
		$(CCCMD) y.tab.c

y.tab.c :	upars.y 
		yacc -v ../upars.y

lex.yy.c :	upars.x
		lex ../upars.x


include $(GMAKERULES)

OLDdepend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e '/\.\.\/h/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

# DO NOT DELETE THIS LINE -- make depend uses it
