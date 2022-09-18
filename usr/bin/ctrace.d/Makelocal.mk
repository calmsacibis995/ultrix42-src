# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)

OBJECTS=main.o lookup.o trace.o
OBJECTS_MISC=parser.o scanner.o

# directory for runtime.c source
LIB = /usr/lib/ctrace

# preprocessor symbols
CFLAGS = -O
#The CDEFINES has only one quote because the source code
#that uses it (in main.c, maybe more?) has the closing
# quote !!!!
CDEFINES = -DLIB=\"${LIB}
CINCLUDES=-I. -I..
YFLAGS = -d

all: ctrace
ctrace: $(OBJECTS) $(OBJECTS_MISC)
	$(LDCMD) $(OBJECTS) $(OBJECTS_MISC)

$(OBJECTS):
	$(CCCMD) ../$<

main.o: main.c
lookup.o: lookup.c
trace.o: trace.c
parser.o: parser.y
	$(YACC) $(YFLAGS) ../$<
	$(CCCMD) y.tab.c
	$(RM) y.tab.c
	$(MV) y.tab.o $@
scanner.o: scanner.l
	$(LEX) $(LFLAGS) ../$<
	$(CCCMD) lex.yy.c
	$(RM) lex.yy.c
	$(MV) lex.yy.o $@

install:
	install -c -m 755 -o bin -g bin -s ctrace ${DESTROOT}/usr/bin/ctrace 
	install -c -m 755 -o bin -g bin ../ctcr ${DESTROOT}/usr/bin/ctcr 
	$(RM) ${DESTROOT}/usr/bin/ctc
	ln ${DESTROOT}/usr/bin/ctcr ${DESTROOT}/usr/bin/ctc
	-(if [ ! -d $(DESTROOT)/${LIB} ] ; then \
	    mkdir $(DESTROOT)/${LIB} ;\
	else \
	    true ;\
	fi)
	install -c -m 644 -o bin -g bin ../runtime.c $(DESTROOT)/${LIB}/runtime.c 

include $(GMAKERULES)
