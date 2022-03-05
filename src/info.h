
// info.h

#ifndef __INFO_H__
#define __INFO_H__

#define INFO_HASHPOOL_SIZE 256

typedef struct info_s {
	struct info_s	*hash_next;
	struct info_s	*next;

	char				*name;
	char				*value;

} info_t;

typedef struct ctxinfo_s {

	info_t	*info_hash[INFO_HASHPOOL_SIZE];
	info_t	*info_list;

	int		cur; // current infos
	int		max; // max    infos

} ctxinfo_t;

//Cvar system calls this when a CVAR_SERVERINFO cvar changes
void			SV_ServerinfoChanged (char *key, char *string);

// get key in buffer
char			*Info_ValueForKey (const char *s, const char *const key, char *const buffer, size_t buffersize);
// remove key from buffer
void			Info_RemoveKey (char *s, const char *key);
// put key in buffer
void			Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize);
// print infostring on console
void			Info_Print (char *s);



//============================================================
// Alternative variant manipulation with info strings
//============================================================



// return value for given key
char			*Info_Get(ctxinfo_t *ctx, const char *name, char *buf, int bufsize);
// set value for given key
qbool			Info_Set (ctxinfo_t *ctx, const char *name, const char *value);
// remove given key
qbool			Info_Remove (ctxinfo_t *ctx, const char *name);
// remove all infos
void			Info_RemoveAll (ctxinfo_t *ctx);
// convert old way infostring to new way: \name\qqshka\noaim\1\ to hashed variant
qbool			Info_Convert(ctxinfo_t *ctx, char *str);

void			Info_PrintList(ctxinfo_t *ctx);

// this enables some debug tools
//#define INFODEBUG 1

void			Info_Init(void);

#endif /* !__INFO_H__ */

