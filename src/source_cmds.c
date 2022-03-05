/*
source related commadns
*/

#include "qtv.h"

static void qtv_parse_options(qtvoptions_t* options, int first_arg)
{
	int i;

	memset(options, 0, sizeof(*options));
	for (i = first_arg; i < Cmd_Argc() - 1; ++i) {
		if (!stricmp(Cmd_Argv(i), "password")) {
			const char* custom_pw = Cmd_Argv(i + 1);

			if (custom_pw[0]) {
				options->flags |= QTV_CUSTOM_PASSWORD;
				strlcpy(options->client_password, custom_pw, sizeof(options->client_password));

				if (strlen(custom_pw) < sizeof(options->client_password)) {
					Sys_Printf("... using custom password\n");
				}
				else {
					Sys_Printf("... using custom password (WARNING: truncated)\n");
				}
			}
			++i;
		}
		else if (!stricmp(Cmd_Argv(i), "delay")) {
			const char* delay_ = Cmd_Argv(i + 1);
			float delay = atof(delay_);

			if (delay || delay_[0] == '0') {
				Sys_Printf("... using custom delay (%fs)\n", delay);
				options->flags |= QTV_CUSTOM_PARSEDELAY;
				options->ingame_delay = delay;
			}
			++i;
		}
		else if (!stricmp(Cmd_Argv(i), "address")) {
			const char* custom_addr = Cmd_Argv(i + 1);

			// allow this to be blank
			options->flags |= QTV_CUSTOM_ADDRESS;
			strlcpy(options->address, custom_addr, sizeof(options->address));
			Sys_Printf("... using custom address (%s)\n", options->address);

			++i;
		}
		else {
			Sys_Printf("... unknown option %s\n", Cmd_Argv(i));
		}
	}
}

// connect via tcp to qtv source, that may be other qtv or server or whatever
void qtv_f(void)
{
	char addr[MAX_QPATH] = {0};
	char* password = Cmd_Argv(2);
	sv_t *qtv;
	qtvoptions_t options;

	if (Cmd_Argc() < 2 || !*Cmd_Argv(1)) // not less than one param, first param non empty
	{
		Sys_Printf("Usage: %s ip:port [password] [options]\n", Cmd_Argv(0));
		Sys_Printf("Options:\n");
		Sys_Printf("  password <password>      // password for connecting clients\n");
		Sys_Printf("  delay <delay>            // in seconds (defaults to 'parse_delay')\n");
		Sys_Printf("  address <address>        // address mvdsv will broadcast to server-browsers\n");
		return;
	}

	qtv_parse_options(&options, 3);

	snprintf(addr, sizeof(addr), "tcp:%s", Cmd_Argv(1));
	qtv = QTV_NewServerConnection2(&g_cluster, addr, password, false, false, false, false, &options);

	if (!qtv)
	{
		Sys_Printf("Failed to connect to server %s, connection aborted\n", addr);
		return;
	}

	Sys_Printf("Source registered %s (#%d)\n", addr, qtv->streamid);
}

// opened .mvd demo as source
// absolute path prohibitet, but playdemo qw/demo.mvd is ok
void playdemo_f(void)
{
	char addr[MAX_QPATH] = {0};

	if (Cmd_Argc() < 2 || !*Cmd_Argv(1))  // not less than one param, first param non empty
	{
		Sys_Printf("Usage: %s demoname.mvd\n", Cmd_Argv(0));
		return;
	}

	snprintf(addr, sizeof(addr), "file:%s", Cmd_Argv(1));

	if (!QTV_NewServerConnection(&g_cluster, addr, Cmd_Argv(2), false, false, false, false)) {
		Sys_Printf("Failed to open demo %s\n", addr);
		return;
	}

	Sys_Printf("Source registered %s\n", addr);
}

void sourceclose_f(void)
{
	sv_t *qtv;
	unsigned int id, cnt;
	qbool all;

	if (Cmd_Argc() < 2 || !*Cmd_Argv(1))  // not less than one param, first param non empty
	{
		Sys_Printf("Usage: %s <#id | all>\n", Cmd_Argv(0));
		return;
	}

	if (!g_cluster.servers) {
		Sys_Printf("source list alredy empty\n");
		return;
	}

	id  = (unsigned int) atoi(Cmd_Argv(1));
	all = !stricmp(Cmd_Argv(1), "all");
	cnt = 0;

	for (qtv = g_cluster.servers; qtv; qtv = qtv->next)
	{
		if (all || qtv->streamid == id) 
		{
			Sys_Printf("Source id:%d aka %s will be dropped asap\n", qtv->streamid, qtv->server);
			qtv->drop = true;
			cnt++;
		}
	}

	if (!cnt)
		Sys_Printf("Source id:%d not found\n", id);
}


void sourcelist_f(void)
{
    char *type;
	sv_t *qtv;

	if (!g_cluster.NumServers)
		Sys_Printf("Sources list: empty\n");

	Sys_Printf("Sources list:\n"
						"%4.4s %4.4s %3.3s %4.4s %s\n", "#Id", "Type", "Tmp", "Echo", "Name");

	for (qtv = g_cluster.servers; qtv; qtv = qtv->next)
	{
		switch(qtv->src.type)
		{
			case SRC_DEMO: type = "demo"; break;
			case SRC_TCP:  type = "tcp";  break;
			case SRC_BAD:  type = "bad";  break;
			default:	   type = "unkn"; break; // unknown
		}

		Sys_Printf("%4d %4.4s %3.3s %4.4s %s\n", qtv->streamid, type, 
			(qtv->DisconnectWhenNooneIsWatching ? "yes" : "no"), 
			(qtv->EchoInServerConsole ? "yes" : "no"), qtv->server);
	}

	Sys_Printf("\nId = Source ID.\n");
	Sys_Printf("Type = Type of source.\n");
	Sys_Printf("Tmp = Disconnect when noone is watching.\n");
	Sys_Printf("Echo = Echo the console from this source to the server console.\n");
	Sys_Printf("Name = The name of the source.\n");
}

void status_f(void)
{
	Sys_Printf("Status:\n");
	Sys_Printf(" servers: %4i/%i\n", g_cluster.NumServers, maxservers.integer);
	Sys_Printf(" clients: %4i/%i\n", g_cluster.numproxies, get_maxclients());

	Sys_Printf("Options:\n");
	Sys_Printf("   hostname: %s\n", hostname.string);
	Sys_Printf("    mvdport: %i%s\n", mvdport.integer, g_cluster.tcpsocket == INVALID_SOCKET ? " (INVALID)" : "");
	Sys_Printf("    udpport: %i%s\n", mvdport.integer, g_cluster.udpsocket == INVALID_SOCKET ? " (INVALID)" : "");
	Sys_Printf(" allow_http: %s\n", allow_http.integer ? "yes" : "no");
}

void quit_f(void)
{
	// if at least some parameter provided, then use non clean exit
	if (Cmd_Argc() > 1)
		Sys_Exit(0); // immidiate, non clean
	else
		g_cluster.wanttoexit = true; // delayed exit, clean
}

void clientlist(sv_t *qtv, qbool showempty)
{
	int c;
	oproxy_t *tmp;
	char name[MAX_INFO_KEY];
	char ip[] = "xxx.xxx.xxx.xxx";

	for (c = 0, tmp = qtv->proxies; tmp; tmp = tmp->next)
	{
		if (tmp->drop)
			continue;

		if (!c)
		{
			Sys_Printf("source: %d, %s\n"
							  "userid ip              name\n"
							  "------ --------------- ----\n", qtv->streamid, qtv->server);
		}

		Sys_Printf("%6d %15s %s\n", tmp->id, Net_BaseAdrToString(&tmp->addr, ip, sizeof(ip)), Info_Get(&tmp->ctx, "name", name, sizeof(name)));
		c++;
	}	

	if (c)
	{
		Sys_Printf("%d total users\n", c);
	}
	else
	{
		if (showempty)
			Sys_Printf("source: %d, %s, no clients\n", qtv->streamid, qtv->server);
	}
}

void clientlist_f(void)
{
	int c;
	sv_t *qtv = NULL;
	unsigned int id = (unsigned int) atoi(Cmd_Argv(1));
	qbool showempty = !!id;

	for (c = 0, qtv = g_cluster.servers; qtv; qtv = qtv->next)
	{
		if (id && qtv->streamid != id)
			continue;

		if (!c)
			Sys_Printf("Client list:\n");

		clientlist(qtv, showempty);
		c++;
	}

	if (c)
		Sys_Printf("%d sources found%s\n", c, showempty ? "" : ", empty sources are not shown");
	else
		Sys_Printf("source list empty or no particular stream was found\n", id);
}

void kick_f(void)
{
	sv_t *qtv = NULL;
	int id = atoi(Cmd_Argv(1));
	oproxy_t *tmp;
	char name[MAX_INFO_KEY];
	char ip[] = "xxx.xxx.xxx.xxx";

	for (qtv = g_cluster.servers; qtv; qtv = qtv->next)
	{
		for (tmp = qtv->proxies; tmp; tmp = tmp->next)
		{
			if (tmp->id != id)
				continue;

			tmp->drop = true;
			Sys_Printf("kicked: %d %s %s\n", tmp->id, Net_BaseAdrToString(&tmp->addr, ip, sizeof(ip)), Info_Get(&tmp->ctx, "name", name, sizeof(name)));
			return;
		}	
	}

	Sys_Printf("client with id %d not found\n", id);
}

void showoutput_f(void)
{
	sv_t *qtv;
	unsigned int id;
	qbool all;
	qbool show;

	if (Cmd_Argc() < 3 || !*Cmd_Argv(1) || !*Cmd_Argv(2))  // not less than one param, first param non empty
	{
		Sys_Printf("Usage: %s <#id | all> <0|1>\n", Cmd_Argv(0));
		return;
	}

	if (!g_cluster.servers) 
	{
		Sys_Printf("There are no sources.\n");
		return;
	}

	id  = (unsigned int) atoi(Cmd_Argv(1));
	all = !stricmp(Cmd_Argv(1), "all");
	show = (qbool) atoi(Cmd_Argv(2));

	for (qtv = g_cluster.servers; qtv; qtv = qtv->next)
	{
		if (all || qtv->streamid == id) 
		{
			qtv->EchoInServerConsole = show;
		}
	}
}

/*
===========
serverinfo_f

Examine or change the serverinfo string
===========
*/
void serverinfo_f (void)
{
	cvar_t	*var;
	char *s;
	char	*key, *value;

	if (Cmd_Argc() == 1)
	{
		Sys_Printf ("Server info settings:\n");
		Info_Print (g_cluster.info);
		Sys_Printf ("[%d/%d]\n", strlen(g_cluster.info), MAX_SERVERINFO_STRING);
		return;
	}

	if (Cmd_Argc() == 2)
	{
		char cur_value[MAX_INFO_KEY] = "";

		s = Info_ValueForKey(g_cluster.info, Cmd_Argv(1), cur_value, sizeof(cur_value));
		if (*s)
			Sys_Printf ("Serverinfo %s: \"%s\"\n", Cmd_Argv(1), s);
		else
			Sys_Printf ("No such key %s\n", Cmd_Argv(1));
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Sys_Printf ("usage: serverinfo [ <key> [ <value> ] ]\n");
		return;
	}

	key = Cmd_Argv(1);
	value = Cmd_Argv(2);

	if (key[0] == '*')
	{
		Sys_Printf ("Star variables cannot be changed.\n");
		return;
	}

	// force serverinfo "0" vars to be "".
	if (!strcmp(value, "0"))
		value = "";

	// if the key is also a serverinfo cvar, change it too
	var = Cvar_Find(key);
	if (var && (var->flags & CVAR_SERVERINFO))
	{
		Cvar_Set (var, value); // this call SV_ServerinfoChanged() as well.
	}
	else
	{
		SV_ServerinfoChanged(key, value);
	}
}

void Source_Init(void)
{
	Cvar_Register (&maxservers);
	Cvar_Register (&upstream_timeout);
	Cvar_Register (&parse_delay);
	Cvar_Register (&qtv_reconnect_delay);
	Cvar_Register (&qtv_max_reconnect_delay);
	Cvar_Register (&qtv_backoff);

	Cmd_AddCommand ("qtv", qtv_f);
	Cmd_AddCommand ("playdemo", playdemo_f);
	Cmd_AddCommand ("sourcelist", sourcelist_f);
	Cmd_AddCommand ("sourceclose", sourceclose_f);
	Cmd_AddCommand ("showoutput", showoutput_f);
	Cmd_AddCommand ("status", status_f);
	Cmd_AddCommand ("quit", quit_f);

	Cmd_AddCommand ("clientlist", clientlist_f);
	Cmd_AddCommand ("kick", kick_f);

	Cmd_AddCommand ("serverinfo", serverinfo_f);
}

