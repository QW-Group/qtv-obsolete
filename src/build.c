
// build.c - generate build number

#include "qtv.h"


#if 0 

//returns SVN revision number
int Sys_Build_Number( void )
{
	static int b = 0;

	if (b)
		return b;

	{
		char rev_num[] = "$Revision: 59 $";

		if (!strnicmp(rev_num, "$Revision:", sizeof("$Revision:") - 1))
			b = atoi(rev_num + sizeof("$Revision:") - 1);
	}

	return b;
}

#else

static char *date = __DATE__ ;
static char *mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char mond[12] = { 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

// returns days since Oct 24 1996
int Sys_Build_Number( void )
{
	int m = 0;
	int d = 0;
	int y = 0;
	static int b = 0;

	if (b != 0)
		return b;

	for (m = 0; m < 11; m++)
	{
		if (strncmp( &date[0], mon[m], 3 ) == 0)
			break;
		d += mond[m];
	}

	d += atoi( &date[4] ) - 1;

	y = atoi( &date[7] ) - 1900;

	b = d + (int)((y - 1) * 365.25);

	if (((y % 4) == 0) && m > 1)
	{
		b += 1;
	}

	b -= 35778; // Dec 16 1998

	return b;
}
#endif
