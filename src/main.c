/*
Contains the control routines
*/

#include "qtv.h"
#include <signal.h>

#ifndef _WIN32
#include <sys/stat.h>
#include <dirent.h>
#endif // _WIN32


const char* demos_allowed_ext[] = { ".mvd", ".gz", ".zip", ".bz2" };
const int demos_allowed_ext_count = sizeof(demos_allowed_ext)/sizeof(*demos_allowed_ext);

cvar_t version          = {"*version", "QTV " PROXY_VERSION, CVAR_ROM | CVAR_SERVERINFO};

cvar_t developer        = {"developer", ""};
cvar_t shownet          = {"shownet", ""};

cvar_t hostname         = {"hostname", DEFAULT_HOSTNAME, CVAR_SERVERINFO};
cvar_t address          = {"address", ""};
cvar_t hosttitle        = {"hosttitle", ""};
cvar_t admin_password   = {"admin_password", ""};

int SortFilesByDate(const void *a, const void *b) 
{
	const availdemo_t *aa = (availdemo_t*)a;
	const availdemo_t *bb = (availdemo_t*)b;

	if (aa->time < bb->time)
		return 1;
	if (aa->time > bb->time)
		return -1;

	if (aa->smalltime < bb->smalltime)
		return 1;
	if (aa->smalltime > bb->smalltime)
		return -1;

	return 0;
}

void Cluster_BuildAvailableDemoList(cluster_t *cluster)
{
	if (cluster->last_demos_update && cluster->last_demos_update + DEMOS_UPDATE_TIME > cluster->curtime)
		return; // Do not update demos too fast, this save CPU time.

	cluster->last_demos_update = cluster->curtime;

	cluster->availdemoscount = 0;

	#ifdef _WIN32
	{
		WIN32_FIND_DATA ffd;
		HANDLE h;
		char path[256];
		int ext;

		for (ext = 0; ext < demos_allowed_ext_count; ext++)
		{
			snprintf(path, sizeof(path), "%s/*%s", DEMO_DIR, demos_allowed_ext[ext]);

			h = FindFirstFile(path, &ffd);
			if (h != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (cluster->availdemoscount == sizeof(cluster->availdemos)/sizeof(cluster->availdemos[0]))
						break;

					strlcpy(cluster->availdemos[cluster->availdemoscount].name, ffd.cFileName, sizeof(cluster->availdemos[0].name));
					cluster->availdemos[cluster->availdemoscount].size = ffd.nFileSizeLow;
					cluster->availdemos[cluster->availdemoscount].time = ffd.ftLastWriteTime.dwHighDateTime;
					cluster->availdemos[cluster->availdemoscount].smalltime = ffd.ftLastWriteTime.dwLowDateTime;
					cluster->availdemoscount++;
				} while(FindNextFile(h, &ffd));

				FindClose(h);
			}
		} // for
	}
	#else // _WIN32
	{
		DIR *dir;
		struct dirent *ent;
		struct stat sb;
		char fullname[512];
		qbool valid;
		int ext;

		dir = opendir(DEMO_DIR);	// Yeek!
		if (dir)	
		{
			for(;;)
			{
				if (cluster->availdemoscount == sizeof(cluster->availdemos)/sizeof(cluster->availdemos[0]))
					break;

				ent = readdir(dir);
				
				if (!ent)
					break;

				if (*ent->d_name == '.')
					continue; // Ignore 'hidden' files.

				valid = false;
				for (ext = 0; ext < demos_allowed_ext_count && !valid; ext++)
				{
					if(stricmp(demos_allowed_ext[ext], FS_FileExtension(ent->d_name)) == 0) {
						valid = true;
					}
				}
				if (!valid)
					continue; // Ignore non *.mvd *.zip *.gz etc...

				snprintf(fullname, sizeof(fullname), "%s/%s", DEMO_DIR, ent->d_name);
				
				if (stat(fullname, &sb))
					continue; // Some kind of error.

				strlcpy(cluster->availdemos[cluster->availdemoscount].name, ent->d_name, sizeof(cluster->availdemos[0].name));
				cluster->availdemos[cluster->availdemoscount].size = sb.st_size;
				cluster->availdemos[cluster->availdemoscount].time = sb.st_mtime;
				cluster->availdemoscount++;
			}

			closedir(dir);
		}
		else
		{
			Sys_Printf("Couldn't open dir for demo listings\n");
		}
	}
	#endif // _WIN32 else

	qsort(cluster->availdemos, cluster->availdemoscount, sizeof(cluster->availdemos[0]), SortFilesByDate);
}


// Sleep and handle keyboard input.
void Cluster_Sleep(cluster_t *cluster)
{
	sv_t *sv;
	int m;
	struct timeval timeout;
	fd_set socketset;

	FD_ZERO(&socketset);
	m = 0;

	for (sv = cluster->servers; sv; sv = sv->next)
	{
		if (sv->src.type == SRC_TCP && sv->src.s != INVALID_SOCKET)
		{
			FD_SET(sv->src.s, &socketset);
			if (sv->src.s >= m)
				m = sv->src.s + 1;
		}
	}

	if (cluster->tcpsocket != INVALID_SOCKET)
	{
		FD_SET(cluster->tcpsocket, &socketset);
		if (cluster->tcpsocket >= m)
			m = cluster->tcpsocket + 1;
	}

	#ifndef _WIN32
	FD_SET(STDIN, &socketset);
	if (STDIN >= m)
		m = STDIN + 1;
	#endif // _WIN32


	#ifdef _WIN32

	//if (!m) // FIXME: my'n windows XP eat 50% CPU if here mvdport 0 and no clients, lame work around, any better solution?
		Sleep(1);
	
	#else //_WIN32

	usleep(100000);

	#endif // _WIN32 else

	if ( 1 )
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;
	}
	else
	{
		timeout.tv_sec = 100/1000;			// 0 seconds.
		timeout.tv_usec = (100%1000)*1000;	// 100 milliseconds timeout.
	}

	m = select(m, &socketset, NULL, NULL, &timeout);

	Sys_ReadSTDIN(cluster, socketset);
}

void Cluster_Run(cluster_t *cluster, qbool dowait, qbool down)
{
	sv_t *sv, *old;

	FixSayFloodProtect();

	if (dowait)
		Cluster_Sleep(cluster); // Sleep and serve console input.

	cluster->curtime = Sys_Milliseconds();

	Cbuf_Execute(); // Process console commands.

	// Cbuf_Execute() may take some time so set 
	// current time again, not sure that right way.
	cluster->curtime = Sys_Milliseconds();

	for (sv = cluster->servers; sv; )
	{
		old = sv; // Save sv_t in old, because QTV_Run(old) may free(old).
		sv = sv->next;
		if (down)
			old->drop = true; // we going down, free it
		QTV_Run(cluster, old);
	}

	// Check changes of mvdport variable and do appropriate action.
	SV_CheckNETPorts(cluster);

	// Do UDP related things.
	SV_UDP_Frame(cluster);

	// Look for any other proxies wanting to muscle in on the action.
	SV_FindProxies(cluster->tcpsocket, cluster, 16);

	// Serve pending proxies.
	SV_ReadPendingProxies(cluster); 

	// Periodically check is it time to remove some bans.
	SV_CleanBansIPList();
}

qbool cluster_initialized;

cluster_t g_cluster; // Seems fte qtv tryed do not make it global, see no reason for this.

int main(int argc, char **argv)
{
	#ifdef _CRTDBG_MAP_ALLOC
	{
		// report memory leaks on program exit in output window under MSVC
		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	}
	#endif

	#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
	#endif

	#ifdef _WIN32
	{
		WSADATA discard;
		WSAStartup(MAKEWORD(2,0), &discard);
	}
	#endif // _WIN32

	memset(&g_cluster, 0, sizeof(g_cluster));

	g_cluster.tcpsocket   = INVALID_SOCKET;
	g_cluster.udpsocket   = INVALID_SOCKET;
	g_cluster.buildnumber = Sys_Build_Number();
	g_cluster.nextUserId  = 0;

	Info_Init();	// Info strings init.
	Cbuf_Init();	// Command buffer init.
	Cmd_Init();		// Register basic commands.
	Cl_Cmds_Init();	// Init client commands.
	Cvar_Init();	// Variable system init.
	Source_Init();	// Add source related commands.
	Forward_Init(); // Register some vars.
	Pending_Init();	// Register some vars.
	Http_Init();	// Register some vars.
	UDP_Init();		// UDP system init.
	Ban_Init();		// Init banning system.

	Cvar_Register(&version);

	Cvar_Register(&developer);
	Cvar_Register(&shownet);

	Cvar_Register(&hostname);
	Cvar_Register(&address);
	Cvar_Register(&hosttitle);
	Cvar_Register(&admin_password);

	cluster_initialized = true;

	Sys_Printf("QTV %s, build %i (build date: %s)\n", PROXY_VERSION, g_cluster.buildnumber, BUILD_DATE);

	Cmd_ExcuteDefaultCfg(argc, argv);
	// Process command line arguments.
	Cmd_StuffCmds(argc, argv);
	Cbuf_Execute();

	for ( ;; ) 
	{
		Cluster_Run(&g_cluster, true, false);
		// if we want to exit, re run it with down = true, so we have chance free some resources
		if (g_cluster.wanttoexit)
		{
			Cluster_Run(&g_cluster, false, true);
			break;
		}
	}

	Cmd_DeInit();		// this is optional, but helps me check memory leaks
	Cvar_DeInit();		// this is optional, but helps me check memory leaks
	return 0;
}

