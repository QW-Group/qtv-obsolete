/*
	qw.c
*/

#include "qtv.h"

void QTV_DefaultMovevars(movevars_t *vars)
{
	vars->gravity			= 800;
	vars->maxspeed			= 320;
	vars->spectatormaxspeed = 500;
	vars->accelerate		= 10;
	vars->airaccelerate		= 0.7f;
	vars->waterfriction		= 4;
	vars->entgrav			= 1;
	vars->stopspeed			= 10;
	vars->wateraccelerate	= 10;
	vars->friction			= 4;
}

void BuildServerData(sv_t *tv, netmsg_t *msg, int servercount)
{
	WriteByte(msg, svc_serverdata);
	if (tv->extension_flags_fte1) {
		WriteLong(msg, PROTOCOL_VERSION_FTE);
		WriteLong(msg, tv->extension_flags_fte1);
	}
	if (tv->extension_flags_fte2) {
		WriteLong(msg, PROTOCOL_VERSION_FTE2);
		WriteLong(msg, tv->extension_flags_fte2);
	}
	if (tv->extension_flags_mvd1) {
		WriteLong(msg, PROTOCOL_VERSION_MVD1);
		WriteLong(msg, tv->extension_flags_mvd1);
	}
	WriteLong(msg, PROTOCOL_VERSION);
	WriteLong(msg, servercount);

	WriteString(msg, tv->gamedir);

	WriteFloat(msg, 0);
	WriteString(msg, tv->mapname);

	// get the movevars
	WriteFloat(msg, tv->movevars.gravity);
	WriteFloat(msg, tv->movevars.stopspeed);
	WriteFloat(msg, tv->movevars.maxspeed);
	WriteFloat(msg, tv->movevars.spectatormaxspeed);
	WriteFloat(msg, tv->movevars.accelerate);
	WriteFloat(msg, tv->movevars.airaccelerate);
	WriteFloat(msg, tv->movevars.wateraccelerate);
	WriteFloat(msg, tv->movevars.friction);
	WriteFloat(msg, tv->movevars.waterfriction);
	WriteFloat(msg, tv->movevars.entgrav);

	WriteByte(msg, svc_stufftext);
	WriteString2(msg, "fullserverinfo \"");
	WriteString2(msg, tv->serverinfo);
	WriteString(msg, "\"\n");
}

int SendList(sv_t *qtv, int first, const filename_t *list, int svc, netmsg_t *msg)
{
	int i;

	WriteByte(msg, svc);
	WriteByte(msg, first);
	for (i = first+1; i < 256; i++)
	{
//		printf("write %i: %s\n", i, list[i].name);
		WriteString(msg, list[i].name);
		if (!*list[i].name)	//fixme: this probably needs testing for where we are close to the limit
		{	//no more
			WriteByte(msg, 0);
			return -1;
		}

		if (msg->cursize > 768)
		{	//truncate
			i--;
			break;
		}
	}
	WriteByte(msg, 0);
	WriteByte(msg, i);

	return i;
}

int SendCurrentUserinfos(sv_t *tv, int cursize, netmsg_t *msg, int i, int thisplayer)
{
	if (i < 0)
		return i;
	if (i >= MAX_CLIENTS)
		return i;

	for (; i < MAX_CLIENTS; i++)
	{

/* hrm
		if (i == thisplayer && (!tv || !(tv->controller || tv->proxyplayer)))
		{
			WriteByte(msg, svc_updateuserinfo);
			WriteByte(msg, i);
			WriteLong(msg, i);
			WriteString2(msg, "\\*spectator\\1\\name\\");

			if (tv && tv->hostname[0])
				WriteString(msg, tv->hostname);
			else
				WriteString(msg, "FTEQTV");


			WriteByte(msg, svc_updatefrags);
			WriteByte(msg, i);
			WriteShort(msg, 9999);

			WriteByte(msg, svc_updateping);
			WriteByte(msg, i);
			WriteShort(msg, 0);

			WriteByte(msg, svc_updatepl);
			WriteByte(msg, i);
			WriteByte(msg, 0);

			continue;
		}
*/

		if (!tv)
			continue;

		if (msg->cursize+cursize+strlen(tv->players[i].userinfo) > 768)
		{
			return i;
		}

		WriteByte(msg, svc_updateuserinfo);
		WriteByte(msg, i);
		WriteLong(msg, tv->players[i].userid);
		WriteString(msg, tv->players[i].userinfo);

		WriteByte(msg, svc_updatefrags);
		WriteByte(msg, i);
		WriteShort(msg, tv->players[i].frags);

		WriteByte(msg, svc_updateping);
		WriteByte(msg, i);
		WriteShort(msg, tv->players[i].ping);

		WriteByte(msg, svc_updatepl);
		WriteByte(msg, i);
		WriteByte(msg, tv->players[i].packetloss);
	}

	i++;

	return i;
}

void WriteEntityState(sv_t *tv, netmsg_t *msg, entity_state_t *es)
{
	int i;
	WriteByte(msg, es->modelindex);
	WriteByte(msg, es->frame);
	WriteByte(msg, es->colormap);
	WriteByte(msg, es->skinnum);
	for (i = 0; i < 3; i++)
	{
		WriteCoord(tv, msg, es->origin[i]);
		WriteAngle(tv, msg, es->angles[i]);
	}
}

int SendCurrentBaselines(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_ENTITIES)
		return i;

	for (; i < MAX_ENTITIES; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}

		if (tv->entity[i].baseline.modelindex)
		{
			WriteByte(msg, svc_spawnbaseline);
			WriteShort(msg, i);
			WriteEntityState(tv, msg, &tv->entity[i].baseline);
		}
	}

	return i;
}

int SendCurrentLightmaps(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_LIGHTSTYLES)
		return i;

	for (; i < MAX_LIGHTSTYLES; i++)
	{
		if (msg->cursize+cursize+strlen(tv->lightstyle[i].name) > maxbuffersize)
		{
			return i;
		}
		WriteByte(msg, svc_lightstyle);
		WriteByte(msg, i);
		WriteString(msg, tv->lightstyle[i].name);
	}
	return i;
}

int SendStaticSounds(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_STATICSOUNDS)
		return i;

	for (; i < MAX_STATICSOUNDS; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}
		if (!tv->staticsound[i].soundindex)
			continue;

		WriteByte(msg, svc_spawnstaticsound);
		WriteCoord(tv, msg, tv->staticsound[i].origin[0]);
		WriteCoord(tv, msg, tv->staticsound[i].origin[1]);
		WriteCoord(tv, msg, tv->staticsound[i].origin[2]);
		WriteByte(msg, tv->staticsound[i].soundindex);
		WriteByte(msg, tv->staticsound[i].volume);
		WriteByte(msg, tv->staticsound[i].attenuation);
	}

	return i;
}

int SendStaticEntities(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_STATICENTITIES)
		return i;

	for (; i < MAX_STATICENTITIES; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}
		if (!tv->spawnstatic[i].modelindex)
			continue;

		WriteByte(msg, svc_spawnstatic);
		WriteEntityState(tv, msg, &tv->spawnstatic[i]);
	}

	return i;
}

//returns the next prespawn 'buffer' number to use, or -1 if no more
int Prespawn(sv_t *qtv, int curmsgsize, netmsg_t *msg, int bufnum, int thisplayer)
{
	int r, ni;
	r = bufnum;

	ni = SendCurrentUserinfos(qtv, curmsgsize, msg, bufnum, thisplayer);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_CLIENTS;

	ni = SendCurrentBaselines(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_ENTITIES;

	ni = SendCurrentLightmaps(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_LIGHTSTYLES;

	ni = SendStaticSounds(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_STATICSOUNDS;

	ni = SendStaticEntities(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_STATICENTITIES;

	if (bufnum == 0)
		return -1;

	return r;
}

void SV_WriteDelta(sv_t* tv, int entnum, const entity_state_t *from, const entity_state_t *to, netmsg_t *msg, qbool force)
{
	unsigned int i;
	unsigned int bits;

	bits = 0;
	if (from->angles[0] != to->angles[0])
		bits |= U_ANGLE1;
	if (from->angles[1] != to->angles[1])
		bits |= U_ANGLE2;
	if (from->angles[2] != to->angles[2])
		bits |= U_ANGLE3;

	if (from->origin[0] != to->origin[0])
		bits |= U_ORIGIN1;
	if (from->origin[1] != to->origin[1])
		bits |= U_ORIGIN2;
	if (from->origin[2] != to->origin[2])
		bits |= U_ORIGIN3;

	if (from->colormap != to->colormap)
		bits |= U_COLORMAP;
	if (from->skinnum != to->skinnum)
		bits |= U_SKIN;
	if (from->modelindex != to->modelindex)
		bits |= U_MODEL;
	if (from->frame != to->frame)
		bits |= U_FRAME;
	if (from->effects != to->effects)
		bits |= U_EFFECTS;

	if (bits & 255)
		bits |= U_MOREBITS;



	if (!bits && !force)
		return;

	i = (entnum&511) | (bits&~511);
	WriteShort (msg, i);

	if (bits & U_MOREBITS)
		WriteByte (msg, bits&255);
/*
#ifdef PROTOCOLEXTENSIONS
	if (bits & U_EVENMORE)
		WriteByte (msg, evenmorebits&255);
	if (evenmorebits & U_YETMORE)
		WriteByte (msg, (evenmorebits>>8)&255);
#endif
*/
	if (bits & U_MODEL)
		WriteByte (msg,	to->modelindex&255);
	if (bits & U_FRAME)
		WriteByte (msg, to->frame);
	if (bits & U_COLORMAP)
		WriteByte (msg, to->colormap);
	if (bits & U_SKIN)
		WriteByte (msg, to->skinnum);
	if (bits & U_EFFECTS)
		WriteByte (msg, to->effects&0x00ff);
	if (bits & U_ORIGIN1)
		WriteCoord (tv, msg, to->origin[0]);
	if (bits & U_ANGLE1)
		WriteAngle (tv, msg, to->angles[0]);
	if (bits & U_ORIGIN2)
		WriteCoord (tv, msg, to->origin[1]);
	if (bits & U_ANGLE2)
		WriteAngle (tv, msg, to->angles[1]);
	if (bits & U_ORIGIN3)
		WriteCoord (tv, msg, to->origin[2]);
	if (bits & U_ANGLE3)
		WriteAngle (tv, msg, to->angles[2]);
}

void Prox_SendInitialEnts(sv_t *qtv, oproxy_t *prox, netmsg_t *msg)
{
	frame_t *frame;
	int i, entnum;
	WriteByte(msg, svc_packetentities);
	frame = &qtv->frame[qtv->netchan.incoming_sequence & (MAX_ENTITY_FRAMES-1)];
	for (i = 0; i < frame->numents; i++)
	{
		entnum = frame->entnums[i];
		SV_WriteDelta(qtv, entnum, &qtv->entity[entnum].baseline, &frame->ents[i], msg, true);
	}
	WriteShort(msg, 0);
}

