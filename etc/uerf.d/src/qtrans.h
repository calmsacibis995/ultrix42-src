/*  sccsid  =  @(#)qtrans.h	4.2   (ULTRIX)   9/4/90 */

struct qt_entry {
	short qcond1;	/* qualifying condition */
	short qcond2;	/* qualifying condition */
	short qcond3;	/* qualifying condition */
	char field_leftbit;
	char field_rightbit;
	unsigned long test_value;
	char *translate_string;
};

#define MAX_SIPS 20
#define MAX_CONDS 60
int sip[MAX_SIPS+1];
unsigned short cnd[MAX_CONDS+1];	/* use cnd[x] for condition x */

