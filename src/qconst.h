/*
 qconst.h
*/

#ifndef __QCONST_H__
#define __QCONST_H__

//======================================
#define MAX_QPATH		64

//======================================

#define MAX_CLIENTS			32

#define	PROTOCOL_VERSION	28

//======================================

#define	MAX_SERVERINFO_STRING	1024		// standard quake has 512 here.
#define MAX_USERINFO			192
#define MAX_INFO_KEY 			64

#define MAX_LIST	256
#define MAX_MODELS	MAX_LIST
#define MAX_SOUNDS	MAX_LIST
#define MAX_ENTITIES		512
#define MAX_STATICSOUNDS	64
#define MAX_STATICENTITIES	128
#define MAX_LIGHTSTYLES		64

#define MAX_ENTITY_FRAMES	64

//======================================

#define MAX_STATS 32
#define	STAT_HEALTH			0
#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster
#define STAT_ITEMS			15

#define STAT_TIME 17	//A ZQ hack, sending time via a stat.
						//this allows l33t engines to interpolate properly without spamming at a silly high fps.
#define STAT_MATCHSTARTTIME     18		// Server should send this as msec (int)

//======================================
//flags on entities
#define	U_ORIGIN1	(1<<9)
#define	U_ORIGIN2	(1<<10)
#define	U_ORIGIN3	(1<<11)
#define	U_ANGLE2	(1<<12)
#define	U_FRAME		(1<<13)
#define	U_REMOVE	(1<<14)		// REMOVE this entity, don't add it
#define	U_MOREBITS	(1<<15)

// if MOREBITS is set, these additional flags are read in next
#define	U_ANGLE1	(1<<0)
#define	U_ANGLE3	(1<<1)
#define	U_MODEL		(1<<2)
#define	U_COLORMAP	(1<<3)
#define	U_SKIN		(1<<4)
#define	U_EFFECTS	(1<<5)
#define	U_SOLID		(1<<6)		// the entity should be solid for prediction

//======================================
//flags on players in mvds
#define DF_ORIGIN		(1<<0)
#define DF_ANGLES		(1<<3)
#define DF_EFFECTS		(1<<6)
#define DF_SKINNUM		(1<<7)
#define DF_DEAD			(1<<8)
#define DF_GIB			(1<<9)
#define DF_WEAPONFRAME	(1<<10)
#define DF_MODEL		(1<<11)
//======================================
// svc list
//

// look svc.h for details

#define SVC(svc) svc,
typedef enum 
{
	#include "svc.h"
} svcOps_e;
#undef SVC

//======================================
// svc_print messages have an id, so messages can be filtered
#define	PRINT_LOW		0
#define	PRINT_MEDIUM	1
#define	PRINT_HIGH		2
#define	PRINT_CHAT		3	// also go to chat buffer

//======================================
// qtv clc list
//

#define qtv_clc_stringcmd    1


//======================================
// qtv flushing/proxy client state
//

//					false	0 == we are ready to recive mvd data and etc
//					true	1 == wait when we flush our buffer and issue Net_SendConnectionMVD()

#define PS_WAIT_SOUNDLIST	2
#define PS_WAIT_MODELLIST	3
#define PS_WAIT_SPAWN		4


#endif // __QCONST_H__

