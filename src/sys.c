/*
System dependant stuff, live hard.
Also contain some "misc" functions, have no idea where to put it, u r welcome to sort out it.
*/
 
#include "qtv.h"


#ifdef _WIN32

//FIXME: replace this shit with linux/FreeBSD code, so we will be equal on all OSes

int qsnprintf(char *buffer, size_t count, char const *format, ...)
{
	int ret;
	va_list argptr;
	if (!count)
		return 0;
	va_start(argptr, format);
	ret = _vsnprintf(buffer, count, format, argptr);
	buffer[count - 1] = 0;
	va_end(argptr);
	return ret;
}

int qvsnprintf(char *buffer, size_t count, const char *format, va_list argptr)
{
	int ret;
	if (!count)
		return 0;
	ret = _vsnprintf(buffer, count, format, argptr);
	buffer[count - 1] = 0;
	return ret;
}

#endif // _WIN32

//
// Finds the first occurance of a char in a string starting from the end.
// 
char *strchrrev(char *str, char chr)
{
	char *firstchar = str;
	for (str = str + strlen(str)-1; str >= firstchar; str--)
	{
		if (*str == chr)
			return str;
	}

	return NULL;
}

//
// Check if a string ends with a specified substring.
//
int strendswith(const char *s1, const char *s2)
{
	int len1 = strlen(s1);
	int len2 = strlen(s2);

	if (len2 > len1)
		return -1;

	return strcmp(s1 + (len1 - len2), s2);
}

#if defined(__linux__) || defined(_WIN32) || defined(__CYGWIN__)

/* 
 * Functions strlcpy, strlcat
 * was copied from FreeBSD 4.10 libc: src/lib/libc/string/
 *
 *  // VVD
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0)
	{
		do
		{
			if ((*d++ = *s++) == 0)
				break;
		}
		while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0)
	{
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

size_t strlcat(char *dst, char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));       /* count does not include NUL */
}

#endif // defined(__linux__) || defined(_WIN32) || defined(__CYGWIN__)

ullong Sys_Milliseconds(void)
{
	static time_t sys_timeBase = 0;

#ifdef _WIN32
	struct timeb tb;

	ftime(&tb);

	if (!sys_timeBase)
	{
		sys_timeBase = tb.time;
	}

	return (tb.time - sys_timeBase) * 1000 + tb.millitm;
#else // _WIN32
	//assume every other system follows standards.
	struct timeval tv;

	gettimeofday(&tv, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = tv.tv_sec;
	}

	return (tv.tv_sec - sys_timeBase) * 1000 + tv.tv_usec / 1000;
#endif // _WIN32 else
}

char	*redirect_buf		= NULL;
int		redirect_buf_size	= 0;

void Sys_RedirectStart(char *buf, int size)
{
	if (redirect_buf) {
		Sys_RedirectStop();
		Sys_Printf("BUG: second Sys_RedirectStart() without Sys_RedirectStop()\n");
	}
	
	if (size < 1 || !buf)
		return; // nigga plz

	redirect_buf		= buf;
	redirect_buf[0]		= 0;
	redirect_buf_size	= size;
}

void Sys_RedirectStop(void)
{
	if (!redirect_buf)
		Sys_Printf("BUG: Sys_RedirectStop() without Sys_RedirectStart()\n");

	redirect_buf		= NULL;
	redirect_buf_size	= 0;
}

void Sys_Printf(char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	unsigned char *t;
	
	va_start (argptr, fmt);
	vsnprintf (string, sizeof(string), fmt, argptr);
	va_end (argptr);

	// cluster may be NULL, if someday u start use it...
	// some Sys_Printf used as Sys_Printf("lalala\n"); so...

	if (redirect_buf && redirect_buf_size > 0)
		strlcat(redirect_buf, string, redirect_buf_size);

	// do this conversion after copy to redirection buffer, let quake clients use red letters and etc
	for (t = (unsigned char*)string; *t; t++)
	{
		if (*t >= 146 && *t < 156)
			*t = *t - 146 + '0';
		if (*t == 143)
			*t = '.';
		if (*t == 157 || *t == 158 || *t == 159)
			*t = '-';
		if (*t >= 128)
			*t -= 128;
		if (*t == 16)
			*t = '[';
		if (*t == 17)
			*t = ']';
		if (*t == 29)
			*t = '-';
		if (*t == 30)
			*t = '-';
		if (*t == 31)
			*t = '-';
		if (*t == '\a')	//doh. :D
			*t = ' ';
	}

	printf("%s", string);
}

// Used only for stuff that never gets redirected 
// (so that the server console is more pleasant to use).
void Sys_ConPrintf(sv_t *tv, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	
	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	if (tv && tv->EchoInServerConsole)
		Sys_Printf("%s: %s", tv->server, string);
}

// print debug
// this is just wrapper for Sys_Printf(), but print nothing if developer 0
void Sys_DPrintf(char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	
	if (!developer.integer)
		return;
	
	va_start (argptr, fmt);
	vsnprintf (string, sizeof(string), fmt, argptr);
	va_end (argptr);

	Sys_Printf("%s", string);
}


void Sys_Exit(int code)
{
	exit(code);
}

void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[2048];

	va_start (argptr, error);
	vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	strlcat(text, "\n", sizeof(text));
	Sys_Printf(text);

	Sys_Exit (1);
}


/*
===================
Sys_malloc

Use it instead of malloc so that if memory allocation fails,
the program exits with a message saying there's not enough memory
instead of crashing after trying to use a NULL pointer.
It also sets memory to zero.
===================
*/

#ifndef _CRTDBG_MAP_ALLOC

void *Sys_malloc (size_t size)
{
	void *p = malloc(size);

	if (!p)
		Sys_Error ("Sys_malloc: Not enough memory free");
	memset(p, 0, size);

	return p;
}

#endif // _CRTDBG_MAP_ALLOC

char *Sys_strdup (const char *src)
{
	char *p = strdup(src);

	if (!p)
		Sys_Error ("Sys_strdup: Not enough memory free");

	return p;
}

// qqshka - hmm, seems in C this is macroses, i don't like macroses,
// however this functions work wrong on unsigned types!!!

#ifdef KTX_MIN

double min( double a, double b )
{
	return ( a < b ? a : b );
}

#endif

#ifdef KTX_MAX

double max( double a, double b )
{
	return ( a > b ? a : b );
}

#endif

double bound( double a, double b, double c )
{
	return ( a >= c ? a : b < a ? a : b > c ? c : b);
}

// handle keyboard input
void Sys_ReadSTDIN(cluster_t *cluster, fd_set socketset)
{
#ifdef _WIN32

	for (;;)
	{
		char c;

		if (!_kbhit())
			break;
		c = _getch();

		if (c == '\n' || c == '\r')
		{
			Sys_Printf("\n");
			if (cluster->inputlength)
			{
				cluster->commandinput[cluster->inputlength] = '\0';
				Cbuf_InsertText(cluster->commandinput);

				cluster->inputlength = 0;
				cluster->commandinput[0] = '\0';
			}
		}
		else if (c == '\b')
		{
			if (cluster->inputlength > 0)
			{
				Sys_Printf("%c", c);
				Sys_Printf(" ", c);
				Sys_Printf("%c", c);

				cluster->inputlength--;
				cluster->commandinput[cluster->inputlength] = '\0';
			}
		}
		else
		{
			Sys_Printf("%c", c);
			if (cluster->inputlength < sizeof(cluster->commandinput)-1)
			{
				cluster->commandinput[cluster->inputlength++] = c;
				cluster->commandinput[cluster->inputlength] = '\0';
			}
		}
	}

#else

	if (FD_ISSET(STDIN, &socketset))
	{
		cluster->inputlength = read (STDIN, cluster->commandinput, sizeof(cluster->commandinput));
		if (cluster->inputlength >= 1)
		{
			cluster->commandinput[cluster->inputlength-1] = 0;        // rip off the /n and terminate
			cluster->inputlength--;

			if (cluster->inputlength)
			{
				cluster->commandinput[cluster->inputlength] = '\0';
				Cbuf_InsertText(cluster->commandinput);

				cluster->inputlength = 0;
				cluster->commandinput[0] = '\0';
			}
		}
	}

#endif
}

/*==================
Sys_ReplaceChar
Replace char in string
==================*/
void Sys_ReplaceChar(char *s, char from, char to)
{
	if (s)
		for ( ;*s ; ++s)
			if (*s == from)
				*s = to;
}

//=====================
// our hash function
//=====================
unsigned long Sys_HashKey (const char *str)
{
	unsigned long hash = 0;
	int c;

	// the (c&~32) makes it case-insensitive
	// hash function known as sdbm, used in gawk
	while ((c = *str++))
        hash = (c &~ 32) + (hash << 6) + (hash << 16) - hash;

    return hash;
}

//=====================
// just convert uptime seconds in days hours and minutes
//=====================
void Get_Uptime(ullong uptime_seconds, unsigned int *days, unsigned int *h, unsigned int *m)
{
	*days = uptime_seconds / (24 * 60 * 60); // whole days
	uptime_seconds %= 24 * 60 * 60;
	*h    = uptime_seconds / (60 * 60); // hours 
	uptime_seconds %= 60 * 60;
	*m    = uptime_seconds / 60; // minutes
}

