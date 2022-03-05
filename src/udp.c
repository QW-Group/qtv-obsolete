/*
UDP protocl related things. At the moment just support "status" connectionless feature.
*/

#include "qtv.h"
#include "time.h"

// out of band message id bytes (check protocol.h in QW server for full list)
// M = master, S = server, C = client, A = any
#define	A2C_PRINT			'n'	// print a message on client
#define	A2A_PING			'k'	// respond with an A2A_ACK
#define	A2A_ACK				'l'	// general acknowledgement without info
#define	S2M_HEARTBEAT		'a'	// + serverinfo + userlist + fraglist

//====================================================================

#define QW_MASTERS_FORCE_RE_INIT (60 * 60 * 24) // seconds, force re-init masters time to time, so we add proper masters if there was some ip/dns changes
#define QW_MASTER_HEARTBEAT_SECONDS (60 * 5) // seconds, frequency of heartbeat

#define QW_DEFAULT_MASTER_SERVERS "qwmaster.ocrana.de masterserver.exhale.de master.quakeworld.nu asgaard.morphos-team.net"
#define QW_DEFAULT_MASTER_SERVER_PORT 27000

static cvar_t masters_list = {"masters", QW_DEFAULT_MASTER_SERVERS};

//====================================================================

void SV_CheckUDPPort(cluster_t *cluster, int port)
{
	// remember current time so we does not hammer it too fast.
	cluster->udpport_last_time_check = cluster->curtime;

	// close socket first.
	if (cluster->udpsocket != INVALID_SOCKET)
	{
		closesocket(cluster->udpsocket);
		cluster->udpsocket = INVALID_SOCKET;

		Sys_Printf("%s is now closed\n", "updport");
	}

	// open socket if required.
	if (port)
	{
		SOCKET news = Net_UDPOpenSocket(port);

		if (news != INVALID_SOCKET)
		{
			cluster->udpsocket = news;

			Sys_Printf("%s %d opened\n", "updport", port);
		}
		else
		{
			Sys_Printf("%s %d failed to open\n", "updport", port);
		}
	}
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
static void SVC_Ping (cluster_t *cluster)
{
	char data = A2A_ACK;
	Net_SendPacket(cluster, 1, &data, &cluster->net_from);
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
#define STATUS_OLDSTYLE					0
#define	STATUS_SERVERINFO				1
#define	STATUS_PLAYERS					2
#define	STATUS_SPECTATORS				4
//#define STATUS_SPECTATORS_AS_PLAYERS	8 //for ASE - change only frags: show as "S"
//#define STATUS_SHOWTEAMS				16

static void SVC_Status (cluster_t *cluster)
{
	static char buf_data[MSG_BUF_SIZE]; // static  - so it not allocated each time
	static char tmp[1024 + 1]; // static too

	netmsg_t buf;
	int opt = (Cmd_Argc() > 1) ? atoi(Cmd_Argv(1)) : 0; // there possible to provide options for status command.

	InitNetMsg(&buf, buf_data, sizeof(buf_data));

	WriteLong(&buf, (unsigned int)-1);	// -1 sequence means out of band
	WriteByte(&buf, A2C_PRINT);

	if (opt == STATUS_OLDSTYLE || (opt & STATUS_SERVERINFO))
	{
		snprintf(tmp, sizeof(tmp), "%s\n", cluster->info);
		WriteString2(&buf, tmp);
	}

	if (opt == STATUS_OLDSTYLE || (opt & (STATUS_PLAYERS | STATUS_SPECTATORS)))
	{
		int top = 0, bottom = 0, ping = 666, connect_time;
		char *frags = "0", *skin = "";
		sv_t *stream;
		oproxy_t *prx;
		char name[MAX_INFO_KEY];

		for (stream = cluster->servers; stream; stream = stream->next)
		{
			for (prx = stream->proxies; prx; prx = prx->next)
			{
				if (prx->drop)
					continue;

				Info_Get(&prx->ctx, "name", name, sizeof(name));
				connect_time = (int)((cluster->curtime - prx->init_time)/1000/60); // milliseconds to seconds and then to minutes.

				snprintf(tmp, sizeof(tmp), "%i %s %i %i \"%s\" \"%s\" %i %i\n", prx->id, frags, connect_time, ping, name, skin, top, bottom);

				WriteString2(&buf, tmp);
			}
		}
	}

	if (buf.cursize>=buf.maxsize)
	{
		Sys_DPrintf("SVC_Status: buffer overflowed %u/%u\n", buf.cursize, buf.maxsize);
		return; // overflowed
	}

	// send the datagram
	Net_SendPacket(cluster, buf.cursize, buf.data, &cluster->net_from);
}

static void SV_ConnectionlessPacket(cluster_t *cluster)
{
	char	cmd[128] = ""; // nor more than 128 bytes. I don't know such long commands anyway.
	char	*c;

	// skip the -1 marker
	ReadLong(&cluster->net_message);
	// Red string in.
	ReadString(&cluster->net_message, cmd, sizeof(cmd));
	// Tokenize it.
	Cmd_TokenizeString(cmd);

	c = Cmd_Argv(0);

	if (!strcmp(c, "ping") || (c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')))
		SVC_Ping(cluster);
	else if (!strcmp(c, "status"))
		SVC_Status(cluster);
	else
		Sys_DPrintf("bad connectionless packet from %s:\n%s\n", inet_ntoa(cluster->net_from.sin_addr), cmd);
}

static void SV_ReadPackets(cluster_t *cluster)
{
	// now deal with new packets
	while (Net_GetPacket(cluster, &cluster->net_message))
	{
		if (SV_IsBanned(&cluster->net_from))
		{
//			SV_SendBan ();	// tell them we aren't listening...
			continue;
		}

		// check for connectionless packet (0xffffffff) first
		if (*(int *)cluster->net_message.data == -1)
		{
			SV_ConnectionlessPacket(cluster);
			continue;
		}
	}
}

//====================================================================
// REGISTRATION ON MASTER
//====================================================================

static master_t	*QRY_Master_ByAddr(cluster_t *cluster, struct sockaddr_in *addr)
{
	int						i;
	master_t				*m;

	for (i = 0, m = cluster->masters.master; i < MAX_MASTERS; i++, m++)
	{
		if (m->state != ms_used)
			continue; // master slot unused

		if (Net_CompareAddress(addr, &m->addr))
			return m;
	}

	return NULL;
}

static qbool QRY_AddMaster(cluster_t *cluster, const char *master)
{
	int						i;
	master_t				*m;
	struct sockaddr_in		addr;

	if (!Net_StringToAddr(&addr, master, QW_DEFAULT_MASTER_SERVER_PORT))
	{
		Sys_Printf("failed to add master server: %s\n", master);
		return false;
	}

	if (QRY_Master_ByAddr(cluster, &addr))
	{
		Sys_Printf("failed to add master server: %s - already added!\n", master);
		return false;
	}

	for (i = 0, m = cluster->masters.master; i < MAX_MASTERS; i++, m++)
	{
		if (m->state == ms_used)
			continue; // master slot used

		memset(m, 0, sizeof(*m)); // reset data in slot

		m->state = ms_used;
		m->addr = addr;

		Sys_Printf("master server added: %s\n", master);
		return true;
	}

	Sys_Printf("failed to add master server: %s\n", master);
	return false;
}

static void QRY_Force_Heartbeat(cluster_t *cluster)
{
	cluster->masters.last_heartbeat = time(NULL) - QW_MASTER_HEARTBEAT_SECONDS - 1; // trigger heartbeat ASAP
}

// clear masters
static void QRY_MastersInit(cluster_t *cluster)
{
	memset(&cluster->masters, 0, sizeof(cluster->masters));
	cluster->masters.init_time = time(NULL);

	QRY_Force_Heartbeat(cluster);  // trigger heartbeat ASAP
}

// check if "masters" cvar changed and do appropriate action
static void QRY_CheckMastersModified(cluster_t *cluster)
{
	char *mlist;

	// for fix issues with DNS and such force masters re-init time to time
	if (time(NULL) - cluster->masters.init_time > QW_MASTERS_FORCE_RE_INIT)
	{
		Sys_DPrintf("forcing masters re-init\n");
		masters_list.modified = true;
	}

	// "masters" cvar was not modified, do nothing
	if (!masters_list.modified)
		return;

	// clear masters
	QRY_MastersInit(cluster);

	// add all masters
	for ( mlist = masters_list.string; (mlist = COM_Parse(mlist)); )
	{
		QRY_AddMaster(cluster, com_token);
	}

	masters_list.modified = false;
}

// heartbeat master servers.
// send a message to the master every few minutes.
static void QRY_HeartbeatMasters(cluster_t *cluster)
{
	char		string[128];
	int			i, len;
	master_t	*m;
	time_t		current_time = time(NULL);
	char		buf[] = "xxx.xxx.xxx.xxx:xxxxx";

	if (current_time < cluster->masters.last_heartbeat + QW_MASTER_HEARTBEAT_SECONDS // not right time.
		|| cluster->udpsocket == INVALID_SOCKET // socket invalid.
	)
		return; // not yet
	
	cluster->masters.last_heartbeat = current_time;
	cluster->masters.heartbeat_sequence++;
	snprintf(string, sizeof(string), "%c\n%i\n%i\n", S2M_HEARTBEAT, cluster->masters.heartbeat_sequence, cluster->numproxies);
	len = strlen(string);

	if (developer.integer > 1)
		Sys_DPrintf("heartbeat:\n%s\n", string);

	for (i = 0, m = cluster->masters.master; i < MAX_MASTERS; i++, m++)
	{
		if (m->state != ms_used)
			continue; // master slot not used
		
		Sys_DPrintf("heartbeat master: %s\n", Net_AdrToString(&m->addr, buf, sizeof(buf)));
		Net_SendPacket(cluster, len, string, &m->addr);
	}
}

//====================================================================

void SV_UDP_Frame(cluster_t *cluster)
{
	// Process UDP packets.
	// NOTE: WE DO NOT select() on UDP socket, so it really suck in terms of responsivness,
	// but it should not matter as long as we process connectionless packets ONLY.
	// We do not select() on UDP sockets because we do not want QTV wake up too frequently.
	SV_ReadPackets(cluster);
	// check is "masters" variable changed
	QRY_CheckMastersModified(cluster);
	// send heartbeat to masters time to time
	QRY_HeartbeatMasters(cluster);
}

//====================================================================

static void heartbeat_f(void)
{
	QRY_Force_Heartbeat(&g_cluster);
}

void UDP_Init(void)
{
	// Init net_message.
	InitNetMsg(&g_cluster.net_message, g_cluster.net_message_buffer, sizeof(g_cluster.net_message_buffer));

	Cvar_Register(&masters_list);
	Cmd_AddCommand("heartbeat", heartbeat_f);

	// clear masters
	QRY_MastersInit(&g_cluster);
}
