/*
	msg.c
*/

#include "qtv.h"

#define Q_rint(x) ((x) > 0 ? (int) ((x) + 0.5) : (int) ((x) - 0.5))

void InitNetMsg(netmsg_t *b, char *buffer, int bufferlength)
{
	memset (b, 0, sizeof (*b));
	b->data = buffer;
	b->maxsize = bufferlength;
}

void ClearNetMsg(netmsg_t *b)
{
	b->cursize = 0;
	b->readpos = 0;
	b->startpos = 0;
}

//probably not the place for these any more..
unsigned char ReadByte(netmsg_t *b)
{
	if (b->readpos >= b->cursize)
	{
		b->readpos = b->cursize+1;
		return 0;
	}
	return b->data[b->readpos++];
}

unsigned short ReadShort(netmsg_t *b)
{
	int b1, b2;
	b1 = ReadByte(b);
	b2 = ReadByte(b);

	return b1 | (b2<<8);
}

unsigned int ReadLong(netmsg_t *b)
{
	int s1, s2;
	s1 = ReadShort(b);
	s2 = ReadShort(b);

	return s1 | (s2<<16);
}

float ReadAngle16(netmsg_t* b)
{
	return ReadShort(b) * (360.0 / 65536);
}

float ReadAngle(sv_t* tv, netmsg_t* b)
{
	if (tv->extension_flags_fte1 & FTE_PEXT_FLOATCOORDS) {
		return ReadAngle16(b);
	}
	return ReadByte(b) * (360.0 / 256);
}

float ReadCoord(sv_t* tv, netmsg_t* b)
{
	if (tv->extension_flags_fte1 & FTE_PEXT_FLOATCOORDS) {
		return ReadFloat(b);
	}
	return ReadShort(b) * (1.0 / 8);
}

unsigned int BigLong(unsigned int val)
{
	union {
		unsigned int i;
		unsigned char c[4];
	} v;

	v.i = val;
	return (v.c[0]<<24) | (v.c[1] << 16) | (v.c[2] << 8) | (v.c[3] << 0);
}

unsigned int SwapLong(unsigned int val)
{
	union {
		unsigned int i;
		unsigned char c[4];
	} v;
	unsigned char s;

	v.i = val;
	s = v.c[0];
	v.c[0] = v.c[3];
	v.c[3] = s;
	s = v.c[1];
	v.c[1] = v.c[2];
	v.c[2] = s;

	return v.i;
}

float ReadFloat(netmsg_t *b)
{
	union {
		unsigned int i;
		float f;
	} u;

	u.i = ReadLong(b);
	return u.f;
}

void ReadString(netmsg_t *b, char *string, int maxlen)
{
	maxlen--;	//for null terminator
	while(maxlen)
	{
		*string = ReadByte(b);
		if (!*string)
			return;
		string++;
		maxlen--;
	}
	*string++ = '\0';	//add the null
}

void WriteByte(netmsg_t *b, unsigned char c)
{
	if (b->cursize>=b->maxsize)
		return;
	b->data[b->cursize++] = c;
}

void WriteShort(netmsg_t *b, unsigned short l)
{
	WriteByte(b, (l&0x00ff)>>0);
	WriteByte(b, (l&0xff00)>>8);
}

void WriteLong(netmsg_t *b, unsigned int l)
{
	WriteByte(b, (l&0x000000ff)>>0);
	WriteByte(b, (l&0x0000ff00)>>8);
	WriteByte(b, (l&0x00ff0000)>>16);
	WriteByte(b, (l&0xff000000)>>24);
}

void WriteFloat(netmsg_t *b, float f)
{
	union {
		unsigned int i;
		float f;
	} u;

	u.f = f;
	WriteLong(b, u.i);
}

void WriteString2(netmsg_t *b, const char *str)
{	//no null terminator, convienience function.
	while(*str)
		WriteByte(b, *str++);
}

void WriteAngle16(netmsg_t* b, float f)
{
	WriteShort(b, Q_rint(f * 65536.0 / 360.0) & 65535);
}

void WriteAngle(sv_t* tv, netmsg_t* b, float f)
{
	if (tv->extension_flags_fte1 & FTE_PEXT_FLOATCOORDS) {
		WriteAngle16(b, f);
	}
	else {
		WriteByte(b, Q_rint(f * 256.0 / 360.0) & 255);
	}
}

void WriteCoord(sv_t* tv, netmsg_t* b, float f)
{
	if (tv->extension_flags_fte1 & FTE_PEXT_FLOATCOORDS) {
		WriteFloat(b, f);
	}
	else {
		WriteShort(b, (int)(f * 8));
	}
}

void WriteString(netmsg_t *b, const char *str)
{
	while(*str)
		WriteByte(b, *str++);
	WriteByte(b, 0);
}

void WriteData(netmsg_t *b, const char *data, int length)
{
	int i;
	unsigned char *buf;

	if (b->cursize + length > b->maxsize)	// urm, that's just too big. :(
		return;
	buf = (unsigned char *)b->data + b->cursize;
	for (i = 0; i < length; i++)
		*buf++ = *data++;
	b->cursize += length;
}
