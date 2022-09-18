/*	@(#)doprnt.s	4.3	ULTRIX	9/10/90	*/
	# C library -- conversions

/************************************************************************
 *		Modification history
 *
 *	Jon Reeves, 04-Dec-89
 * 011	Cleaned up error handling
 *
 *	Jon Reeves, 11-Oct-89
 * 010	G-float %E format tried to adjust the sign instead of the e.
 *
 *	Jon Reeves, 14-Jun-89
 * 009	Add ANSI-mandated %p (treated as %08x), %n, L size flag
 *
 *	Jon Reeves, 10-May-89
 * 008	Rounding was inconsistent.  D-float has 16-1/2 significant
 *	digits, G-float has 15-1/2, but they were being rounded at
 *	17 and 15, respectively.  They are now rounded at 17 and 16.
 *
 *	Andy Gadsby, 27-Nov-86
 * INTL	Added code to allow change of radix character.
 *
 *	David L Ballenger, 7-Jun-1985
 * 006	Fix rounding problem when printing gfloat numbers.
 *	Add handling of %i, %I, and %X for System V competibility.
 *
 *	Stephen Reilly, 14-May-84
 * 005- Changed the symbol MVAXI to GFLOAT to reflect the meaning of
 *	of the code.
 *
 *	Stephen Reilly, 02-May-84
 * 004- Rounding would sometimes cause overflow causing bogus results
 *
 *	Stephen Reilly, 04-Apr-84
 * 003- Change the symbol MicroVAX to MVAXI
 *
 *	Stephen Reilly, 05-Dec-83
 * 002- Have to change a line in skpc becuase it screws-up an awk
 *	script that translate things to use the Microvax library
 *
 *	Stephen Reilly, 04-Oct-83:
 * 001 - Added code to handle gfloat numbers for MicroVAX
 *
 *
 ***********************************************************************/

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/



.globl	__doprnt
.globl	__flsbuf
.globl  __lc_radix		# INTL - international radix character

#define vbit 1
#define flags r10
#define ndfnd 0
#define prec 1
#define zfill 2
#define minsgn 3
#define plssgn 4
#define numsgn 5
#define caps 6
#define blank 7
#define gflag 8
#define dpflag 9

/*
  The precision for gfloat is only 15 digits whereas for double it is 17.
	slr001
  Don't let these defines fool you, there's hand tuning needed, too.
*/

# ifdef	GFLOAT				

# define fltprec 16

#else

# define fltprec 17

# endif
#define width r9
#define ndigit r8
#define llafx r7
#define lrafx r6
#define fdesc -4(fp)
#define exp -8(fp)
#define sexp -12(fp)
#define nchar -16(fp)
#define sign -17(fp)
	.set ch.zer,'0			# cpp doesn't like single appostrophes

	.align 2
strtab:		# translate table for detecting null and percent
	.byte	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	.byte	16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	.byte	' ,'!,'",'#,'$, 0,'&,'','(,'),'*,'+,',,'-,'.,'/
	.byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,':,';,'<,'=,'>,'?
	.byte	'@,'A,'B,'C,'D,'E,'F,'G,'H,'I,'J,'K,'L,'M,'N,'O
	.byte	'P,'Q,'R,'S,'T,'U,'V,'W,'X,'Y,'Z,'[,'\,'],'^,'_
	.byte	'`,'a,'b,'c,'d,'e,'f,'g,'h,'i,'j,'k,'l,'m,'n,'o
	.byte	'p,'q,'r,'s,'t,'u,'v,'w,'x,'y,'z,'{,'|,'},'~,127
	.byte	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143
	.byte	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159
	.byte	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175
	.byte	176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191
	.byte	192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207
	.byte	208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223
	.byte	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239
	.byte	240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255

	.align	1
__doprnt:
	.word	0xfc0			# uses r11-r6
	jbr	doit

strfoo:
	clrl r4					# fix interrupt race
	jbr strok				# and try again
strmore:
	movzbl (r1)+,r2			# one char
	tstb strtab[r2]			# translate
	jeql stresc2			# bad guy in disguise (outbuf is full)
strout2:		# enter here to force out r2; r0,r1 must be set
	pushr $3				# save input descriptor
	pushl fdesc				# FILE
	pushl r2				# the char
	calls $2,__flsbuf		# please empty the buffer and handle 1 char
	tstl r0					# successful?
	jlss strouterr				# no
	incl nchar				# count the char
	popr $3					# get input descriptor back
strout:			# enter via bsb with (r0,r1)=input descriptor
	movab strtab,r3			# table address
	movq *fdesc,r4			# output descriptor
	jbs $31,r4,strfoo		# negative count is a no no
strok:
	addl2 r0,nchar			# we intend to move this many chars
/******* Start bogus movtuc workaround  *****/
	clrl r2
	tstl    r0
	bleq    movdon
movlp:
	tstl    r4
	bleq    movdon
	movzbl  (r1)+,r3
	tstb    strtab[r3]
	bneq    1f
	mnegl   $1,r2
	decl    r1
	brb     movdon
1:
	movb    r3,(r5)+
	decl    r4
	sobgtr  r0,movlp
  /******* End bogus movtuc workaround ***
	movtuc r0,(r1),$0,(r3),r4,(r5)
	movpsl r2                       /*  squirrel away condition codes */
  /******* End equally bogus movtuc ****/
movdon: movq r4,*fdesc                  /*  update output descriptor */
	subl2 r0,nchar			# some chars not moved
	jbs $vbit,r2,stresc		# terminated by escape?
	sobgeq r0,strmore		# no; but out buffer might be full
stresc:
	rsb

strouterr:
	popr $3					# put stack back in order
	mnegl $1,nchar				# set error return value
	movab strtab+1,r1			# point to nul char to exit
stresc2:
	incl r0					# fix the length
	decl r1					# and the addr
	movl $1<vbit,r2			# fake condition codes
	rsb

errdone:
	jbcs $31,nchar,prdone	# set error bit
prdone:
	movl nchar,r0
	ret

doit:
	movab -256(sp),sp		# work space
	movl 4(ap),r11			# addr of format string
	movl 12(ap),fdesc		# output FILE ptr
	movl 8(ap),ap			# addr of first arg
	clrl nchar				# number of chars transferred
loop:
	jbs $31,nchar,prdone		# bail out if I/O error
	movzwl $65535,r0		# pseudo length
	movl r11,r1				# fmt addr
		# comet sucks.
	movq *fdesc,r4
	subl3 r1,r5,r2
	jlss lp1
	cmpl r0,r2
	jleq lp1
	movl r2,r0
lp1:
		#
	bsbw strout				# copy to output, stop at null or percent
	movl r1,r11				# new fmt
	jbc $vbit,r2,loop		# if no escape, then very long fmt
	tstb (r11)+				# escape; null or percent?
	jeql prdone				# null means end of fmt

	movl sp,r5			# reset output buffer pointer
	clrq r9				# width; flags
	clrq r6				# lrafx,llafx
longorunsg:				# we can ignore both of these distinctions
short:
L4a:
	movzbl (r11)+,r0		# so capital letters can tail merge
L4:	caseb r0,$' ,$'x-' 		# format char
L5:
	.word space-L5			# space
	.word fmtbad-L5			# !
	.word fmtbad-L5			# "
	.word sharp-L5			# #
	.word fmtbad-L5			# $
	.word fmtbad-L5			# %
	.word fmtbad-L5			# &
	.word fmtbad-L5			# '
	.word fmtbad-L5			# (
	.word fmtbad-L5			# )
	.word indir-L5			# *
	.word plus-L5			# +
	.word fmtbad-L5			# ,
	.word minus-L5			# -
	.word dot-L5			# .
	.word fmtbad-L5			# /
	.word gnum0-L5			# 0
	.word gnum-L5			# 1
	.word gnum-L5			# 2
	.word gnum-L5			# 3
	.word gnum-L5			# 4
	.word gnum-L5			# 5
	.word gnum-L5			# 6
	.word gnum-L5			# 7
	.word gnum-L5			# 8
	.word gnum-L5			# 9
	.word fmtbad-L5			# :
	.word fmtbad-L5			# ;
	.word fmtbad-L5			# <
	.word fmtbad-L5			# =
	.word fmtbad-L5			# >
	.word fmtbad-L5			# ?
	.word fmtbad-L5			# @
	.word fmtbad-L5			# A
	.word fmtbad-L5			# B
	.word fmtbad-L5			# C
	.word decimal-L5		# D
	.word capital-L5		# E
	.word fmtbad-L5			# F
	.word capital-L5		# G
	.word fmtbad-L5			# H
	.word decimal-L5		# I
	.word fmtbad-L5			# J
	.word fmtbad-L5			# K
	.word longorunsg-L5		# L
	.word fmtbad-L5			# M
	.word fmtbad-L5			# N
	.word octal-L5			# O
	.word fmtbad-L5			# P
	.word fmtbad-L5			# Q
	.word fmtbad-L5			# R
	.word fmtbad-L5			# S
	.word fmtbad-L5			# T
	.word unsigned-L5		# U
	.word fmtbad-L5			# V
	.word fmtbad-L5			# W
	.word capital-L5		# X
	.word fmtbad-L5			# Y
	.word fmtbad-L5			# Z
	.word fmtbad-L5			# [
	.word fmtbad-L5			# \
	.word fmtbad-L5			# ]
	.word fmtbad-L5			# ^
	.word fmtbad-L5			# _
	.word fmtbad-L5			# `
	.word fmtbad-L5			# a
	.word fmtbad-L5			# b
	.word charac-L5			# c
	.word decimal-L5		# d
	.word scien-L5			# e
	.word float-L5			# f
	.word general-L5		# g
	.word short-L5			# h
	.word decimal-L5		# i
	.word fmtbad-L5			# j
	.word fmtbad-L5			# k
	.word longorunsg-L5		# l
	.word fmtbad-L5			# m
	.word count-L5			# n
	.word octal-L5			# o
	.word pointer-L5		# p
	.word fmtbad-L5			# q
	.word fmtbad-L5			# r
	.word string-L5			# s
	.word fmtbad-L5			# t
	.word unsigned-L5		# u
	.word fmtbad-L5			# v
	.word fmtbad-L5			# w
	.word hex-L5			# x
fmtbad:
	movb r0,(r5)+			# print the unfound character
	jeql errdone			# dumb users who end the format with a %
	jbr prbuf
capital:
	bisl2 $1<caps,flags		# note that it was capitalized
	xorb2 $'a^'A,r0			# make it small
	jbr L4					# and try again

count:
	movl (ap)+,r2			# fetch arg
	movl nchar,(r2)			# store count so far
	jbr loop			# look for more work

string:
	movl ndigit,r0
	jbs $prec,flags,L20		# max length was specified
	mnegl $1,r0			# default max length
L20:	movl (ap)+,r2			# addr first byte
	locc $0,r0,(r2)			# find the zero at the end
	movl r1,r5			# addr last byte +1
	movl r2,r1			# addr first byte
	jbr prstr

htab:	.byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'a,'b,'c,'d,'e,'f
Htab:	.byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F

octal:
	movl $30,r2			# init position
	movl $3,r3			# field width
	movab htab,llafx	# translate table
	jbr L10

pointer:
	movl $1<zfill|1<prec,flags	# leading 0 fill, length given
	movl $8,ndigit			# 8 digits
/*	Fall through to normal hex handling	*/
hex:
	movl $28,r2			# init position
	movl $4,r3			# field width
	movab htab,llafx	# translate table
	jbc $caps,flags,L10
	movab Htab,llafx
L10:	mnegl r3,r6			# increment
	clrl r1
	addl2 $4,r5			# room for left affix (2) and slop [forced sign?]
	movl (ap)+,r0			# fetch arg
L11:	extzv r2,r3,r0,r1		# pull out a digit
	movb (llafx)[r1],(r5)+		# convert to character
L12:	acbl $0,r6,r2,L11		# continue until done
	clrq r6				# lrafx, llafx
	clrb (r5)			# flag end
	skpc $'0,$11,4(sp)		# skip over leading zeroes
	jbc $numsgn,flags,prn3	# easy if no left affix
	tstl -4(ap)				# original value
	jeql prn3			# no affix on 0, for some reason
	cmpl r3,$4			# were we doing hex or octal?
	jneq L12a			# octal
	movb $'x,r0
	jbc $caps,flags,L12b
	movb $'X,r0
L12b:	movb r0,-(r1)
	movl $2,llafx		# leading 0x for hex is an affix
L12a:	movb $'0,-(r1)	# leading zero for octal is a digit, not an affix
	jbr prn3			# omit sign (plus, blank) massaging

unsigned:
lunsigned:
	bicl2 $1<plssgn|1<blank,flags	# omit sign (plus, blank) massaging
	extzv $1,$31,(ap),r0		# right shift logical 1 bit
	cvtlp r0,$10,(sp)		# convert [n/2] to packed
	movp $10,(sp),8(sp)		# copy packed
	addp4 $10,8(sp),$10,(sp)	# 2*[n/2] in packed, at (sp)
	blbc (ap)+,L14			# n was even
	addp4 $1,pone,$10,(sp)		# n was odd
	jbr L14

patdec:					# editpc pattern for decimal printing
	.byte 0xAA			# eo$float 10
	.byte 0x01			# eo$end_float
	.byte 0				# eo$end

decimal:
	cvtlp (ap)+,$10,(sp)		# 10 digits max
	jgeq L14
	incl llafx			# minus sign is a left affix
L14:	editpc $10,(sp),patdec,8(sp)	# ascii at 8(sp); r5=end+1
 #	skpc $' ,$11,8(sp)		# skip leading blanks; r1=first
	skpc $0x20,$11,8(sp)		# slr002 

prnum:			# r1=addr first byte, r5=addr last byte +1, llafx=size of signs
				# -1(r1) vacant, for forced sign
	tstl llafx
	jneq prn3			# already some left affix, dont fuss
	jbc $plssgn,flags,prn2
	movb $'+,-(r1)		# needs a plus sign
	jbr prn4
prn2:	jbc $blank,flags,prn3
	movb $' ,-(r1)		# needs a blank sign
prn4:	incl llafx
prn3:	jbs $prec,flags,prn1
	movl $1,ndigit		# default precision is 1
prn1:	subl3 r1,r5,lrafx	# raw width
	subl2 llafx,lrafx	# number of digits
	subl2 lrafx,ndigit	# number of leading zeroes needed
	jleq prstr			# none
	addl2 llafx,r1		# where current digits start
	pushl r1			# movcx gobbles registers
		# check bounds on users who say %.300d
	movab 32(r5)[ndigit],r2
	subl2 fp,r2
	jlss prn5
	subl2 r2,ndigit
prn5:
		#
	movc3 lrafx,(r1),(r1)[ndigit]	# make room in middle
	movc5 $0,(r1),$ch.zer,ndigit,*(sp)	# '0 fill
	subl3 llafx,(sp)+,r1	# first byte addr
	addl3 lrafx,r3,r5	# last byte addr +1

prstr:			# r1=addr first byte; r5=addr last byte +1
				# width=minimum width; llafx=len. left affix
				# ndigit=<avail>
	subl3 r1,r5,ndigit		# raw width
	subl3 ndigit,width,r0	# pad length
	jleq padlno				# in particular, no left padding
	jbs $minsgn,flags,padlno
			# extension for %0 flag causing left zero padding to field width
	jbs $zfill,flags,padlz
			# this bsbb needed even if %0 flag extension is removed
	bsbb padb				# blank pad on left
	jbr padnlz
padlz:
	movl llafx,r0
	jleq padnlx				# left zero pad requires left affix first
	subl2 r0,ndigit			# part of total length will be transferred
	subl2 r0,width			# and will account for part of minimum width
	bsbw strout				# left affix
padnlx:
	subl3 ndigit,width,r0	# pad length
	bsbb padz				# zero pad on left
padnlz:
	jbs $31,nchar,prdone		# bail out if I/O error
			# end of extension for left zero padding
padlno:			# remaining: root, possible right padding
	subl2 ndigit,width		# root reduces minimum width
	movl ndigit,r0			# root length
p1:	bsbw strout				# transfer to output buffer
p3:	jbc $vbit,r2,padnpct	# percent sign (or null byte via %c) ?
	jbs $31,nchar,prdone		# bail out if I/O error
	decl r0					# yes; adjust count
	movzbl (r1)+,r2			# fetch byte
	movq *fdesc,r4			# output buffer descriptor
	sobgeq r4,p2			# room at the out [inn] ?
	bsbw strout2			# no; force it, then try rest
	jbr p3					# here we go 'round the mullberry bush, ...
p2:	movb r2,(r5)+			# hand-deposit the percent or null
	incl nchar				# count it
	movq r4,*fdesc			# store output descriptor
	jbr p1					# what an expensive hiccup!
padnpct:
	movl width,r0	# size of pad
	jleq loop
	bsbb padb
	jbr loop

padz:
	movb $'0,r2
	jbr pad
padb:
	movb $' ,r2
pad:
	subl2 r0,width			# pad width decreases minimum width
	pushl r1				# save non-pad addr
	movl r0,llafx			# remember width of pad
	subl2 r0,sp				# allocate
	movc5 $0,(r0),r2,llafx,(sp)	# create pad string
	movl llafx,r0			# length
	movl sp,r1				# addr
	bsbw strout
	addl2 llafx,sp			# deallocate
	movl (sp)+,r1			# recover non-pad addr
	rsb

pone:	.byte	0x1C			# packed 1
	
charac:
	movl (ap)+,r0		# word containing the char
	movb r0,(r5)+		# one byte, that's all

prbuf:
	movl sp,r1			# addr first byte
	jbr prstr

space:	bisl2 $1<blank,flags		# constant width e fmt, no plus sign
	jbr L4a
sharp:	bisl2 $1<numsgn,flags		# 'self identifying', please
	jbr L4a
plus:	bisl2 $1<plssgn,flags		# always print sign for floats
	jbr L4a
minus:	bisl2 $1<minsgn,flags		# left justification, please
	jbr L4a
gnum0:	jbs $ndfnd,flags,gnum
	jbs $prec,flags,gnump		# ignore when reading precision
	bisl2 $1<zfill,flags		# leading zero fill, please
gnum:	jbs $prec,flags,gnump
	moval (width)[width],width	# width *= 5;
	movaw -ch.zer(r0)[width],width	# width = 2*witdh + r0 - '0';
	jbr gnumd
gnump:	moval (ndigit)[ndigit],ndigit	# ndigit *= 5;
	movaw -ch.zer(r0)[ndigit],ndigit # ndigit = 2*ndigit + r0 - '0';
gnumd:	bisl2 $1<ndfnd,flags		# digit seen
	jbr L4a
dot:	clrl ndigit			# start on the precision
	bisl2 $1<prec,flags
	bicl2 $1<ndfnd,flags
	jbr L4a
indir:
	jbs $prec,flags,in1
	movl (ap)+,width		# width specified by parameter
	jgeq gnumd
	xorl2 $1<minsgn,flags		# parameterized left adjustment
	mnegl width,width
	jbr gnumd
in1:
	movl (ap)+,ndigit		# precision specified by paratmeter
	jgeq gnumd
	mnegl ndigit,ndigit
	jbr gnumd

float:
	jbs $prec,flags,float1
	movl $6,ndigit			# default # digits to right of decpt.
float1:	bsbw fltcvt
	addl3 exp,ndigit,r7
	movl r7,r6			# for later "underflow" checking
	bgeq fxplrd
	clrl r7				# poor programmer planning
fxplrd:	cmpl r7,$31			# expressible in packed decimal?
	bleq fnarro			# yes
	movl $31,r7
fnarro:	subl3 $fltprec,r7,r0		# slr001 where to round
	ashp r0,$fltprec,(sp),$5,r7,16(sp)	# slr001 do it
	bvc fnovfl
		# band-aid for microcode error (spurious overflow)
	#	clrl r0				# assume even length result
	#	jlbc r7,fleven			# right
	#	movl $4,r0			# odd length result
	#fleven:	cmpv r0,$4,16(sp),$0		# top digit zero iff true overflow
	#	bneq fnovfl
		# end band-aid
	aobleq $0,r6,fnovfl		# if "underflow" then jump
	movl r7,r0
	incl exp
	incl r7
	ashp r0,$1,pone,$0,r7,16(sp)
	ashl $-1,r7,r0			# displ to last byte
	bisb2 sign,16(sp)[r0]		# insert sign
fnovfl:
	movab 16(sp),r1		# packed source
	movl r7,r6		# packed length
	pushab prnum	# goto prnum after fall-through call to fedit


	# enter via bsb
	#	r1=addr of packed source
	#	   16(r1) used to unpack source
	#	   48(r1) used to construct pattern to unpack source
	#	   48(r1) used to hold result
	#	r6=length of packed source (destroyed)
	#	exp=# digits to left of decimal point (destroyed)
	#	ndigit=# digits to right of decimal point (destroyed)
	#	sign=1 if negative, 0 otherwise
	# stack will be used for work space for pattern and unpacked source 
	# exits with
	#	r1=addr of punctuated result
	#	r5=addr of last byte +1
	#	llafx=1 if minus sign inserted, 0 otherwise
fedit:
	pushab 48(r1)			# save result addr
	movab 48(r1),r3			# pattern addr
	movb $0x03,(r3)+		# eo$set_signif
	movc5 $0,(r1),$0x91,r6,(r3)	# eo$move 1
	clrb (r3)				# eo$end
	editpc r6,(r1),48(r1),16(r1)	# unpack 'em all
	subl3 r6,r5,r1			# addr unpacked source
	movl (sp),r3			# punctuated output placed here
	clrl llafx
	jlbc sign,f1
	movb $'-,(r3)+		# negative
	incl llafx
f1:	movl exp,r0
	jgtr f2
	movb $'0,(r3)+		# must have digit before decimal point
	jbr f3
f2:	cmpl r0,r6			# limit on packed length
	jleq f4
	movl r6,r0
f4:	subl2 r0,r6			# eat some digits
	subl2 r0,exp		# from the exponent
	movc3 r0,(r1),(r3)	# (most of the) digits to left of decimal point
	movl exp,r0			# need any more?
	jleq f3
	movc5 $0,(r1),$'0,r0,(r3)	# '0 fill
f3:	movl ndigit,r0		# # digits to right of decimal point
	jgtr f5
	jbs $numsgn,flags,f5	# no decimal point unless forced
	jbcs $dpflag,flags,f6	# no decimal point
f5:	movb __lc_radix,(r3)+	# INTL the decimal point
f6:	mnegl exp,r0		# "leading" zeroes to right of decimal point
	jleq f9
	cmpl r0,ndigit		# cant exceed this many
	jleq fa
	movl ndigit,r0
fa:	subl2 r0,ndigit
	movc5 $0,(r1),$'0,r0,(r3)
f9:	movl ndigit,r0
	cmpl r0,r6			# limit on packed length
	jleq f7
	movl r6,r0
f7:	subl2 r0,ndigit		# eat some digits from the fraction
	movc3 r0,(r1),(r3)	# (most of the) digits to right of decimal point
	movl ndigit,r0			# need any more?
	jleq f8
		# check bounds on users who say %.300f
	movab 32(r3)[r0],r2
	subl2 fp,r2
	jlss fb
	subl2 r2,r0			# truncate, willy-nilly
	movl r0,ndigit		# and no more digits later, either
fb:
		#
	subl2 r0,ndigit		# eat some digits from the fraction
	movc5 $0,(r1),$'0,r0,(r3)	# '0 fill
f8:	movl r3,r5			# addr last byte +1
	popr $1<1			# [movl (sp)+,r1] addr first byte
	rsb

patexp:	.byte	0x03			# eo$set_signif
	.byte	0x44,'e			# eo$insert 'e
	.byte	0x42,'+			# eo$load_plus '+
	.byte	0x04			# eo$store_sign
# ifdef	GFLOAT
 # slr001 The exponent can be 3 characters long for gfloat numbers
	.byte	0x93			# eo$move slr001
# else
	.byte	0x92			# eo$move 2
#endif
	.byte	0			# eo$end

scien:
	incl ndigit
	jbs $prec,flags,L23
	movl $7,ndigit
L23:	bsbw fltcvt			# get packed digits
	movl ndigit,r7
	cmpl r7,$31				# expressible in packed decimal?
	jleq snarro				# yes
	movl $31,r7
snarro:	subl3 $fltprec,r7,r0		# slr001 rounding position
	ashp r0,$fltprec,(sp),$5,r7,16(sp) # slr001 shift and round
	bvc snovfl
		# band-aid for microcode error (spurious overflow)
	#	clrl r0				# assume even length result
	#	jlbc ndigit,sceven		# right
	#	movl $4,r0			# odd length result
	#sceven:	cmpv r0,$4,16(sp),$0		# top digit zero iff true overflow
	#	bneq snovfl
		# end band-aid
	incl exp			# rounding overflowed to 100...
	subl3 $1,r7,r0
	ashp r0,$1,pone,$0,r7,16(sp)
	ashl $-1,r7,r0		# displ to last byte
	bisb2 sign,16(sp)[r0]		# insert sign
snovfl:
	jbs $gflag,flags,gfmt		# %g format
	movab 16(sp),r1
	bsbb eedit
eexp:
	movl r1,r6		# save fwa from destruction by cvtlp
	subl3 $1,sexp,r0	# 1P exponent


# ifdef	GFLOAT
 # slr001	The exponent for gfloat numbers can be 3 characters long

	cvtlp r0,$3,(sp)	# slr001 need three positions for exponent
	editpc $3,(sp),patexp,(r5) # slr001
# else
	cvtlp r0,$2,(sp)	# packed
	editpc $2,(sp),patexp,(r5)
# endif
	movl r6,r1		# fwa
	jbc $caps,flags,prnum
# ifdef	GFLOAT
	xorb2 $'e^'E,-5(r5)	# extra digit for exponent -- adjust 'e'
# else
	xorb2 $'e^'E,-4(r5)
# endif
	jbr prnum

eedit:
	movl r7,r6		# packed length
	decl ndigit		# 1 digit before decimal point
	movl exp,sexp	# save from destruction
	movl $1,exp		# and pretend
	jbr fedit

gfmt:
	addl3 $3,exp,r0		# exp is 1 more than e
	jlss gfmte		# (e+1)+3<0, e+4<=-1, e<=-5
	subl2 $3,r0		# exp [==(e+1)]
	cmpl r0,ndigit
	jgtr gfmte		# e+1>n, e>=n
gfmtf:
	movl r7,r6
	subl2 r0,ndigit		# n-e-1
	movab 16(sp),r1
	bsbw fedit
g1:	jbs $numsgn,flags,g2
	jbs $dpflag,flags,g2	# dont strip if no decimal point
g3:	cmpb -(r5),$'0		# strip trailing zeroes
	jeql g3
	cmpb (r5), __lc_radix	# INTL. and trailing decimal point
	jeql g2
	incl r5
g2:	jbc $gflag,flags,eexp
	jbr prnum
gfmte:
	movab 16(sp),r1		# packed source
	bsbw eedit
	jbsc $gflag,flags,g1	# gflag now means "use %f" [hence no exponent]

general:
	jbs $prec,flags,gn1
	movl $6,ndigit		# default precision is 6 significant digits
gn1:	tstl ndigit		# cannot allow precision of 0
	jgtr gn2
	movl $1,ndigit		# change 0 to 1, willy-nilly
gn2:	jbcs $gflag,flags,L23
	jbr L23			# safety net

	# convert double-floating at (ap) to 17-digit packed at (sp),
	# set 'sign' and 'exp', advance ap.
fltcvt:
# ifdef	GFLOAT

/*
*	The following code is edit SLR001 upto the # else
*
*	The following code was taken from the OTSCVTRT.MAR routine 
*	which is part of the RTL sources. Most of the comments and
*	code are not changed except where needed.  The routine g_text
*	is a jacket routine to the ots$$cvt_g_t_r8 and is not part
*	of the actual OTSCVTRT.MAR routine. 
*
*	I have tried to keep
*	the code as close as possible to the orginal so if any
*	bugs occur in the RTL routine, those bugs can be easily added to
*	this routine.
*
* facility: language-independent support library
*
* abstract:
*
*	a routine to convert g and h floating values to a string of
*	ascii digits and an exponent.  it is meant to be used as
*	a base for floating point output conversion routines.
*
* environment: user mode, ast reentrant
*
*
*/


/*
* equated symbols:
*

* stack frame offsets from r7
* common frame for kernel convert routines
*/
# define packed -8
					# temp for packed representation
# define gflags packed-4
					# flags for outer and inner routines
# define sig_digits gflags-4
					# significant digits
# define string_addr sig_digits-4
					# address of temp string
# define sig string_addr-4
					# sign
# define dec_exp sig-4
					# decimal exponent
# define offset dec_exp-4

					# offset
# define rt_rnd offset-4
					# right round point
# define common_frame rt_rnd
					# common frame size


				# binnum holds the 4 long-words of
				# the binary fraction. it is initialized
				# with the "straightened out" fraction
				# bits from the h-floating number.
				# binnum+0<0> is the least significant bit
				# binnum+12<31> is the most sig bit
# define binnum 0
# define int binnum+16
				# int must be 1st word after the 4
				# longwords of binnum. it is used to catch
				# the binary for the 9 decimal digits
				# when binnum is multiplied by 10**9.

# define binexp int+4
				# the binary exponent. it is initialized
				# from the h-floating exponent.
# define prodf_4 binexp+4
				# a temporary for helping with the
				# 4x4 multiple precision multiply.
				# this word never gets all
				# the appropriate cross-products added in
				# and is not really part of the result.
				# it's here because "emul" always gives
				# double l-word products even when the low
				# word isn't needed (wanted).

/*
*	WARNING:
*		Because of some preprocessor problem the variable 
*		was hard coded in the RMUL routine. So if prodf 
*		changes you must also change the hard coded version
*/
# define prodf prodf_4+4
				# the 4 long-words of prodf must start
				# just after prodf_4 (which is always
				# used as prodf-4).

# define cry prodf+16
				# used for a "carry save" multiply.

# define local_frame cry+16	
				# size of data area to allocate on stack


# define t_bexp 16	
				# the binary exponent is bytes 16-17 of each table entry

# define t_dexp 18	
				# the decimal exponent is bytes 18-19

/*
*	This is a jacket routine that is needed to interface
*	doprnt with the gfloat conversion routine
*/

g_text:		movab	4(sp),r5		# r5 -> addr to receive number
		movl	sp,r1			# r1 -> start of common_frame
		movad	(ap)+,r0		# r0 -> addr of number to convert
		movab	common_frame(sp),sp	# subract common_frame from stack
		movl	r5,string_addr(r1)	# store addr to common_frame
		movl	$17,sig_digits(r1)	# cheat and say that we need 17
		pushr	$1<8			# save reg 8
		jsb	ots$$cvt_g_t_r8		# call the convertion routine
		popr	$1<8			# restore reg 8
		movl	dec_exp(r1),exp		# get computed exponent
		movb	sig(r1),sign		# get computed sign
		movl	r1,sp			# restore stack
		rsb				# return

#
/*
* functional description:
*
*	this routine converts a g or h floating point value to a string
*	of ascii digits.  it is intended to form the base of a
*	language's floating point output conversion routine.
*
*
* calling sequence:
*
*	movab	common_frame, r1	; see common_frame definition above
*	movl	string_address, string_addr(r1)
*	movl	sig_digits, sig_digits(r1)
*	movl	user_flags, gflags(r1)
*	movl	rt_round, rt_rnd(r1)	;  optional
*	movab	value, r0
*	jsb	ots$$cvt_x_t_r8		; x is the datatype, g or h
*	; outputs are:
*	;	offset(r1) - offset
*	;	dec_exp(r1) - decimal exponent
*	;	sig(r1) - sign
*
* input parameters:
*
*	value				; floating value to be converted
*	sig_digits(r1)			; number of significant digits to
*					; generate.  if neither v_truncate
*					; or v_round_right is set, the
*					; value will be rounded to this
*					; many digits.
*	gflags(r1)			; caller supplied flags:
*	    v_truncate = 24		; truncate, don't round.
*	    v_round_right = 25		; round "rt_round" digits to
*					; right of decimal point.
*	rt_rnd(r1)			; number of places to the right
*					; of the decimal point to round
*					; after.  ignored if v_round_right
*					; is clear.
*
* implicit inputs:
*
*	none
*
* output parameters:
*
*	out_string			; string with result.  it will
*					; not have valid digits after the
*					; requested number of significant
*					; digits.
*					; the length must be at least:
*					; (9*int((sig_digits+8)/9))+2
*	offset				; the offset into out_string at
*					; which the first significant digit
*					; may be found.  it is guaranteed
*					; to be either 0 or 1.
*	exponent			; the signed decimal exponent of
*					; the value, assuming a radix point
*					; immediately to the left of the
*					; most significant digit.
*	sign				; -1 if the value is negative
*					; 0 if the value is zero
*					; 1 if the value is positive
*
* implicit outputs:
*
*	none
*
* side effects:
*
*	alters registers r0 through r8.
*
*			  this routine does not check the length, it
*			  is up to the caller to insure the correct
*			  length is present.
*
*/

/*
*	.sbttl	ots$$cvt_g_t_r8
*/

/*
* jsb entry point
*/
	.globl	ots$$cvt_g_t_r8

ots$$cvt_g_t_r8:
	movl	r1, r7			# use r7 as base
	clrl	dec_exp(r7)		# init decimal exponent
tstval_g:
	extv	$4, $12, (r0), r1	# test for zero and negative
	jneq	1f			# not zero
	jbr	zero			# is zero
1:	jlss	neg_val_g		# negative?
 #	movl	$1, sig(r7)		# no, set sign
	clrl	sig(r7)
	jbr	notres_g		# continue
neg_val_g:
	extzv	$0, $11, r1, r1		# reserved operand?
	jneq	1f			# no
 #	calls	$0,5f 			# reserved operand
	cmpzv	$4, $12, (r0), $0x800	# still reserved?
	jneq	tstval_g		# no, try again
	jbr	zero			# still reserved, call it zero
 #5:	.word	^m<>
 #	movab	w'cvt_handler, (fp)	# enable condition handler
 #	tstg	(r0)			# force reserved operand fault
 #	ret				# continue

1:
 #	mnegl	$1, sig(r7)		# set negative sign
        	movl	$1, sig(r7)
notres_g:
	subl2	$local_frame, sp	# allocate local data on stack
	movl	sp, r8			# setup pointer to local data area
	subl3	$0x400, r1, binexp(r8)	# remove excess from exponent

		/* pick up g-floating fraction and store as a left
		* normalized unsigned 4-longword integer with the binary
		* point between bits 32 & 31 of "binnum+12"
		*/

	rotl	$16, (r0), r4		# get high fraction
	rotl	$16, 4(r0), r3		# get low fraction

		/* denormalize by 1 bit to insert
		* the hidden bit. 
		*/

	clrq	binnum+0(r8)		# clear low order bits
	clrl	r2
	extv	$21, $32, r2, binnum+8(r8)
	extzv	$21, $31, r3, r4
	bisl3	$0x80000000, r4, binnum+12(r8)	# and set hidden bit
	jbr	begsrc		# now convert the value


		/* now search the power-of-ten table to find
		* an entry close to the value stored
		* in binexp & binnum. then divide (or rather
		* multiply by the reciprocal) binexp & binnum
		* by that table entry to get the resultant
		* fraction into the range:
		*	 1.0 .gt. (fraction * 2** exponent) .ge. 0.1

		* the table search is broken into three pieces: the
		* big number exponential search (starting at bigexp), 
		* the small number exponential search (starting at
		* smlexp), and the middle number search of the linear
		* portion of the table (starting at srclin).
		*/

begsrc:	movaw	tm16, r2		# get 1st adr of linear table
	cmpw	t_bexp(r2), binexp(r8)	# compare with entry's bin exp
	jgtr	smlexp			# branch for small numbers
 #	cmpw	<t16-tm16>+t_bexp(r2), binexp(r8)
		cmpw	0x290(r2),binexp(r8)
					# compare with last linear entry
	jgtr	srclin			# branch for linear search

		/* the two searches which follow (bigexp & smlexp) find
		* the table entry closest to the number stored in
		* binexp(r8). this table entry is used to divide (or
		* multiply by the reciprocal) binexp & binnum.
		*/

bigexp:	movaw	t16, r2			# exponential search for big numbers
	jbr	bigex1

smlexp:	movaw	tsmall, r2		# exponential search for small numbers
bigex1:	cvtwl	t_bexp(r2), r0		# get power-of-2 from table
	ashl	$-1, r0, r1		# for large, calc: 1.5*entry
	jgeq	bigex2			# xfer for big nums (positive exponent)
	ashl	$-1, r1, r1		# for small, calc: .75*entry
	mnegl	r1, r1
bigex2:	addl2	r1, r0			# form .75*entry or 1.5*entry
					# r0 now contains value half way
					# between this and next entry.
	cmpw	r0, binexp(r8)		# is this closest table entry?
	jgeq	bigex3			# if yes, xfer
 #	addl2	$<t1-t0>, r2		# no, go look at next entry
		addl2	$0x14,r2
	jbr	bigex1

bigex3:	bsbw	rmul			# yes, go mul by reciprocal
	jbr	begsrc			#   and go try again

srclin:
		/* the conversion will take place from the linear (in
		* powers of ten) part of the table.
		* the decimal_exponent = 1 + log10(2) * (bin_exp - 1). use this
		* approximation to get the 1st probe into the table.
		* this approx may be 1 small, but no more than that.
		* the approx has been tested exhaustively over the
		* range -106 .le. bin_exp .le. +108 and always works
		* except for bin_exp=1 which has a special code hack.
		*/

	subl3	$1, binexp(r8), r1	# get (binexp - 1)
	jeql	srcl1			# if binexp=+1, return 0 (hack)
	mull2	$1233, r1		# 1233 = 4096 * log10(2)
	ashl	$-12, r1, r1		# remove the 4096 factor
	incl	r1			# final +1

srcl1:	
 #	mull2	$<t1-t0>, r1		# mul by size of table entry
		mull2	$0x14,r1
	addl2	r1, r2			# get index*size+tm16
 #	addl2	$<t0-tm16>, r2		# get index*size+t0
		addl2	$0x140,r2

	cmpw	t_bexp(r2), binexp(r8)	# compare exponents
	jgtr	found			# xfer if entry .gt. binnum

		# the next instruction is commented out. it can not xfer.
/*	jlss	small			; xfer if entry too small */
	cmpl	12(r2), binnum+12(r8)	# compare high-order fraction
	jgtru	found
	jlssu	small
	cmpl	8(r2), binnum+8(r8)
	jgtru	found
	jlssu	small
	cmpl	4(r2), binnum+4(r8)
	jgtru	found
	jlssu	small
	cmpl	0(r2), binnum+0(r8)	# compare low-order fraction
	jgtru	found
small:	
 #	addl2	$<t1-t0>, r2		# advance to next table item
		addl2	$0x14,r2

		/* final check for debugging. remove these next three
		* instructions after all the testing is done. (or
		* leave them in-- they don't really hurt.)
		*/

/*	cmpw	t_bexp(r2), binexp(r8)	; final size check
*	jgtr	found
*	halt				; bad index formula
*/

found:	movaw	t0, r0			# get table base adr
	cmpl	r2, r0
	jeql	muldun			# if 0, don't mul by 1.0
	bsbw	rmul			# and multiply by reciprocal

muldun:		/* binexp should now contain 0, -1, -2, or -3.
		* shift binnum right by that number of places
		* in order to reduce binexp to zero, thus
		* finally finishing with the binary exponent
		* round using the bits shifted off to the right
		*/

	mnegl	binexp(r8), r0			# find bit $ from binexp
	jeql	getdig				# if 0, skip right shift

	subl3	$1, r0, r1			# get pos of 1st discarded bit
	extzv	r1, $1, binnum+0(r8), r1	# get 1st discarde bit

	extv	r0, $32, binnum+0(r8), binnum+0(r8)
	extv	r0, $32, binnum+4(r8), binnum+4(r8)
	extv	r0, $32, binnum+8(r8), binnum+8(r8)
	clrl	binnum+16(r8)			# next extv will get 0's here
	extv	r0, $32, binnum+12(r8), binnum+12(r8)
/*	clrl	binexp(r8)			; binexp now reduced to zero */

	addl2	r1, binnum+0(r8)		# round with 1st discarded bit
	adwc	$0, binnum+4(r8)
	adwc	$0, binnum+8(r8)
	adwc	$0, binnum+12(r8)

getdig:	movl	string_addr(r7), r5		# get adr for digit string
	addl3	$1, sig_digits(r7), r6		# number of digits wanted
	movl	$1, offset(r7)			# initial offset
 #	movb	$^a/0/, (r5)+			# start out with a zero
 #		movb	$0x30,(r5)+

		/* now mul the binnum fraction by 10**9 in order to
		* force 9 digits to the left of the decimal point.
		* then convert that 9 digit binary integer to a
		* string for output in the final answer. repeat
		* the process until enough digits are output.
		*/

/* .macro imul2 i, r, ?l
*	emul	i, r, $0, r0
*	tstl	r
*	jgeq	l
*	addl2	i, r1
*l:	movl	r0, r
*	addl2	r1, 4+r
*.endm imul2
*/

diglup:	clrl	int(r8)			# clear for digits left of bin point

		/* multiply 4-long-words by 10**9, propogating carries
		* across the long-word boundaries.
		*/

	movl	$1000000000, r2			# setup 10**9

/*	imul2	r2, binnum+12(r8)	*/

		emul	r2, binnum+12(r8), $0, r0
		tstl	binnum+12(r8)
		jgeq	1f
		addl2	r2, r1
1:		movl	r0, binnum+12(r8)
		addl2	r1, 4+binnum+12(r8)

/*	imul2	r2, binnum+8(r8)		*/

		emul	r2, binnum+8(r8), $0, r0
		tstl	binnum+8(r8)
		jgeq	2f
		addl2	r2, r1
2:		movl	r0, binnum+8(r8)
		addl2	r1, 4+binnum+8(r8)

	adwc	$0, int(r8)

/*	imul2	r2, binnum+4(r8)		*/

		emul	r2, binnum+4(r8), $0, r0
		tstl	binnum+4(r8)
		jgeq	3f
		addl2	r2, r1
3:		movl	r0, binnum+4(r8)
		addl2	r1, 4+binnum+4(r8)

	adwc	$0, binnum+12(r8)
	adwc	$0, int(r8)
 #	imul2	r2, binnum+0(r8)


		emul	r2, binnum+0(r8), $0, r0
		tstl	binnum+0(r8)
		jgeq	4f
		addl2	r2, r1
4:		movl	r0, binnum+0(r8)
		addl2	r1, 4+binnum+0(r8)

	adwc	$0, binnum+8(r8)
	adwc	$0, binnum+12(r8)
	adwc	$0, int(r8)

		# convert binary num now left of decimal point into
		# 9 packed digits.

 #	cvtlp	int(r8),$9,packed(r7)	# store 9 packed digits
 #	movb	-(r5), r4			# save byte
 #	cvtps	$9,packed(r7),$9,(r5)	# convert to separate
 #	movb	r4, (r5)+			# restore byte
 # 	addl2	$9, r5				# advance output string adr
 	subl2	$9, r6				# 9 more digits
 #	jleq	round				# loop for more?
		jleq	finis1
		cvtlp	int(r8),$9,packed(r7)	# Get the first nine digits
		ashp	$9,$9,packed(r7),$0,$19,(r5)
						# At the end we will have 18
						# digits but we need space for 19
 #	movb	-(r5), r4			# save byte
	jbr	diglup				# yes


/*
* this routine rounds the value to the given number of significant
* digits, unless flag v_truncate is on.  if so, the value is truncated
* at the next digit.

round:
	decl	r6
	addl2	r6, r5			# find least significant + 1
	jbs	$v_truncate, gflags(r7), finis	# truncate if desired
	jbc	$v_round_right, gflags(r7), 5f	# round to right of dec pt?
	addl3	dec_exp(r7), rt_rnd(r7), r1	# yes, find it
	jlss	finis			# exit if round to zero
	cmpl	r1, sig_digits(r7)	# round to right of $ sig digits?
	jgeq	5f			# yes, use number of significant
					# digits instead.
	incl	r1			# finish calculation
	addl3	r1, string_addr(r7), r5	# get rounding character address
5:
 #	cmpb	(r5), $^a/5/		# round?
		cmpb	(r5), $0x35
	jlss	finis			# no, just finish
	movl	r5, r0			# save position
1:
 #	cmpb	-(r0), $^a/9/		# if this is a 9...
		cmpb	-(r0),$0x39
	jlss	2f
 #	movb	$^a/0/, (r0)		# then it becomes a zero
		movb	$30,(r0)
	jbr	1b			# and we continue
2:	incb	(r0)			# else this is last carry
	subl2	string_addr(r7), r0	# do we need to change offset
	jgtr	finis			# no
	clrl	offset(r7)		# yes, set new offset
	incl	dec_exp(r7)		# set new exponent
*/

/*	
* all done.
*/
finis1:	cvtlp	int(r8),$9,packed(r7)	# Get the trailing nine digits
	addp4	$9,packed(r7),$19,(r5)	# Add the front nine with the back nine
	ashp	$-1,$fltprec+3,(r5),$0,$fltprec+2,12(r5)	# Get the 17 significant digits
	ashp	$-1,$fltprec+2,12(r5),$5,$fltprec,(r5)	# Round on the 17th digit to 
						# get the 16 significant digits
	bvc	1f				# slr001 br if no overflow
/*
 *		SLR001
 *  This will fix the problem when the rounding causes the most signif. digit
 *  to cause an overflow.
 */
	ashp	$-1,$fltprec+2,12(r5),$5,$fltprec,(r5)	# Round on the 17th digit to 
						# get the 16 significant digits
	incl	dec_exp(r7)			# Adjust the exponent to
						# account for the rounding.

1:	bisb2	sig(r7),7(r5)			# Set sign
finis:
	addl2	$local_frame, sp		# restore stack pointer
	movl	r7, r1			# restore frame pointer
	rsb				# return to caller

zero:
 #	movl	string_addr(r7), r1	# get string address
 #	movc5	$0, (sp), $^a/0/, sig_digits(r7), (r1) # zero fill string
	clrl	offset(r7)		# clear offset and exponent
	movl	$1,dec_exp(r7)
	clrl	sig(r7)			# clear sign
	cvtlp	$0,$fltprec,(r5)
	movl	r7, r1			# restore frame pointer
	rsb				# return to caller


/* this is the subroutine which does the multiple
* precision multiplies. it is called with bsb or jsb
* with r2 containing a pointer to an appropriate
* entry in the power-of-ten table. binexp & binnum
*  are multiplied by the reciprocal of this entry, 
* with the results going to
* binexp & binnum, and decexp is updated with the
* power_of_ten value.
* this routine clobbers r0-r1, r3-r6, and changes r2.
*/

 #ots$$cvt_mul::
rmul:					# entry point	

		/* find the reciprocal table entry pointed to by r2.
		* r2 contains the base (t0) plus an "index". the
		* reciprocal entry has an adr of "t0-index" which
		* is calculated by 2*t0-(t0+index), or 2*t0-r2.
		*/

	moval	t0, r1			# get base adr
	addl2	r1, r1			# 2*base
	subl3	r2, r1, r2		# get adr of reciprocal entry

	clrl	prodf-4(r8)		# init product
	clrq	prodf+0(r8)
	clrq	prodf+8(r8)

	clrq	cry+0(r8)		# clear carries
	clrq	cry+8(r8)

		/* this macro has the function r=a*b, with the carries
		* going into the 4 l-words at "cry". a and b are
		* unsigned long-words. r is an unsigned double long-word.
		*  removing this macro definition (which is only used once), 
		*  and expanding the code where it is used, obscures the function.
		*/

/* macro lmul a, b, r, ?l1, ?l2, ?l3
*	movl	a, r0			# get first operand
*	jeql	l3			# skip if zero
*	movl	b, r1			# get second operand
*	jeql	l3			# skip if zero
*	emul	r0, r1, $0, r0		# form product of a and b
*	tstl	a
*	jgeq	l1
*	addl2	b, r1			# if a<0, fixup for neg sign
*l1:	tstl	b
*	jgeq	l2
*	addl2	a, r1			# if b<0, fixup for neg sign
*l2:	addl2	r0, r			# add low product into result
*	adwc	r1, 4+r			# add hi product into result
*	adwc	$0, cry+8-prodf+r	# and save carries
*l3:
*	.endm lmul
*/
		/* the following loop forms all the cross-products
		* required for a 4-long-word by 4-long-word multiply.
		* only the high 4 long-words are accumulated. the byte
		* table at "bytab" shows the indicies used for the
		* long-word operands and the resulting double-long-
		* word products.
		*/

	movaw	bytab, r3		# init byte table index
bytlup:	cvtbl	(r3)+, r4		# setup 1st index
	jlss	bytdun			# and test for end
	cvtbl	(r3)+, r5		# setup 2nd index
	cvtbl	(r3)+, r6		# setup 3rd index

/*	lmul	binnum(r8)[r4], 0(r2)[r5], prodf_4(r8)[r6]  */

		movl	binnum(r8)[r4], r0	# get first operand
		jeql	l3			# skip if zero
		movl	0(r2)[r5], r1		# get second operand
		jeql	l3			# skip if zero
		emul	r0, r1, $0, r0		# form product of a and b
		tstl	binnum(r8)[r4]
		jgeq	l1
		addl2	0(r2)[r5], r1		# if a<0, fixup for neg sign
l1:		tstl    0(r2)[r5]	
		jgeq	l2
		addl2	binnum(r8)[r4], r1	# if b<0, fixup for neg sign
l2:		addl2	r0, prodf_4(r8)[r6]	# add low product into result
		adwc	r1, 4+prodf_4(r8)[r6]	# add hi product into result
 #		adwc	$0, cry+8-prodf+prodf_4(r8)[r6]	# and save carries
/*
*	This had to be hard code because of preprocessor problems
*/
			adwc	$0,0x30(r8)[r6]
l3:

	jbr	bytlup			# loop

bytdun:
/*	incl	cry+0(r8)		; small extra fudge */
	addl2	cry+0(r8), prodf+0(r8)	# put carries into sum
	adwc	cry+4(r8), prodf+4(r8)
	adwc	cry+8(r8), prodf+8(r8)
	adwc	cry+12(r8), prodf+12(r8)

	extzv	$31, $1, prodf+12(r8), r1	# get normalize bit

		/* normalized operands cannot produce a result
		* un-normalized by more than one bit position. so
		* if norm_bit=1, shift left by 0
		* if norm_bit=0, shift left by 1 and sub 1 from exp
		*/

	jneq	nosub1			# xfer if norm_bit = 1
	decl	binexp(r8)		# norm_bit = 0, sub 1 from exponent

		# move the product from prodf to binnum, normalizing
		# it one bit position if required.

nosub1:	addl2	$31, r1				# do extv's from bit 31 or 32

	extv	r1, $32, prodf-4(r8), binnum+0(r8)	# shift left 0 or 1 bit
	extv	r1, $32, prodf+0(r8), binnum+4(r8)
	extv	r1, $32, prodf+4(r8), binnum+8(r8)
	extv	r1, $32, prodf+8(r8), binnum+12(r8)

	cvtwl	t_bexp(r2), r1		# extract binary exponent
	addl2	r1, binexp(r8)		# add exponents for mul

		/* the binary exponent moves toward zero while the
		* decimal exponent moves away from zero by an amount
		* about equal to log(bin exp).
		*/

	cvtwl	t_dexp(r2), r1		# get equivalent decimal exponent
	subl2	r1, dec_exp(r7)		# and sub it from result exp

	rsb				# return

 #	.sbttl	tables


 #.macro number a1, a2, a3, a4, a5, a6, a7
 #	.long ^x'a5+<<^x'a6@-31>&1>, ^x'a4, ^x'a3, ^x'a2
 #	.word ^d<a1>, ^d<a7>
 #.endm number

#define number(a1,a2,a3,a4,a5,a6,a7)	 \
	.long 0x/**/a5+((0x/**/a6>>31)&1), 0x/**/a4,0x/**/a3,0x/**/a2  \
 ;	.word a1,a7

		# this macro creates a table entry of the following form:

		#	.long < least sig bits>	:     0(r2)
		#	.long <   ........    >	:     4(r2)
		#	.long <   ........    >	:     8(r2)
		#	.long < most sig bits >	:    12(r2)
		#	.word < binary exp    >	:t_bexp(r2)
		#	.word < decimal exp   >	:t_dexp(r2)

 #t_bexp=16	# the binary exponent is bytes 16-17 of each table entry
 #t_dexp=18	# the decimal exponent is bytes 18-19


/*            value = fraction * 2**power_of_2 = 10**power_of_10

*	the hex fraction is stored as a 4 long-word unsigned integer,
*	left normalized, with the binary point left of bit 31
*	of the most significant long-word.

*	the fraction is guaranteed correct for the four high-order
*	long-words. about 16 bits of the fifth low-order long-word may
*	be in error. the check line at the bottom of the table is
*	the product of the first and last table entries. it would
*	equal exactly 1.0 if every bit of the 5 long-words were correct.

*	      decimal,<-------5 long-word hex fraction----------->, decimal
*	       power ,<--msb--------------------------------lsb-->,  power
*	       of 2                                                  of 10
*
*
*/

	.align	2

tsmall:
 #	number (-27213,d986c20b,686da869,5d1d4fd8,5b05f4c2,eef0fb87,-8192)
	number (-13606,a6dd04c8,d2ce9fde,2de38123,a1c3cffc,203028da,-4096)
	number ( -6803,ceae534f,34362de4,492512d4,f2ead2cb,8263aa10,-2048)
	number ( -3401,a2a682a5,da57c0bd,87a60158,6bd3f698,f53e881e,-1024)
	number ( -1700,9049ee32,db23d21c,7132d332,e3f204d4,e73177c2, -512)
	number (  -850,c0314325,637a1939,fa911155,fefb5308,a23e2b15, -256)
	number (  -425,ddd0467c,64bce4a0,ac7cb3f6,d05ddbde,e26ca3df, -128)
	number (  -212,a87fea27,a539e9a5,3f2398d7,47b36224,2a1fed70,  -64)
 # tm32:
	number (  -106,cfb11ead,453994ba,67de18ed,a5814af2,b5b1a20,  -32)
/*	number (  -102,81ceb32c,4b43fcf4,80eacf94,8770ced7,4718f05a,  -31)
*	number (   -99,a2425ff7,5e14fc31,a1258379,a94d028d,18df2c73,  -30)
*	number (   -96,cad2f7f5,359a3b3e, 96ee458,13a04330,5f16f793,  -29)
*	number (   -93,fd87b5f2,8300ca0d,8bca9d6e,188853fc,76dcb57b,  -28)
*	number (   -89,9e74d1b7,91e07e48,775ea264,cf55347d,ca49f16f,  -27)
*	number (   -86,c6120625,76589dda,95364afe,32a819d,3cdc6dcd,  -26)
*	number (   -83,f79687ae,d3eec551,3a83ddbd,83f52204,8c138944,  -25)
*	number (   -79,9abe14cd,44753b52,c4926a96,72793542,d78c35ce,  -24)
*	number (   -76,c16d9a00,95928a27,75b7053c,f178293,8d6f434a,  -23)
*	number (   -73,f1c90080,baf72cb1,5324c68b,12dd6338,70cb1420,  -22)
*	number (   -69,971da050,74da7bee,d3f6fc16,ebca5e03,467eec97,  -21)
*	number (   -66,bce50864,92111aea,88f4bb1c,a6bcf584,181ea7c0,  -20)
*	number (   -63,ec1e4a7d,b69561a5,2b31e9e3,d06c32e5,1e2651b1,  -19)
*	number (   -59,9392ee8e,921d5d07,3aff322e,62439fcf,32d7f311,  -18)
*	number (   -56,b877aa32,36a4b449,9befeb9,fad487c2,ff8defdb,  -17)
*/
tm16:	number (   -53,e69594be,c44de15b,4c2ebe68,7989a9b3,bf716bd5,  -16)
	number (   -49,901d7cf7,3ab0acd9,f9d3701,4bf60a10,57a6e369,  -15)
	number (   -46,b424dc35,95cd80f,538484c1,9ef38c94,6d909c46,  -14)
	number (   -43,e12e1342,4bb40e13,2865a5f2,6b06fb9,88f4c35a,  -13)
	number (   -39,8cbccc09,6f5088cb,f93f87b7,442e45d3,f598fa1c,  -12)
	number (   -36,afebff0b,cb24aafe,f78f69a5,1539d748,f2ff38a8,  -11)
	number (   -33,dbe6fece,bdedd5be,b573440e,5a884d1b,2fbf06d5,  -10)
	number (   -29,89705f41,36b4a597,31680a88,f8953030,fdd76447,   -9)
	number (   -26,abcc7711,8461cefc,fdc20d2b,36ba7c3d,3d4d3d5c,   -8)
	number (   -23,d6bf94d5,e57a42bc,3d329076,4691b4c,8ca08cb8,   -7)
	number (   -19,8637bd05,af6c69b5,a63f9a49,c2c1b10f,d7e457f7,   -6)
	number (   -16,a7c5ac47,1b478423,fcf80dc,33721d53,cddd6df6,   -5)
	number (   -13,d1b71758,e219652b,d3c36113,404ea4a8,c154c978,   -4)
	number (    -9,83126e97,8d4fdf3b,645a1cac,83126e9,78d4fdee,   -3)
	number (    -6,a3d70a3d,70a3d70a,3d70a3d7,a3d70a3,d70a3d6c,   -2)
	number (    -3,cccccccc,cccccccc,cccccccc,cccccccc,cccccccc,   -1)
 #ots$$a_cvt_tab::
t0:	number (     1,80000000,00000000,00000000,00000000,00000000,    0)
t1:	number (     4,a0000000,00000000,00000000,00000000,00000000,    1)
	number (     7,c8000000,00000000,00000000,00000000,00000000,    2)
	number (    10,fa000000,00000000,00000000,00000000,00000000,    3)
	number (    14,9c400000,00000000,00000000,00000000,00000000,    4)
	number (    17,c3500000,00000000,00000000,00000000,00000000,    5)
	number (    20,f4240000,00000000,00000000,00000000,00000000,    6)
	number (    24,98968000,00000000,00000000,00000000,00000000,    7)
	number (    27,bebc2000,00000000,00000000,00000000,00000000,    8)
	number (    30,ee6b2800,00000000,00000000,00000000,00000000,    9)
	number (    34,9502f900,00000000,00000000,00000000,00000000,   10)
	number (    37,ba43b740,00000000,00000000,00000000,00000000,   11)
	number (    40,e8d4a510,00000000,00000000,00000000,00000000,   12)
	number (    44,9184e72a,00000000,00000000,00000000,00000000,   13)
	number (    47,b5e620f4,80000000,00000000,00000000,00000000,   14)
	number (    50,e35fa931,a0000000,00000000,00000000,00000000,   15)
t16:	number (    54,8e1bc9bf,4000000,00000000,00000000,00000000,   16)
/*	number (    57,b1a2bc2e,c5000000,00000000,00000000,00000000,   17)
*	number (    60,de0b6b3a,76400000,00000000,00000000,00000000,   18)
*	number (    64,8ac72304,89e80000,00000000,00000000,00000000,   19)
*	number (    67,ad78ebc5,ac620000,00000000,00000000,00000000,   20)
*	number (    70,d8d726b7,177a8000,00000000,00000000,00000000,   21)
*	number (    74,87867832,6eac9000,00000000,00000000,00000000,   22)
*	number (    77,a968163f, a57b400,00000000,00000000,00000000,   23)
*	number (    80,d3c21bce,cceda100,00000000,00000000,00000000,   24)
*	number (    84,84595161,401484a0,00000000,00000000,00000000,   25)
*	number (    87,a56fa5b9,9019a5c8,00000000,00000000,00000000,   26)
*	number (    90,cecb8f27,f4200f3a,00000000,00000000,00000000,   27)
*	number (    94,813f3978,f8940984,40000000,00000000,00000000,   28)
*	number (    97,a18f07d7,36b90be5,50000000,00000000,00000000,   29)
*	number (   100,c9f2c9cd, 4674ede,a4000000,00000000,00000000,   30)
*	number (   103,fc6f7c40,45812296,4d000000,00000000,00000000,   31)
* t32:
*/
	number (   107,9dc5ada8,2b70b59d,f0200000,00000000,00000000,   32)
	number (   213,c2781f49,ffcfa6d5,3cbf6b71,c76b25fb,50f80800,   64)
	number (   426,93ba47c9,80e98cdf,c66f336c,36b10137,234f3fc,  128)
	number (   851,aa7eebfb,9df9de8d,ddbb901b,98feeab7,851e4cbb,  256)
	number (  1701,e319a0ae,a60e91c6,cc655c54,bc5058f8,9c658389,  512)
	number (  3402,c9767586,81750c17,650d3d28,f18b50ce,526b9865, 1024)
	number (  6804,9e8b3b5d,c53d5de4,a74d28ce,329ace52,6a31978c, 2048)
	number ( 13607,c4605202,8a20979a,c94c153f,804a4a92,65761f39, 4096)
 #	number ( 27214,96a3a1d1,7faf211a,c7c2892,305f4e12,72b205f, 8192)
 #                   0,ffffffff,ffffffff,ffffffff,ffffffff,ffff5eb4	; 1.0 if exact)

		# this table contains the byte indicies for the
		# multiple precision multiply cross products.
		# the 1st and 2nd entries on each line are the indicies
		# for the multiplicand and the multiplier. the third
		#  entry is the product index.

bytab:	.byte	0,3,0
	.byte	3,0,0
	.byte	2,1,0
	.byte	1,2,0
	.byte	1,3,1
	.byte	3,1,1
	.byte	2,2,1
	.byte	2,3,2
	.byte	3,2,2
	.byte	3,3,3
	.byte	-1		# end flag


# else

	clrb sign
	movd (ap)+,r5
	jeql fzero
	bgtr fpos
	mnegd r5,r5
	incb sign
fpos:
	extzv $7,$8,r5,r2		# exponent of 2
	movab -0200(r2),r2		# unbias
	mull2 $59,r2			# 59/196: 3rd convergent continued frac of log10(2)
	jlss eneg
	movab 196(r2),r2
eneg:
	movab -98(r2),r2
	divl2 $196,r2
	bsbw expten
	cmpd r0,r5
	bgtr ceil
	incl r2
ceil:	movl r2,exp
	mnegl r2,r2
	cmpl r2,$29			# 10^(29+9) is all we can handle
	bleq getman
	muld2 ten16,r5
	subl2 $16,r2
getman:	addl2 $9,r2			# -ceil(log10(x)) + 9
	jsb expten
	emodd r0,r4,r5,r0,r5		# (r0+r4)*r5; r0=int, r5=frac
fz1:	cvtlp r0,$9,16(sp)		# leading 9 digits
	ashp $8,$9,16(sp),$0,$17,4(sp)	# as top 9 of 17
	emodd ten8,$0,r5,r0,r5
	cvtlp r0,$8,16(sp)		# trailing 8 digits
		# if precision >= 17, must round here
	movl ndigit,r7			# so figure out what precision is
	pushab scien
	cmpl (sp)+,(sp)
	jleq gm1			# who called us?
	addl2 exp,r7			# float; adjust for exponent
gm1:	cmpl r7,$17
	jlss gm2
	cmpd r5,$0d0.5			# must round here; check fraction
	jlss gm2
	bisb2 $0x10,8+4(sp)		# increment l.s. digit
gm2:		# end of "round here" code
	addp4 $8,16(sp),$17,4(sp)	# combine leading and trailing
	bisb2 sign,12(sp)		# and insert sign
	rsb
fzero:	clrl r0
	movl $1,exp		# 0.000e+00 and 0.000 rather than 0.000e-01 and .000
	jbr fz1

	.align 2
lsb: .long 0x00010000		# lsb in the crazy floating-point format

	# return 10^r2 as a double float in r0||r1 and 8 extra bits of precision in r4
	# preserve r2, r5||r6
expten:
	movd $0d1.0,r0			# begin computing 10^exp10
	clrl r4				# bit counter
	movad ten1,r3			# table address
	tstl r2
	bgeq e10lp
	mnegl r2,r2			# get absolute value
	jbss $6,r2,e10lp		# flag as negative
e10lp:	jbc r4,r2,el1			# want this power?
	muld2 (r3),r0			# yes
el1:	addl2 $8,r3			# advance to next power
	aobleq $5,r4,e10lp		# through 10^32
	jbcc $6,r2,el2			# correct for negative exponent
	divd3 r0,$0d1.0,r0		# by taking reciprocal
	cmpl $28,r2
	jneq enm28
	addl2 lsb,r1			# 10**-28 needs lsb incremented
enm28:	mnegl r2,r2			# original exponent of 10
el2:	addl3 $5*8,r2,r3		# negative bit positions are illegal?
	jbc r3,xlsbh-5,eoklsb
	subl2 lsb,r1			# lsb was too high
eoklsb:
	movzbl xprec[r2],r4		# 8 extra bits
	rsb

	# powers of ten
	.align	2
ten1:	.word	0x4220,0,0,0
ten2:	.word	0x43c8,0,0,0
ten4:	.word	0x471c,0x4000,0,0
ten8:	.word	0x4dbe,0xbc20,0,0
ten16:	.word	0x5b0e,0x1bc9,0xbf04,0
ten32:	.word	0x759d,0xc5ad,0xa82b,0x70b6

	# whether lsb is too high or not
	.byte 1:0,1:0,1:0,1:0,1:1,1:0,1:1,1:0	# -40 thru -33
	.byte 1:0,1:1,1:0,1:0,1:0,1:0,1:1,1:0	# -32 thru -25
	.byte 1:0,1:0,1:1,1:1,1:1,1:1,1:0,1:0	# -24 thru -17
	.byte 1:0,1:1,1:0,1:0,1:1,1:1,1:1,1:1	# -16 thru -9
	.byte 1:1,1:1,1:1,1:0,1:0,1:0,1:0,1:1	# -8  thru -1
xlsbh:
	.byte 1:0,1:0,1:0,1:0,1:0,1:0,1:0,1:0	# 0 thru 7
	.byte 1:0,1:0,1:0,1:0,1:0,1:0,1:0,1:0	# 8 thru 15
	.byte 1:0,1:0,1:0,1:0,1:0,1:0,1:0,1:0	# 16 thru 23
	.byte 1:0,1:1,1:1,1:0,1:1,1:1,1:1,1:1	# 24 thru 31
	.byte 1:1,1:1,1:1,1:1,1:1,1:1,1:1    	# 32 thru 38

	# bytes of extra precision
	.byte           0x56,0x76,0xd3,0x88,0xb5,0x62	# -38 thru -33
	.byte 0xba,0xf5,0x32,0x3e,0x0e,0x48,0xdb,0x51	# -32 thru -25
	.byte 0x53,0x27,0xb1,0xef,0xeb,0xa5,0x07,0x49	# -24 thru -17
	.byte 0x5b,0xd9,0x0f,0x13,0xcd,0xff,0xbf,0x97	# -16 thru -9
	.byte 0xfd,0xbc,0xb6,0x23,0x2c,0x3b,0x0a,0xcd	# -8  thru -1
xprec:
	.byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00	# 0  thru 7
	.byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00	# 8  thru 15
	.byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00	# 16 thru 23
	.byte 0x00,0xa0,0xc8,0x3a,0x84,0xe4,0xdc,0x92	# 24 thru 31
	.byte 0x9b,0x00,0xc0,0x58,0xae,0x18,0xef     	# 32 thru 38
# endif
