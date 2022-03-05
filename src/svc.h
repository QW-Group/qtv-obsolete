// svc.h

// 1-10

SVC(svc_bad)
SVC(svc_nop)
SVC(svc_disconnect)
SVC(svc_updatestat)				// [qbyte] [qbyte]
SVC(svc_version_UNUSED)			// [long] server version
SVC(svc_nqsetview)				// [short] entity number
SVC(svc_sound)					// <see code>
SVC(svc_nqtime)					// [float] server time
SVC(svc_print)					// [qbyte] id [string] null terminated string
SVC(svc_stufftext)				// [string] stuffed into client's console buffer
								// the string should be \n terminated
SVC(svc_setangle)				// [angle3] set the view angle to this absolute value

// 11-20

SVC(svc_serverdata)				// [long] protocol ...
SVC(svc_lightstyle)				// [qbyte] [string]
SVC(svc_nqupdatename)			// [qbyte] [string]
SVC(svc_updatefrags)			// [qbyte] [short]
SVC(svc_nqclientdata)			// <shortbits + data>
SVC(svc_stopsound_UNUSED)		// <see code>
SVC(svc_nqupdatecolors)			// [qbyte] [qbyte] [qbyte]
SVC(svc_particle)				// [vec3] <variable>
SVC(svc_damage)

SVC(svc_spawnstatic)

// 21-30

SVC(svc_spawnstatic2_UNUSED)
SVC(svc_spawnbaseline)

SVC(svc_temp_entity)			// variable
SVC(svc_setpause)				// [qbyte] on / off
SVC(svc_nqsignonnum)			// [qbyte]  used for the signon sequence

SVC(svc_centerprint)			// [string] to put in center of the screen

SVC(svc_killedmonster)
SVC(svc_foundsecret)

SVC(svc_spawnstaticsound)		// [coord3] [qbyte] samp [qbyte] vol [qbyte] aten

SVC(svc_intermission)			// [vec3_t] origin [vec3_t] angle

// 31-40

SVC(svc_finale)					// [string] text

SVC(svc_cdtrack)				// [qbyte] track
SVC(svc_sellscreen_UNUSED)

// used number 34, same as svc_smallkick, so...
//SVC(svc_cutscene_UNUSED)		//hmm... nq only... added after qw tree splitt?

SVC(svc_smallkick)				// set client punchangle to 2
SVC(svc_bigkick)				// set client punchangle to 4

SVC(svc_updateping)				// [qbyte] [short]
SVC(svc_updateentertime)		// [qbyte] [float]

SVC(svc_updatestatlong)			// [qbyte] [long]

SVC(svc_muzzleflash)			// [short] entity

SVC(svc_updateuserinfo)			// [qbyte] slot [long] uid
								// [string] userinfo

// 41-50

SVC(svc_download)				// [short] size [size bytes]
SVC(svc_playerinfo)				// variable
SVC(svc_nails)					// [qbyte] num [48 bits] xyzpy 12 12 12 4 8
SVC(svc_chokecount)				// [qbyte] packets choked
SVC(svc_modellist)				// [strings]
SVC(svc_soundlist)				// [strings]
SVC(svc_packetentities)			// [...]
SVC(svc_deltapacketentities)	// [...]
SVC(svc_maxspeed)				// maxspeed change, for prediction
SVC(svc_entgravity)				// gravity change, for prediction

// 51-60

SVC(svc_setinfo)				// setinfo on a client
SVC(svc_serverinfo)				// serverinfo
SVC(svc_updatepl)				// [qbyte] [qbyte]
SVC(svc_nails2)					// qwe - [qbyte] num [52 bits] nxyzpy 8 12 12 12 4 8

SVC(svc_unused_55)
SVC(svc_unused_56)
SVC(svc_unused_57)
SVC(svc_unused_58)
SVC(svc_unused_59)
SVC(svc_unused_60)
SVC(svc_unused_61)
SVC(svc_unused_62)
SVC(svc_unused_63)
SVC(svc_unused_64)
SVC(svc_unused_65)
SVC(svc_unused_66)
SVC(svc_unused_67)
SVC(svc_unused_68)
SVC(svc_unused_69)
SVC(svc_unused_70)
SVC(svc_unused_71)
SVC(svc_unused_72)
SVC(svc_unused_73)
SVC(svc_unused_74)
SVC(svc_unused_75)
SVC(svc_unused_76)
SVC(svc_unused_77)
SVC(svc_unused_78)
SVC(svc_unused_79)
SVC(svc_unused_80)
SVC(svc_unused_81)
SVC(svc_unused_82)
SVC(svc_unused_83)
//#ifdef FTE_PEXT2_VOICECHAT
SVC(svc_fte_voicechat)			// FTE voice chat.
//#endif

