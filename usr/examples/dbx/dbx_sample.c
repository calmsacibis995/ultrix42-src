/* This program is a crude editor that can make very simple changes 
 * to lines of text.
 */

#include <stdio.h>
#include <signal.h>
#define MAX 80

void getline();
void stredit();
void handler();
extern char *getenv();

int choice = 0;

main(argc,argv)
int argc;
char **argv;
{
	char str[MAX];
	char *tmp;
	char newline;
	int i;

	/* 
	 * Declare a signal handler for ^C
	 */
	signal(SIGINT,handler);
	/*
	 * A text string argument may be entered:
	 * 1. as a command line argument
	 * 2. as the value of an environment variable
	 * 3. interactively
	 *
	 * Once a command line argument or environment string has
	 * been processed, the user is prompted for additional text.
	 * If both a command line argument and an environment string
	 * are given, only the command line argument is processed.
	 */
	str[0]='\0';
	if(argc > 1)
		strncpy(str,*++argv,MAX);
	else if ((tmp = getenv("TEXT")) != 0)
		strncpy(str,tmp,MAX);

	if(str[0]=='\0'){
		printf("\n\nEnter a text line: ");
		getline(str);
	}
	i = strlen(str);
	printf("\n");
	printf("Choose an editing change:\n\n");
	printf("    1  UPPERCASE\n");
	printf("    2  lowercase\n");
	printf("    3  Initial Capital On All Words\n");
	printf("    4  No blanks\n");
	printf("    5  Exit\n\n");
	printf("Enter your choice: ");
	scanf("%d%c", &choice,&newline);
	stredit(str);
	printf("\n%s\n", str);
}

void getline(st)
char *st;
{
	 int i;

	 for(i=0; i<MAX ; i++)
	 {
	     st[i]=getchar();
	     if (st[i]=='\n')
	         break;
	 }
	 st[++i]='\0';
}

void stredit(source)
char source[];
{
	register char *start;


	if(*source == '\0')
		return;
	switch(choice) {
	/* Convert to upper case */
	case 1:	
		while(*source != '\0'){
			if(!isspace(*source))
				*source = toupper(*source);
			source++;
		}
		break;
	/* Convert to lower case */
	case 2:	
		while(*source != '\0'){
			if(!isspace(*source))
				*source = tolower(*source);
			source++;
		}
		break;
	/* Capitalize first letter of each word */
	case 3:	
		if(!isspace(*source))
			*source = toupper(*source);
		source++;
		while(*source != '\0'){
			if(isspace(*(source-1)) && !isspace(*source))
				*source = toupper(*source);
			source++;
		}
		break;
	/* Remove all blanks */
	case 4:
		start=source;
		while(*source != '\0'){
			while(*source && isspace(*source))
				source++;
			while(*source && !isspace(*source))
				*start++ = *source++;
		}
		*start = *source;
		break;

	case 5:
		exit(0);
	default:
		strcpy(source,"Invalid edit choice.\n");
		break;
	}
}
/*
 * Signal handler for ^C
 */
void
handler(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
	fprintf(stderr,"\n\n^C disabled -  Re-enter input:\n");
}
