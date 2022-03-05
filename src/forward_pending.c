/*
	forward_pending.c
*/

#include "qtv.h"

cvar_t mvdport          = {"mvdport", PROX_DEFAULT_LISTEN_PORT};
cvar_t maxclients       = {"maxclients", "1000", CVAR_SERVERINFO};
cvar_t allow_http       = {"allow_http", "1"};
cvar_t qtv_password     = {"qtv_password", ""};

int get_maxclients(void)
{
	return bound(0, maxclients.integer, MAX_QTV_CLIENTS);
}

// this just can't be done as macro, so I wrote this function
char *QTV_SV_HEADER(oproxy_t *prox, float qtv_ver)
{
	static char header[1024];

	int ext = (prox->qtv_ezquake_ext & QTV_EZQUAKE_EXT_NUM);

	if (ext)
	{
		snprintf(header, sizeof(header), "QTVSV %g\n" QTV_EZQUAKE_EXT ": %d\n", qtv_ver, ext);
	}
	else
	{
		snprintf(header, sizeof(header), "QTVSV %g\n", qtv_ver);
	}

	return header;
}

static void SV_ReadSourceListRequest(cluster_t *cluster, oproxy_t *pend)
{
	sv_t *qtv = NULL;

	// Lists sources that are currently playing.
	Net_ProxyPrintf(pend, "%s", QTV_SV_HEADER(pend, QTV_VERSION));

	if (!cluster->servers)
	{
		Net_ProxyPrintf(pend, "PERROR: No sources currently available\n");
	}
	else
	{
		for (qtv = cluster->servers; qtv; qtv = qtv->next)
			Net_ProxyPrintf(pend, "ASOURCE: %i: %15s: %15s\n", qtv->streamid, qtv->server, qtv->hostname);
	}

	Net_ProxyPrintf(pend, "\n");
	pend->flushing = true;
}

static sv_t *SV_ReadReceiveRequest(cluster_t *cluster, oproxy_t *pend)
{
	sv_t *qtv = NULL;

	// A client connection request without a source.
	if (cluster->NumServers == 1)
	{	
		// Only one stream anyway.
		qtv = cluster->servers;
	}
	else
	{	
		// Try and hunt down an explicit stream (rather than a user-recorded one).
		int numfound = 0;
		sv_t *suitable = NULL;
		
		for (qtv = cluster->servers; qtv; qtv = qtv->next)
		{
			if (!qtv->DisconnectWhenNooneIsWatching)
			{
				suitable = qtv;
				numfound++;
			}
		}

		if (numfound == 1)
			qtv = suitable;
	}

	if (!qtv)
	{
		Net_ProxyPrintf(pend, "%s"
							  "PERROR: Multiple streams are currently playing\n\n",
							  QTV_SV_HEADER(pend, QTV_VERSION));
		pend->flushing = true;
	}

	return qtv;
}

static void SV_ReadDemoListRequest(cluster_t *cluster, oproxy_t *pend)
{
	// Lists demos that are available.
	int i;

	pend->_bufferautoadjustmaxsize_ = 1024 * 1024; // NOTE: this allow 1MB buffer...

	Cluster_BuildAvailableDemoList(cluster);

	Net_ProxyPrintf(pend, "%s", QTV_SV_HEADER(pend, QTV_VERSION));

	if (!cluster->availdemoscount)
	{
		Net_ProxyPrintf(pend, "PERROR: No demos currently available\n");
	}
	else
	{
		for (i = 0; i < cluster->availdemoscount; i++)
		{
			Net_ProxyPrintf(pend, "ADEMO: %i: %15s\n", cluster->availdemos[i].size, cluster->availdemos[i].name);
		}
	}

	Net_ProxyPrintf(pend, "\n");
	pend->flushing = true;
}

static sv_t *SV_ReadSourceRequest(cluster_t *cluster, const char *sourcename)
{
	char *s = NULL;
	sv_t *qtv = NULL;

	// Connects, creating a new source.
	while (*sourcename == ' ')
		sourcename++;
	for (s = (char *)sourcename; *s; s++)
	{
		if (*s < '0' || *s > '9')
			break;
	}

	if (*s)
	{
		qtv = QTV_NewServerConnection(cluster, sourcename, "", false, true, true, false);
	}
	else
	{
		// Numerical source, use a stream id.
		for (qtv = cluster->servers; qtv; qtv = qtv->next)
		{
			if (qtv->streamid == (unsigned int) atoi(sourcename))
				break;
		}
	}

	return qtv;
}

static sv_t *SV_ReadDemoRequest(cluster_t *cluster, oproxy_t *pend, const char *demoname)
{
	// Starts a demo off the server... source does the same thing though...
	char buf[256];
	sv_t *qtv = NULL;

	snprintf(buf, sizeof(buf), "file:%s", demoname);
	qtv = QTV_NewServerConnection(cluster, buf, "", false, true, true, false);
	if (!qtv)
	{
		Net_ProxyPrintf(pend, "%s"
							  "PERROR: couldn't open demo\n\n",
							  QTV_SV_HEADER(pend, QTV_VERSION));
		pend->flushing = true;
	}

	return qtv;
}

static float SV_ReadVersionRequest(const char *version)
{
	float usableversion = atof(version);

	switch ((int)usableversion)
	{
		case 1:
			// Got a usable version.
			break;
		default:
			// Not recognised.
			usableversion = 0;
			break;
	}

	return usableversion;
}

static void SV_CheckProxyTimeout(cluster_t *cluster, oproxy_t *pend)
{
	if (pend->io_time + max(10 * 1000, 1000 * downstream_timeout.integer) <= cluster->curtime)
	{
		Sys_DPrintf("SV_ReadPendingProxy: id #%d, pending stream timeout, dropping\n", pend->id);
		if (developer.integer > 1)
			Sys_DPrintf("SV_ReadPendingProxy: inbuffer: %s\n", pend->inbuffer);

		pend->drop = true;
	}
}

static void SV_ReadUntilDrop(oproxy_t *pend)
{
	// Peform reading on buffer file if we have an empty buffer.
	if (!pend->_buffersize_ && pend->buffer_file)
	{
		pend->_buffersize_ += fread(pend->_buffer_, 1, pend->_buffermaxsize_, pend->buffer_file);

		if (!pend->_buffersize_)
		{
			fclose(pend->buffer_file);
			pend->buffer_file = NULL;
		}
	}

	// Ok we have empty buffer, now we can drop.
	if (!pend->_buffersize_) 
	{
		Sys_DPrintf("SV_ReadPendingProxy: id #%d, empty buffer, dropping\n", pend->id);
		if (developer.integer > 1)
			Sys_DPrintf("SV_ReadPendingProxy: inbuffer: %s\n", pend->inbuffer);

		pend->drop = true;
	}
}

//
// Receives a request from a stream client and makes sure it's something we recognize (HTTP or QTV).
//
static qbool SV_ReceivePendingProxyRequest(cluster_t *cluster, oproxy_t *pend)
{
	int len;
	char *s = NULL;
	char *inbuf = (char *)pend->inbuffer;

	len = sizeof(pend->inbuffer) - pend->inbuffersize - 1;
	len = recv(pend->sock, (char *) pend->inbuffer + pend->inbuffersize, len, 0);
	SV_ProxySocketIOStats(pend, len, 0);
	
	// Remote side closed connection.
	if (len == 0)
	{
		Sys_DPrintf("SV_ReadPendingProxy: id #%d, remote side closed connection, dropping\n", pend->id);
		if (developer.integer > 1)
			Sys_DPrintf("SV_ReadPendingProxy: inbuffer: %s\n", pend->inbuffer);

		pend->drop = true;
		return false;
	}

	// No new data, or read error.
	if (len < 0) 
	{
		return false;
	}

	pend->io_time = cluster->curtime; // Update IO activity.

	pend->inbuffersize += len;
	pend->inbuffer[pend->inbuffersize] = 0; // So strings functions are happy.

	if (pend->inbuffersize < 5)
		return false; 	// Don't have enough yet.

	// Make sure we have a request we understand.
	if (     strncmp(inbuf, "QTV\r", 4)
		&&   strncmp(inbuf, "QTV\n", 4)
		&& ( !allow_http.integer 
    			|| (    strncmp(inbuf, "GET ",  4)
    				 && strncmp(inbuf, "POST ", 5)
    			   )
		   )
	   )
	{	
		Sys_DPrintf("SV_ReadPendingProxy: id #%d, unknown client, dropping\n", pend->id);
		if (developer.integer > 1)
			Sys_DPrintf("SV_ReadPendingProxy: inbuffer: %s\n", pend->inbuffer);
	
		// I have no idea what the smeg you are.
		pend->drop = true;
		return false;
	}

	// Make sure there's a double \n somewhere.
	for (s = inbuf; *s; s++)
	{
		if (s[0] == '\n' && (s[1] == '\n' || (s[1] == '\r' && s[2] == '\n')))
			break;
	}

	if (!*s)
		return false; // Don't have enough yet.

	return true;
}

static qbool SV_CheckForHTTPRequest(cluster_t *cluster, oproxy_t *pend)
{
	char *inbuf = (char *)pend->inbuffer;

	if (!strncmp(inbuf, "POST ", 5))
	{
		HTTPSV_PostMethod(cluster, pend);

		return false;	// Not keen on this..
	}
	else if (!strncmp(inbuf, "GET ", 4))
	{
		HTTPSV_GetMethod(cluster, pend);

		pend->flushing = true;

		return false;
	}

	return true;
}

typedef enum {
	QTVAM_NONE,
	QTVAM_PLAIN,
	QTVAM_CCITT,
	QTVAM_MD4,
	QTVAM_SHA3_512,
} authmethod_t;

static qbool SV_QTVValidateAuthentication(authmethod_t authmethod, const char* password_supplied, const char* authchallenge, const char* our_password)
{
	// Server doesn't require password...
	if (our_password == NULL || our_password[0] == '\0') {
		return true;
	}

	// Client didn't supply password...
	if (password_supplied == NULL || password_supplied[0] == '\0') {
		return false;
	}

	// Client didn't supply authentication method...
	if (authmethod == QTVAM_NONE) {
		return false;
	}

	if (authmethod == QTVAM_PLAIN) {
		return strcmp(password_supplied, our_password) == 0;
	}

	if (authmethod == QTVAM_CCITT) {
		short crc;

		crc = QCRC_Block((unsigned char*)authchallenge, strlen(authchallenge));
		crc = QCRC_Block_Continue((unsigned char*)our_password, strlen(our_password), crc);

		return (crc == atoi(password_supplied));
	}

	if (authmethod == QTVAM_MD4) {
		char hash[512];
		int md4sum[4];

		snprintf(hash, sizeof(hash), "%s%s", authchallenge, our_password);
		Com_BlockFullChecksum(hash, strlen(hash), (unsigned char*)md4sum);
		snprintf(hash, sizeof(hash), "%X%X%X%X", md4sum[0], md4sum[1], md4sum[2], md4sum[3]);
		return strcmp(password_supplied, hash) == 0;
	}

	if (authmethod == QTVAM_SHA3_512) {
		sha3_context c;
		const uint8_t *byte_hash;
		char hex_hash[SHA3_512_DIGEST_HEX_STR_SIZE] = {0};

		sha3_Init512(&c);
		sha3_Update(&c, authchallenge, strlen(authchallenge));
		sha3_Update(&c, our_password, strlen(our_password));
		byte_hash = sha3_Finalize(&c);
		sha3_512_ByteToHex(hex_hash, byte_hash);
		return strcmp(password_supplied, hex_hash) == 0;
	}


	// Unknown authentication method
	return false;
}

static qbool SV_CheckForQTVRequest(cluster_t *cluster, oproxy_t *pend)
{
	qbool raw = false;
	sv_t *qtv = NULL;
	char *colon = NULL;
	char userinfo[sizeof(pend->inbuffer)] = {0};
	float usableversion = 0;
	int parse_end = 0;
	char *e = (char *)pend->inbuffer;
	char *s = e;
	char password[256] = { 0 };
	authmethod_t authmethod = QTVAM_NONE;

	// Parse a QTV request.
	while (*e && parse_end == 0)
	{
		if (*e == '\n' || *e == '\r')
		{
			if (e[0] == '\n' && e[1] == '\n')
			{
				parse_end = e + 2 - (char *)pend->inbuffer;
			}
			else if (e[0] == '\n' && e[1] == '\r' && e[2] == '\n')
			{
				parse_end = e + 3 - (char *)pend->inbuffer;
			}

			*e = '\0';
			colon = strchr(s, ':');

			if (*s)
			{
				if (!colon)
				{
					Sys_DPrintf("qtv cl, got (%s)\n", s);

					if (!strcmp(s, "QTV"))
					{
						// Just a qtv request.
					}
					else if (!strcmp(s, "SOURCELIST"))
					{
						SV_ReadSourceListRequest(cluster, pend);
					}
					else if (!strcmp(s, "REVERSE"))
					{	
						// This is actually a server trying to connect to us
						// start up a new stream.
					}
					else if (!strcmp(s, "RECEIVE"))
					{
						strlcpy(pend->authsource, "RECEIVE:", sizeof(pend->authsource));
						qtv = SV_ReadReceiveRequest(cluster, pend);
					}
					else if (!strcmp(s, "DEMOLIST"))
					{	
						SV_ReadDemoListRequest(cluster, pend);
					}
					else if (!strcmp(s, "AUTH"))
					{
						// Part of the connection process, can be ignored if there's no password.
					}
					else
					{
						Sys_Printf("(pending proxy #%3i): Unrecognised token in QTV connection request (%s)\n", pend->id, s);
					}
				}
				else
				{
					*colon++ = '\0';
					while (colon[0] == ' ') {
						colon++;
					}
					Sys_DPrintf("qtv cl, got (%s) (%s)\n", s, colon);

					if (!strcmp(s, "VERSION"))
					{
						usableversion = SV_ReadVersionRequest(colon);
					}
					else if (!strcmp(s, QTV_EZQUAKE_EXT))
					{
						// We set this ASAP.
						pend->qtv_ezquake_ext = (atoi(colon) & QTV_EZQUAKE_EXT_NUM);
					}
					else if (!strcmp(s, "RAW"))
					{
						raw = atoi(colon);
					}
					else if (!strcmp(s, "SOURCE"))
					{
						strlcpy(pend->authsource, "SOURCE:", sizeof(pend->authsource));
						strlcat(pend->authsource, colon, sizeof(pend->authsource));
						qtv = SV_ReadSourceRequest(cluster, colon);
					}
					else if (!strcmp(s, "DEMO"))
					{
						strlcpy(pend->authsource, "DEMO:", sizeof(pend->authsource));
						strlcat(pend->authsource, colon, sizeof(pend->authsource));
						qtv = SV_ReadDemoRequest(cluster, pend, colon);
					}
					else if (!strcmp(s, "AUTH"))
					{	
						authmethod_t this_method = QTVAM_NONE;
						if (!strcmp(colon, "NONE")) {
							this_method = QTVAM_NONE;
						}
						else if (!strcmp(colon, "PLAIN")) {
							this_method = QTVAM_PLAIN;
						}
						else if (!strcmp(colon, "CCITT")) {
							this_method = QTVAM_CCITT;
						}
						else if (!strcmp(colon, "MD4")) {
							this_method = QTVAM_MD4;
						}
						else if (!strcmp(colon, "SHA3_512")) {
							this_method = QTVAM_SHA3_512;
						}
						authmethod = max(authmethod, this_method);
					}
					else if (!strcmp(s, "PASSWORD")) {
						if (colon[0] == '"') {
							colon++;

							if (*(e - 1) == '"') {
								*(e - 1) = '\0';
							}
						}

						strlcpy(password, colon, sizeof(password));
					}
					else if (!strcmp(s, "USERINFO")) {
						strlcpy(userinfo, colon, sizeof(userinfo)); // Can't use it right now, qtv may be NULL atm.
					}
					else
					{
						Sys_Printf("(pending proxy #%3i): Unrecognised token in QTV connection request (%s)\n", pend->id, s);
					}
				}
			}

			s = e + 1;
		}

		e++;
	}

	//
	// Skip connection part in input buffer but not whole buffer because there may be some command from client already.
	//
	if (parse_end > 0 && parse_end < sizeof(pend->inbuffer))
	{
		pend->inbuffersize -= parse_end;
		memmove(pend->inbuffer, pend->inbuffer + parse_end, pend->inbuffersize);
	}
	else
	{
		pend->inbuffersize = 0; // Something wrong, just skip all input buffer.
	}

	pend->qtv_clversion = usableversion;

	if (pend->flushing)
		return false;

	if (userinfo[0]) {
		Info_Convert(&pend->ctx, userinfo);
	}

	// Workaround for ezquake clients (possibly others) not re-sending source when replying to challenge
	if (!qtv && password[0] && pend->authsource[0]) {
		if (!strncmp(pend->authsource, "SOURCE:", sizeof("SOURCE:") - 1)) {
			qtv = SV_ReadSourceRequest(cluster, pend->authsource + sizeof("SOURCE:") - 1);
		}
		else if (!strncmp(pend->authsource, "DEMO:", sizeof("DEMO:") - 1)) {
			qtv = SV_ReadDemoRequest(cluster, pend, colon);
		}
		else if (!strncmp(pend->authsource, "RECEIVE:", sizeof("RECEIVE:") - 1)) {
			qtv = SV_ReadReceiveRequest(cluster, pend);
		}
	}

	pend->authenticated = false;
	if (qtv) {
		const char* required = (qtv->custom_flags & QTV_CUSTOM_PASSWORD ? qtv->custom_password : qtv_password.string);

		pend->authenticated = SV_QTVValidateAuthentication(authmethod, password, pend->authchallenge, required);
		if (!pend->authenticated) {
			if (authmethod == QTVAM_CCITT && !password[0]) {
				Net_ProxyPrintf(
					pend, "%s"
					"AUTH: CCITT\n"
					"CHALLENGE: %s\n\n",
					QTV_SV_HEADER(pend, QTV_VERSION),
					pend->authchallenge
				);
			}
			else if (authmethod == QTVAM_MD4 && !password[0]) {
				Net_ProxyPrintf(
					pend, "%s"
					"AUTH: MD4\n"
					"CHALLENGE: %s\n\n",
					QTV_SV_HEADER(pend, QTV_VERSION),
					pend->authchallenge
				);
			}
			else if (authmethod == QTVAM_SHA3_512 && !password[0]) {
				Net_ProxyPrintf(
					pend, "%s"
					"AUTH: SHA3_512\n"
					"CHALLENGE: %s\n\n",
					QTV_SV_HEADER(pend, QTV_VERSION),
					pend->authchallenge
				);
			}
			else {
				Net_ProxyPrintf(pend, "%s"
					"PERROR: Authentication failure\n\n",
					QTV_SV_HEADER(pend, QTV_VERSION));

				pend->flushing = true;
			}
			return false;
		}
	}

	if (!usableversion)
	{
		Net_ProxyPrintf(pend, "%s"
							  "PERROR: Requested protocol version not supported\n\n",
							  QTV_SV_HEADER(pend, QTV_VERSION));

		pend->flushing = true;
		return false;
	}

	if (!qtv)
	{
		Net_ProxyPrintf(pend, "%s"
							  "PERROR: No stream selected\n\n",
							  QTV_SV_HEADER(pend, QTV_VERSION));

		pend->flushing = true;
		return false;
	}

	pend->qtv = qtv;

	// link it to the list
	pend->next = qtv->proxies;
	qtv->proxies = pend;

	if (!raw)
	{
		Net_ProxyPrintf(pend, "%s"
							  "BEGIN: %s\n\n", QTV_SV_HEADER(pend, QTV_VERSION), qtv->server);
	}

	Prox_FixName(qtv, pend);

	// Send message to all proxies what we have new client.
	Prox_UpdateProxiesUserList(qtv, pend, QUL_ADD);

	Net_SendConnectionMVD(qtv, pend);

	return true;
}

// Returns true if the pending proxy should be unlinked
// truth does not imply that it should be freed/released, just unlinked.
static qbool SV_ReadPendingProxy(cluster_t *cluster, oproxy_t *pend)
{
	pend->inbuffer[pend->inbuffersize] = 0; // So strings functions are happy.

	// Check if this proxy is timing out.
	SV_CheckProxyTimeout(cluster, pend);

	if (pend->drop)
	{
		// Free memory/handles and indicate we want this stream client unlinked by returning true.
		SV_FreeProxy(pend);
		return true; 
	}

	Net_TryFlushProxyBuffer(cluster, pend);

	if (pend->flushing)
	{
		SV_ReadUntilDrop(pend);

		// NOTE: if flushing is true, we do not peform any reading below, just wait until buffer is empty, then drop.
		return false;
	}

	// Try receiving data from the client and make sure it's of a valid type (HTTP or QTV).
	if (!SV_ReceivePendingProxyRequest(cluster, pend))
	{
		return false;
	}

	// Parse HTTP request.
	if (!SV_CheckForHTTPRequest(cluster, pend))
	{
		return false;
	}

	if (!SV_CheckForQTVRequest(cluster, pend))
	{
		return false;
	}

	return true;
}

//
// Pending proxies are clients that have requests pending,
// when they have been served they will be unlinked.
//
void SV_ReadPendingProxies(cluster_t *cluster)
{
	oproxy_t *pend, *pend2, *pend3;

	// unlink (probably) from the head
	while (cluster->pendingproxies)
	{
		pend = cluster->pendingproxies->next;

		if (SV_ReadPendingProxy(cluster, cluster->pendingproxies))
		{
			cluster->pendingproxies = pend;
		}
		else
		{
			break;
		}
	}

	// unlink (probably) from the body/tail
	for (pend = cluster->pendingproxies; (pend && pend->next); )
	{
		pend2 = pend->next;
		pend3 = pend2->next;

		if (SV_ReadPendingProxy(cluster, pend2))
		{
			pend->next = pend3;
			pend = pend3;
		}
		else
		{
			pend = pend2;
		}
	}
}

static qbool client_ids[MAX_QTV_CLIENTS];

static int SV_ProxyGetId(void)
{
	int id, i;

	for (i = 0; i < MAX_QTV_CLIENTS; i++)
	{
		id = (i + g_cluster.nextUserId) % MAX_QTV_CLIENTS;

		if (!client_ids[id])
		{
			client_ids[id] = true;
			g_cluster.nextUserId = (id + 1) % MAX_QTV_CLIENTS;
			return id + 1;
		}
	}

	Sys_Error("SV_ProxyGetId: no free id");

	return 0;
}

static void SV_ProxyFreeId(int id)
{
	id--;
	if (id < 0 || id >= MAX_QTV_CLIENTS)
		Sys_Error("SV_ProxyFreeId: wrong id %d", id + 1);

	client_ids[id] = false;
}

// Just allocate memory and set some fields, do not perform any linkage to any list.
// s = Either a socket or file (demo).
// socket = Decides if "s" is a socket or a file.
oproxy_t *SV_NewProxy(void *s, qbool socket, struct sockaddr_in *addr)
{
	oproxy_t *prox = Sys_malloc(sizeof(*prox));

	// Dynamic buffer allocation.
	prox->_buffermaxsize_ = MAX_OPROXY_BUFFER;
	prox->_buffer_ = Sys_malloc(prox->_buffermaxsize_);

	// Set socket/file based on source.
	prox->sock = (socket ? *(SOCKET*)s : INVALID_SOCKET);
	prox->file = (socket ?        NULL : (FILE*)s);
	if (addr)
	{
		prox->addr = *addr;
	}

	g_cluster.numproxies++;

	prox->ctx.max		= MAX_PROXY_INFOS;

	prox->init_time		= Sys_Milliseconds();
	prox->io_time		= Sys_Milliseconds();

	prox->id			= SV_ProxyGetId();

	// Since infostrings count is limited, we reserve at least the name.
	if(!Info_Set(&prox->ctx, "name", "")) 
	{
		Sys_Printf("Can't reserve name for client, dropping\n");
		prox->drop = true;
	}

	{
		int i;

		//generate a random challenge
		Net_BaseAdrToString(&prox->addr, prox->authchallenge, sizeof(prox->authchallenge));
		for (i = strlen(prox->authchallenge); i < sizeof(prox->authchallenge) - 1; i++) {
			prox->authchallenge[i] = rand() % (127 - 33) + 33;
		}
	}

	if (developer.integer > 1)
		Sys_DPrintf("SV_NewProxy: New proxy id #%d\n", prox->id);
	
	return prox;
}

// Just free memory and handles, do not perfrom removing from any list.
void SV_FreeProxy(oproxy_t *prox)
{
	SV_ProxyFreeId(prox->id);

	if (prox->qtv)
		Prox_UpdateProxiesUserList(prox->qtv, prox, QUL_DEL);

	if (prox->file)
		fclose(prox->file);
	if (prox->sock != INVALID_SOCKET)
		closesocket(prox->sock);

	if (prox->download)
		fclose(prox->download);
	if (prox->buffer_file)
		fclose(prox->buffer_file);

	Info_RemoveAll(&prox->ctx); // free malloced data

	g_cluster.numproxies--;

	Sys_free(prox->_buffer_); // free buffer
	Sys_free(prox);
}

// add read/write stats for prox, qtv, cluster
void SV_ProxySocketIOStats(oproxy_t *prox, int r, int w)
{
	r = max(0, r);
	w = max(0, w);

	if (r)
	{
		prox->socket_stat.r += r; // individual

		if (prox->qtv)
			prox->qtv->proxies_socket_stat.r += r; // cummulative for all proxies of particular qtv

		g_cluster.socket_stat.r += r; // cummulative for all
	}

	if (w)
	{
		prox->socket_stat.w += w; // individual

		if (prox->qtv)
			prox->qtv->proxies_socket_stat.w += w; // cummulative for all proxies of particular qtv

		g_cluster.socket_stat.w += w; // cummulative for all
	}
}

// Return true if we could try accept more clients.
static qbool SV_FindProxy(SOCKET qtv_sock, cluster_t *cluster)
{
	oproxy_t *prox;
	SOCKET sock;
	struct sockaddr_in addr;
	socklen_t addrlen;

	unsigned long nonblocking = true;

	if (qtv_sock == INVALID_SOCKET)
		return false;

	addrlen = sizeof(addr);
	if ((sock = accept(qtv_sock, (struct sockaddr *) &addr, &addrlen)) == INVALID_SOCKET)
		return false;

	// We could check if we are full early ofcourse and skip function altogether,
	// but it all matter of taste and how you see such situation should be handled.
	// IMO we should accept connections and close them, that would tell remote side what we are full.
	if (cluster->numproxies >= get_maxclients())
	{
		// FIXME: probably we should send something as reply...
		closesocket(sock);
		return true;
	}

	if (SV_IsBanned(&addr))
	{
		char ip[] = "xxx.xxx.xxx.xxx";
		Sys_DPrintf("rejected connect from banned ip: %s\n", Net_BaseAdrToString(&addr, ip, sizeof(ip)));
		closesocket(sock);
		return true;
	}

	if (ioctlsocket(sock, FIONBIO, &nonblocking) == INVALID_SOCKET) 
	{
		Sys_Printf("SV_FindProxies: ioctl FIONBIO: (%i)\n", qerrno);
		closesocket(sock);
		return true;
	}

	if (!TCP_Set_KEEPALIVE(sock))
	{
		Sys_Printf("SV_FindProxies: TCP_Set_KEEPALIVE: failed\n");
		closesocket(sock);
		return true;
	}

	prox = SV_NewProxy((void*) &sock, true, &addr);

	prox->next = cluster->pendingproxies;
	cluster->pendingproxies = prox;

	return true;
}

void SV_FindProxies(SOCKET qtv_sock, cluster_t *cluster, unsigned int proxies)
{
	unsigned int i;
	for (i = 0; i < proxies; i++) {
		if (!SV_FindProxy(qtv_sock, cluster)) {
			return;
		}
	}
}

static void SV_CheckMVDPort(cluster_t *cluster, int port)
{
	// remember current time so we does not hammer it too fast.
	cluster->mvdport_last_time_check = cluster->curtime;

	// close socket first.
	if (cluster->tcpsocket != INVALID_SOCKET)
	{
		closesocket(cluster->tcpsocket);
		cluster->tcpsocket = INVALID_SOCKET;

		Sys_Printf("%s is now closed\n", mvdport.name);
	}

	// open socket if required.
	if (port)
	{
		SOCKET news = Net_TCPListenPort(port);

		if (news != INVALID_SOCKET)
		{
			cluster->tcpsocket = news;

			Sys_Printf("%s %d opened\n", mvdport.name, port);
		}
		else
		{
			Sys_Printf("%s %d failed to open\n", mvdport.name, port);
		}
	}
}

void SV_CheckNETPorts(cluster_t *cluster)
{
	qbool tcpport_modified = false;
	qbool udpport_modified = false;
	int newp = bound(0, mvdport.integer, 65534);

	// Lets check if we should do some action with TCP socket. open/close.
	if (   (cluster->tcpsocket != INVALID_SOCKET && !newp) // we should close socket.
		|| (cluster->tcpsocket == INVALID_SOCKET && newp && (cluster->mvdport_last_time_check + 30 * 1000 < cluster->curtime)) // we should open socket.
	)
		tcpport_modified = true; // force open/close.

	// Lets check if we should do some action with UDP socket. open/close.
	if (   (cluster->udpsocket != INVALID_SOCKET && !newp) // we should close socket.
		|| (cluster->udpsocket == INVALID_SOCKET && newp && (cluster->udpport_last_time_check + 30 * 1000 < cluster->curtime)) // we should open socket.
	)
		udpport_modified = true; // force open/close.

	if (mvdport.modified)
		tcpport_modified = udpport_modified = true;

	if (tcpport_modified)
		SV_CheckMVDPort(cluster, newp);

	// As long as we use same port number for UPD/TCP I have to call SV_CheckUDPPort() from here.
	if (udpport_modified)
		SV_CheckUDPPort(cluster, newp);

	// we apply all we want, if we don't then repeat it later.
	mvdport.modified = false;
}

void Prox_FixName(sv_t *qtv, oproxy_t *prox)
{
	oproxy_t *pcl;
	char	*p;
	int		dupc = 1;
	char	newname[MAX_INFO_KEY], tmp[MAX_INFO_KEY], tmp2[MAX_INFO_KEY];

	// get name
	Info_Get(&prox->ctx, "name", tmp, sizeof(tmp)); // save old/current name for some time
	// copy name
	strlcpy (newname, tmp, sizeof(newname));

	for (p = newname; *p && (*p & 127) == ' '; p++)
		; // empty operator, we search where prefixed spaces ends

	if (p != newname) // skip prefixed spaces, if any, even whole string of spaces
		strlcpy(newname, p, sizeof(newname));

	for (p = newname + strlen(newname) - 1; p >= newname; p--)
	{
		if (*p && (*p & 127) != ' ') // skip spaces in suffix, if any
		{
			p[1] = 0;
			break;
		}
	}

	if (strcmp(tmp, newname))
	{
		Info_Set(&prox->ctx, "name", newname); // set name with skipped spaces or trimmed
		Info_Get(&prox->ctx, "name", newname, sizeof(newname));
	}

	if (!newname[0] || !stricmp(newname, "console") || strchr(newname, '#') || strchr(newname, ':') || strstr(newname, "&c") || strstr(newname, "&r"))
	{
		Info_Set(&prox->ctx, "name", "unnamed"); // console or empty name not allowed, using "unnamed" instead
		Info_Get(&prox->ctx, "name", newname, sizeof(newname));
	}

	// check to see if another user by the same name exists

	for (pcl = qtv->proxies; pcl; pcl = pcl->next)
	{
		if (prox == pcl)
			continue; // ignore self

		if (!stricmp(Info_Get(&pcl->ctx, "name", tmp, sizeof(tmp)), newname))
			break; // onoz, dup name
	}

	if (!pcl)
		return; // not found dup name

	p = newname;

	if (p[0] == '(' && isdigit(p[1]))
	{
		p++;

		while(isdigit(p[0]))
			p++;

		if (p[0] != ')')
			p = newname;
		else
			p++;
	}

	strlcpy(tmp, p, sizeof(tmp)); // this must skip from (1)qqshka prefix (1)

	while (1)
	{
		snprintf(newname, sizeof(newname), "(%d)%-.25s", dupc++, tmp);

		for (pcl = qtv->proxies; pcl; pcl = pcl->next)
		{
			if (prox == pcl)
				continue; // ignore self
    
			if (!stricmp(Info_Get(&pcl->ctx, "name", tmp2, sizeof(tmp2)), newname))
				break; // onoz, dup name
		}

		if (!pcl) // not found dup name, so set new name
		{
			if(!Info_Set(&prox->ctx, "name", newname))
			{
				Sys_Printf("(proxy #%3i): can't set duplicate name for client, dropping\n", prox->id);
				prox->drop = true;
			}

			return;
		}
	}
}

void Pending_Init(void)
{
	Cvar_Register (&mvdport);
	Cvar_Register (&maxclients);
	Cvar_Register (&allow_http);
	Cvar_Register (&qtv_password);
}
