#ifndef lint
static	char	*sccsid = "@(#)crypt.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983, 1989 by			*
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
/*
 * Modification history:
 *
 * 9 Oct 89	D. Long
 *	The function setkey() must always be defined.  Return error ENOSYS
 *	if encryption not available.
 *
 * 14 Jun 89	D. Long
 *	Fixed erroneous call to setkey.  Do it inline if ndef CRYPT.
 *
 * 07 Jun 89	D. Long
 *	Added crypt16().
 */
/*
 * This file is derived from
 * @(#)crypt.c	4.1 (Berkeley) 12/21/80
 */
#include <errno.h>
#ifndef CRYPT
/*
 * This program implements a one way data encryption
 * algorithm to encrypt passwords.
 */
#endif CRYPT

static	char	IP[] = {
	58,50,42,34,26,18,10, 2,
	60,52,44,36,28,20,12, 4,
	62,54,46,38,30,22,14, 6,
	64,56,48,40,32,24,16, 8,
	57,49,41,33,25,17, 9, 1,
	59,51,43,35,27,19,11, 3,
	61,53,45,37,29,21,13, 5,
	63,55,47,39,31,23,15, 7,
};

static	char	FP[] = {
	40, 8,48,16,56,24,64,32,
	39, 7,47,15,55,23,63,31,
	38, 6,46,14,54,22,62,30,
	37, 5,45,13,53,21,61,29,
	36, 4,44,12,52,20,60,28,
	35, 3,43,11,51,19,59,27,
	34, 2,42,10,50,18,58,26,
	33, 1,41, 9,49,17,57,25,
};

static	char	PC1_C[] = {
	57,49,41,33,25,17, 9,
	 1,58,50,42,34,26,18,
	10, 2,59,51,43,35,27,
	19,11, 3,60,52,44,36,
};

static	char	PC1_D[] = {
	63,55,47,39,31,23,15,
	 7,62,54,46,38,30,22,
	14, 6,61,53,45,37,29,
	21,13, 5,28,20,12, 4,
};

static	char	shifts[] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,
};

static	char	PC2_C[] = {
	14,17,11,24, 1, 5,
	 3,28,15, 6,21,10,
	23,19,12, 4,26, 8,
	16, 7,27,20,13, 2,
};

static	char	PC2_D[] = {
	41,52,31,37,47,55,
	30,40,51,45,33,48,
	44,49,39,56,34,53,
	46,42,50,36,29,32,
};

static	char	C[28];
static	char	D[28];
static	char	KS[16][48];

static	char	E[48];
static	char	e[] = {
	32, 1, 2, 3, 4, 5,
	 4, 5, 6, 7, 8, 9,
	 8, 9,10,11,12,13,
	12,13,14,15,16,17,
	16,17,18,19,20,21,
	20,21,22,23,24,25,
	24,25,26,27,28,29,
	28,29,30,31,32, 1,
};

#ifndef CRYPT
setkey(key)
char *key;
{
	extern int errno;

	errno = ENOSYS;
	return;
}
#endif	CRYPT

static	char	S[8][64] = {
	14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
	 0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
	 4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
	15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13,

	15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
	 3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
	 0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
	13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9,

	10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
	13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
	13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
	 1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12,

	 7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
	13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
	10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
	 3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14,

	 2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
	14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
	 4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
	11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3,

	12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
	10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
	 9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
	 4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13,

	 4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
	13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
	 1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
	 6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12,

	13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
	 1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
	 7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
	 2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11,
};

static	char	P[] = {
	16, 7,20,21,
	29,12,28,17,
	 1,15,23,26,
	 5,18,31,10,
	 2, 8,24,14,
	32,27, 3, 9,
	19,13,30, 6,
	22,11, 4,25,
};

static	char	L[32], R[32];
static	char	tempL[32];
static	char	f[32];

static	char	preS[48];


#if	defined(CRYPT)||defined(_POSIX_SOURCE)
encrypt(block, edflag)
int edflag;
#else
encrypt(block)
#endif
char *block;
{
#ifndef CRYPT
	int i;
#endif CRYPT
	register t, j, k;

#ifndef	CRYPT
#ifdef	_POSIX_SOURCE
	if(edflag) {
		errno = ENOSYS;
		return;
	}
#endif	_POSIX_SOURCE
#endif	CRYPT
	for (j=0; j<64; j++)
		L[j] = block[IP[j]-1];
#ifndef CRYPT
	for (i=0; i<16; i++) {
#endif CRYPT
		for (j=0; j<32; j++)
			tempL[j] = R[j];
		for (j=0; j<48; j++)
			preS[j] = R[E[j]-1] ^ KS[i][j];
		for (j=0; j<8; j++) {
			t = 6*j;
			k = S[j][(preS[t+0]<<5)+
				(preS[t+1]<<3)+
				(preS[t+2]<<2)+
				(preS[t+3]<<1)+
				(preS[t+4]<<0)+
				(preS[t+5]<<4)];
			t = 4*j;
			f[t+0] = (k>>3)&01;
			f[t+1] = (k>>2)&01;
			f[t+2] = (k>>1)&01;
			f[t+3] = (k>>0)&01;
		}
		for (j=0; j<32; j++)
			R[j] = L[j] ^ f[P[j]-1];
		for (j=0; j<32; j++)
			L[j] = tempL[j];
	}
	for (j=0; j<32; j++) {
		t = L[j];
		L[j] = R[j];
		R[j] = t;
	}
	for (j=0; j<64; j++)
		block[j] = L[FP[j]-1];
}

static expand(in, out)
register char *in, *out;
{
	register int j, c;
	int i;

	for(i=0; (c = *in) && i<64; in++){
		for(j=6; j >= 0; j--)
			*out++ = (c>>j) & 01;
		*out++ = 0;
		i += 8;
	}
	for(j=i; j < 64; j++)
		*out++ = 0;
}

char *
crypt(pw,salt)
char *pw;
char *salt;
{
#ifndef CRYPT
	register i, j, c, k;
	int	t, temp;
#endif CRYPT
	static char block[66], iobuf[16];

	expand(pw, block);

#ifndef CRYPT
	for (i=0; i<28; i++) {
		C[i] = block[PC1_C[i]-1];
		D[i] = block[PC1_D[i]-1];
	}
	for (i=0; i<16; i++) {
		for (k=0; k<shifts[i]; k++) {
			t = C[0];
			for (j=0; j<28-1; j++)
				C[j] = C[j+1];
			C[27] = t;
			t = D[0];
			for (j=0; j<28-1; j++)
				D[j] = D[j+1];
			D[27] = t;
		}
		for (j=0; j<24; j++) {
			KS[i][j] = C[PC2_C[j]-1];
			KS[i][j+24] = D[PC2_D[j]-28-1];
		}
	}
#endif CRYPT

	for(i=0; i<66; i++)
		block[i] = 0;

	for(i=0;i<48;i++)
		E[i] = e[i];

	for(i=0;i<2;i++){
		c = *salt++;
		iobuf[i] = c;
		if(c>'Z') c -= 6;
		if(c>'9') c -= 7;
		c -= '.';
		for(j=0;j<6;j++){
			if((c>>j) & 01){
				temp = E[6*i+j];
				E[6*i+j] = E[6*i+j+24];
				E[6*i+j+24] = temp;
				}
			}
		}
	
	for(i=0; i<25; i++)
#if	defined(CRYPT)||defined(_POSIX_SOURCE)
		encrypt(block,0);
#else
		encrypt(block);
#endif
	
	for(i=0; i<11; i++){
		c = 0;
		for(j=0; j<6; j++){
			c <<= 1;
			c |= block[6*i+j];
			}
		c += '.';
		if(c>'9') c += 7;
		if(c>'Z') c += 6;
		iobuf[i+2] = c;
	}
	iobuf[i+2] = 0;
	if(iobuf[1]==0)
		iobuf[1] = iobuf[0];
	return(iobuf);
}

char *
crypt16(pw,salt)
char *pw;
char *salt;
{
	register i, j, c, k;
	int	t, temp;
	static char block[66], iobuf[32];

	expand(pw, block);

#ifndef CRYPT
	for (i=0; i<28; i++) {
		C[i] = block[PC1_C[i]-1];
		D[i] = block[PC1_D[i]-1];
	}
	for (i=0; i<16; i++) {
		for (k=0; k<shifts[i]; k++) {
			t = C[0];
			for (j=0; j<28-1; j++)
				C[j] = C[j+1];
			C[27] = t;
			t = D[0];
			for (j=0; j<28-1; j++)
				D[j] = D[j+1];
			D[27] = t;
		}
		for (j=0; j<24; j++) {
			KS[i][j] = C[PC2_C[j]-1];
			KS[i][j+24] = D[PC2_D[j]-28-1];
		}
	}
#endif CRYPT

	for(i=0; i<66; i++)
		block[i] = 0;

	for(i=0;i<48;i++)
		E[i] = e[i];

	for(i=0;i<2;i++){
		c = *salt++;
		iobuf[i] = c;
		if(c>'Z') c -= 6;
		if(c>'9') c -= 7;
		c -= '.';
		for(j=0;j<6;j++){
			if((c>>j) & 01){
				temp = E[6*i+j];
				E[6*i+j] = E[6*i+j+24];
				E[6*i+j+24] = temp;
				}
			}
		}
	
	for(i=0; i<20; i++)
#if	defined(CRYPT)||defined(_POSIX_SOURCE)
		encrypt(block,0);
#else
		encrypt(block);
#endif
	
	for(i=0; i<11; i++){
		c = 0;
		for(j=0; j<6; j++){
			c <<= 1;
			c |= block[6*i+j];
			}
		c += '.';
		if(c>'9') c += 7;
		if(c>'Z') c += 6;
		iobuf[i+2] = c;
	}
	for(i=0; i < 16 && pw[i]; i++) ;
	if(i > 8)
		pw += 8;
	else
		pw += i;
	expand(pw, block);
	for (i=0; i<28; i++) {
		C[i] = block[PC1_C[i]-1];
		D[i] = block[PC1_D[i]-1];
	}
	for (i=0; i<16; i++) {
		for (k=0; k<shifts[i]; k++) {
			t = C[0];
			for (j=0; j<28-1; j++)
				C[j] = C[j+1];
			C[27] = t;
			t = D[0];
			for (j=0; j<28-1; j++)
				D[j] = D[j+1];
			D[27] = t;
		}
		for (j=0; j<24; j++) {
			KS[i][j] = C[PC2_C[j]-1];
			KS[i][j+24] = D[PC2_D[j]-28-1];
		}
	}
	for(i=0; i <66; i++)
		block[i] = 0;
	for(i=0; i <5; i++)
#if	defined(CRYPT)||defined(_POSIX_SOURCE)
		encrypt(block,0);
#else
		encrypt(block);
#endif
	for(i=0; i<11; i++){
		c = 0;
		for(j=0; j<6; j++){
			c <<= 1;
			c |= block[6*i+j];
			}
		c += '.';
		if(c>'9') c += 7;
		if(c>'Z') c += 6;
		iobuf[i+13] = c;
	}
	iobuf[24] = 0;
	if(iobuf[1]==0)
		iobuf[1] = iobuf[0];
	return(iobuf);
}
