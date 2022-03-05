/*
	httpsv.c
*/

#include "qtv.h"
#define CRLF "\r\n"

//main reason to use connection close is because we're lazy and don't want to give sizes in advance (yes, we could use chunks..)

static const char qfont_table[256] = {
	  0, '#', '#', '#', '#', '.', '#', '#',
	'#',   9, 10,  '#', ' ',  13, '.', '.',
	'[', ']', '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', '.', '<', '=', '>',
	' ', '!', '"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', '<',
 
	'<', '=', '>', '#', '#', '.', '#', '#',
	'#', '#', ' ', '#', ' ', '>', '.', '.',
	'[', ']', '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', '.', '<', '=', '>',
	' ', '!', '"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', '<'
};

char *HTMLprintf(char *outb, int outl, qbool qfont, const char *fmt, ...)
{
	va_list val;
	char qfmt[8192*4];
	char *inb = qfmt;

	va_start(val, fmt);
	vsnprintf(qfmt, sizeof(qfmt), fmt, val);
	va_end(val);

	outl--;
	outl -= 5;
	while (outl > 0 && *inb)
	{
		if (qfont)
			*inb = qfont_table[*(unsigned char*)inb];

		if (*inb == '<')
		{
			*outb++ = '&';
			*outb++ = 'l';
			*outb++ = 't';
			*outb++ = ';';
			outl -= 4;
		}
		else if (*inb == '>')
		{
			*outb++ = '&';
			*outb++ = 'g';
			*outb++ = 't';
			*outb++ = ';';
			outl -= 4;
		}
		else if (*inb == '&')
		{
			*outb++ = '&';
			*outb++ = 'a';
			*outb++ = 'm';
			*outb++ = 'p';
			*outb++ = ';';
			outl -= 5;
		}
		else if (*inb == '"')
		{
			*outb++ = '&';
			*outb++ = 'q';
			*outb++ = 'u';
			*outb++ = 'o';
			*outb++ = 't';
			*outb++ = ';';
			outl -= 6;
		}
		else
		{
			*outb++ = *inb;
			outl -= 1;
		}
		inb++;
	}
	*outb++ = 0;

	return outb;
}

int HTTPSV_UnescapeURL(const char *url, char *out, size_t outsize)
{
	const char *s = url;
	int i = 0;
	int c;

	while (*s && (*s != ' ') && (*s != '\n') && (*s != '\r') && (i < (outsize - 1)))
	{
		if (s[0] == '%')
		{
			s++;
			if (!s[0] || !s[1] || (sscanf(s, "%2x", &c) == EOF))
			{
				*out = 0;
				return -1;
			}

			*out++ = (char)c;
			s += 2;
		}
		else
		{
			*out++ = *s++;
		}

		i++;
	}
	
	*out = 0;

	return 0;
}

// escapes certain characters with %hex codes, returns length of string in out_buf
size_t HTTPSV_EscapeURL(const char *url, char *out, size_t outsize)
{
	char *out_buf = out;
	const char *s = url;
	const char *escaped_chars = " <>#%{}|\\^~[]`;/?:@=&$'";
	const char *hex = "0123456789ABCDEF";
	size_t wrote_chars = 0;

	while (*s && wrote_chars < outsize)
	{
		if (strchr(escaped_chars, *s))
		{
			if (wrote_chars + 3 < outsize)
			{
				int character = *s++;
				*out++ = '%';
				*out++ = hex[character / 16];
				*out++ = hex[character % 16];
				wrote_chars += 3;
			}
			else
			{
				break;
			}
		}
		else
		{
			*out++ = *s++;
			wrote_chars++;
		}
	}

	if (wrote_chars < outsize)
	{
		*out = '\0';
		return wrote_chars;
	}
	else
	{
		out_buf[outsize-1] = '\0';
		return outsize-1;
	}
}

void HTTPSV_SendHTTPHeader(cluster_t *cluster, oproxy_t *dest, char *error_code, char *content_type, qbool nocache)
{
	char *s;
	char buffer[2048];
	int ierr = atoi(error_code);
	const char *httpreply = "OK";

	switch (ierr) {
	case 200: httpreply = "OK"; break;
	case 400: httpreply = "Bad Request"; break;
	case 403: httpreply = "Forbidden"; break;
	case 404: httpreply = "Not Found"; break;
	}

	if (nocache)
	{
		s =	"HTTP/1.1 %s %s" CRLF
			"Content-Type: %s" CRLF
			"Cache-Control: no-cache, must-revalidate" CRLF
			"Expires: Mon, 26 Jul 1997 05:00:00 GMT" CRLF
			"Connection: close" CRLF
			CRLF;
	}
	else
	{
		s =	"HTTP/1.1 %s %s" CRLF
			"Content-Type: %s" CRLF
			"Connection: close" CRLF
			CRLF;
	}

	snprintf(buffer, sizeof(buffer), s, error_code, httpreply, content_type);

	Net_ProxySend(cluster, dest, buffer, strlen(buffer));
}

void HTTPSV_SendHTMLHeader(cluster_t *cluster, oproxy_t *dest, char *title)
{
	char *s;
	char buffer[2048];

	s =	
    "<?xml version=\"1.0\"?>\n"
    "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>\n\n"
		"<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\n"
		"  <head>\n"
		"    <meta http-equiv=\"content-type\" content=\"text/html; charset=iso-8859-1\" />\n"
		"    <title>%s</title>\n"
		"    <link rel=\"StyleSheet\" href=\"/style.css\" type=\"text/css\" />\n"
		"    <link rel=\"alternate\" title=\"RSS\" href=\"/rss\" type=\"application/rss+xml\" />\n"
		"    <script src=\"/script.js\" type=\"text/javascript\"></script>\n"
		"  </head>\n\n"
		"  <body>\n\n"
		"    <div id=\"navigation\"><span><a href=\"/nowplaying/\">Live</a></span><span><a href=\"/demos/\">Demos</a></span><span><a href=\"/admin/\">Admin</a></span><span><a href=\"" QTV_HELP_URL "\" target=\"_blank\">Help</a></span></div>\n\n";

	snprintf(buffer, sizeof(buffer), s, title);

	Net_ProxySend(cluster, dest, buffer, strlen(buffer));
}

void HTTPSV_SendHTMLFooter(cluster_t *cluster, oproxy_t *dest)
{
	char *s;
	char buffer[2048];

	snprintf(buffer, sizeof(buffer), "\n    <p id='version'><strong><a href='" QTV_PROJECT_URL "'>QTV</a> %s, build %i</strong></p>\n", PROXY_VERSION, cluster->buildnumber);
	Net_ProxySend(cluster, dest, buffer, strlen(buffer));

	s = 
		"\n  </body>\n\n"
		"</html>\n";
	Net_ProxySend(cluster, dest, s, strlen(s));
}

qbool HTTPSV_GetHeaderField(const char *s, const char *field, char *buffer, int buffersize)
{
	char *start = NULL;
	char *end = NULL;
	char *copy = NULL;
	char *colon = NULL;
	qbool status = false;
	int fieldnamelen = strlen(field);
	#define EAT_WHITESPACE(str) while (*str && (*str == ' ' || *str == '\t')) str++;

	buffer[0] = 0;

	copy = Sys_strdup(s);
	start = end = copy;

	while (*end)
	{
		if (*end == '\n')
		{
			*end = '\0';
			colon = strchr(start, ':');
			
			if (!colon)
			{
				// Header exists, but has no value. (Not sure if this is proper according to RFC).
				if (!strncmp(field, start, fieldnamelen))
				{
					if (start[fieldnamelen] <= ' ')
					{
						status = true;
						break;
					}
				}
			}
			else
			{
				if (fieldnamelen == colon - start)
				{
					if (!strncmp(field, start, colon - start))
					{
						colon++;
						
						EAT_WHITESPACE(colon);

						while (--buffersize > 0)
						{
							if (*colon == '\r' || *colon == '\n')
								break;
							*buffer++ = *colon++;
						}
						*buffer = 0;
						
						status = true;
						break;
					}
				}
			}
			start = end + 1;
		}

		end++;
	}

	Sys_free(copy);

	return status;
}

char *HTTPSV_ParsePOST(char *post, char *buffer, int buffersize)
{
	while(*post && *post != '&')
	{
		if (--buffersize>0)
		{
			if (*post == '+')
				*buffer++ = ' ';
			else if  (*post == '%')
			{
				*buffer = 0;
				post++;
				if (*post == '\0' || *post == '&')
					break;
				else if (*post >= 'a' && *post <= 'f')
					*buffer += 10 + *post-'a';
				else if (*post >= 'A' && *post <= 'F')
					*buffer += 10 + *post-'A';
				else if (*post >= '0' && *post <= '9')
					*buffer += *post-'0';

				*buffer <<= 4;

				post++;
				if (*post == '\0' || *post == '&')
					break;
				else if (*post >= 'a' && *post <= 'f')
					*buffer += 10 + *post-'a';
				else if (*post >= 'A' && *post <= 'F')
					*buffer += 10 + *post-'A';
				else if (*post >= '0' && *post <= '9')
					*buffer += *post-'0';

				buffer++;
			}
			else
				*buffer++ = *post;
		}
		post++;
	}
	*buffer = 0;

	return post;
}

void HTTPSV_PostMethod(cluster_t *cluster, oproxy_t *pend)
{
	char tempbuf[512];
	char *s;
	char *postpath = (char *)pend->inbuffer + sizeof("POST ") - 1;
	char *postdata = NULL;
	int len;

	// Get the post data.
	{
		for (s = (char *)pend->inbuffer; *s; s++)
		{
			if (s[0] == '\n' && (s[1] == '\n' || (s[1] == '\r' && s[2] == '\n')))
				break;
		}

		if (s[0] == '\n' && s[1] == '\n')
		{
			s += 2;
		}
		else if (s[0] == '\n' && s[1] == '\r' && s[2] == '\n')
		{
			s += 3;
		}

		postdata = s;
	}

	if (!HTTPSV_GetHeaderField((char *)pend->inbuffer, "Content-Length", tempbuf, sizeof(tempbuf)))
	{
		s = "HTTP/1.1 411 OK" CRLF
			"Content-Type: text/html" CRLF
			"Connection: close" CRLF
			CRLF
			"<html>\n\n  <head>\n    <title>QuakeTV</title>\n  </head>\n\n  <body>\n    <p>No Content-Length was provided.</p>\n  </body>\n\n</html>\n";
		Net_ProxySend(cluster, pend, s, strlen(s));
		pend->flushing = true;
		return;
	}

	len = max(0, atoi(tempbuf));
	if (pend->inbuffersize + len >= sizeof(pend->inbuffer) - 20)
	{	
		// Too much data.
		pend->flushing = true;
		return;
	}

	len = postdata - (char *)pend->inbuffer + len;

	if (len > pend->inbuffersize)
		return;	// Still need the body.

	if (!strncmp(postpath, "/admin", 6))
	{
		HTTPSV_GenerateAdmin(cluster, pend, 0, postdata);
	}
	else
	{
		s = "HTTP/1.1 404 OK" CRLF
			"Content-Type: text/html" CRLF
			"Connection: close" CRLF
			CRLF
			"<html>\n\n  <head>\n    <title>QuakeTV</title>\n  </head>\n\n  <body>\n    <p>That HTTP method is not supported for that URL.</p>\n  </body>\n\n</html>\n";
		Net_ProxySend(cluster, pend, s, strlen(s));
	}

	pend->flushing = true;
	return;
}

int HTTPSV_GetURLFromRequest(char *dst, const char *src, size_t size)
{
	size_t i = 0;

	if (!strncmp(src, "GET", 3))
	{
		src += 3;
	}
	else if (!strncmp(src, "POST", 4))
	{
		src += 4;
	}
	else
	{
		return -1;
	}

	while (src[0] && (src[0] == ' '))
	{
		src++;
	}

	while (src[0] && (src[0] != '\t') && (src[0] != '\n') && (src[0] != '\r') && (src[0] != ' '))
	{
		if (i >= (size - 1))
		{
			*dst = 0;
			return -1;
		}

		*dst++ = *src++;
		i++;
	}

	*dst = 0;

	return 0;
}

void HTTPSV_GetMethod(cluster_t *cluster, oproxy_t *pend)
{
	#define URLCOMPARE(url, str, skip) (!strncmp(url, str, (skip = sizeof(str) - 1)))
	char *s;
	char *getpath;
	char geturl[1024];
	int skiplen = 0;

	HTTPSV_GetURLFromRequest(geturl, (char *)pend->inbuffer, sizeof(geturl));
	
	getpath = geturl;

	// RFC 2616 requires us to be able to parse an absolute URI also.
	if (URLCOMPARE(getpath, "http://", skiplen))
	{
		getpath += skiplen;
		while (*getpath && (*getpath != '\t') && (*getpath != '\r') && (*getpath != '\n') && (*getpath != ' ') && (*getpath != '/'))
		{
			getpath++;
		}
	}

	if (URLCOMPARE(getpath, "/nowplaying", skiplen))
	{
		HTTPSV_GenerateNowPlaying(cluster, pend);
	}
	else if (URLCOMPARE(getpath, "/watch.qtv?sid=", skiplen))
	{
		HTTPSV_GenerateQTVStub(cluster, pend, "", getpath + skiplen);
	}
	else if (URLCOMPARE(getpath, "/watch.qtv?demo=", skiplen))
	{
		HTTPSV_GenerateQTVStub(cluster, pend, "file:", getpath + skiplen);
	}
	else if (URLCOMPARE(getpath, "/join.qtv?sid=", skiplen))
	{
		HTTPSV_GenerateQTVJoinStub(cluster, pend, getpath + skiplen);
	}
	else if (URLCOMPARE(getpath, "/admin", skiplen))
	{
		HTTPSV_GenerateAdmin(cluster, pend, 0, NULL);
	}
	else if (URLCOMPARE(getpath, "/demos", skiplen))
	{
		HTTPSV_GenerateDemoListing(cluster, pend);
	}
	else if (!strcmp(getpath, "/style.css"))
	{
		HTTPSV_GenerateCSSFile(cluster, pend);
	}
	else if (!strcmp(getpath, "/script.js"))
	{
		HTTPSV_GenerateJSFile(cluster, pend);
	}
	else if (URLCOMPARE(getpath, "/levelshots/", skiplen))
	{
		HTTPSV_GenerateLevelshot(cluster, pend, getpath + skiplen);
	}
	else if (URLCOMPARE(getpath, "/dl/demos/", skiplen))
	{
		HTTPSV_GenerateDemoDownload(cluster, pend, getpath + skiplen);
	}
	else if (URLCOMPARE(getpath, "/rss", skiplen))
	{
		HTTPSV_GenerateRSS(cluster, pend, "");
	}
	else if (URLCOMPARE(getpath, "/status", skiplen))
	{
		HTTPSV_GenerateQTVStatus(cluster, pend, getpath + skiplen);
	}
	else if (!strcmp(FS_FileExtension(getpath), ".png"))
	{
		s = strchrrev(getpath, '/') + 1;
		HTTPSV_GenerateImage(cluster, pend, s);
	}
	else if (!strcmp(getpath, "/about"))
	{	
		// Redirect them to our funky website.
		s = "HTTP/1.0 302 Found" CRLF
			"Location: " QTV_PROJECT_URL CRLF
			CRLF;
		Net_ProxySend(cluster, pend, s, strlen(s));
	}
	else if (!strcmp(getpath, "/"))
	{
		s = "HTTP/1.0 302 Found" CRLF
			"Location: /nowplaying/" CRLF
			CRLF;
		Net_ProxySend(cluster, pend, s, strlen(s));
	}
	else
	{
		HTTPSV_SendHTTPHeader(cluster, pend, "404", "text/html", true);
		HTTPSV_SendHTMLHeader(cluster, pend, "Address not recognised");
		s = "    <h1>Address not recognised</h1>\n";
		Net_ProxySend(cluster, pend, s, strlen(s));
		HTTPSV_SendHTMLFooter(cluster, pend);
	}
}

//
// Gets the hostname from the header provided by the web client.
//
qbool HTTPSV_GetHostname(cluster_t *cluster, oproxy_t *dest, char *hostname, int buffersize)
{
	char *s = NULL;

	if (!HTTPSV_GetHeaderField((char *)dest->inbuffer, "Host", hostname, buffersize))
	{
		HTTPSV_SendHTTPHeader(cluster, dest, "400", "text/html", true);
		HTTPSV_SendHTMLHeader(cluster, dest, "QuakeTV: Error");

		s = "    <p>Your client did not send a Host field, which is required in HTTP/1.1<br />"
			"Please try a different browser.</p>\n"
			"\n  </body>\n\n"
			"</html>\n";

		Net_ProxySend(cluster, dest, s, strlen(s));
		return false;
	}

	return true;
}
