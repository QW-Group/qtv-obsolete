// ban.c - banning related code, boldly stolen from mvdsv

#include "qtv.h"
#include "time.h"

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

#define	MAX_IPFILTERS	1024

typedef union
{
	unsigned int	u;
	unsigned char	b[4];
} b2i_t;

typedef enum
{
	ipft_ban,
	ipft_safe
} ipfiltertype_t;

typedef struct
{
	unsigned	mask;
	unsigned	compare;
//	int			level;
	double		time; // for ban expiration
	ipfiltertype_t type;
} ipfilter_t;


static ipfilter_t	ipfilters[MAX_IPFILTERS];
static int			numipfilters;

//cvar_t	filterban = {"filterban", "1"};

/*
=================
SV_FilterPacket
=================
*/
qbool SV_IsBanned (struct sockaddr_in *addr)
{
	int				i;
	unsigned int	in;

	in = addr->sin_addr.s_addr;

	for (i=0 ; i<numipfilters ; i++)
	{
		if ( ipfilters[i].type == ipft_ban && (in & ipfilters[i].mask) == ipfilters[i].compare )
		{
//			return (int)filterban.value;
			return true;
		}
	}

//	return !(int)filterban.value;
	return false;
}


/*
=================
StringToFilter
=================
*/
static qbool StringToFilter (char *s, ipfilter_t *f)
{
	char	num[128];
	int		i, j;
	b2i_t	b = {0}; // ip octet.
	b2i_t	m = {0}; // mask octet.

	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			//Sys_Printf("Bad filter address: %s\n", s);
			return false;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b.b[i] = atoi(num);
		if (b.b[i] != 0)
			m.b[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = m.u;
	f->compare = b.u;

	return true;
}

/*
=================
SV_AddIP_f
=================
*/
static void SV_AddIP_f (void)
{
	int		i;
	double	t = 0;
	char	*s;
	time_t	long_time = time(NULL);
	ipfilter_t f;
	ipfiltertype_t ipft = ipft_ban; // default is ban

	if (!StringToFilter (Cmd_Argv(1), &f) || f.compare == 0)
	{
		Sys_Printf("Bad filter address: %s\n", Cmd_Argv(1));
		return;
	}

	s = Cmd_Argv(2);
	if ( !s[0] || !strcmp(s, "ban"))
		ipft = ipft_ban;
	else if (!strcmp(s, "safe"))
		ipft = ipft_safe;
	else {
		Sys_Printf("Wrong filter type %s, use ban or safe\n", Cmd_Argv(2));
		return;
	}

	s = Cmd_Argv(3);
	if (long_time > 0)
	{
		if (*s == '+')     // "addip 127.0.0.1 ban +10" will ban for 10 seconds from current time
			s++;
		else
			long_time = 0; // "addip 127.0.0.1 ban 1234567" will ban for some seconds since 00:00:00 GMT, January 1, 1970

		t = (sscanf(s, "%lf", &t) == 1) ? t + long_time : 0;
	}

	f.time = t;
	f.type = ipft;

	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].compare == 0xffffffff || (ipfilters[i].mask == f.mask && ipfilters[i].compare == f.compare))
			break;		// free spot

	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			Sys_Printf("IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	ipfilters[i] = f;
}

/*
=================
SV_RemoveIP_f
=================
*/
static void SV_RemoveIP_f (void)
{
	ipfilter_t	f;
	int			i, j;

	if (!StringToFilter (Cmd_Argv(1), &f))
	{
		Sys_Printf("Bad filter address: %s\n", Cmd_Argv(1));
		return;
	}

	for (i=0 ; i<numipfilters ; i++)
	{
		if (ipfilters[i].mask == f.mask && ipfilters[i].compare == f.compare)
		{
			for (j=i+1 ; j<numipfilters ; j++)
				ipfilters[j-1] = ipfilters[j];
			numipfilters--;
			Sys_Printf("Removed.\n");
			return;
		}
	}

	Sys_Printf("Didn't find %s.\n", Cmd_Argv(1));
}

/*
=================
SV_ListIP_f
=================
*/
static void SV_ListIP_f (void)
{
	time_t	long_time = time(NULL);
	int		i;
	b2i_t	b = {0};

	Sys_Printf("Filter list:\n");
	for (i=0 ; i<numipfilters ; i++)
	{
		b.u = ipfilters[i].compare;
		Sys_Printf("%3i.%3i.%3i.%3i | ", b.b[0], b.b[1], b.b[2], b.b[3]);
		switch((int)ipfilters[i].type)
		{
			case ipft_ban:  Sys_Printf(" ban"); break;
			case ipft_safe: Sys_Printf("safe"); break;
			default: Sys_Printf("unkn"); break;
		}
		if (ipfilters[i].time)
			Sys_Printf(" | %i s", (int)(ipfilters[i].time-long_time));

		Sys_Printf("\n");
	}
}

/*
=================
SV_WriteIP_f
=================
*/
static void SV_WriteIP_f (void)
{
	FILE	*f;
	char	name[1024], *s;
	b2i_t	b = {0};
	int		i;

	snprintf (name, sizeof(name), "qtv/listip.cfg");

	Sys_Printf("Writing %s.\n", name);

	f = fopen (name, "wb");
	if (!f)
	{
		Sys_Printf("Couldn't open %s\n", name);
		return;
	}

	// write safe filters first
	for (i=0 ; i<numipfilters ; i++)
	{
		if(ipfilters[i].type != ipft_safe)
			continue;

		b.u = ipfilters[i].compare;
		fprintf (f, "addip %i.%i.%i.%i safe %.0f\n", b.b[0], b.b[1], b.b[2], b.b[3], ipfilters[i].time);
	}

	for (i=0 ; i<numipfilters ; i++)
	{
		if(ipfilters[i].type == ipft_safe)
			continue; // ignore safe, we already save it

		switch((int)ipfilters[i].type)
		{
			case ipft_ban:  s = " ban"; break;
			case ipft_safe: s = "safe"; break;
			default: s = "unkn"; break;
		}
		b.u = ipfilters[i].compare;
		fprintf (f, "addip %i.%i.%i.%i %s %.0f\n", b.b[0], b.b[1], b.b[2], b.b[3], s, ipfilters[i].time);
	}

	fclose (f);
}

static void Do_BanList(ipfiltertype_t ipft)
{
	time_t	long_time = time(NULL);
	int		i;
	b2i_t	b;

	for (i=0 ; i<numipfilters ; i++)
	{
		if (ipfilters[i].type != ipft)
			continue;

		b.u = ipfilters[i].compare;
		Sys_Printf("%3i|%3i.%3i.%3i.%3i", i, b.b[0], b.b[1], b.b[2], b.b[3]);
		switch((int)ipfilters[i].type)
		{
			case ipft_ban:  Sys_Printf("| ban"); break;
			case ipft_safe: Sys_Printf("|safe"); break;
			default: Sys_Printf("|unkn"); break;
		}

		if (ipfilters[i].time)
		{
			long df = ipfilters[i].time-long_time;
			long d, h, m, s;
			d = df / (60*60*24);
			df -= d * 60*60*24;
			h = df / (60*60);
			df -= h * 60*60;
			m = df /  60;
			df -= m * 60;
			s = df;

			if (d)
				Sys_Printf("|%4ldd:%2ldh", d, h);
			else if (h)
				Sys_Printf("|%4ldh:%2ldm", h, m);
			else
				Sys_Printf("|%4ldm:%2lds", m, s);
		}
		else
		{
			Sys_Printf("|permanent");
		}

		Sys_Printf("\n");
	}
}

static void SV_BanList (void)
{
	unsigned char blist[64] = "Ban list:", id[64] = "id", ipmask[64] = "ip mask", type[64] = "type", expire[64] = "expire";

	if (numipfilters < 1)
	{
		Sys_Printf("Ban list: empty\n");
		return;
	}

	Sys_Printf("%s\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n"
				"%3.3s|%15.15s|%4.4s|%9.9s\n",
				blist, id, ipmask, type, expire);

	Do_BanList(ipft_safe);
	Do_BanList(ipft_ban);
}

static qbool SV_CanAddBan (ipfilter_t *f)
{
	int i;

	if (f->compare == 0)
		return false;

	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].mask == f->mask && ipfilters[i].compare == f->compare && ipfilters[i].type == ipft_safe)
			return false; // can't add filter f because present "safe" filter

	return true;
}

static void SV_RemoveBansIPFilter (int i)
{
	for (; i + 1 < numipfilters; i++)
		ipfilters[i] = ipfilters[i + 1];

	numipfilters--;
}

static void SV_Cmd_Banip_f(void)
{
	b2i_t		b = {0};
	double		d;
	int			c, t;
	ipfilter_t  f;
	char		arg2[32], arg2c[sizeof(arg2)], tmp_str[256];

	c = Cmd_Argc ();
	if (c < 3)
	{
		Sys_Printf("usage: %s <ip> <time<s m h d>>\n", Cmd_Argv(0));
		return;
	}

	if (!StringToFilter (Cmd_Argv(1), &f))
	{
		Sys_Printf("ban: bad ip address: %s\n", Cmd_Argv(1));
		return;
	}

	if (!SV_CanAddBan(&f))
	{
		Sys_Printf("ban: can't ban such ip: %s\n", Cmd_Argv(1));
		return;
	}

	strlcpy(arg2, Cmd_Argv(2), sizeof(arg2));

	// sscanf safe here since sizeof(arg2) == sizeof(arg2c), right?
	if (sscanf(arg2, "%d%s", &t, arg2c) != 2 || strlen(arg2c) != 1)
	{
		Sys_Printf("ban: wrong time arg\n");
		return;
	}

	d = t = bound(0, t, 999);
	switch(arg2c[0])
	{
		case 's': break; // seconds is seconds
		case 'm': d *= 60; break; // 60 seconds per minute
		case 'h': d *= 60*60; break; // 3600 seconds per hour
		case 'd': d *= 60*60*24; break; // 86400 seconds per day
		default:
		Sys_Printf("ban: wrong time arg\n");
		return;
	}

	b.u = f.compare;
	Sys_Printf("%3i.%3i.%3i.%3i was banned for %d%s\n", b.b[0], b.b[1], b.b[2], b.b[3], t, arg2c);

	snprintf(tmp_str, sizeof(tmp_str), "addip %i.%i.%i.%i ban %s%.0lf\n", b.b[0], b.b[1], b.b[2], b.b[3], d ? "+" : "", d);
	Cbuf_AddText(tmp_str);
	Cbuf_AddText("writeip\n");
}

static void SV_Cmd_Banremove_f(void)
{
	b2i_t	b = {0};
	int		id;

	if (Cmd_Argc () < 2)
	{
		Sys_Printf("usage: %s [banid]\n", Cmd_Argv(0));
		SV_BanList();
		return;
	}

	id = atoi(Cmd_Argv(1));

	if (id < 0 || id >= numipfilters)
	{
		Sys_Printf("Wrong ban id: %d\n", id);
		return;
	}

	if (ipfilters[id].type == ipft_safe)
	{
		Sys_Printf("Can't remove such ban with id: %d\n", id);
		return;
	}

	b.u = ipfilters[id].compare;
	Sys_Printf("%3i.%3i.%3i.%3i was unbanned\n", b.b[0], b.b[1], b.b[2], b.b[3]);

	SV_RemoveBansIPFilter (id);
	Cbuf_AddText("writeip\n");
}

void SV_CleanBansIPList (void)
{
	time_t	long_time = time(NULL);
	int     i;

	for (i = 0; i < numipfilters;)
	{
		if (ipfilters[i].time && ipfilters[i].time <= long_time)
		{
			SV_RemoveBansIPFilter (i);
		}
		else
		{
			i++;
		}
	}
}

void Ban_Init(void)
{
//	Cvar_Register(&filterban);

	Cmd_AddCommand ("addip", SV_AddIP_f);
	Cmd_AddCommand ("removeip", SV_RemoveIP_f);
	Cmd_AddCommand ("listip", SV_ListIP_f);
	Cmd_AddCommand ("writeip", SV_WriteIP_f);

	Cmd_AddCommand("banip", SV_Cmd_Banip_f);
	Cmd_AddCommand("banremove", SV_Cmd_Banremove_f);
}

