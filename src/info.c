/*
Manipulation with various info strings
*/

#include "qtv.h"

//Cvar system calls this when a CVAR_SERVERINFO cvar changes
void SV_ServerinfoChanged (char *key, char *string)
{
	char value[MAX_INFO_KEY] = "";

	// force serverinfo "0" vars to be "".
	if (!strcmp(string, "0"))
		string = "";

	if (strcmp(string, Info_ValueForKey (g_cluster.info, key, value, sizeof(value))))
	{
		Info_SetValueForStarKey (g_cluster.info, key, string, MAX_SERVERINFO_STRING);
	}
}

char *Info_ValueForKey (const char *s, const char *const key, char *const buffer, size_t buffersize)
{
	size_t keylen = strlen(key);
	
	// check arguments
	if (s[0] != '\\' || keylen == 0 || buffersize < 2) 
		goto notfound;
	
	buffersize--; // make space for the null-terminator

	do {
		size_t matched = 0;
		const char* keyp = key;

		s++; // skip backslash

		while (*s && *keyp && *s == *keyp && *s != '\\') {
			matched++;
			++keyp;
			++s;
		}
		if (matched == keylen && *s == '\\') {
			// match value
			size_t copied = 0;

			s++;
			while (*s && *s != '\\' && copied < buffersize) {	
				buffer[copied++] = *s++;
			}
			buffer[copied] = '\0'; // space for this is always available, see above
			return buffer;
		}
		else {
			while (*s && *s != '\\') {
				s++; // skip the key
			}
			if (*s) {
				s++; // skip backslash
			}
			while (*s && *s != '\\') {
				s++; // skip value
			}
		}
	} while (*s);

notfound:
	buffer[0] = '\0';
	return buffer;
}

void Info_RemoveKey (char *s, const char *key)
{
	char	*start;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	if (strstr (key, "\\"))
	{
//		printf ("Key has a slash\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey))
		{
			memmove(start, s, strlen(s) + 1);
			return;
		}

		if (!*s)
			return;
	}

}

void Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize)
{
	char	newv[1024], *v;
	int		c;

	if (strstr (key, "\\") || strstr (value, "\\") )
	{
//		printf ("Key has a slash\n");
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"") )
	{
//		printf ("Key has a quote\n");
		return;
	}

	if (strlen(key) >= MAX_INFO_KEY || strlen(value) >= MAX_INFO_KEY)
	{
//		printf ("Key or value is too long\n");
		return;
	}

	// this next line is kinda trippy
	if (*(v = Info_ValueForKey(s, key, newv, sizeof(newv))))
	{
		// key exists, make sure we have enough room for new value, if we don't,
		// don't change it!
		if (strlen(value) - strlen(v) + strlen(s) + 1 > maxsize)
		{
	//		Con_TPrintf (TL_INFOSTRINGTOOLONG);
			return;
		}
	}


	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	snprintf (newv, sizeof(newv), "\\%s\\%s", key, value);

	if ((int)(strlen(newv) + strlen(s) + 1) > maxsize)
	{
//		printf ("info buffer is too small\n");
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newv;
	while (*v)
	{
		c = (unsigned char)*v++;

//		c &= 127;		// strip high bits
		if (c > 13) // && c < 127)
			*s++ = c;
	}
	*s = 0;
}

void Info_Print (char *s)
{
	char key[512];
	char value[512];
	char *o;
	int l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Sys_Printf ("%s ", key);

		if (!*s)
		{
			Sys_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Sys_Printf ("%s\n", value);
	}
}

//============================================================
// Alternative variant manipulation with info strings
//============================================================




// used internally
static info_t *_Info_Get (ctxinfo_t *ctx, const char *name)
{
	info_t *a;
	int key;

	if (!ctx || !name || !name[0])
		return NULL;

	key = Sys_HashKey (name) % INFO_HASHPOOL_SIZE;

	for (a = ctx->info_hash[key]; a; a = a->hash_next)
		if (!stricmp(name, a->name))
			return a;

	return NULL;
}

char *Info_Get(ctxinfo_t *ctx, const char *name, char *buf, int bufsize)
{
	info_t *a = _Info_Get(ctx, name);

	if (!buf || bufsize < 1)
		Sys_Error("Info_Get: buf");

	strlcpy(buf, a ? a->value : "", bufsize);
	return buf;
}

qbool Info_Set (ctxinfo_t *ctx, const char *name, const char *value)
{
	info_t	*a;
	int key;

	if (!value)
		value = "";

	if (!ctx || !name || !name[0])
		return false;

	if (strchr(name, '\\') || strchr(value, '\\'))
		return false;
	if (strchr(name, 128 + '\\') || strchr(value, 128 + '\\'))
		return false;
	if (strchr(name, '"') || strchr(value, '"'))
		return false;
	if (strchr(name, '\r') || strchr(value, '\r')) // bad for print functions
		return false;
	if (strchr(name, '\n') || strchr(value, '\n')) // bad for print functions
		return false;
	if (strchr(name, '$') || strchr(value, '$')) // variable expansion may be exploited, escaping this
		return false;
	if (strchr(name, ';') || strchr(value, ';')) // interpreter may be haxed, escaping this
		return false;

	if (strlen(name) >= MAX_INFO_KEY || strlen(value) >= MAX_INFO_KEY)
		return false; // too long name/value, its wrong

	key = Sys_HashKey(name) % INFO_HASHPOOL_SIZE;

	// if already exists, reuse it
	for (a = ctx->info_hash[key]; a; a = a->hash_next)
		if (!stricmp(name, a->name))
		{
			Sys_free (a->value);
			break;
		}

	// not found, create new one
	if (!a)
	{
		if (ctx->cur >= ctx->max)
			return false; // too much infos

		a = (info_t *) Sys_malloc (sizeof(info_t));
		a->next = ctx->info_list;
		ctx->info_list = a;
		a->hash_next = ctx->info_hash[key];
		ctx->info_hash[key] = a;

		ctx->cur++; // increase counter

		// copy name
		a->name = Sys_strdup (name);
	}

	// copy value
#if 0
	{
		// unfortunatelly evil users use non printable/control chars, so that does not work well
		a->value = Sys_strdup (value);
	}
#else
	{
		// skip some control chars, doh
		char v_buf[MAX_INFO_KEY] = {0}, *v = v_buf;
		int i;

		for (i = 0; value[i]; i++) // len of 'value' should be less than MAX_INFO_KEY according to above checks
		{
			if ((unsigned char)value[i] > 13)
				*v++ = value[i];
		}
		*v = 0;

		a->value = Sys_strdup (v_buf);
	}
#endif

	// hrm, empty value, remove it then
	if (!a->value[0])
	{
		return Info_Remove(ctx, name);
	}

	return true;
}

// used internally
static void _Info_Free(info_t *a)
{
	if (!a)
		return;

	Sys_free (a->name);
	Sys_free (a->value);
	Sys_free (a);
}

qbool Info_Remove (ctxinfo_t *ctx, const char *name)
{
	info_t *a, *prev;
	int key;

	if (!ctx || !name || !name[0])
		return false;

	key = Sys_HashKey (name) % INFO_HASHPOOL_SIZE;

	prev = NULL;
	for (a = ctx->info_hash[key]; a; a = a->hash_next)
	{
		if (!stricmp(name, a->name))
		{
			// unlink from hash
			if (prev)
				prev->hash_next = a->hash_next;
			else
				ctx->info_hash[key] = a->hash_next;
			break;
		}
		prev = a;
	}

	if (!a)
		return false;	// not found

	prev = NULL;
	for (a = ctx->info_list; a; a = a->next)
	{
		if (!stricmp(name, a->name))
		{
			// unlink from info list
			if (prev)
				prev->next = a->next;
			else
				ctx->info_list = a->next;

			// free
			_Info_Free(a);

			ctx->cur--; // decrease counter

			return true;
		}
		prev = a;
	}

	Sys_Error("Info_Remove: info list broken");
	return false; // shut up compiler
}

// remove all infos
void Info_RemoveAll (ctxinfo_t *ctx)
{
	info_t	*a, *next;

	if (!ctx)
		return;

	for (a = ctx->info_list; a; a = next) {
		next = a->next;

		// free
		_Info_Free(a);
	}
	ctx->info_list = NULL;
	ctx->cur = 0; // set counter to 0

	// clear hash
	memset (ctx->info_hash, 0, sizeof(ctx->info_hash));
}

qbool Info_Convert(ctxinfo_t *ctx, char *str)
{
	char name[MAX_INFO_KEY], value[MAX_INFO_KEY], *start;

	if (!ctx)
		return false;

	for ( ; str && str[0]; )
	{
		if (!(str = strchr(str, '\\')))
			break;

		start = str; // start of name

		if (!(str = strchr(start + 1, '\\')))  // end of name
			break;

		strlcpy(name, start + 1, min(str - start, (int)sizeof(name)));

		start = str; // start of value

		str = strchr(start + 1, '\\'); // end of value

		strlcpy(value, start + 1, str ? min(str - start, (int)sizeof(value)) : (int)sizeof(value));

		Info_Set(ctx, name, value);
	}

	return true;
}

void Info_PrintList(ctxinfo_t *ctx)
{
	info_t *a;
	int cnt = 0;

	if (!ctx)
		return;

	for (a = ctx->info_list; a; a = a->next)
	{
		Sys_Printf("%-20s %s\n", a->name, a->value);
		cnt++;
	}

	Sys_Printf(" %d infos\n", cnt);
}

#ifdef INFODEBUG


ctxinfo_t g_ctx;


void Info_PrintHash(ctxinfo_t *ctx)
{
	info_t *a;
	int i, cnt = 0;

	if (!ctx)
		return;

	for (i = 0; i < INFO_HASHPOOL_SIZE; i++)
	{
		if (!(a = ctx->info_hash[i]))
			continue;

		Sys_Printf("KEY: %d\n", i);

		for (; a; a = a->hash_next)
		{
			Sys_Printf("'%s' is '%s'\n", a->name, a->value);
			cnt++;
		}
	}

	Sys_Printf("%d infos\n", cnt);
}

void infoset_f(void)
{
	qbool set = Info_Set(&g_ctx, Cmd_Argv(1), Cmd_Argv(2));

	Sys_Printf("info '%s' %sset to '%s'\n", Cmd_Argv(1), set ? "" : "not ", Cmd_Argv(2));
}

void infoget_f(void)
{
	Sys_Printf("'%s' is '%s'\n", Cmd_Argv(1), Info_Get(&g_ctx, Cmd_Argv(1)));
}

void inforemove_f(void)
{
	qbool rem = Info_Remove(&g_ctx, Cmd_Argv(1));

	Sys_Printf("'%s' %sremoved\n", Cmd_Argv(1), rem ? "" : "not ");
}

void inforemoveall_f(void)
{
	Info_RemoveAll(&g_ctx);

	Sys_Printf("removeall\n");
}

void infoconvert_f(void)
{
	qbool conv = Info_Convert(&g_ctx, Cmd_Argv(1));

	Sys_Printf("'%s' %s\n", Cmd_Argv(1), conv ? "converted" : "not converted");
}

void infoprinthash_f(void)
{
	Info_PrintHash(&g_ctx);
}

void infoprintlist_f(void)
{
	Info_PrintList(&g_ctx);
}

void infotest_f(void)
{
	int i, j, cnt = atoi(Cmd_Argv(1));
	char name[MAX_INFO_KEY], value[MAX_INFO_KEY];

	for (i = 0; i < cnt; i++)
	{
		for (j = 0; j < MAX_INFO_KEY; j++)
			name[j] = 'a' + rand() % ('z' - 'a');

		name[MAX_INFO_KEY-1] = 0;

		for (j = 0; j < MAX_INFO_KEY; j++)
			value[j] = 'a' + rand() % ('z' - 'a');

		value[MAX_INFO_KEY-1] = 0;

		Info_Set(&g_ctx, name, value);
	}
}


#endif

void Info_Init(void)
{
#ifdef INFODEBUG

	memset(&g_ctx, 0, sizeof(g_ctx));
	g_ctx.max = 10000;

	Cmd_AddCommand ("infotest",			infotest_f);
	Cmd_AddCommand ("infoset",			infoset_f);
	Cmd_AddCommand ("infoget",			infoget_f);
	Cmd_AddCommand ("inforemove",		inforemove_f);
	Cmd_AddCommand ("inforemoveall",	inforemoveall_f);
	Cmd_AddCommand ("infoconvert",		infoconvert_f);
	Cmd_AddCommand ("infoprinthash",	infoprinthash_f);
	Cmd_AddCommand ("infoprintlist",	infoprintlist_f);

#endif
}
