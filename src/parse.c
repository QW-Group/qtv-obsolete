/*
	parse.c
*/

#include "qtv.h"

// since svc code transfered over byte, than it nor more than 256
#define SVC(svc) #svc,
char *svc_strings[] = 
{
	#include "svc.h"
};
#undef SVC
#define svc_strings_size (sizeof(svc_strings) / sizeof(svc_strings[0]))

#define ParseError(m) (m)->cursize = (m)->cursize+1	//

static void ParseServerData(sv_t *tv, netmsg_t *m, int to, unsigned int playermask)
{
	int protocol;
	qbool qw;
	oproxy_t *prox;

	while (true) {
		protocol = ReadLong(m);
		if (protocol == PROTOCOL_VERSION_FTE) {
			tv->extension_flags_fte1 = ReadLong(m);
			Sys_Printf("FTE extension flags: %u\n", tv->extension_flags_fte1);
			continue;
		}

		if (protocol == PROTOCOL_VERSION_FTE2) {
			tv->extension_flags_fte2 = ReadLong(m);
			Sys_Printf("FTE2 extension flags: %u\n", tv->extension_flags_fte2);
			continue;
		}

		if (protocol == PROTOCOL_VERSION_MVD1) {
			tv->extension_flags_mvd1 = ReadLong(m);
			Sys_Printf("MVD1 extension flags: %u\n", tv->extension_flags_mvd1);
			continue;
		}

		if (protocol != PROTOCOL_VERSION) {
			ParseError(m); // FIXME: WTF, we need drop proxy instead?
			return;
		}

		break;
	}

	tv->qstate = qs_parsingconnection;

	tv->clservercount = ReadLong(m);	// We don't care about server's servercount, it's all reliable data anyway.

	ReadString(m, tv->gamedir, sizeof(tv->gamedir));
	if (!tv->gamedir[0]) // default gamedir is qw
		strlcpy(tv->gamedir, "qw", sizeof(tv->gamedir));

//	tv->thisplayer = MAX_CLIENTS-1;
	/*tv->servertime =*/ ReadFloat(m);

	ReadString(m, tv->mapname, sizeof(tv->mapname));

	qw = !strcmp(tv->gamedir, "qw");
	// Show Gamedir if different than qw.
	Sys_ConPrintf(tv, "---------------------\n");
	Sys_ConPrintf(tv, "Map: %s%s%s\n", tv->mapname, (qw ? "" : "\nGamedir: "), (qw ? "" : tv->gamedir));
	Sys_ConPrintf(tv, "---------------------\n");

	// Get the movevars.
	tv->movevars.gravity			= ReadFloat(m);
	tv->movevars.stopspeed			= ReadFloat(m);
	tv->movevars.maxspeed			= ReadFloat(m);
	tv->movevars.spectatormaxspeed	= ReadFloat(m);
	tv->movevars.accelerate			= ReadFloat(m);
	tv->movevars.airaccelerate		= ReadFloat(m);
	tv->movevars.wateraccelerate	= ReadFloat(m);
	tv->movevars.friction			= ReadFloat(m);
	tv->movevars.waterfriction		= ReadFloat(m);
	tv->movevars.entgrav			= ReadFloat(m);

//	tv->maxents = 0;	//clear these
	memset(tv->players,     0, sizeof(tv->players));
	                        
	memset(tv->modellist,   0, sizeof(tv->modellist));
	memset(tv->soundlist,   0, sizeof(tv->soundlist));
	memset(tv->lightstyle,  0, sizeof(tv->lightstyle));
	memset(tv->entity,      0, sizeof(tv->entity));

	memset(tv->staticsound, 0, sizeof(tv->staticsound));
	tv->staticsound_count = 0;
	memset(tv->spawnstatic, 0, sizeof(tv->spawnstatic));
	tv->spawnstatic_count = 0;

	tv->cdtrack			  = 0;

	QTV_SetupFrames(tv); // This memset to 0 too some data and something also.

	strlcpy(tv->status, "Receiving soundlist", sizeof(tv->status));
	tv->server_data_parsed = true;

	for (prox = tv->proxies; prox; prox = prox->next)
		if (prox->qtv_ezquake_ext & QTV_EZQUAKE_EXT_DOWNLOAD) // We need it for clients with full download support.
			prox->flushing = true;
}

static void ParseCDTrack(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	tv->cdtrack = ReadByte(m);
}

static void ParseStufftext(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	char text[1024];
	char value[256];
	qbool fromproxy;

	ReadString(m, text, sizeof(text));
//	Sys_Printf("stuffcmd: %s", text);

	if (strstr(text, "screenshot"))
		return;

	if (!strcmp(text, "skins\n"))
	{
		strlcpy(tv->status, "On server", sizeof(tv->status));

		tv->qstate = qs_active;

		return;
	}
	else if (!strncmp(text, "fullserverinfo ", 15))
	{
		char qtv_version[32];
		snprintf(qtv_version, sizeof(qtv_version), "%g", QTV_VERSION);

		// FIXME: clear all this block { ... }

		text[strlen(text)-1] = '\0';
		text[strlen(text)-1] = '\0';

		// Copy over the server's serverinfo.
		strlcpy(tv->serverinfo, text+16, sizeof(tv->serverinfo));

		Info_ValueForKey(tv->serverinfo, "*qtv", value, sizeof(value));
		if (*value)
			fromproxy = true;
		else
			fromproxy = false;

		// Add on our extra infos.
		Info_SetValueForStarKey(tv->serverinfo, "*qtv", qtv_version, sizeof(tv->serverinfo));
//		Info_SetValueForStarKey(tv->serverinfo, "*z_ext", Z_EXT_STRING, sizeof(tv->serverinfo));

		Info_ValueForKey(tv->serverinfo, "hostname", tv->hostname, sizeof(tv->hostname));

		// Change the hostname (the qtv's hostname with the server's hostname in brackets)
		Info_ValueForKey(tv->serverinfo, "hostname", value, sizeof(value));
		if (fromproxy && strchr(value, '(') && value[strlen(value)-1] == ')')	// Already has brackets.
		{	
			// The from-proxy check is because it's fairly common to find a qw server with brackets after it's name.
			char *s;
			s = strchr(value, '(');	// So strip the parent proxy's hostname, and put our hostname first, leaving the origional server's hostname within the brackets.
			snprintf(text, sizeof(text), "%s %s", hostname.string, s);
		}
		else
		{
			if (tv->src.type == SRC_DEMO)
				snprintf(text, sizeof(text), "%s (recorded from: %s)", hostname.string, value);
			else
				snprintf(text, sizeof(text), "%s (live: %s)", hostname.string, value);
		}
		Info_SetValueForStarKey(tv->serverinfo, "hostname", text, sizeof(tv->serverinfo));

		return;
	}
	else if (!strncmp(text, "cmd ", 4))
	{
		return;	// Commands the game server asked for are pointless.
	}
	else if (!strncmp(text, "reconnect", 9))
	{
		return;
	}
	else if (!strncmp(text, "packet ", 7))
	{
		tv->drop = true;	// This should never happen.
		return;
	}
	else if (!strncmp(text, "//finalscores ", sizeof("//finalscores ") - 1))
	{
		// Ah, this is a lastscores HACK from KTX
		int arg = 1, i;
		int k_ls = bound(0, tv->lastscores_idx, MAX_LASTSCORES-1);

		Cmd_TokenizeString(text + 2); // + 2 skips prefixed //

		memset(&tv->lastscores[k_ls], 0, sizeof(tv->lastscores[k_ls]));

		strlcpy(tv->lastscores[k_ls].date, Cmd_Argv(arg++), sizeof(tv->lastscores[k_ls].date));
		strlcpy(tv->lastscores[k_ls].type, Cmd_Argv(arg++), sizeof(tv->lastscores[k_ls].type));
		strlcpy(tv->lastscores[k_ls].map,  Cmd_Argv(arg++), sizeof(tv->lastscores[k_ls].map));
		strlcpy(tv->lastscores[k_ls].e1,   Cmd_Argv(arg++), sizeof(tv->lastscores[k_ls].e1));
		strlcpy(tv->lastscores[k_ls].s1,   Cmd_Argv(arg++), sizeof(tv->lastscores[k_ls].s1));
		strlcpy(tv->lastscores[k_ls].e2,   Cmd_Argv(arg++), sizeof(tv->lastscores[k_ls].e2));
		strlcpy(tv->lastscores[k_ls].s2,   Cmd_Argv(arg++), sizeof(tv->lastscores[k_ls].s2));

		// Something goes wrong.
		if ( !tv->lastscores[k_ls].date[0] )
		{
			memset(&tv->lastscores[k_ls], 0, sizeof(tv->lastscores[k_ls]));	
			return;
		}

		// Check do we have same lastcores alredy, and ignore it if we have.
		for ( i = 0; i < MAX_LASTSCORES; i++ )
		{
			if ( i == k_ls || !tv->lastscores[i].date[0] )
				continue;

			if (    !strcmp(tv->lastscores[i].date, tv->lastscores[k_ls].date)
				 && !strcmp(tv->lastscores[i].type, tv->lastscores[k_ls].type)
				 && !strcmp(tv->lastscores[i].map,  tv->lastscores[k_ls].map)
				 && !strcmp(tv->lastscores[i].e1,   tv->lastscores[k_ls].e1)
				 && !strcmp(tv->lastscores[i].e2,   tv->lastscores[k_ls].e2)
				 && !strcmp(tv->lastscores[i].s1,   tv->lastscores[k_ls].s1)
				 && !strcmp(tv->lastscores[i].s2,   tv->lastscores[k_ls].s2)
			   )
			{
				// Same lastscores, probably someone is watching demo again and again, so do not add million same lastcores.
				memset(&tv->lastscores[k_ls], 0, sizeof(tv->lastscores[k_ls]));	
				return;
			}
		}

		tv->lastscores_idx = ( k_ls + 1 ) % MAX_LASTSCORES;

		return;
	}
	else
	{
		return;
	}
}

static void ParseSetInfo(sv_t *tv, netmsg_t *m)
{
	int pnum;
	char key[64];
	char value[256];
	pnum = ReadByte(m);
	ReadString(m, key, sizeof(key));
	ReadString(m, value, sizeof(value));

	if (pnum < MAX_CLIENTS)
		Info_SetValueForStarKey(tv->players[pnum].userinfo, key, value, sizeof(tv->players[pnum].userinfo));
}

static void ParseServerinfo(sv_t *tv, netmsg_t *m)
{
	char key[64];
	char value[256];
	ReadString(m, key, sizeof(key));
	ReadString(m, value, sizeof(value));

	if (!strcmp(key, "status") && (!strcmp(value, "Standby") || !strcmp(value, "standby"))) {
		tv->match_start_time = 0;
		tv->match_start_local_curtime = 0;
	}

	if (strcmp(key, "hostname"))	// Don't allow the hostname to change, but allow the server to change other serverinfos.
		Info_SetValueForStarKey(tv->serverinfo, key, value, sizeof(tv->serverinfo));
}

static void ParsePrint(sv_t *tv, netmsg_t *m, int to, unsigned int mask, qbool printsource)
{
	char text[1024];
	int level;

	level = ReadByte(m);
	ReadString(m, text, sizeof(text)-2);

/* unused?
	if (level == 3)
	{
		strlcpy(buffer + 2, text, sizeof(buffer) - 2);
		buffer[1] = 1;
	}
	else
	{
		strlcpy(buffer + 1, text, sizeof(buffer) - 1);
	}
	buffer[0] = svc_print;
*/

	if (to == dem_all || to == dem_read)
	{
		if (level > 1)
		{
			if (printsource)
			{
				Sys_ConPrintf(tv, "");
			}
			
			if (tv->EchoInServerConsole)
				Sys_Printf("%s", text);
		}
	}
}

static void ParseCenterprint(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	char text[1024];
	ReadString(m, text, sizeof(text));
}

static int ParseList(sv_t *tv, netmsg_t *m, filename_t *list, int to, unsigned int mask)
{
	int first;

	first = ReadByte(m)+1;
	for (; first < MAX_LIST; first++)
	{
		ReadString(m, list[first].name, sizeof(list[first].name));
//		printf("read %i: %s\n", first, list[first].name);
		if (!*list[first].name)
			break;
//		printf("%i: %s\n", first, list[first].name);
	}

	return ReadByte(m);
}

static void ParseEntityState(sv_t *tv, entity_state_t *es, netmsg_t *m)	// For baselines/static entities.
{
	int i;

	es->modelindex = ReadByte(m);
	es->frame = ReadByte(m);
	es->colormap = ReadByte(m);
	es->skinnum = ReadByte(m);
	for (i = 0; i < 3; i++)
	{
		es->origin[i] = ReadCoord(tv, m);
		es->angles[i] = ReadAngle(tv, m);
	}
}

static void ParseBaseline(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int entnum;
	entnum = ReadShort(m);
	if (entnum >= MAX_ENTITIES)
	{
		ParseError(m);
		return;
	}
	ParseEntityState(tv, &tv->entity[entnum].baseline, m);
}

static void ParseStaticSound(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	if (tv->staticsound_count == MAX_STATICSOUNDS)
	{
		tv->staticsound_count--;	// Don't be fatal.
		Sys_ConPrintf(tv, "Too many static sounds\n");
	}

	tv->staticsound[tv->staticsound_count].origin[0] = ReadCoord(tv, m);
	tv->staticsound[tv->staticsound_count].origin[1] = ReadCoord(tv, m);
	tv->staticsound[tv->staticsound_count].origin[2] = ReadCoord(tv, m);
	tv->staticsound[tv->staticsound_count].soundindex = ReadByte(m);
	tv->staticsound[tv->staticsound_count].volume = ReadByte(m);
	tv->staticsound[tv->staticsound_count].attenuation = ReadByte(m);

	tv->staticsound_count++;
}

static void ParseIntermission(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	ReadCoord(tv, m);
	ReadCoord(tv, m);
	ReadCoord(tv, m);
	ReadAngle(tv, m);
	ReadAngle(tv, m);
	ReadAngle(tv, m);
}

void ParseSpawnStatic(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	if (tv->spawnstatic_count == MAX_STATICENTITIES)
	{
		tv->spawnstatic_count--;	// Don't be fatal.
		Sys_ConPrintf(tv, "Too many static entities\n");
	}

	ParseEntityState(tv, &tv->spawnstatic[tv->spawnstatic_count], m);

	tv->spawnstatic_count++;
}

//extern const usercmd_t nullcmd;
static void ParsePlayerInfo(sv_t *tv, netmsg_t *m, qbool clearoldplayers)
{
//	usercmd_t nonnullcmd;
	int flags;
	int num;
	int i;

	if (clearoldplayers)
	{
		for (i = 0; i < MAX_CLIENTS; i++)
		{	// Hide players
			// they'll be sent after this packet.
			tv->players[i].active = false;
		}
	}

	num = ReadByte(m);
	if (num >= MAX_CLIENTS)
	{
		num = 0;	// don't be fatal.
		Sys_ConPrintf(tv, "Too many svc_playerinfos, wrapping\n");
	}
	tv->players[num].old = tv->players[num].current;

	flags = ReadShort(m);
	tv->players[num].gibbed = !!(flags & DF_GIB);
	tv->players[num].dead = !!(flags & DF_DEAD);
	tv->players[num].current.frame = ReadByte(m);

	for (i = 0; i < 3; i++)
	{
		if (flags & (DF_ORIGIN << i))
			tv->players[num].current.origin[i] = ReadCoord(tv, m);
	}

	for (i = 0; i < 3; i++)
	{
		if (flags & (DF_ANGLES << i))
		{
			tv->players[num].current.angles[i] = ReadAngle16(m);
		}
	}

	if (flags & DF_MODEL)
		tv->players[num].current.modelindex = ReadByte (m);

	if (flags & DF_SKINNUM)
		tv->players[num].current.skinnum = ReadByte (m);

	if (flags & DF_EFFECTS)
		tv->players[num].current.effects = ReadByte (m);

	if (flags & DF_WEAPONFRAME)
		tv->players[num].current.weaponframe = ReadByte (m);

	tv->players[num].active = true;
}

static int readentitynum(netmsg_t *m, unsigned int *retflags)
{
	int entnum;
	unsigned int flags;
//	unsigned short moreflags = 0;

	flags = ReadShort(m);

	if (!flags)
	{
		*retflags = 0;
		return 0;
	}

	entnum = flags&511;
	flags &= ~511;

	if (flags & U_MOREBITS)
	{
		flags |= ReadByte(m);

/*		if (flags & U_EVENMORE)
			flags |= ReadByte(m)<<16;
		if (flags & U_YETMORE)
			flags |= ReadByte(m)<<24;
*/	}

/*	if (flags & U_ENTITYDBL)
		entnum += 512;
	if (flags & U_ENTITYDBL2)
		entnum += 1024;
*/
	*retflags = flags;

	return entnum;
}

static void ParseEntityDelta(sv_t *tv, netmsg_t *m, entity_state_t *old, entity_state_t *new, unsigned int flags, entity_t *ent, qbool forcerelink)
{
	memcpy(new, old, sizeof(entity_state_t));

	if (flags & U_MODEL)
		new->modelindex = ReadByte(m);
	if (flags & U_FRAME)
		new->frame = ReadByte(m);
	if (flags & U_COLORMAP)
		new->colormap = ReadByte(m);
	if (flags & U_SKIN)
		new->skinnum = ReadByte(m);
	if (flags & U_EFFECTS)
		new->effects = ReadByte(m);

	if (flags & U_ORIGIN1)
		new->origin[0] = ReadCoord(tv, m);
	if (flags & U_ANGLE1)
		new->angles[0] = ReadAngle(tv, m);
	if (flags & U_ORIGIN2)
		new->origin[1] = ReadCoord(tv, m);
	if (flags & U_ANGLE2)
		new->angles[1] = ReadAngle(tv, m);
	if (flags & U_ORIGIN3)
		new->origin[2] = ReadCoord(tv, m);
	if (flags & U_ANGLE3)
		new->angles[2] = ReadAngle(tv, m);
}

static int ExpandFrame(unsigned int newmax, frame_t *frame)
{
#if 1
	// qqshka's fixed way

	return (newmax < frame->maxents);

#else
	// FTE's auto allocation

	entity_state_t *newents;
	unsigned short *newnums;

	if (newmax < frame->maxents)
		return true;

	newmax += 16;

	Sys_Printf("max %d\n", newmax);

	newents = malloc(sizeof(*newents) * newmax);
	if (!newents)
		return false;
	newnums = malloc(sizeof(*newnums) * newmax);
	if (!newnums)
	{
		free(newents);
		return false;
	}

	memcpy(newents, frame->ents, sizeof(*newents) * frame->maxents);
	memcpy(newnums, frame->entnums, sizeof(*newnums) * frame->maxents);

	if (frame->ents)
		free(frame->ents);
	if (frame->entnums)
		free(frame->entnums);
	
	frame->ents = newents;
	frame->entnums = newnums;
	frame->maxents = newmax;
	return true;
#endif
}

static void ParsePacketEntities(sv_t *tv, netmsg_t *m, int deltaframe)
{
	frame_t *newframe;
	frame_t *oldframe;
	int oldcount;
	int newnum, oldnum;
	int newindex, oldindex;
	unsigned int flags;

	if (deltaframe != -1)
		deltaframe &= (MAX_ENTITY_FRAMES-1);

	deltaframe = tv->netchan.incoming_sequence & (MAX_ENTITY_FRAMES-1);
	tv->netchan.incoming_sequence++;
	newframe = &tv->frame[tv->netchan.incoming_sequence & (MAX_ENTITY_FRAMES-1)];

	if (deltaframe != -1)
	{
		oldframe = &tv->frame[deltaframe];
		oldcount = oldframe->numents;
	}
	else
	{
		oldframe = NULL;
		oldcount = 0;
	}

	oldindex = 0;
	newindex = 0;

	for(;;)
	{
		newnum = readentitynum(m, &flags);
		if (!newnum)
		{
			// End of packet
			// any remaining old ents need to be copied to the new frame
			while (oldindex < oldcount)
			{
				if (!ExpandFrame(newindex, newframe))
					break;

				memcpy(&newframe->ents[newindex], &oldframe->ents[oldindex], sizeof(entity_state_t));
				newframe->entnums[newindex] = oldframe->entnums[oldindex];
				newindex++;
				oldindex++;
			}
			break;
		}

		if (oldindex >= oldcount)
			oldnum = 0xffff;
		else
			oldnum = oldframe->entnums[oldindex];
		while(newnum > oldnum)
		{
			if (!ExpandFrame(newindex, newframe))
				break;

			memcpy(&newframe->ents[newindex], &oldframe->ents[oldindex], sizeof(entity_state_t));
			newframe->entnums[newindex] = oldframe->entnums[oldindex];
			newindex++;
			oldindex++;

			if (oldindex >= oldcount)
				oldnum = 0xffff;
			else
				oldnum = oldframe->entnums[oldindex];
		}

		if (newnum < oldnum)
		{	
			// This ent wasn't in the last packet.

			if (flags & U_REMOVE)
			{	
				// Remove this ent... just don't copy it across.
				continue;
			}

			if (!ExpandFrame(newindex, newframe))
				break;
			ParseEntityDelta(tv, m, &tv->entity[newnum].baseline, &newframe->ents[newindex], flags, &tv->entity[newnum], true);
			newframe->entnums[newindex] = newnum;
			newindex++;
		}
		else if (newnum == oldnum)
		{
			if (flags & U_REMOVE)
			{	
				// Remove this ent... just don't copy it across.
				oldindex++;
				continue;
			}

			if (!ExpandFrame(newindex, newframe))
				break;

			ParseEntityDelta(tv, m, &oldframe->ents[oldindex], &newframe->ents[newindex], flags, &tv->entity[newnum], false);
			newframe->entnums[newindex] = newnum;
			newindex++;
			oldindex++;
		}

	}

	newframe->numents = newindex;
	return;

/*

	// Luckilly, only updated entities are here, so that keeps cpu time down a bit.
	for (;;)
	{
		flags = ReadShort(m);
		if (!flags)
			break;

		entnum = flags & 511;
		if (tv->maxents < entnum)
			tv->maxents = entnum;
		flags &= ~511;
		memcpy(&tv->entity[entnum].old, &tv->entity[entnum].current, sizeof(entity_state_t));	//ow.
		if (flags & U_REMOVE)
		{
			tv->entity[entnum].current.modelindex = 0;
			continue;
		}
		if (!tv->entity[entnum].current.modelindex)	//lerp from baseline
		{
			memcpy(&tv->entity[entnum].current, &tv->entity[entnum].baseline, sizeof(entity_state_t));
			forcerelink = true;
		}
		else
			forcerelink = false;

		if (flags & U_MOREBITS)
			flags |= ReadByte(m);
		if (flags & U_MODEL)
			tv->entity[entnum].current.modelindex = ReadByte(m);
		if (flags & U_FRAME)
			tv->entity[entnum].current.frame = ReadByte(m);
		if (flags & U_COLORMAP)
			tv->entity[entnum].current.colormap = ReadByte(m);
		if (flags & U_SKIN)
			tv->entity[entnum].current.skinnum = ReadByte(m);
		if (flags & U_EFFECTS)
			tv->entity[entnum].current.effects = ReadByte(m);

		if (flags & U_ORIGIN1)
			tv->entity[entnum].current.origin[0] = ReadShort(m);
		if (flags & U_ANGLE1)
			tv->entity[entnum].current.angles[0] = ReadByte(m);
		if (flags & U_ORIGIN2)
			tv->entity[entnum].current.origin[1] = ReadShort(m);
		if (flags & U_ANGLE2)
			tv->entity[entnum].current.angles[1] = ReadByte(m);
		if (flags & U_ORIGIN3)
			tv->entity[entnum].current.origin[2] = ReadShort(m);
		if (flags & U_ANGLE3)
			tv->entity[entnum].current.angles[2] = ReadByte(m);

		tv->entity[entnum].updatetime = tv->curtime;
		if (!tv->entity[entnum].old.modelindex)	//no old state
			memcpy(&tv->entity[entnum].old, &tv->entity[entnum].current, sizeof(entity_state_t));	//copy the new to the old, so we don't end up with interpolation glitches
	}
*/
}

static void ParseUpdatePing(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	int pnum;
	int ping;
	pnum = ReadByte(m);
	ping = ReadShort(m);

	if (pnum < MAX_CLIENTS)
		tv->players[pnum].ping = ping;
	else
	{
		Sys_ConPrintf(tv, "svc_updateping: invalid player number\n");
	}
}

static void ParseUpdateFrags(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	int pnum;
	int frags;
	pnum = ReadByte(m);
	frags = (signed short)ReadShort(m);

	if (pnum < MAX_CLIENTS)
		tv->players[pnum].frags = frags;
	else
	{
		Sys_ConPrintf(tv, "svc_updatefrags: invalid player number\n");
	}
}

static void ParseUpdateStat(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int pnum;
	int value;
	int statnum;

	statnum = ReadByte(m);
	value = ReadByte(m);

	if (statnum < MAX_STATS)
	{
		for (pnum = 0; pnum < MAX_CLIENTS; pnum++)
		{
			if (mask & (1<<pnum))
				tv->players[pnum].stats[statnum] = value;
		}
	}
	else
	{
		Sys_ConPrintf(tv, "svc_updatestat: invalid stat number\n");
	}
}

static void ParseUpdateStatLong(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int pnum;
	int value;
	int statnum;

	statnum = ReadByte(m);
	value = ReadLong(m);

	if (statnum == STAT_MATCHSTARTTIME && value != tv->match_start_time) {
		tv->match_start_time = value;
		tv->match_start_local_curtime = tv->curtime;
	}

	if (statnum < MAX_STATS)
	{
		for (pnum = 0; pnum < MAX_CLIENTS; pnum++)
		{
			if (mask & (1<<pnum))
				tv->players[pnum].stats[statnum] = value;
		}
	}
	else
	{
		Sys_ConPrintf(tv, "svc_updatestatlong: invalid stat number\n");
	}
}

static void ParseUpdateUserinfo(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	int pnum, id;
	pnum = ReadByte(m);
	id = ReadLong(m);
	if (pnum < MAX_CLIENTS)
	{
		tv->players[pnum].userid = id;
		ReadString(m, tv->players[pnum].userinfo, sizeof(tv->players[pnum].userinfo));
	}
	else
	{
		Sys_ConPrintf(tv, "svc_updateuserinfo: invalid player number\n");
		while (ReadByte(m))	//suck out the message.
		{
		}
	}
}

static void ParsePacketloss(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int pnum;
	int value;

	pnum = ReadByte(m)%MAX_CLIENTS;
	value = ReadByte(m);

	if (pnum < MAX_CLIENTS)
		tv->players[pnum].packetloss = value;
	else
	{
		Sys_ConPrintf(tv, "svc_updatepl: invalid player number\n");
	}
}

static void ParseUpdateEnterTime(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	unsigned int pnum;
	float value;

	pnum = ReadByte(m)%MAX_CLIENTS;
	value = ReadFloat(m);

	if (pnum < MAX_CLIENTS)
		tv->players[pnum].entertime = value;
	else
	{
		Sys_ConPrintf(tv, "svc_updateentertime: invalid player number\n");
	}
}

static void ParseSound(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
#define	SND_VOLUME		(1<<15)		// a qbyte
#define	SND_ATTENUATION	(1<<14)		// a qbyte

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0
	int i;
	int channel;

	channel = (unsigned short)ReadShort(m);
	if (channel & SND_VOLUME) {
		ReadByte(m); // vol
	}
	if (channel & SND_ATTENUATION) {
		ReadByte(m); // atten
	}
	ReadByte(m); // sound_num
	for (i = 0; i < 3; i++) {
		ReadCoord(tv, m);
	}
}

static void ParseDamage(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	ReadByte (m);
	ReadByte (m);
	ReadCoord (tv, m);
	ReadCoord (tv, m);
	ReadCoord (tv, m);
}

enum {
	TE_SPIKE			= 0,
	TE_SUPERSPIKE		= 1,
	TE_GUNSHOT			= 2,
	TE_EXPLOSION		= 3,
	TE_TAREXPLOSION		= 4,
	TE_LIGHTNING1		= 5,
	TE_LIGHTNING2		= 6,
	TE_WIZSPIKE			= 7,
	TE_KNIGHTSPIKE		= 8,
	TE_LIGHTNING3		= 9,
	TE_LAVASPLASH		= 10,
	TE_TELEPORT			= 11,

	TE_BLOOD			= 12,
	TE_LIGHTNINGBLOOD	= 13,
};

static void ParseTempEntity(sv_t *tv, netmsg_t *m, int to, unsigned int mask)
{
	int i;

	i = ReadByte (m);
	switch(i)
	{
	case TE_SPIKE:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_SUPERSPIKE:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_GUNSHOT:
		ReadByte (m);

		ReadCoord (tv, m);
		ReadCoord (tv, m);
		ReadCoord (tv, m);
		break;
	case TE_EXPLOSION:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_TAREXPLOSION:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_LIGHTNING1:
	case TE_LIGHTNING2:
	case TE_LIGHTNING3:
		ReadShort (m);

		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);

		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_WIZSPIKE:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_KNIGHTSPIKE:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_LAVASPLASH:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_TELEPORT:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_BLOOD:
		ReadByte(m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	case TE_LIGHTNINGBLOOD:
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		ReadCoord(tv, m);
		break;
	default:
		Sys_ConPrintf(tv, "temp entity %i not recognised\n", i);
		return;
	}
}

void ParseLightstyle(sv_t *tv, netmsg_t *m)
{
	int style;
	style = ReadByte(m);
	if (style < MAX_LIGHTSTYLES)
		ReadString(m, tv->lightstyle[style].name, sizeof(tv->lightstyle[style].name));
	else
	{
		Sys_ConPrintf(tv, "svc_lightstyle: invalid lightstyle index (%i)\n", style);
		while (ReadByte(m))	// Suck out the message.
		{
		}
	}
}

void ParseNails(sv_t *tv, netmsg_t *m, qbool nails2)
{
	int count = (unsigned char)ReadByte(m);
	int i;

	while(count-- > 0)
	{
		if (nails2)
			ReadByte(m);
		for (i = 0; i < 6; i++)
			ReadByte(m);
	}
}

void ParseDownload(sv_t *tv, netmsg_t *m)
{
	int size, b;

	size = (signed short)ReadShort(m);
	ReadByte(m); // percent

	if (size < 0)
	{
		Sys_ConPrintf(tv, "Downloading failed\n");
		tv->drop = true;
		return;
	}

	for (b = 0; b < size; b++)
		ReadByte(m);
}

void ParseDisconnect(sv_t *tv, netmsg_t *m)
{
	#define ENDOFDEMO "EndOfDemo" // this is what mvdsv append to end of demo/stream

	char EndOfDemo[256] = {0};

	ReadString(m, EndOfDemo, sizeof(EndOfDemo));

	if (strcmp(EndOfDemo, ENDOFDEMO))
	{
		Sys_ConPrintf(tv, "WARNING: non standard disconnect message '%s'\n", EndOfDemo);
	}
}

//#ifdef FTE_PEXT2_VOICECHAT
void ParseFTEVoiceChat(sv_t *tv, netmsg_t *m)
{
	int bytes;

	/* sender = */ ReadByte(m);
	/* gen = */    ReadByte(m);
	/* seq =*/     ReadByte(m);
	bytes =		   (signed short)ReadShort(m);

	if (bytes < 0)
	{
		Sys_ConPrintf(tv, "Voice chat failed\n");
		tv->drop = true;
		return;
	}

	for (; bytes; bytes--)
		ReadByte(m);
}
//#endif

void ShowMvdHeaderInfo(sv_t *tv, int length, int to, int mask)
{
	char *str_to = "unknown";

	if (!shownet.integer)
		return;

	switch (to)
	{
		case dem_multiple:
			str_to = "dem_multiple";
			break;
		case dem_single:
			str_to = "dem_single";
			break;
		case dem_stats:
			str_to = "dem_stats";
			break;
		case dem_read:
			str_to = "dem_read";
			break;
		case dem_all:
			str_to = "dem_all";
			break;
		default:
			str_to = "unknown";
			break;
	}

	Sys_ConPrintf(tv, "MVD msg: size: %4d type: %s mask: 0x%x\n", length, str_to, mask);

	// List the names of the players that will receive this message.
	if ((shownet.integer >= 2) && (to != dem_all) && (to != dem_read))
	{
		int player;
		qbool first = true;
		char name[64];

		if (tv->EchoInServerConsole)
		{
			Sys_Printf("  [");
		
			for (player = 0; player < MAX_CLIENTS; player++)
			{	
				// Is this player supposed to receive the message?
				if (mask & (1 << player))
				{
					Info_ValueForKey(tv->players[player].userinfo, "name", name, sizeof(name));
					
					if (!first)
					{
						Sys_Printf(", ");
					}

					Sys_Printf(name);
					first = false;
				}
			}

			Sys_Printf("]\n");
		}
	}
}

void ParseMessage(sv_t *tv, char *buffer, int length, int to, int mask)
{
	svcOps_e svc;
	int i;
	netmsg_t buf;
	qbool clearoldplayers = true;

	if (shownet.integer)
		ShowMvdHeaderInfo(tv, length, to, mask);

	InitNetMsg(&buf, buffer, length);
	buf.cursize = length;

	while (buf.readpos < buf.cursize)
	{
		if (buf.readpos > buf.cursize)
		{
			Sys_ConPrintf(tv, "Read past end of parse buffer\n");
			return;
		}

		buf.startpos = buf.readpos;

		svc = ReadByte(&buf);

		if (shownet.integer)
		{
			Sys_ConPrintf(tv, "svc: %3d: %s\n", svc, ( svc >=0 && svc < svc_strings_size && svc_strings[svc] ) ? svc_strings[svc] : "UNKNOWN");
		}

		// Only print the server name once if we get several consecutive svc_print messages
		// since one message can be split up in several pieces.
		tv->svc_print_servername = !(tv->prev_was_svc_print && (svc == svc_print));
		tv->prev_was_svc_print = false;

		switch (svc)
		{
			case svc_bad:
			{
				ParseError(&buf);
				Sys_ConPrintf(tv, "ParseMessage: svc_bad\n");
				return;
			}
			case svc_nop:	// Quakeworld isn't meant to send these.
			{
				Sys_ConPrintf(tv, "svc_nop\n");
				break;
			}
			case svc_disconnect:
			{
				// Spike:
				// mvdsv safely terminates it's mvds with an svc_disconnect.
				// the client is meant to read that and disconnect without reading the intentionally corrupt packet following it.
				// however, our demo playback is chained and looping and buffered.
				// so we've already found the end of the source file and restarted parsing.
				// so there's very little we can do except crash ourselves on the EndOfDemo text following the svc_disconnect
				// that's a bad plan, so just stop reading this packet.

				// qqshka: no, we trying parse it

				ParseDisconnect(tv, &buf);
				break;
			}
			case svc_updatestat:
			{
				ParseUpdateStat(tv, &buf, to, mask);
				break;
			}
			// svc_version			4	// [long] server version
			// svc_setview			5	// [short] entity number
			case svc_sound:
			{
				ParseSound(tv, &buf, to, mask);
				break;
			}
			// svc_time			7	// [float] server time
			case svc_print:
			{
				ParsePrint(tv, &buf, to, mask, tv->svc_print_servername);
				tv->prev_was_svc_print = true;
				break;
			}
			case svc_stufftext:
			{
				ParseStufftext(tv, &buf, to, mask);
				break;
			}
			case svc_setangle:
			{
				ReadByte(&buf);
				#if 0
				tv->proxyplayerangles[0] = ReadByte(&buf)*360.0/255;
				tv->proxyplayerangles[1] = ReadByte(&buf)*360.0/255;
				tv->proxyplayerangles[2] = ReadByte(&buf)*360.0/255;
				#else
				ReadAngle(tv, &buf);
				ReadAngle(tv, &buf);
				ReadAngle(tv, &buf);
				#endif
				break;
			}
			case svc_serverdata:
			{
				ParseServerData(tv, &buf, to, mask);
				break;
			}
			case svc_lightstyle:
			{
				ParseLightstyle(tv, &buf);
				break;
			}
			// svc_updatename		13	// [qbyte] [string]
			case svc_updatefrags:
			{
				ParseUpdateFrags(tv, &buf, to, mask);
				break;
			}
			// svc_clientdata		15	// <shortbits + data>
			// svc_stopsound		16	// <see code>
			// svc_updatecolors		17	// [qbyte] [qbyte] [qbyte]
			case svc_particle:
			{
				ReadCoord(tv, &buf);
				ReadCoord(tv, &buf);
				ReadCoord(tv, &buf);
				ReadByte(&buf);
				ReadByte(&buf);
				ReadByte(&buf);
				ReadByte(&buf);
				ReadByte(&buf);
				break;
			}
			case svc_damage:
			{
				ParseDamage(tv, &buf, to, mask);
				break;
			}
			case svc_spawnstatic:
			{
				ParseSpawnStatic(tv, &buf, to, mask);
				break;
			}
			// svc_spawnstatic2	21
			case svc_spawnbaseline:
			{
				ParseBaseline(tv, &buf, to, mask);
				break;
			}
			case svc_temp_entity:
			{
				ParseTempEntity(tv, &buf, to, mask);
				break;
			}
			case svc_setpause:	// [qbyte] on / off
			{
				tv->paused = ReadByte(&buf);
				break;
			}
			// svc_signonnum		25	// [qbyte]  used for the signon sequence
			case svc_centerprint:
			{
				ParseCenterprint(tv, &buf, to, mask);
				break;
			}
			case svc_killedmonster:
			{
				break;
			}
			case svc_foundsecret:
			{
				break;
			}			
			case svc_spawnstaticsound:
			{
				ParseStaticSound(tv, &buf, to, mask);
				break;
			}
			case svc_intermission:
			{
				ParseIntermission(tv, &buf, to, mask);
				break;
			}
			case svc_finale:
			{
				while(ReadByte(&buf))
					;
				break;
			}
			case svc_cdtrack:
			{
				ParseCDTrack(tv, &buf, to, mask);
				break;
			}
			// svc_sellscreen		33
			case svc_smallkick:
			case svc_bigkick:
				break;
			case svc_updateping:
			{
				ParseUpdatePing(tv, &buf, to, mask);
				break;
			}
			case svc_updateentertime:
			{
				ParseUpdateEnterTime(tv, &buf, to, mask);
				break;
			}
			case svc_updatestatlong:
			{
				ParseUpdateStatLong(tv, &buf, to, mask);
				break;
			}
			case svc_muzzleflash:
			{
				ReadShort(&buf);
				break;
			}
			case svc_updateuserinfo:
			{
				ParseUpdateUserinfo(tv, &buf, to, mask);
				break;
			}
			case svc_download:	// [short] size [size bytes]
			{
				ParseDownload(tv, &buf);
				break;
			}
			case svc_playerinfo:
			{
				ParsePlayerInfo(tv, &buf, clearoldplayers);
				clearoldplayers = false;
				break;
			}
			case svc_nails:
			{			
				ParseNails(tv, &buf, false);
				break;
			}
			case svc_chokecount:
			{
				ReadByte(&buf);
				break;
			}
			case svc_modellist:
			{
				const char* map;

				i = ParseList(tv, &buf, tv->modellist, to, mask);

				// If the map doesn't have friendly descriptive name, use basic instead
				map = tv->modellist[1].name;
				if (map[0] && !tv->mapname[0]) {
					const char* trimmed = strchr(map, '/');

					strlcpy(tv->mapname, trimmed && trimmed[1] ? trimmed + 1 : map, sizeof(tv->mapname));
				}

				if (!i) {
					strlcpy(tv->status, "Prespawning", sizeof(tv->status));
				}
				break;
			}
			case svc_soundlist:
			{
				i = ParseList(tv, &buf, tv->soundlist, to, mask);
				if (!i)
					strlcpy(tv->status, "Receiving modellist", sizeof(tv->status));
				break;
			}
			case svc_packetentities:
			{
				ParsePacketEntities(tv, &buf, -1);
				break;
			}
			case svc_deltapacketentities:
			{
				ParsePacketEntities(tv, &buf, ReadByte(&buf));
				break;
			}
			case svc_entgravity:		// Gravity change, for prediction
			{
				ReadFloat(&buf);
				break;
			}
			case svc_maxspeed:
			{
				ReadFloat(&buf);
				break;
			}
			case svc_setinfo:
			{
				ParseSetInfo(tv, &buf);
				break;
			}
			case svc_serverinfo:
			{
				ParseServerinfo(tv, &buf);
				break;
			}
			case svc_updatepl:
			{
				ParsePacketloss(tv, &buf, to, mask);
				break;
			}
			case svc_nails2:
			{
				ParseNails(tv, &buf, true);
				break;
			}
// FTE voice chat.
//#ifdef FTE_PEXT2_VOICECHAT
			case svc_fte_voicechat:
			{
				ParseFTEVoiceChat(tv, &buf);
				break;
			}
//#endif
			default:
			{
				unsigned int message_type;
				buf.readpos = buf.startpos;
				message_type = (unsigned int)ReadByte(&buf);
				
				if ((message_type >= 0) && (message_type < svc_strings_size))
				{
					Sys_ConPrintf(tv, "Can't handle %s (%i)\n", svc_strings[message_type], message_type);
				}
				else
				{
					Sys_ConPrintf(tv, "Can't handle svc %i\n", message_type);
				}

				return;
			}
		}
	}
}

