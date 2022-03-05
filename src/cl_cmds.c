/*
commands which comes from clients
*/

#include "qtv.h"

//==========================================
// download vars

cvar_t	allow_download 			= {"allow_download", 		"0"};
cvar_t	allow_download_skins	= {"allow_download_skins",	"0"};
cvar_t	allow_download_models	= {"allow_download_models",	"0"};
cvar_t	allow_download_sounds	= {"allow_download_sounds",	"0"};
cvar_t	allow_download_maps		= {"allow_download_maps",	"0"};
//cvar_t	allow_download_pakmaps	= {"allow_download_pakmaps", "0"};
cvar_t	allow_download_demos	= {"allow_download_demos",	"0"};
cvar_t	allow_download_other	= {"allow_download_other",	"0"};

cvar_t	demo_dir				= {"demo_dir", 	"demos"};

//==========================================
// say/say_team vars

cvar_t floodprot = {"floodprot", "4 2 2"};


//============================================================================
// search utils

oproxy_t *Prox_by_ID(sv_t *qtv, int id)
{
	oproxy_t *prox;

	if (!qtv || id < 1)
		return NULL;

	for (prox = qtv->proxies; prox; prox = prox->next)
		if (prox->id == id)
			return prox;

	return NULL;
}

oproxy_t *Prox_by_Name(sv_t *qtv, const char *name)
{
	oproxy_t *prox;
	char tmp[MAX_INFO_KEY];

	if (!qtv || !name || !name[0])
		return NULL;

	for (prox = qtv->proxies; prox; prox = prox->next)
		if (!strcmp(name, Info_Get(&prox->ctx, "name", tmp, sizeof(tmp))))
			return prox;

	return NULL;
}

oproxy_t *Prox_by_ID_or_Name(sv_t *qtv, const char *id_or_name)
{
	oproxy_t *prox = Prox_by_ID(qtv, atoi(id_or_name ? id_or_name : "0"));

	if (prox)
		return prox;

	return Prox_by_Name(qtv, id_or_name);
}

//============================================================================
// Misc utils

unsigned int Proxy_UsersCount(const sv_t *qtv)
{
	unsigned int c = 0;
	const oproxy_t *tmp;

	for (tmp = qtv->proxies; tmp; tmp = tmp->next)
	{
		if (tmp->drop)
			continue;
		c++;
	}	
	
	return c;
}

//============================================================================
// say/say_team
//

int fp_messages = 1, fp_persecond = 1, fp_secondsdead = 10;

void FixSayFloodProtect(void)
{
	if (!floodprot.modified)
		return;

	Cmd_TokenizeString (floodprot.string);

	fp_messages		= bound(1, atoi(Cmd_Argv(0)), MAX_FP_CMDS-1);
	fp_persecond	= bound(1, atoi(Cmd_Argv(1)), 999999);
	fp_secondsdead	= bound(1, atoi(Cmd_Argv(2)), 999999);

	floodprot.modified = false;

	Sys_Printf("floodprot: after %d msgs per %d seconds, silence for %d seconds\n",
													 fp_messages, fp_persecond, fp_secondsdead);
}

qbool isSayFlood(sv_t *qtv, oproxy_t *p)
{
	int idx;
	ullong say_time;

	idx = bound(0, p->fp_s.last_cmd, MAX_FP_CMDS-1);
	say_time = p->fp_s.cmd_time[idx];

	if ( g_cluster.curtime < p->fp_s.locked )
	{
		Prox_Printf(&g_cluster, p, dem_all, (unsigned)-1, PRINT_CHAT, 
			"You can't talk for %u more seconds\n", (unsigned int) ((p->fp_s.locked - g_cluster.curtime) / 1000));

		return true; // flooder
	}

	if ( say_time && ( g_cluster.curtime - say_time < fp_persecond * 1000 ) )
	{
		Prox_Printf(&g_cluster, p, dem_all, (unsigned)-1, PRINT_CHAT, 
				"FloodProt: You can't talk for %d seconds\n", fp_secondsdead);

		p->fp_s.locked = g_cluster.curtime + 1000 * fp_secondsdead;
		p->fp_s.warnings += 1; // collected but unused stat

		return true; // flooder
	}

	p->fp_s.cmd_time[idx] = g_cluster.curtime;

	if ( ++idx >= fp_messages )
		idx = 0;

	p->fp_s.last_cmd = idx;

	return false;
}

static int CmdToUpstream(sv_t *qtv, char *cmd)
{
	char buffer[1024 + 100];
	netmsg_t msg;

	if (qtv->src.type != SRC_TCP)
		return 0; // can't send anything anyway

	InitNetMsg(&msg, buffer, sizeof(buffer));

	WriteShort  (&msg, 2 + 1 + strlen(cmd) + 1); // short + byte + null terminated string
	WriteByte   (&msg, qtv_clc_stringcmd);
	WriteString (&msg, cmd);

	// may overflow our source and drop, so better do not send this cmd, caller must check success instead
	if (qtv->UpstreamBufferSize + msg.cursize > sizeof(qtv->UpstreamBuffer))
		return 0;

	Net_QueueUpstream(qtv, msg.cursize, msg.data);

	return 1;
}

void Clcmd_Say_f(sv_t *qtv, oproxy_t *prox)
{
	char buffer[1024 + 100], text[1024], text2[1024] = {0}, name[MAX_INFO_KEY], *args, *prefix;
	netmsg_t msg;
	int j, ret;
	qbool say_game = false;

	if (isSayFlood(qtv, prox))
		return; // flooder

	// Get rid of quotes
	{
		args = Cmd_Args();

		if (args[0] == '"' && (j = strlen(args)) > 2)
		{
			strlcpy(text2, args + 1, sizeof(text2));
			text2[(int)bound(0, j - 2, sizeof(text2)-1)] = 0;
			args = text2;
		}
	}

	prefix = "";

	if (!stricmp(Cmd_Argv(0), "say_game"))
	{
		say_game = true;	
	}
	else if (!strnicmp(args, "say_game ", sizeof("say_game ") - 1))
	{
		say_game = true;

		args += sizeof("say_game ") - 1;
		prefix = "say_game ";
	}

	if (say_game)
	{
		ret = snprintf(text, sizeof(text), "%s \"%s#%d:%s: %s\"", Cmd_Argv(0), prefix, prox->id, Info_Get(&prox->ctx, "name", name, sizeof(name)), args);
		if (ret >= sizeof(text) || !CmdToUpstream(qtv, text) )
			Sys_Printf("say_game failed\n");

		return;
	}

	ret = snprintf(text, sizeof(text), "#0:qtv_%s:#%d:%s: %s", Cmd_Argv(0), prox->id, Info_Get(&prox->ctx, "name", name, sizeof(name)), args);
	if (ret >= sizeof(text)) {
		Sys_Printf("say failed\n");
		return;
	}

	InitNetMsg(&msg, buffer, sizeof(buffer));

	WriteByte(&msg, 0); // 0 ms
	WriteByte(&msg, dem_all); // msg type
	WriteLong(&msg, strlen(text) + 3); // msg len, txt must be not longer than 1024

	WriteByte 	(&msg, svc_print);
	WriteByte	(&msg, PRINT_CHAT);
	WriteString (&msg, text);

	// FIXME: well, it a good question how to send this to users
	SV_ForwardStream(qtv, (unsigned char *)msg.data, msg.cursize);
}

//============================================================================
// /users
//

void Clcmd_Users_f(sv_t *qtv, oproxy_t *prox)
{
	int c;
	oproxy_t *tmp;
	char name[MAX_INFO_KEY];

	Sys_Printf("%s: userid name\n"
					  "%s: ------ ----\n", qtv->server, qtv->server);

	for (c = 0, tmp = qtv->proxies; tmp; tmp = tmp->next)
	{
		if (tmp->drop)
			continue;

		Sys_Printf("%6d %s\n", tmp->id, Info_Get(&tmp->ctx, "name", name, sizeof(name)));
		c++;
	}	

	Sys_Printf("%i total users\n", c);
}

// { extension which allow have user list on client side

// generate userlist message about "prox" and put in "str"
static void str_qtvuserlist(char *str, size_t str_size, oproxy_t *prox, qtvuserlist_t action, qbool prefix)
{
	char  name[MAX_INFO_KEY];

	switch ( action )
	{
		case QUL_ADD:
		case QUL_CHANGE:
			snprintf(str, str_size, "%squl %d %d \"%s\"\n", prefix ? "//" : "", action, prox->id, Info_Get(&prox->ctx, "name", name, sizeof(name)));

		break;

		case QUL_DEL:
			snprintf(str, str_size, "%squl %d %d\n", prefix ? "//" : "", action, prox->id);

		break;

		default:
			Sys_Printf("str_qtvuserlist: proxy #%d unknown action %d\n", prox->id, action);

		return;		
	}
}

// generate userlist message about "prox" and put in "msg"
static void msg_qtvuserlist(netmsg_t *msg, oproxy_t *prox, qtvuserlist_t action)
{
	char  str[512] = {0};

	str_qtvuserlist(str, sizeof(str), prox, action, true);

	if (!str[0])
		return; // fail of some kind

	WriteByte(msg, svc_stufftext);
	WriteString(msg, str);
}

// send update to upstream about userlist
static void Prox_UpstreamUserListUpdate(sv_t *qtv, oproxy_t *prox, qtvuserlist_t action)
{
	char str[512] = {0};

	str_qtvuserlist(str, sizeof(str), prox, action, false);

	if (!str[0])
		return; // fail of some kind

	CmdToUpstream(qtv, str); // we do not check failure there
}

// send user list to upsteram at connect time
void Prox_UpstreamSendInitialUserList(sv_t *qtv)
{
	oproxy_t *tmp;

	for (tmp = qtv->proxies; tmp; tmp = tmp->next)
	{
		if (tmp->drop)
			continue;

		Prox_UpstreamUserListUpdate(qtv, tmp, QUL_ADD);
	}	
}

// send userlist message about "prox" to all proxies and to upstream too
void Prox_UpdateProxiesUserList(sv_t *qtv, oproxy_t *prox, qtvuserlist_t action)
{
	oproxy_t *tmp;
	char buffer[1024];
	netmsg_t msg;

	// inform upstream too
	Prox_UpstreamUserListUpdate(qtv, prox, action);

	InitNetMsg(&msg, buffer, sizeof(buffer));

	msg_qtvuserlist(&msg, prox, action);

	if (!msg.cursize)
		return;

	// inform clients/downstreams
	for (tmp = qtv->proxies; tmp; tmp = tmp->next)
	{
		if (tmp->drop)
			continue;

		if (tmp->qtv_ezquake_ext & QTV_EZQUAKE_EXT_QTVUSERLIST)
			Prox_SendMessage(&g_cluster, tmp, msg.data, msg.cursize, dem_read, (unsigned)-1);        
	}
}

// send userlist to this "prox", do it once, so we do not send it on each level change
void Prox_SendInitialUserList(sv_t *qtv, oproxy_t *prox)
{
	oproxy_t *tmp;
	char buffer[1024];
	netmsg_t msg;

	// we must alredy have list
	if (prox->connected_at_least_once)
		return;

	// we do not support this extension
	if (!(prox->qtv_ezquake_ext & QTV_EZQUAKE_EXT_QTVUSERLIST))
		return;

	InitNetMsg(&msg, buffer, sizeof(buffer));

	for (tmp = qtv->proxies; tmp; tmp = tmp->next)
	{
		if (tmp->drop)
			continue;

		msg.cursize = 0;

		msg_qtvuserlist(&msg, tmp, QUL_ADD);

		if (!msg.cursize)
			continue;

		Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_read, (unsigned)-1);
	}	
}

// }

//============================================================================
// stuff commands
//

char *stuffed_cmds[] =
{
	"lastscores",
	"follow"
};

const int stuffed_cmds_cnt = sizeof(stuffed_cmds) / sizeof(stuffed_cmds[0]);

void msg_StuffCommand(netmsg_t *msg, const char *cmd)
{
	WriteByte(msg, svc_stufftext);
	WriteString(msg, cmd);
}

// send commands to this "prox", do it once, so we do not send it on each level change
void Prox_StuffCommands(sv_t *qtv, oproxy_t *prox)
{
	char buffer[1024], cmd[1024];
	netmsg_t msg;
	int i;

	// we must alredy have it
	if (prox->connected_at_least_once)
		return;

	// we do not support this extension
	if (!(prox->qtv_ezquake_ext & QTV_EZQUAKE_EXT_DOWNLOAD))
		return;

	InitNetMsg(&msg, buffer, sizeof(buffer));

	for (i = 0; i < stuffed_cmds_cnt; i++)
	{
		msg.cursize = 0;

		snprintf(cmd, sizeof(cmd), "alias %s cmd %s %%0\n", stuffed_cmds[i], stuffed_cmds[i]);
		msg_StuffCommand(&msg, cmd);

		if (!msg.cursize)
			continue;

		Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_read, (unsigned)-1);
	}	
}


//============================================================================
// download
//

/*
==================
Clcmd_CompleteDownoload
==================

This is a sub routine for  Clcmd_NextDownload_f(), called when download complete, we set up some fields for prox.
*/

static void Clcmd_CompleteDownload(sv_t *qtv, oproxy_t *prox)
{
	if (prox->download)
		fclose (prox->download);

	prox->download 		= NULL;
//	prox->downloadsize	= prox->downloadcount = 0;
	prox->file_percent	= 0; //bliP: file percent


	// if map changed tell the client to reconnect
// FIXME: we need do something like prox->flushing = true; instead of commented out code
/*
	if (prox->spawncount != svs.spawncount)
	{
		char *str = "changing\nreconnect\n";

		ClientReliableWrite_Begin (prox, svc_stufftext, strlen(str)+2);
		ClientReliableWrite_String (prox, str);
	}
*/
}

/*
==================
Clcmd_NextDownload_f
==================
*/

static qbool Sub_Clcmd_NextDownload (sv_t *qtv, oproxy_t *prox)
{
	char	read_buf[FILE_TRANSFER_BUF_SIZE];
	int		r, tmp;

	char 	buffer[FILE_TRANSFER_BUF_SIZE + 100];
	netmsg_t msg;

	if (!prox->download)
		return true; // no thx

	InitNetMsg(&msg, buffer, sizeof(buffer));

	tmp = prox->downloadsize - prox->downloadcount;

	r = FILE_TRANSFER_BUF_SIZE;

	if (r > tmp)
		r = tmp;

	if (prox->_buffersize_ + r > 0.5 * prox->_buffermaxsize_)
		return true; // we alredy have too much in buffer, can't add data to buffer

	r = fread (read_buf, 1, r, prox->download);
	prox->downloadcount += r;
	// simple one: if we sent all data, percent is 100, if not all data sent, then percent is not more than 99
	prox->file_percent = ( prox->downloadcount >= prox->downloadsize ? 100 : min(99, 100.0 * prox->downloadcount / max(1, prox->downloadsize)) );

	WriteByte (&msg, svc_download);
	WriteShort(&msg, r);
	WriteByte (&msg, prox->file_percent);
	WriteData (&msg, read_buf, r);

	Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_all, (unsigned)-1);

	if (prox->downloadcount == prox->downloadsize)
	{
		Clcmd_CompleteDownload(qtv, prox);
		return true; // we done all we want
	}

	return false; // mean we can call this function one more time, since seems buffer is not filled enough
}

static void Clcmd_NextDownload_f (sv_t *qtv, oproxy_t *prox)
{
	int i;
//	static unsigned int last = 0;

	if (prox->drop)
		return;

//	Sys_Printf("time %u\n", g_cluster.curtime - last);

	for (i = 0; i < 10; i++)
	{
//		Sys_Printf("n: %d\n", i);
		if (Sub_Clcmd_NextDownload(qtv, prox))
			break;
	}

//	last = g_cluster.curtime;
}


/*
==================
Clcmd_Download_f
==================
*/

static void Clcmd_Download_f(sv_t *qtv, oproxy_t *prox)
{
	char	*name, n[1024], *p;
	qbool	allow_dl = false;
	qbool	demo_requested = false;	
//	qbool	file_from_pak; // ZOID did file come from pak?

	char 	buffer[6];
	netmsg_t msg;
	int i;

	if (Cmd_Argc() != 2)
	{
		Sys_Printf("download [filename]\n");
		return;
	}

	InitNetMsg(&msg, buffer, sizeof(buffer));

	name = Cmd_Argv(1);

	if (!strncmp(name, "demos/", sizeof("demos/")-1))
	{
		demo_requested = true; // well, we change demos/ prefix to something different,
							   // so this is a hint for check below, this was a demo request

		snprintf(n, sizeof(n), "%s/%s", DEMO_DIR, name + sizeof("demos/")-1);

		name = n;
	}

	Sys_ReplaceChar(name, '\\', '/');

	// MUST be in a subdirectory and not absolute path
	if (!allow_download.integer || !FS_SafePath(name)) {
		goto deny_download;
	}

	if (!strstr(name, "/")) {
		allow_dl = false; // should be in subdir
	}
	else if (name[0] == '.') {
		allow_dl = false; // relative is pointless
	}
	else if (((i = strlen(name)) < 3 ? 0 : !strncmp(name + i - 3, "/..", 4))) {
		allow_dl = false; // no /.. at end (should be caught by FS_SafePath)
	}
	else if (strstr(name, "/../")) {
		allow_dl = false; // // no /../ (should be caught by FS_SafePath)
	}
	else if (!strncmp(name, "skins/", sizeof("skins/") - 1)) {
		allow_dl = allow_download_skins.integer;
	}
	else if (!strncmp(name, "progs/", sizeof("progs/") - 1)) {
		allow_dl = allow_download_models.integer;
	}
	else if (!strncmp(name, "sound/", sizeof("sound/") - 1)) {
		allow_dl = allow_download_sounds.integer;
	}
	else if (!strncmp(name, "maps/", sizeof("maps/") - 1)) {
		allow_dl = allow_download_maps.integer;
	}
	else if (demo_requested) {
		allow_dl = allow_download_demos.integer;
	}
	else {
		allow_dl = allow_download_other.integer;
	}

	if (!allow_dl) {
		goto deny_download;
	}

	// close previous download if any
	Clcmd_CompleteDownload(qtv, prox);

	// lowercase name (needed for casesen file systems)
	for (p = name; *p; p++)
		*p = (char)tolower(*p);

	if (demo_requested)
		prox->download = FS_OpenFile(NULL, name, &prox->downloadsize);
	else
		prox->download = FS_OpenFile(qtv->gamedir[0] ? qtv->gamedir : "qw", name, &prox->downloadsize);
	prox->downloadcount = 0;

	if (!prox->download)
	{
//		Sys_Printf("Couldn't download %s\n", name);
		goto deny_download;
	}

/*
	// special check for maps that came from a pak file
	if (!strncmp(name, "maps/", 5) && file_from_pak && !allow_download_pakmaps.integer)
	{
		// close download if any
		Clcmd_CompleteDownoload(qtv, prox);
		goto deny_download;
	}
*/

	// all checks passed, start downloading
	Clcmd_NextDownload_f (qtv, prox);

//	Sys_Printf("Downloading %s\n", name);

	//download info
	Sys_Printf("File %s is %.0fKB (%.2fMB)\n", name, (float)prox->downloadsize / 1024, (float)prox->downloadsize / 1024 / 1024);

	return;

deny_download:

	WriteByte (&msg, svc_download);
	WriteShort(&msg, (unsigned short) -1);
	WriteByte (&msg, 0);

	Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_all, (unsigned)-1);

	return;
}

/*
==================
Clcmd_StopDownload_f
==================
*/
static void Clcmd_StopDownload_f(sv_t *qtv, oproxy_t *prox)
{
	char 	buffer[6];
	netmsg_t msg;

	if (!prox->download)
		return;

	InitNetMsg(&msg, buffer, sizeof(buffer));

	prox->downloadcount = prox->downloadsize;
	fclose (prox->download);
	prox->download = NULL;
	prox->file_percent = 0; //bliP: file percent

	WriteByte (&msg, svc_download);
	WriteShort(&msg, 0);
	WriteByte (&msg, 100);

	Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_all, (unsigned)-1);

	Sys_Printf("Download stopped\n");
}


//============================================================================
// ptrack/follow

static void apply_pov_msg(sv_t *qtv, oproxy_t *p, netmsg_t *msg)
{
	char 	buf[64];

	if (!p->pov)
		return; // FIXME

	if (p->pov)
		snprintf(buf, sizeof(buf), "track %d\n", qtv->players[(int)bound(0, p->pov - 1, MAX_CLIENTS - 1)].userid);
	else
		snprintf(buf, sizeof(buf), "track off\n"); // FIXME

	msg_StuffCommand(msg, buf);
}

static void apply_pov(sv_t *qtv, oproxy_t *p, oproxy_t *only)
{
	oproxy_t *prox;
	char 	buffer[64 + 10];
	netmsg_t msg;

	InitNetMsg(&msg, buffer, sizeof(buffer));
	apply_pov_msg(qtv, p, &msg);

	if (!msg.cursize)
		return;

	for (prox = qtv->proxies; prox; prox = prox->next)
	{
		if (prox->flushing)
			continue;

		if (only && only != prox)
			continue;

		if (prox->follow_id && prox->follow_id == p->id)
			Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_all, (unsigned)-1);
	}
}

static void Clcmd_Ptrack_f(sv_t *qtv, oproxy_t *prox)
{
	if (Cmd_Argc() != 2)
	{
		prox->pov = 0;
		apply_pov(qtv, prox, NULL);
		return;
	}

	prox->pov = atoi(Cmd_Argv(1));

	if (prox->pov < 0 || prox->pov >= MAX_CLIENTS)
	{
		Sys_Printf("Invalid client to track: %d\n", prox->pov);
		prox->pov = 0;
		apply_pov(qtv, prox, NULL);
		return;
	}

	prox->pov++; // and here we increase it

	apply_pov(qtv, prox, NULL);
}

static void Clcmd_Follow_f(sv_t *qtv, oproxy_t *prox)
{
	oproxy_t	*tmp;
	char		name[MAX_INFO_KEY];

	if (Cmd_Argc() != 2)
	{
		prox->follow_id = 0;
		Sys_Printf("follow: turned off\n");
		return;
	}

	tmp = Prox_by_ID_or_Name(qtv, Cmd_Argv(1));

	if (!tmp)
	{
		Sys_Printf("follow: user %s not found\n", Cmd_Argv(1));
		return;
	}

	if (tmp->id == prox->id)
	{
		Sys_Printf("follow: huh? you can't follow yourself\n");
		return;
	}

	prox->follow_id = tmp->id;
	Sys_Printf("follow: %s\n", Info_Get(&tmp->ctx, "name", name, sizeof(name)));

	// try to switch 'proxy pov' to 'tmp pov' immediately
	apply_pov(qtv, tmp, prox);
}

//============================================================================
// new/soundlist/modellist/spawn

/*
static void Clcmd_New_f(sv_t *qtv, oproxy_t *prox)
{
	if (prox->flushing == true)
	{
		Sys_Printf("wrong usage of %s\n", Cmd_Argv(0));
		return; // kidding us?
	}

	prox->flushing = true;
}
*/

static void Clcmd_send_list(sv_t *qtv, oproxy_t *prox, int svc)
{
	char buffer[MAX_MSGLEN*8];
	netmsg_t msg;
	int prespawn;

	if (!prox->flushing)
	{
		Sys_Printf("wrong usage of %s\n", Cmd_Argv(0));
		return; // kidding us?
	}

	if (qtv->qstate != qs_active || !qtv->server_data_parsed)
	{
		Sys_Printf("%s: proxy #%i not ready for %s\n", qtv->server, prox->id, Cmd_Argv(0));
		prox->flushing = true; // so they get Net_SendConnectionMVD() ASAP later
		return;
	}

	InitNetMsg(&msg, buffer, sizeof(buffer));

	prox->flushing = false; // so we can catch overflow, set it to appropriate values below

	for (prespawn = 0;prespawn >= 0;)
	{
		prespawn = SendList(qtv, prespawn, svc == svc_soundlist ? qtv->soundlist : qtv->modellist, svc, &msg);
		Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_read, (unsigned)-1);
		msg.cursize = 0;
	}

	if (prox->flushing)
	{
		Sys_Printf("Connection data is too big, dropping proxy client #i.\n", prox->id);
		prox->drop = true;	//this is unfortunate...
	}
	else
		prox->flushing = (svc == svc_soundlist ? PS_WAIT_MODELLIST : PS_WAIT_SPAWN);
}

static void Clcmd_Soundlist_f(sv_t *qtv, oproxy_t *prox)
{
	if (prox->flushing != PS_WAIT_SOUNDLIST)
	{
		Sys_Printf("%s: wrong usage of %s by proxy #%i.\n", qtv->server, Cmd_Argv(0), prox->id);
		return;
	}

	Clcmd_send_list(qtv, prox, svc_soundlist);
}

static void Clcmd_Modellist_f(sv_t *qtv, oproxy_t *prox)
{
	if (prox->flushing != PS_WAIT_MODELLIST)
	{
		Sys_Printf("%s: wrong usage of %s by proxy #%i.\n", qtv->server, Cmd_Argv(0), prox->id);
		return;
	}

	Clcmd_send_list(qtv, prox, svc_modellist);
}

static void Clcmd_Spawn_f(sv_t *qtv, oproxy_t *prox)
{
	char buffer[MAX_MSGLEN*8];
	netmsg_t msg;
	int prespawn;

	if (prox->flushing != PS_WAIT_SPAWN)
	{
		// ignore warning if different spawn count, since its probably ok
		if (atoi(Cmd_Argv(1)) == qtv->clservercount)
			Sys_Printf("%s: wrong usage of %s by proxy #%i.\n", qtv->server, Cmd_Argv(0), prox->id);

		return;
	}

	if (qtv->qstate != qs_active || !qtv->server_data_parsed)
	{
		Sys_Printf("%s: proxy not ready for %s\n", qtv->server, Cmd_Argv(0), prox->id);
		prox->flushing = true; // so they get Net_SendConnectionMVD() ASAP later
		return;
	}

	InitNetMsg(&msg, buffer, sizeof(buffer));

	prox->flushing = false; // so we can catch overflow

	for(prespawn = 0;prespawn>=0;)
	{
		prespawn = Prespawn(qtv, 0, &msg, prespawn, MAX_CLIENTS-1);

		Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_read, (unsigned)-1);
		msg.cursize = 0;
	}

	// playerstates arn't actually delta-compressed, so the first send (simply forwarded from server) entirly replaces the old.
	// not really, we need send something from what client/other proxy/whatever will deltaing
	// at least model index is really required, otherwise we got invisible models!
	Prox_SendInitialPlayers(qtv, prox, &msg);
	Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_read, (unsigned)-1);
	msg.cursize = 0;

	//we do need to send players stats
	Prox_SendPlayerStats(qtv, prox);

	//we do need to send entity states.
	Prox_SendInitialEnts(qtv, prox, &msg);
	Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_read, (unsigned)-1);
	msg.cursize = 0;

	WriteByte(&msg, svc_stufftext);
	WriteString(&msg, "skins\n");
	Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_read, (unsigned)-1);
	msg.cursize = 0;

	// if game is paused, inform proxy
	if (qtv->paused) {
		WriteByte(&msg, svc_setpause);
		WriteByte(&msg, qtv->paused);
		Prox_SendMessage(&g_cluster, prox, msg.data, msg.cursize, dem_read, (unsigned)-1);
		msg.cursize = 0;
	}

	Net_TryFlushProxyBuffer(&g_cluster, prox);

	if (prox->flushing)
	{
		Sys_Printf("%s: Connection data is too big, dropping proxy client #%i.\n", qtv->server, prox->id);
		prox->drop = true;	//this is unfortunate...
	}
	else
	{
		prox->connected_at_least_once = true;
		Net_TryFlushProxyBuffer(&g_cluster, prox);
	}
}
//============================================================================

//Allow clients to change userinfo
static void Clcmd_SetInfo_f (sv_t *qtv, oproxy_t *prox)
{
	char oldval[MAX_INFO_KEY], newval[MAX_INFO_KEY];

	if (Cmd_Argc() == 1) {
		Sys_Printf("%s: User info settings (proxy #%i):\n", qtv->server, prox->id);
		Info_PrintList(&prox->ctx);
		return;
	}

	if (Cmd_Argc() != 3) {
		Sys_Printf("%s: Usage: setinfo [ <key> <value> ] (proxy #%i)\n", qtv->server, prox->id);
		return;
	}

// we don't have such limitations atm in QTV?
//	if (Cmd_Argv(1)[0] == '*')
//		return;		// don't set priveledged values

	Info_Get(&prox->ctx, Cmd_Argv(1), oldval, sizeof(oldval)); // save current as old

	Info_Set(&prox->ctx, Cmd_Argv(1), Cmd_Argv(2)); // set new value

	if (!strcmp(Info_Get(&prox->ctx, Cmd_Argv(1), newval, sizeof(newval)), oldval)) // compare new and old
		return; // key hasn't changed

	// process any changed values
	if (!strcmp(Cmd_Argv(1), "name"))
	{
		Prox_FixName(qtv, prox);

		Prox_UpdateProxiesUserList(qtv, prox, QUL_CHANGE);
	}
}
//============================================================================

static void Clcmd_LastScores_f (sv_t *qtv, oproxy_t *prox)
{
	int i, j, cnt;
	int k_ls = bound(0, qtv->lastscores_idx, MAX_LASTSCORES-1);
	char *e1, *e2, *le1, *le2, *s1, *s2, *map, *last, *cur, *date;

	le1 = le2 = last = "";

	for ( j = k_ls, cnt = i = 0; i < MAX_LASTSCORES; i++, j = (j+1 >= MAX_LASTSCORES) ? 0: j+1 ) {

		date= qtv->lastscores[j].date;
		cur = qtv->lastscores[j].type;
		map = qtv->lastscores[j].map;
		e1  = qtv->lastscores[j].e1;
		s1  = qtv->lastscores[j].s1;
		e2  = qtv->lastscores[j].e2;
		s2  = qtv->lastscores[j].s2;

		if ( !date[0] || !cur[0] || !e1[0] || !e2[0] )
			continue;

		if (    strcmp(cur, last) // changed game mode
			 || (strcmp(le1, e1) || strcmp(le2, e2)) // changed teams, duelers
		   )
		{
			Sys_Printf("\x90%s \366\363 %s\x91 %s\n", e1, e2, cur); // [dag vs qqshka] duel
		}

		Sys_Printf("   %3s:%-3s \x8D %-8.8s %s\n", s1, s2, map, date); // -5:100 > dm6

		last = cur;
		le1 = e1;
		le2 = e2;
		cnt++;
	}

	if ( cnt )
		Sys_Printf("\n"
		"Lastscores: %d entr%s found\n", cnt, (cnt ? "y" : "ies"));
	else
		Sys_Printf("Lastscores data empty\n");
}



//============================================================================

char cl_cmd[MAX_PROXY_INBUFFER]; // global so it does't allocated on stack, this save some CPU I think


typedef struct
{
	char	*name;
	void	(*func) (sv_t *qtv, oproxy_t *prox);
}
ucmd_t;

static ucmd_t ucmds[] =
{
	{"say", 			Clcmd_Say_f},
	{"say_team",		Clcmd_Say_f},
	{"say_game",		Clcmd_Say_f},

	{"setinfo",			Clcmd_SetInfo_f},

	{"users",			Clcmd_Users_f},

	{"lastscores",		Clcmd_LastScores_f},

	{"download",		Clcmd_Download_f},
// no, we do it different way, no need to spam it
//	{"nextdl",			Clcmd_NextDownload_f},
	{"stopdl",			Clcmd_StopDownload_f},

	{"ptrack",			Clcmd_Ptrack_f},
	{"follow",			Clcmd_Follow_f},

// "qul" - is a command for user list update.
// We does not process user list update from downstream.
// This could happend in case of chaining (downstream is QTV).
	{"qul",				NULL},

//	{"qtvnew",			Clcmd_New_f},
	{"qtvsoundlist",	Clcmd_Soundlist_f},
	{"qtvmodellist",	Clcmd_Modellist_f},
	{"qtvspawn",		Clcmd_Spawn_f},

// argh, server send it in mvd demo too, FIXME

	{"new",				NULL},
	{"soundlist",		NULL},
	{"modellist",		NULL},
	{"spawn",			NULL},
	{"prespawn",		NULL},
	{"begin",			NULL},

	{NULL, NULL}
};

void Proxy_ExecuteClCmd(sv_t *qtv, oproxy_t *prox, char *cmd)
{
    char *arg0, result[1024 * 8] = {0};
    qbool found = false;
	ucmd_t *u;

//	Sys_Printf("cl cmd: %s\n", cmd);

	Cmd_TokenizeString (cmd);

	arg0 = Cmd_Argv(0);

	Sys_RedirectStart(result, sizeof(result));

	for (u = ucmds; u->name; u++)
	{
		if (!strcmp(arg0, u->name))
		{
			if (u->func)
				u->func(qtv, prox);
			found = true;
			break;
		}
	}

	if (!found)
		Sys_Printf("%s (proxy #%3i): Bad user command: %s\n", qtv->server, prox->id, arg0);

	Sys_RedirectStop();

	if (result[0])
		Prox_Printf(&g_cluster, prox, dem_all, (unsigned)-1, PRINT_HIGH, "%s\n", result);
}

void Proxy_ReadInput(sv_t *qtv, oproxy_t *prox)
{
	int len, parse_end, clc;
	netmsg_t buf;

	if (prox->drop)
		return;

//	if (prox->flushing)
//		return;

	len = sizeof(prox->inbuffer) - prox->inbuffersize - 1; // -1 since it null terminated

	if (len)
	{
		len = recv(prox->sock, ((char *) prox->inbuffer) + prox->inbuffersize, len, 0);
		SV_ProxySocketIOStats(prox, len, 0);

		if (len == 0)
		{
			Sys_Printf("%s (proxy #%3i): Read error from QTV client, dropping.\n", qtv->server, prox->id);
			prox->drop = true;
			return;
		}
		else if (len < 0)
		{
//			return;
			len = 0;
		}

		prox->inbuffersize += len;
		prox->inbuffer[prox->inbuffersize] = 0; // null terminated
	}

	if (prox->inbuffersize < 2)
		return; // we need at least size

	InitNetMsg(&buf, (char *)prox->inbuffer, prox->inbuffersize);
	buf.cursize		= prox->inbuffersize;

	parse_end 		= 0;

	while(buf.readpos < buf.cursize)
	{
//		Sys_Printf("%d %d\n", buf.readpos, buf.cursize);

		if (buf.readpos > buf.cursize)
		{
			prox->drop = true;
			Sys_Printf("%s (proxy #%3i): Proxy_ReadInput: Read past end of parse buffer.\n", qtv->server, prox->id);
			return;
		}

		buf.startpos = buf.readpos;

		if (buf.cursize - buf.startpos < 2)
			break; // we need at least size

		len = ReadShort(&buf);

		if (len > (int)sizeof(prox->inbuffer) - 1 || len < 3)
		{
			prox->drop = true;
			Sys_Printf("%s (proxy #%3i): Proxy_ReadInput: can't handle such long/short message: %i\n", qtv->server, prox->id, len);
			return;
		}

		if (len > buf.cursize - buf.startpos)
			break; // not enough data yet

		parse_end = buf.startpos + len; // so later we know which part of buffer we alredy served

		switch (clc = ReadByte(&buf))
		{
			case qtv_clc_stringcmd:
				cl_cmd[0] = 0;
				ReadString(&buf, cl_cmd, sizeof(cl_cmd));
				Proxy_ExecuteClCmd(qtv, prox, cl_cmd);
				break;

			default:
				prox->drop = true;
				Sys_Printf("%s (proxy #%3i): Proxy_ReadInput: can't handle clc %i\n", qtv->server, prox->id, clc);
				return;
		}
	}

	if (parse_end)
	{
		prox->inbuffersize -= parse_end;
		memmove(prox->inbuffer, prox->inbuffer + parse_end, prox->inbuffersize);
	}
}

void Proxy_ReadProxies(sv_t *qtv)
{
	oproxy_t *prox;

	for (prox = qtv->proxies; prox; prox = prox->next)
	{
		Proxy_ReadInput(qtv, prox);

		if (prox->download)
			Clcmd_NextDownload_f(qtv, prox);
	}
}

//============================================================================

void Cl_Cmds_Init(void)
{
	Cvar_Register(&floodprot);

	Cvar_Register(&allow_download);
	Cvar_Register(&allow_download_skins);
	Cvar_Register(&allow_download_models);
	Cvar_Register(&allow_download_sounds);
	Cvar_Register(&allow_download_maps);
//	Cvar_Register(&allow_download_pakmaps);
	Cvar_Register(&allow_download_demos);
	Cvar_Register(&allow_download_other);

	Cvar_Register(&demo_dir);
}
