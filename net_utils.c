/*
Misc net functions/utils
*/

#include "qtv.h"

qbool Net_StringToAddr(struct sockaddr_in *address, const char *host, int defaultport)
{
	struct hostent *phe;
	struct sockaddr_in sin = {0};
	char	*colon;
	char	copy[128]; // copy host name without port here.

	memset (address, 0, sizeof(*address));

	sin.sin_family = AF_INET;
	sin.sin_port = htons(defaultport);

	strlcpy (copy, host, sizeof(copy));
	// strip off a trailing :port if present
	for (colon = copy ; *colon ; colon++)
	{
		if (*colon == ':')
		{
			*colon = 0;
			sin.sin_port = htons((short)atoi(colon+1));
		}
	}

	/* Map host name to IP address, allowing for dotted decimal */
	if((phe = gethostbyname(copy)))
	{
		memcpy((char *)&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if((sin.sin_addr.s_addr = inet_addr(copy)) == INADDR_NONE)
	{
		Sys_DPrintf("Net_StringToAddr: wrong host: %s\n", copy);
		return false;
	}

	*address = sin;
	return true;
}

// return true if adresses equal
qbool Net_CompareAddress(struct sockaddr_in *a, struct sockaddr_in *b)
{
	if (!a || !b)
		return false;

	return (
			!memcmp(&a->sin_addr, &b->sin_addr, sizeof(a->sin_addr))
			&&
		    !memcmp(&a->sin_port, &b->sin_port, sizeof(a->sin_port))
		    );
}

char *Net_BaseAdrToString (struct sockaddr_in *a, char *buf, size_t bufsize)
{
	// Windows have inet_ntop only starting from Vista. Sigh.
#ifdef _WIN32
	const char *result = inet_ntoa(a->sin_addr);
#else
	const char *result = inet_ntop(a->sin_family, &a->sin_addr, buf, bufsize);
#endif

	strlcpy(buf, result ? result : "", bufsize);

	return buf;
}

char *Net_AdrToString (struct sockaddr_in *a, char *buf, size_t bufsize)
{
	if (*Net_BaseAdrToString(a, buf, bufsize))
	{
		char port[] = ":xxxxx";
		snprintf(port, sizeof(port), ":%i", (int)ntohs(a->sin_port));
		strlcat(buf, port, bufsize);
	}

	return buf;
}

qbool TCP_Set_KEEPALIVE(int sock)
{
	int		iOptVal = 1;

	if (sock == INVALID_SOCKET)
	{
		Sys_Printf("TCP_Set_KEEPALIVE: invalid socket\n");
		return false;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void*)&iOptVal, sizeof(iOptVal)) == -1) 
	{
		Sys_Printf("TCP_Set_KEEPALIVE: setsockopt SO_KEEPALIVE: (%i): %s\n", qerrno, strerror (qerrno));
		return false;
	}

#if defined(__linux__)

	//	The time (in seconds) the connection needs to remain idle before TCP starts sending keepalive probes, 
	//  if the socket option SO_KEEPALIVE has been set on this socket.

	iOptVal = 60;

	if (setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, (void*)&iOptVal, sizeof(iOptVal)) == -1) 
	{
		Sys_Printf("TCP_Set_KEEPALIVE: setsockopt TCP_KEEPIDLE: (%i): %s\n", qerrno, strerror(qerrno));
		return false;
	}

	//  The time (in seconds) between individual keepalive probes.
	iOptVal = 30;

	if (setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, (void*)&iOptVal, sizeof(iOptVal)) == -1) 
	{
		Sys_Printf("TCP_Set_KEEPALIVE: setsockopt TCP_KEEPINTVL: (%i): %s\n", qerrno, strerror(qerrno));
		return false;
	}

	//  The maximum number of keepalive probes TCP should send before dropping the connection. 
	iOptVal = 6;

	if (setsockopt(sock, SOL_TCP, TCP_KEEPCNT, (void*)&iOptVal, sizeof(iOptVal)) == -1) 
	{
		Sys_Printf("TCP_Set_KEEPALIVE: setsockopt TCP_KEEPCNT: (%i): %s\n", qerrno, strerror(qerrno));
		return false;
	}
#else
	// FIXME: windows, bsd etc...
#endif

	return true;
}

SOCKET Net_TCPListenPort(int port)
{
	SOCKET				sock = INVALID_SOCKET;
	qbool				error = true;
	struct sockaddr_in	address = {0};
	unsigned long		nonblocking = true;
	int					reuseaddr = 1;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((u_short)port);

	if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		// error.
	}
	else if (ioctlsocket (sock, FIONBIO, &nonblocking) == -1)
	{
		// error.
	}
	else if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&reuseaddr, sizeof(reuseaddr)) == -1)
	{
		// error.
	}
	else if (bind (sock, (void *)&address, sizeof(address)) == -1)
	{
		// error.
	}
	else if (listen(sock, 3) == -1)	// Don't listen for too many clients.
	{
		// error.
	}
	else if (!TCP_Set_KEEPALIVE(sock))
	{
		// error.
	}
	else
	{
		// success.
		error = false;	
	}

	// clean up in casse of error.
	if (error)
	{
		if (sock != INVALID_SOCKET)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
		}
	}

	return sock;
}

//=============================================================================

SOCKET Net_UDPOpenSocket(int port)
{
	SOCKET				sock = INVALID_SOCKET;
	qbool				error = true;
	struct sockaddr_in	address = {0};
	unsigned long		nonblocking = true;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((u_short)port);

	if ((sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		// error.
	}
	else if (ioctlsocket (sock, FIONBIO, &nonblocking) == -1)
	{
		// error.
	}
	else if (bind (sock, (void *)&address, sizeof(address)) == -1)
	{
		// error.
	}
	else
	{
		// success.
		error = false;	
	}

	// clean up in casse of error.
	if (error)
	{
		if (sock != INVALID_SOCKET)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
		}
	}

	return sock;
}

int Net_GetPacket(cluster_t *cluster, netmsg_t *msg)
{
	int ret;
	socklen_t fromlen = sizeof (cluster->net_from);

	ClearNetMsg(msg);

	if (cluster->udpsocket == INVALID_SOCKET)
		return false;

	ret = recvfrom(cluster->udpsocket, (char *)msg->data, msg->maxsize, 0, (struct sockaddr *) &cluster->net_from, &fromlen);

	if (ret == -1)
	{
		if (qerrno == EWOULDBLOCK)
			return false;

		if (qerrno == EMSGSIZE)
		{
			Sys_Printf ("NET_GetPacket: Oversize packet from %s\n", inet_ntoa(cluster->net_from.sin_addr));
			return false;
		}

		if (qerrno == ECONNRESET)
		{
			Sys_DPrintf ("NET_GetPacket: Connection was forcibly closed by %s\n", inet_ntoa(cluster->net_from.sin_addr));
			return false;
		}

		Sys_Printf ("NET_GetPacket: recvfrom: (%i): %s\n", qerrno, strerror (qerrno));
		return false;
	}

	if (ret >= msg->maxsize)
	{
		Sys_Printf ("NET_GetPacket: Oversize packet from %s\n", inet_ntoa(cluster->net_from.sin_addr));
		return false;
	}

	msg->cursize = ret;
	msg->data[ret] = 0;

	return ret;
}

void Net_SendPacket(cluster_t *cluster, int length, const void *data, struct sockaddr_in *to)
{
	socklen_t addrlen = sizeof(*to);

	if (cluster->udpsocket == INVALID_SOCKET)
		return;

	if (sendto(cluster->udpsocket, (const char *) data, length, 0, (struct sockaddr *)to, addrlen) == -1)
	{
		if (qerrno == EWOULDBLOCK)
			return;

		if (qerrno == ECONNREFUSED)
			return;

		Sys_Printf ("NET_SendPacket: sendto: (%i): %s\n", qerrno, strerror (qerrno));
	}
}
