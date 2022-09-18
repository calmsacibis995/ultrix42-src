#ifndef lint
static  char    *sccsid = "@(#)cal.c	4.2  (ULTRIX)        8/8/90";
#endif

/***************************************************************************
*
* MODIFICATION HISTORY
*
* 001	Bob Fontaine Wed Aug  8 14:09:18 EDT 1990
*		Fixed cal to conform to XPG3 in that it now prints the 
*		current month as the default or prints the year, or month
*		of the year specified.  Also fixed error messages.
*
**************************************************************************/

#include <sys/types.h>
#include <time.h>
#include <stdio.h>

char	dayw[] = {
	" S  M Tu  W Th  F  S"
};
char	*smon[]= {
	"January", "February", "March", "April",
	"May", "June", "July", "August",
	"September", "October", "November", "December",
};
char	string[432];
main(argc, argv)
char *argv[];
{
	struct tm *dt;
	time_t t;
	register y, i, j;
	int m;

	switch(argc)
	{
		case 1:

			t = time(0);
			dt = localtime(&t);
			m = dt->tm_mon + 1;
			y = dt->tm_year + 1900;
			break;

		case 2:
			y = number(argv[1]);
			if(y<1 || y>9999)
				badarg(2);
			print_year(y);
			break;

		case 3:
			m = number(argv[1]);
			y = number(argv[2]);
			break;

		default:
                        fprintf(stderr,"usage: cal [month] year\n");
                        exit(3);

	}
	
/*
 *	print out just month
 */

	if(m<1 || m>12)
		 badarg(1);
	if(y<1 || y>9999)
		badarg(2);
	printf("   %s %u\n", smon[m-1], y);
	printf("%s\n", dayw);
	cal(m, y, string, 24);
	for(i=0; i<6*24; i+=24)
		pstr(string+i, 24);
	exit(0);

}
/*
 *	print out complete year
 */

/***********************************************************************/
print_year(year)
int year;
{

	register y,i,j;

	y = year;
	if(y<1 || y>9999)
		badarg(2);
	printf("\n\n\n");
	printf("				%u\n", y);
	printf("\n");
	for(i=0; i<12; i+=3) {
		for(j=0; j<6*72; j++)
			string[j] = '\0';
		printf("	 %.3s", smon[i]);
		printf("			%.3s", smon[i+1]);
		printf("		       %.3s\n", smon[i+2]);
		printf("%s   %s   %s\n", dayw, dayw, dayw);
		cal(i+1, y, string, 72);
		cal(i+2, y, string+23, 72);
		cal(i+3, y, string+46, 72);
		for(j=0; j<6*72; j+=72)
			pstr(string+j, 72);
	}
	printf("\n\n\n");
	exit(0);

}
/**********************************************************************/
badarg(err)
int err;
{
	if(err == 1)
		fprintf(stderr,"cal: Bad month specified\n");
	if(err == 2)
		fprintf(stderr,"cal: Bad year specified\n");
	exit(-1);
}


number(str)
char *str;
{
	register n, c;
	register char *s;

	n = 0;
	s = str;
	while(c = *s++) {
		if(c<'0' || c>'9')
			return(0);
		n = n*10 + c-'0';
	}
	return(n);
}

pstr(str, n)
char *str;
{
	register i;
	register char *s;

	s = str;
	i = n;
	while(i--)
		if(*s++ == '\0')
			s[-1] = ' ';
	i = n+1;
	while(i--)
		if(*--s != ' ')
			break;
	s[1] = '\0';
	printf("%s\n", str);
}

char	mon[] = {
	0,
	31, 29, 31, 30,
	31, 30, 31, 31,
	30, 31, 30, 31,
};

cal(m, y, p, w)
char *p;
{
	register d, i;
	register char *s;

	s = p;
	d = jan1(y);
	mon[2] = 29;
	mon[9] = 30;

	switch((jan1(y+1)+7-d)%7) {

	/*
	 *	non-leap year
	 */
	case 1:
		mon[2] = 28;
		break;

	/*
	 *	1752
	 */
	default:
		mon[9] = 19;
		break;

	/*
	 *	leap year
	 */
	case 2:
		;
	}
	for(i=1; i<m; i++)
		d += mon[i];
	d %= 7;
	s += 3*d;
	for(i=1; i<=mon[m]; i++) {
		if(i==3 && mon[m]==19) {
			i += 11;
			mon[m] += 11;
		}
		if(i > 9)
			*s = i/10+'0';
		s++;
		*s++ = i%10+'0';
		s++;
		if(++d == 7) {
			d = 0;
			s = p+w;
			p = s;
		}
	}
}

/*
 *	return day of the week
 *	of jan 1 of given year
 */

jan1(yr)
{
	register y, d;

/*
 *	normal gregorian calendar
 *	one extra day per four years
 */

	y = yr;
	d = 4+y+(y+3)/4;

/*
 *	julian calendar
 *	regular gregorian
 *	less three days per 400
 */

	if(y > 1800) {
		d -= (y-1701)/100;
		d += (y-1601)/400;
	}

/*
 *	great calendar changeover instant
 */

	if(y > 1752)
		d += 3;

	return(d%7);
}
