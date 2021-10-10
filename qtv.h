/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef __CYGWIN__
#undef  _WIN32 // qqshka: this way it compiled on my CYGWIN
#endif

#ifdef __GNUC__
#define LittleLong(x) ({ typeof(x) _x = (x); _x = (((unsigned char *)&_x)[0]|(((unsigned char *)&_x)[1]<<8)|(((unsigned char *)&_x)[2]<<16)|(((unsigned char *)&_x)[3]<<24)); _x; })
#define LittleShort(x) ({ typeof(x) _x = (x); _x = (((unsigned char *)&_x)[0]|(((unsigned char *)&_x)[1]<<8)); _x; })
#else
#define LittleLong(x) (x)
#define LittleShort(x) (x)
#endif

#ifdef _WIN32
	#if defined(_DEBUG) && defined(_MSC_VER)
// uncomment/comment it if you wish/unwish see leaked memory(if any) in output window under MSVC
//		#define _CRTDBG_MAP_ALLOC
		#ifdef _CRTDBG_MAP_ALLOC
			#include <stdlib.h>
			#include <crtdbg.h>
		#endif
	#endif

	// meag: windows now seems to define E* codes
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
	#define _CRT_NO_POSIX_ERROR_CODES
#endif

	#include <conio.h>
	#include <winsock.h>	// This includes windows.h and is the reason for much compiling slowness with windows builds.
	#include <stdlib.h>
	#include <sys/types.h>
	#include <sys/timeb.h>

	#ifdef _MSC_VER
		#pragma comment (lib, "wsock32.lib")
	#endif
	#define qerrno WSAGetLastError()

	// see _CRT_NO_POSIX_ERROR_CODES above
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#define EINPROGRESS WSAEINPROGRESS
	#define ECONNREFUSED WSAECONNREFUSED
	#define ENOTCONN WSAENOTCONN
	#define EMSGSIZE WSAEMSGSIZE
	#define ECONNRESET WSAECONNRESET

	// We have special functions to properly terminate sprintf buffers in windows.
	// we assume other systems are designed with even a minor thought to security.
	#if !defined(__MINGW32_VERSION)
		#define unlink _unlink	// Why do MS have to be so awkward?
	#else
		#define unlink remove	// Seems mingw misses something.
	#endif

	#ifdef _MSC_VER
		// Okay, so warnings are here to help... they're ugly though.
		#pragma warning(disable: 4761)	// integral size mismatch in argument
		#pragma warning(disable: 4244)	// conversion from float to short
		#pragma warning(disable: 4018)	// signed/unsigned mismatch
		#pragma warning(disable: 4267)	// size_t -> int conversions
		#pragma warning(disable: 4201)	// nonstandard extension used : nameless struct/union
		#pragma warning(disable: 4100)	// unreferenced formal parameter
		#pragma warning(disable: 4127)	// conditional expression is constant
		#pragma warning(disable: 4706)	// assignment within conditional expression
	#endif

#elif defined(__CYGWIN__)

	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/errno.h>
	#include <arpa/inet.h>
	#include <stdarg.h>
	#include <netdb.h>
	#include <stdlib.h>
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <ctype.h>

	#define ioctlsocket ioctl
	#define closesocket close

#elif defined(__linux__) || defined(ixemul) || defined(__FreeBSD__)
	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <stdarg.h>
	#include <stdlib.h>
	#include <string.h>
	#include <errno.h>
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <ctype.h>

	#define ioctlsocket ioctl
	#define closesocket close
#else
#error "Please insert required headers here"
//try the cygwin ones
#endif

#ifndef EAGAIN
#define EAGAIN EWOULDBLOCK
#endif

#ifndef pgetaddrinfo
	#ifndef _WIN32
		#define pgetaddrinfo getaddrinfo
		#define pfreeaddrinfo freeaddrinfo
	#endif
#endif
#ifndef SOCKET
	#define SOCKET int
#endif
#ifndef INVALID_SOCKET
	#define INVALID_SOCKET -1
#endif
#ifndef qerrno
	#define qerrno errno
#endif

#include <stdio.h>
#include <string.h>

typedef unsigned long long int ullong;

#ifdef _WIN32

int		qsnprintf(char *str, size_t n, char const *fmt, ...);
int		qvsnprintf(char *buffer, size_t count, const char *format, va_list argptr);

#define snprintf qsnprintf
#define vsnprintf qvsnprintf

#endif

#if defined(__linux__) || defined(_WIN32) || defined(__CYGWIN__)
size_t	strlcpy (char *dst, const char *src, size_t siz);
size_t	strlcat (char *dst, char *src, size_t siz);
#endif

#ifndef _WIN32
//stricmp is ansi, strcasecmp is unix.
	#define stricmp strcasecmp
	#define strnicmp strncasecmp
#endif

#ifndef _WIN32
	#ifndef STDIN
		#define STDIN 0
	#endif
#endif

#ifndef __cplusplus
typedef enum {false, true} qbool;
#else
typedef int qbool;
extern "C" {
#endif


#define MAX_QTV_CLIENTS 2048

//======================================

#define PROXY_VERSION    "1.12-rc1"        // Release version of QTV (not protocol).
#define QTV_VERSION      1.0f              // we are support up to this QTV version.
#define QTV_PROJECT_URL  "https://github.com/deurk/qtv"
#define QTV_HELP_URL     "https://github.com/deurk/qtv/wiki"

// { QTV_EZQUAKE_EXT

#define QTV_EZQUAKE_EXT		"QTV_EZQUAKE_EXT"

#define QTV_EZQUAKE_EXT_DOWNLOAD	(1<<0)		// Well, this is not just download, but also different connection process.
#define QTV_EZQUAKE_EXT_SETINFO		(1<<1)		// Doesn't qtv server/client support setinfo.
#define QTV_EZQUAKE_EXT_QTVUSERLIST	(1<<2)		// Doesn't qtv server/client support qtvuserlist command.

#define QTV_EZQUAKE_EXT_NUM ( QTV_EZQUAKE_EXT_DOWNLOAD | QTV_EZQUAKE_EXT_SETINFO | QTV_EZQUAKE_EXT_QTVUSERLIST )

// }

//======================================

#define BUILD_DATE				__DATE__ ", " __TIME__

#define DEFAULT_HOSTNAME			"Unnamed"

#define PROX_DEFAULTSERVER			"localhost:27500"
#define PROX_DEFAULT_LISTEN_PORT	"27599"

#define	MSG_BUF_SIZE			8192
// qqshka: Its all messy.
// For example ezquake (and FTE?) expect maximum message is MSG_BUF_SIZE == 8192 with mvd header which have not fixed size,
// however fuhquake uses less msg size as I recall.
// mvd header max size is 10 bytes.
// 
// MAX_MVD_SIZE - max size of single mvd message _WITHOUT_ header
#define	MAX_MVD_SIZE			(MSG_BUF_SIZE - 100)

#define MAX_PROXY_INBUFFER		4096
#define MAX_PROXY_BUFFER 		(1<<18)					// must be power-of-two, 1<<18 is 2^18 = 262144 = 256 kilobytes
#define MAX_OPROXY_BUFFER       (1<<16)					// 64 kilobytes
#define PREFERED_PROXY_BUFFER	(1<<15)					// the ammount of data we try to leave in our input buffer (must be large enough to contain any single mvd frame)
#define MAX_PROXY_UPSTREAM 		(1<<16)

#define MAX_PROXY_INFOS			128						// How many settings (count, not bytes) one client may have.

#define MAX_MSGLEN 				1450
#define	FILE_TRANSFER_BUF_SIZE	((MAX_MSGLEN) - 100)


#define DEMOS_UPDATE_TIME		(1000 * 60)				// Update demos not so fast, this save CPU time.

#define RECONNECT_TIME			(1000 * 30)
#define DEMO_RECONNECT_TIME		(1000 * 2)				// If demo end, start play it again faster, do not wait 30 seconds.

#define PROTOCOL_VERSION_FTE    (('F'<<0) + ('T'<<8) + ('E'<<16) + ('X' << 24)) //fte extensions.
#define PROTOCOL_VERSION_FTE2   (('F'<<0) + ('T'<<8) + ('E'<<16) + ('2' << 24))	//fte extensions.
#define PROTOCOL_VERSION_MVD1   (('M'<<0) + ('V'<<8) + ('D'<<16) + ('1' << 24)) //mvdsv extensions.

#define QTV_CUSTOM_PARSEDELAY    (1)
#define QTV_CUSTOM_PASSWORD      (2)
#define QTV_CUSTOM_ADDRESS       (4)

//======================================

#include "qconst.h"
#include "cmd.h"
#include "cvar.h"
#include "info.h"
#include "sha3.h"

typedef struct 
{
	char name[128];
	int size;
	int time, smalltime;
} availdemo_t;

typedef struct 
{
	unsigned int readpos;
	unsigned int cursize;
	unsigned int maxsize;
	char *data;
	unsigned int startpos;
// WTF this does't used?
//	qbool overflowed;
//	qbool allowoverflow;
} netmsg_t;

// FIXME: hehe, nice struct
typedef struct 
{
	unsigned int incoming_sequence;
} netchan_t;


//======================================

// hrm, why we need this type?
typedef struct 
{ 
	char name[MAX_QPATH];
} filename_t;

typedef struct 
{
	float origin[3];
	unsigned char soundindex;
	unsigned char volume;
	unsigned char attenuation;
} staticsound_t;

typedef struct 
{
	float gravity;
	float maxspeed;
	float spectatormaxspeed;
	float accelerate;
	float airaccelerate;
	float waterfriction;
	float entgrav;
	float stopspeed;
	float wateraccelerate;
	float friction;
} movevars_t;

typedef struct 
{
	unsigned char frame;
	unsigned char modelindex;
	unsigned char skinnum;
	float origin[3];
	short velocity[3];
	float angles[3];
	unsigned char effects;
	unsigned char weaponframe;
} player_state_t;

typedef struct 
{
	unsigned int stats[MAX_STATS];
	char userinfo[MAX_USERINFO];

	int userid;

	int ping;
	int packetloss;
	int frags;
	float entertime;

	qbool active;
	qbool gibbed;
	qbool dead;
	player_state_t current;
	player_state_t old;
} playerinfo_t;

typedef struct 
{
	unsigned char frame;
	unsigned char modelindex;
	unsigned char colormap;
	unsigned char skinnum;
	float origin[3];
	float angles[3];
	unsigned char effects;
} entity_state_t;

typedef struct
{
	entity_state_t baseline;
} entity_t;

#define MAX_DEMO_PACKET_ENTITIES 300

typedef struct 
{
	int oldframe;
	int numents;
	int maxents;
	entity_state_t ents[MAX_DEMO_PACKET_ENTITIES];
	unsigned short entnums[MAX_DEMO_PACKET_ENTITIES];
} frame_t;

//======================================

// cmd flood protection

#define MAX_FP_CMDS (10)

typedef struct fp_cmd_s 
{
	ullong locked;
	ullong cmd_time[MAX_FP_CMDS];
	int   last_cmd;
	int   warnings;
} fp_cmd_t;

#ifdef _WIN32
typedef int socklen_t;
#endif

typedef struct io_stat_s 
{
	ullong r; // read bytes
	ullong w; // write bytes
} io_stat_t;

typedef struct sv_s sv_t;

#define MAX_AUTHCHALLENGE_LENGTH 64

//
// "Other proxy" - These are MVD stream clients.
//
typedef struct oproxy_s 
{
	qbool			flushing;
	qbool			drop;							// This proxy is droppable.

	qbool			connected_at_least_once;		// Connection sequence was completed at least once.
	qbool           authenticated;
	char            authchallenge[MAX_AUTHCHALLENGE_LENGTH];
	char            authsource[2 * MAX_QPATH];      // workaround for source not being received when challenge response sent

	sv_t			*qtv;							// To which qtv this client belong, may be NULL in case of pending proxy.

	FILE			*file;							// Recording a demo.
	SOCKET			sock;							// Playing to a proxy.
	struct sockaddr_in addr;						// Client address

	unsigned char	inbuffer[MAX_PROXY_INBUFFER];
	unsigned int	inbuffersize;					// Amount of data available.

	FILE			*buffer_file;					// Well, we can't always put whole file in _buffer_[MAX_PROXY_BUFFER],
													// so instead we put chunk of file in buffer then flush to client,
													// then put next chunk and etc.

	unsigned char	*_buffer_;
	unsigned int	_buffersize_;
	unsigned int	_buffermaxsize_;				// By default its MAX_PROXY_BUFFER
	unsigned int	_bufferautoadjustmaxsize_;		// default is 0, means disabled,
													// well this must allow have really huge buffer,
													// so buffer will be expanded maximum to this size when needed.


	ullong			init_time;						// When this client was created, so we can do some math with it.
	ullong			io_time;						// When was IO activity, so we can timeout it.

	int				id;								// User id for this client.
	int				pov;							// Which player id this client tracks, zero if freefly.
	int				follow_id;						// Which qtv client we follow.

	fp_cmd_t		fp_s;							// Say flood protect.

	float			qtv_clversion;					// Client version, float.
	int				qtv_ezquake_ext;				// QTV ezQuake extensions.

	ctxinfo_t		ctx;							// info context, we store here user setting like name and etc.

	// Download related
	FILE			*download;
	int				downloadsize;					// Total bytes.
	int				downloadcount;					// Bytes sent.
	int				file_percent;

	io_stat_t		socket_stat;					// read/write stats for socket

	struct			oproxy_s *next;
} oproxy_t;


#define MAX_LASTSCORES 32

//
// Our lastscore struct
//
typedef struct lastscores_s 
{ 
	char			date[sizeof("Feb 29, 19:00")];	// Date.
	char			type[16];						// Game type, like duel team etc.
	char			map[32];						// Map name, dm6 dm3 etc.
	char			e1[64];							// First team/person.
	char			s1[64];							// Scores for first team/person.
	char			e2[64];							// Second team/person.
	char			s2[64];							// Scores for second.
} lastscores_t;

//
// Source type enum
//
typedef	enum 
{
		SRC_BAD = 0,
		SRC_DEMO,
		SRC_TCP
} src_t; 

//
// NOTE: If you change source_s you probably need to take a look in close_source() and init_source() also
//
typedef struct source_s 
{
	src_t			type;		// Sources type, so we know we use file or socket or whatever.

	FILE			*f;			// File pointer.
	int				f_size;		// File size.

	SOCKET			s;			// Socket.

} source_t;

//
// NOTE: All these states do not represent if we're connected to a server or not, since we may have some data
//       in internal QTV buffers and successfully stream data to QTV clients and at the same time be not connected to server.
//
typedef enum 
{
	qs_parsingQTVheader,	// In process of parsing a QTV answer.
	qs_parsingconnection,	// We need to recieve some data before will be able stream.
	qs_active				// We are ready to forward/stream data.
} qtv_state_t;

//
// Server - Details about a server connection (also known as stream).
//
struct sv_s
{
	struct sv_s		*next;							// Next cluster->servers connection.

	char			ConnectPassword[64];			// Password given to server.
	char			server[MAX_QPATH];				// Something like tcp:localhost:25000 or demo:dag_vs_griffin_dm6.mvd
	unsigned int	streamid;						// Unique stream/source id.

	qbool			ServerQuery;					// We're quering some info from server/qtv but not trying stream something.
	qbool			DisconnectWhenNooneIsWatching;	// No comments.
	qbool			EchoInServerConsole;			// Echo output from this server in the server console.

	oproxy_t		*proxies;						// List of clients for this QTV stream.

	// Lastscores
	lastscores_t	lastscores[MAX_LASTSCORES];		// Here we store lastscores.
	int				lastscores_idx;					// Index, well it helps understand where to save new lastscore.

	unsigned int	connection_attempts;			// The number of times we've tried to connect unsuccessfully (used for backing off), used when qtv_backoff == 2.
	unsigned int	connection_delay;				// Time to reconnect try, used when qtv_backoff == 1.

	io_stat_t		socket_stat;					// read/write stats for socket
	io_stat_t		proxies_socket_stat;			// read/write stats for proxies sockets

	// Allows options to be set per-connection: aim for QTV on single port to support multiple connections
	int             custom_flags;                   // combination of QTV_CUSTOM_*
	float           custom_parse_delay;             // over-ride of parse_delay cvar
	char            custom_password[64];            // over-ride of qtv_password cvar
	char            custom_address[128];            // over-ride of address cvar

	//
	// Fields above saved on each QTV_Connect()
	//

	// ======= !!!! READ ME !!!! =======
	// we need memset(qtv, 0, sizeof(sv_s)) on each QTV_Connect(), so we will be sure all reset properly,
	// but some fields need to be saved between connections, like password and server name,
	// so we memset() not whole sv_s struct, but only part after this variable "mem_set_point",
	// so if you need save field between connections put it above this variable, otherwise below
	//

	char			mem_set_point;

	// ======= !!!! READ ME !!!! =======

	//
	// Fields below AUTOMATICALLY set to 0 on each QTV_Connect()
	//

	// hm, I am not sure where I must put drop, put it here for now, so it reset to 0 on QTV_Connect, hope that right
	qbool			drop;						// Something bad, close/free this sv_t.

	struct sockaddr_in ServerAddress;			// ip:port for socket

	float			svversion;					// Server/upstream version, NOTE it float: 1.0 or 1.1 or 1.2 and so on...

	// UpStream, requests to qtv
	unsigned char	UpstreamBuffer[MAX_PROXY_UPSTREAM];
	int				UpstreamBufferSize;

	// DownStream, answers on request and sure MVD stream at some point
	unsigned char	buffer[MAX_PROXY_BUFFER];	// This doesn't cycle.
	int				buffersize;					// It memmoves down.

	qtv_state_t		qstate;						// In which state our qtv.

	// Different times in milleseconds.
	ullong			parsetime;
	ullong			curtime;

	ullong			io_time;					// When was IO activity, so we can timeout it.

	ullong			NextConnectAttempt;			// Time of next reconnect attempt.

	char			status[64];					// Set this to status, which this sv_t is in.

	source_t		src;						// Our source type.

	// ============= We fill this after parsing data =============

	netchan_t		netchan;					// FIXME: this will be memset to 0 on each QTV_Connect(), correct?

	char			mapname[256];
	char			hostname[MAX_QPATH];
	char			hosttitle[MAX_QPATH];
	char			gamedir[MAX_QPATH];
	char			serverinfo[MAX_SERVERINFO_STRING];
	movevars_t		movevars;
	qbool           server_data_parsed;

	int				clservercount;

	// Below fields better memset() to 0 on each level change.
	playerinfo_t	players[MAX_CLIENTS];
	filename_t		modellist[MAX_MODELS];
	filename_t		soundlist[MAX_SOUNDS];
	filename_t		lightstyle[MAX_LIGHTSTYLES];
	entity_t		entity[MAX_ENTITIES];
	staticsound_t	staticsound[MAX_STATICSOUNDS];
	int				staticsound_count;
	entity_state_t	spawnstatic[MAX_STATICENTITIES];
	int				spawnstatic_count;

	frame_t         frame[MAX_ENTITY_FRAMES];

	int             cdtrack;
	int             paused;

	qbool           prev_was_svc_print;
	qbool           svc_print_servername;           // Small hack to figure out if we should write
	                                                // the servername when parsing svc_print messages
	                                                // since these can be split up. We don't want the
	                                                // servername to be printed in the middle of a chat message.

	// mvdsv sends STAT_TIME directly to player, so it isn't in .mvd/qtv stream
	//   but STAT_MATCHSTARTTIME is sent every time the player respawns, so can pick this value up and use a local offset to guess
	ullong          match_start_time;
	ullong          match_start_local_curtime;

	unsigned int    extension_flags_fte1;
	unsigned int    extension_flags_fte2;
	unsigned int    extension_flags_mvd1;
};

#define MAX_MASTERS 8 // size for masters fixed size array, I am lazy

//
// master state enum
//
typedef enum
{
	ms_unknown,		// unknown state
	ms_used			// this slot used in masters_t struct
} master_state_t;

//
// single master struct
//
typedef struct master
{
	master_state_t			state;		// master state
	struct sockaddr_in		addr;		// master addr
} master_t;

//
// all masters in one struct
//
typedef struct masters
{
	time_t					init_time;				// this is used to periodical re-init initiation

	time_t					last_heartbeat;			// when we send heartbeat last time
	int						heartbeat_sequence;		// heartbeat sequence number

	master_t				master[MAX_MASTERS];	// masters fixed size array, I am lazy
} masters_t;

//
// Main QTV struct.
//
typedef struct cluster_s 
{
	SOCKET tcpsocket;					// Tcp listening socket (for mvd and listings and stuff).

// { UDP
	SOCKET udpsocket;					// UDP socket (for connectionless commands).

	struct sockaddr_in	net_from;		// last recived UDP packet as from this address.
	netmsg_t			net_message;	// last recived UDP packet
	char				net_message_buffer[MSG_BUF_SIZE]; // last recived UDP packet buffer.

	masters_t			masters;		// masters array.
// }

	char commandinput[512]; 			// Our console input buffer.
	int inputlength; 					// How much data we have in the console buffer, after user presses enter the buffer is sent to the interpreter and this is set to 0.

	char info[MAX_SERVERINFO_STRING];	// Used by cvars which mirrored in serverinfo, this is useless unless UDP will be added to our project.

	ullong curtime;						// Milliseconds.

	ullong mvdport_last_time_check;		// Last time we attemp to open mvdport. Milliseconds.
	ullong udpport_last_time_check;		// Last time we attemp to open udpport. Milliseconds.

	qbool wanttoexit;					// If this is set to true our program will decide to die.

	int buildnumber;					// Just our buildnumber.

	sv_t *servers;						// List of connection to servers.
	int NumServers;						// How many servers that are in the list.

	int	numproxies;						// How much clients we have.

	oproxy_t *pendingproxies;			// Incoming request are queued here, after a request is served it's unlinked from here.

	io_stat_t		socket_stat;		// read/write stats for all sockets

	availdemo_t availdemos[8096];		// lala, insane...
	int availdemoscount;
	ullong last_demos_update;			// Miliseconds, last time when Cluster_BuildAvailableDemoList() was issued, saving CPU time

	int nextUserId;						// Out clients need name/userid.
} cluster_t;

//
// main.c
//

extern			cvar_t developer, shownet, hostname, hosttitle, admin_password;
extern			cluster_t g_cluster;
extern			qbool cluster_initialized;
void			Cluster_BuildAvailableDemoList(cluster_t *cluster);
extern			const char *demos_allowed_ext[];
extern			const int demos_allowed_ext_count;

//
// sys.c
//

extern char		*redirect_buf;
extern int		redirect_buf_size;

void			Sys_RedirectStart(char *buf, int size);				// So Sys_Printf() messages goes to redirect_buf[] too.
void			Sys_RedirectStop(void);

ullong			Sys_Milliseconds (void);
void			Sys_Printf (char *fmt, ...);
void			Sys_ConPrintf(sv_t *tv, char *fmt, ...);
void			Sys_DPrintf(char *fmt, ...);	// This is just wrapper for Sys_Printf(), but print nothing if developer 0
void			Sys_Exit(int code);
void			Sys_Error (char *error, ...);
int				Sys_Build_Number (void);

#ifdef _CRTDBG_MAP_ALLOC
//
// otherwise we see always sys.c and this is not really helpful...
//
// but it should not be issue since we just debug it.
	#define		Sys_malloc(_xxx)	calloc(1, (_xxx))
#else
void			*Sys_malloc (size_t size);
#endif

char			*Sys_strdup (const char *src);
#define			Sys_free(ptr) if(ptr) { free(ptr); ptr = NULL; }

int				strendswith(const char *s1, const char *s2);
char			*strchrrev(char *str, char chr);

// qqshka - hmm, seems in C these are macros, i don't like macros,
// however this functions work incorrectly on unsigned types!!!
#undef max
#undef min

#ifndef min
double			min( double a, double b );
#define KTX_MIN
#endif

#ifndef max
double			max( double a, double b );
#define KTX_MAX
#endif

double			bound( double a, double b, double c );

void			Sys_ReadSTDIN(cluster_t *cluster, fd_set socketset);

void			Sys_ReplaceChar(char *s, char from, char to);		// Replace char in string
unsigned long	Sys_HashKey(const char *str);						// Hash function.
void			Get_Uptime(ullong uptime_seconds, unsigned int *days, unsigned int *h, unsigned int *m); // Converts uptime seconds to days hours and minutes.


// Custom per-stream options
typedef struct qtvoptions_s {
	char client_password[64];
	float ingame_delay;
	char address[128];

	int flags;
} qtvoptions_t;


//
// token.c
//

#define MAX_COM_TOKEN	1024
extern char		com_token[MAX_COM_TOKEN];

char			*COM_Parse (char *data);							// Parse a token out of a string
char			*COM_ParseToken (char *data, char *out, int outsize, const char *punctuation); // FTE token function

//
// source.c
//

extern cvar_t	maxservers;
extern cvar_t	upstream_timeout;
extern cvar_t	parse_delay;
extern cvar_t	qtv_reconnect_delay;
extern cvar_t	qtv_max_reconnect_delay;
extern cvar_t	qtv_backoff;


#define dem_cmd			0
#define dem_read		1
#define dem_set			2
#define dem_multiple	3
#define dem_single		4
#define dem_stats		5
#define dem_all			6

#define dem_mask		7


// Read upstream (mvd or connection header), forawrd stream to clients/qtvs
qbool			Net_ReadStream(sv_t *qtv);

// Puts specified string in upstream queue, uses memcpy(), which means we don't rely on nul-termination but on size.
void			Net_QueueUpstream(sv_t *qtv, int size, char *buffer);

// Same but with var args.
void			Net_UpstreamPrintf(sv_t *qtv, char *fmt, ...);

// Actualy write string to socket.
qbool			Net_WriteUpStream(sv_t *qtv);

// Send connection request to qtv via upstream.
void			Net_SendQTVConnectionRequest(sv_t *qtv, char *authmethod, char *challenge);

// Connect to qtv/server via TCP
qbool			Net_ConnectToTCPServer(sv_t *qtv, char *ip);

// Open demo file on file system and use it as source.
qbool			OpenDemoFile(sv_t *qtv, char *demo);

// This memset to 0 too some data and something also.
void			QTV_SetupFrames(sv_t *qtv);

// Select which method use (demo or tcp or what ever) and connect/open to this source.
qbool			QTV_Connect(sv_t *qtv, const char *serverurl);

// free() memory and etc, unlink from cluster servers list.
void			QTV_Shutdown(cluster_t *cluster, sv_t *qtv);

// find stream by ID
sv_t			*QTV_Stream_by_ID(unsigned int id);

// malloc(qtv) and init, call QTV_Connect and link to servers list.
sv_t			*QTV_NewServerConnection(cluster_t *cluster, const char *server, char *password, 
								qbool force, qbool autoclose, qbool noduplicates, qbool query);

// Same as QTV_NewServerConnection but allow specification of custom options
sv_t* QTV_NewServerConnection2(cluster_t * cluster, const char* server, char* password,
	qbool force, qbool autoclose, qbool noduplicates, qbool query, qtvoptions_t* options);

// add read/write stats for prox, qtv, cluster.
void			QTV_SocketIOStats(sv_t *qtv, int r, int w);

// Parse qtv connection header
qbool			QTV_ParseHeader(sv_t *qtv);

// Check is qtv have source type SRC_DEMO or SRC_TCP, sreamable types in other words, just some small utility.
qbool			IsSourceStream(sv_t *qtv);

// Parse mvd and store data in some structs, so we can generate initial 
// connection mvd data to clients which connects in the middle of the action,
// this is a main qtv's trick.
int				QTV_ParseMVD(sv_t *qtv);

// Main hook to source.c module called from main.c, here we read/write/parse streams, connects/reconnects/shutdown sources.
void			QTV_Run(cluster_t *cluster, sv_t *qtv);

//
// source_cmds.c
//

// Add source related commands.
void			Source_Init(void);

//
// crc.c
//

unsigned short QCRC_Block(const unsigned char *start, int count);
unsigned short QCRC_Value(unsigned short crcvalue);
unsigned short QCRC_Block_Continue(const unsigned char *start, int count, unsigned short crc);

//
// mdfour.c
//

void			Com_BlockFullChecksum(void *buffer, int len, unsigned char *outbuf);

//
// net_utils.c
//

qbool			Net_StringToAddr(struct sockaddr_in *address, const char *host, int defaultport);
// return true if adresses equal
qbool			Net_CompareAddress(struct sockaddr_in *a, struct sockaddr_in *b);
char			*Net_BaseAdrToString (struct sockaddr_in *a, char *buf, size_t bufsize);
char			*Net_AdrToString (struct sockaddr_in *a, char *buf, size_t bufsize);
qbool			TCP_Set_KEEPALIVE(int sock);
SOCKET			Net_TCPListenPort(int port);

SOCKET			Net_UDPOpenSocket(int port);
int				Net_GetPacket(cluster_t *cluster, netmsg_t *msg);
void			Net_SendPacket(cluster_t *cluster, int length, const void *data, struct sockaddr_in *to);

//
// msg.c
//

void			InitNetMsg	(netmsg_t *b, char *buffer, int bufferlength);
void			ClearNetMsg (netmsg_t *b);

// FIXME: Probably not the place for these any more..
unsigned char	ReadByte	(netmsg_t *b);
unsigned short	ReadShort	(netmsg_t *b);
unsigned int	ReadLong	(netmsg_t *b);
float           ReadCoord   (sv_t* tv, netmsg_t* b);
float           ReadAngle   (sv_t* tv, netmsg_t* b);
float           ReadAngle16 (netmsg_t* b);
unsigned int	BigLong		(unsigned int val);
unsigned int	SwapLong	(unsigned int val);
float			ReadFloat	(netmsg_t *b);
void			ReadString	(netmsg_t *b, char *string, int maxlen);
void			WriteByte	(netmsg_t *b, unsigned char c);
void			WriteShort	(netmsg_t *b, unsigned short l);
void			WriteLong	(netmsg_t *b, unsigned int l);
void			WriteFloat	(netmsg_t *b, float f);
void            WriteCoord  (sv_t* sv, netmsg_t* b, float f);
void            WriteAngle  (sv_t* sv, netmsg_t* b, float f);
void            WriteAngle16(netmsg_t* b, float f);

// No null terminator, convienience function.
void			WriteString2(netmsg_t *b, const char *str);
void			WriteString (netmsg_t *b, const char *str);
void			WriteData	(netmsg_t *b, const char *data, int length);

//
// forward.c
//

extern cvar_t	downstream_timeout;

void			Net_TryFlushProxyBuffer(cluster_t *cluster, oproxy_t *prox);
void			Net_ProxySend(cluster_t *cluster, oproxy_t *prox, char *buffer, size_t length);

// printf() to downstream to particular "proxy", handy in some cases, instead of Net_ProxySend()-
void			Net_ProxyPrintf(oproxy_t *prox, char *fmt, ...);
void			Prox_SendMessage(cluster_t *cluster, oproxy_t *prox, char *buf, int length, int dem_type, unsigned int playermask);

// send text as "svc_print" to particular "proxy"
void			Prox_Printf(cluster_t *cluster, oproxy_t *prox, int dem_type, unsigned int playermask, int level, char *fmt, ...);

// FIXME: move to different file, forward.c intended keep something different than this raw protocol things
void			Prox_SendPlayerStats(sv_t *qtv, oproxy_t *prox);

// FIXME: move to different file, forward.c intended keep something different than this raw protocol things
void			Prox_SendInitialPlayers(sv_t *qtv, oproxy_t *prox, netmsg_t *msg);
void			Net_SendConnectionMVD(sv_t *qtv, oproxy_t *prox);
oproxy_t		*Net_FileProxy(sv_t *qtv, char *filename);
qbool			Net_StopFileProxy(sv_t *qtv);

// Forward the stream on to connected clients.
void			SV_ForwardStream(sv_t *qtv, unsigned char *buffer, int length);

// Register some vars.
void			Forward_Init(void);

//
// forward_pending.c
//

extern cvar_t	mvdport;
extern cvar_t	maxclients;
extern cvar_t	allow_http;

// return maxclients.integer bounded to some sane values
int				get_maxclients(void);

// This just can't be done as macro, so I wrote this function
// just generates qtv server header.
char			*QTV_SV_HEADER(oproxy_t *prox, float qtv_ver);

// Look for any other proxies wanting to muscle in on the action.
void			SV_ReadPendingProxies(cluster_t *cluster);

// Serve pending proxies.
void			SV_FindProxies(SOCKET qtv_sock, cluster_t *cluster);

// add read/write stats for prox, qtv, cluster
void			SV_ProxySocketIOStats(oproxy_t *prox, int r, int w);

// Check changes of mvdport variable and do appropriate action.
void			SV_CheckNETPorts(cluster_t *cluster);

// Register some vars.
void			Pending_Init(void);

// Just allocate memory and set some fields, do not perform any linkage to any list.
oproxy_t		*SV_NewProxy(void *s, qbool socket, struct sockaddr_in *addr);

// Just free memory and handles, do not perfrom removing from any list.
void			SV_FreeProxy(oproxy_t *prox);

// This will check what prox do not have duplicate name.
void			Prox_FixName(sv_t *qtv, oproxy_t *prox);

//
// qw.c
//

void			BuildServerData(sv_t *tv, netmsg_t *msg, int servercount);
int				SendList(sv_t *qtv, int first, const filename_t *list, int svc, netmsg_t *msg);

// Returns the next prespawn 'buffer' number to use, or -1 if no more.
int				Prespawn(sv_t *qtv, int curmsgsize, netmsg_t *msg, int bufnum, int thisplayer);
void			Prox_SendInitialEnts(sv_t *qtv, oproxy_t *prox, netmsg_t *msg);

//
// parse.c
//

void			ParseMessage(sv_t *tv, char *buffer, int length, int to, int mask);

//
// httpsv.c
//

char			*HTMLprintf(char *outb, int outl, qbool qfont, const char *fmt, ...);
void			HTTPSV_SendHTTPHeader(cluster_t *cluster, oproxy_t *dest, char *error_code, char *content_type, qbool nocache);
void			HTTPSV_SendHTMLHeader(cluster_t *cluster, oproxy_t *dest, char *title);
void			HTTPSV_SendHTMLFooter(cluster_t *cluster, oproxy_t *dest);
qbool			HTTPSV_GetHeaderField(const char *s, const char *field, char *buffer, int buffersize);
char			*HTTPSV_ParsePOST(char *post, char *buffer, int buffersize);
void			HTTPSV_PostMethod(cluster_t *cluster, oproxy_t *pend); //, char *postdata);
void			HTTPSV_GetMethod(cluster_t *cluster, oproxy_t *pend);
qbool			HTTPSV_GetHostname(cluster_t *cluster, oproxy_t *dest, char *hostname, int buffersize);
int				HTTPSV_UnescapeURL(const char *url, char *out, size_t outsize);
size_t			HTTPSV_EscapeURL(const char *url, char *out, size_t outsize);

//
// httpsv_generate.c
//
extern cvar_t	allow_join;

void			Http_Init(void);
void			HTTPSV_GenerateCSSFile(cluster_t *cluster, oproxy_t *dest);
void			HTTPSV_GenerateJSFile(cluster_t *cluster, oproxy_t *dest);
void			HTTPSV_GenerateNowPlaying(cluster_t *cluster, oproxy_t *dest);
void			HTTPSV_GenerateQTVStub(cluster_t *cluster, oproxy_t *dest, char *streamtype, char *streamid);
void			HTTPSV_GenerateQTVJoinStub(cluster_t *cluster, oproxy_t *dest, char *streamid);
void			HTTPSV_GenerateAdmin(cluster_t *cluster, oproxy_t *dest, int streamid, char *postbody);
void			HTTPSV_GenerateDemoListing(cluster_t *cluster, oproxy_t *dest);
void			HTTPSV_GenerateImage(cluster_t *cluster, oproxy_t *dest, char *imgfilename);
void			HTTPSV_GenerateLevelshot(cluster_t *cluster, oproxy_t *dest, char *name);
void			HTTPSV_GenerateDemoDownload(cluster_t *cluster, oproxy_t *dest, char *name);
void			HTTPSV_GenerateRSS(cluster_t *cluster, oproxy_t *dest, char *str);
void			HTTPSV_GenerateQTVStatus(cluster_t *cluster, oproxy_t *dest, char *str);


//
// cl_cmds.c
//

// search proxy/client by id
oproxy_t		*Prox_by_ID(sv_t *qtv, int id);
// search proxy/client by name
oproxy_t		*Prox_by_Name(sv_t *qtv, const char *name);
// search proxy/client by id or name
oproxy_t		*Prox_by_ID_or_Name(sv_t *qtv, const char *id_or_name);
// Returs count of clients for this stream
unsigned int	Proxy_UsersCount(const sv_t *qtv);

extern cvar_t	floodprot;
extern cvar_t	demo_dir;
#define DEMO_DIR (demo_dir.string[0] ? demo_dir.string : "demos")

void			FixSayFloodProtect(void);

void			Proxy_ReadProxies(sv_t *qtv);
void			Cl_Cmds_Init(void);

typedef enum 
{
	QUL_NONE = 0,	//
	QUL_ADD,		// User joined.
	QUL_CHANGE,		// User changed something like name or something.
	QUL_DEL			// User dropped.
} qtvuserlist_t;

// Send userlist message about "prox" to all proxies.
void			Prox_UpdateProxiesUserList(sv_t *qtv, oproxy_t *prox, qtvuserlist_t action);

// Send userlist to this "prox", do it once, so we do not send it on each level change.
void			Prox_SendInitialUserList(sv_t *qtv, oproxy_t *prox);

// Send user list to upsteram at connect time
void			Prox_UpstreamSendInitialUserList(sv_t *qtv);

// Send commands to "prox" like: lastscores, follow etc.
void			Prox_StuffCommands(sv_t *qtv, oproxy_t *prox);

//
// fs.c
//

FILE			*FS_OpenFile(char *gamedir, char *filename, int *size);

// Open and load file in memory.
// may be used in two ways: 
// 1) user provide buffer, in this case "size" provides buffer size.
// 2) or function automatically allocate it, in this case need _FREE_ memory when it no more needed.
//
// in both cases after returning from function "size" will reflect actual data length.
char			*FS_ReadFile(char *gamedir, char *filename, char *buf, int *size);
void			FS_StripPathAndExtension(char *filepath);

// Return file extension with dot, or empty string if dot not found at all.
const char		*FS_FileExtension (const char *in);
// Absolute paths are prohibited.
qbool			FS_SafePath(const char *in);

//
// bann.c
//

// Init banning system.
void			Ban_Init(void);
// Periodically check is it time to remove some bans.
void			SV_CleanBansIPList(void);
// Return true if add is banned.
qbool			SV_IsBanned (struct sockaddr_in *addr);

//
// udp.c
//

void			UDP_Init(void);
void			SV_CheckUDPPort(cluster_t *cluster, int port);
void			SV_UDP_Frame(cluster_t *cluster);

// PROTOCOL EXTENSIONS
#define FTE_PEXT_FLOATCOORDS  0x00008000

// Commented out extensions don't affect QTV stream
//#define MVD_PEXT1_FLOATCOORDS       (1 <<  0) // FTE_PEXT_FLOATCOORDS but for entity/player coords only
//#define MVD_PEXT1_HIGHLAGTELEPORT   (1 <<  1) // Adjust movement direction for frames following teleport
//#define MVD_PEXT1_SERVERSIDEWEAPON  (1 <<  2) // Server-side weapon selection
//#define MVD_PEXT1_DEBUG_WEAPON      (1 <<  3) // Send weapon-choice explanation to server for logging
//#define MVD_PEXT1_DEBUG_ANTILAG     (1 <<  4) // Send predicted positions to server (compare to antilagged positions)
#define MVD_PEXT1_HIDDEN_MESSAGES   (1 <<  5) // dem_multiple(0) packets are in format (<length> <type-id>+ <packet-data>)*
//#define MVD_PEXT1_SERVERSIDEWEAPON2 (1 <<  6) // Server-side weapon selection supports clc_mvd_weapon_full_impulse

// hidden messages inserted into .mvd files
// embedded in dem_multiple(0) - should be safely skipped in clients
// format is <int:length> <short:type>*   where <type> is duplicated if 0xFFFF.  <length> is length of the data packet, not the header
enum {
	mvdhidden_antilag_position           = 0x0000,  // mvdhidden_antilag_position_header_t mvdhidden_antilag_position_t*
	mvdhidden_usercmd                    = 0x0001,  // <byte: playernum> <byte:dropnum> <byte: msec, vec3_t: angles, short[3]: forward side up> <byte: buttons> <byte: impulse>
	mvdhidden_usercmd_weapons            = 0x0002,  // <byte: source playernum> <int: items> <byte[4]: ammo> <byte: result> <byte*: weapon priority (nul terminated)>
	mvdhidden_demoinfo                   = 0x0003,  // <short: block#> <byte[] content>
	mvdhidden_commentary_track           = 0x0004,  // <byte: track#> [todo... <byte: audioformat> <string: short-name> <string: author(s)> <float: start-offset>?]
	mvdhidden_commentary_data            = 0x0005,  // <byte: track#> [todo... format-specific]
	mvdhidden_commentary_text_segment    = 0x0006,  // <byte: track#> [todo... <float: duration> <string: text (utf8)>]
	mvdhidden_dmgdone                    = 0x0007,  // <byte: type-flags> <short: damaged ent#> <short: damaged ent#> <short: damage>
	mvdhidden_usercmd_weapons_ss         = 0x0008,  // (same format as mvdhidden_usercmd_weapons)
	mvdhidden_usercmd_weapon_instruction = 0x0009,  // <byte: playernum> <byte: flags> <int: sequence#> <int: mode> <byte[10]: weaponlist>
	mvdhidden_paused_duration            = 0x000A,  // <byte: msec> ... actual time elapsed, not gametime (can be used to keep stream running) ... expected to be QTV only
	mvdhidden_extended                   = 0xFFFF   // doubt we'll ever get here: read next short...
};

#ifdef __cplusplus
}
#endif

