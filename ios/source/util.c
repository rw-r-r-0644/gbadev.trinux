
// by oggzee

#include <ogcsys.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <ctype.h>
//#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
//#include "menu.h"
#include "util.h"

/*
extern void* SYS_GetArena2Lo();
extern void* SYS_GetArena2Hi();
extern void* SYS_AllocArena2MemLo(u32 size,u32 align);
extern void* __SYS_GetIPCBufferLo();
extern void* __SYS_GetIPCBufferHi();

static void *mem2_start = NULL;
*/


char* strcopy(char *dest, const char *src, int size)
{
	strncpy(dest,src,size);
	dest[size-1] = 0;
	return dest;
}

char *strappend(char *dest, char *src, int size)
{
	int len = strlen(dest);
	strcopy(dest+len, src, size-len);
	return dest;
}

bool str_replace(char *str, char *olds, char *news, int size)
{
	char *p;
	int len;
	p = strstr(str, olds);
	if (!p) return false;
	// new len
	len = strlen(str) - strlen(olds) + strlen(news);
	// check size
	if (len >= size) return false;
	// move remainder to fit (and nul)
	memmove(p+strlen(news), p+strlen(olds), strlen(p)-strlen(olds)+1);
	// copy new in place
	memcpy(p, news, strlen(news));
	// terminate
	str[len] = 0;
	return true;
}

bool str_replace_all(char *str, char *olds, char *news, int size) {
	int cnt = 0;
	bool ret = str_replace(str, olds, news, size);
	while (ret) {
		ret = str_replace(str, olds, news, size);
		cnt++;
	}
	return (cnt > 0);
}

//bool str_replace_tag_val(char *str, char *tag, char *val)
//{
//	char *p, *end;
//	p = strstr(str, tag);
//	if (!p) return false;
//	p += strlen(tag);
//	end = strstr(p, "</");
//	if (!end) return false;
//	dbg_printf("%s '%.*s' -> '%s'\n", tag, end-p, p, val);
//	// make space for new val
//	memmove(p+strlen(val), end, strlen(end)+1); // +1 for 0 termination
//	// copy over new val
//	memcpy(p, val, strlen(val));
//	return true;
//}

// Thanks Dteyn for this nice feature =)
// Toggle wiilight (thanks Bool for wiilight source)
void wiilight(int enable)
{
	static vu32 *_wiilight_reg = (u32*)0xCD0000C0;
    u32 val = (*_wiilight_reg&~0x20);        
    if(enable) val |= 0x20;             
    *_wiilight_reg=val;            
}


int mbs_len(char *s)
{
	int count, n;
	for (count = 0; *s; count++) {
		n = mblen(s, 4);
		if (n < 0) {
			// invalid char, ignore
			n = 1;
		}
		s += n;
	}
	return count;
}

int mbs_len_valid(char *s)
{
	int count, n;
	for (count = 0; *s; count++) {
		n = mblen(s, 4);
		if (n < 0) {
			// invalid char, stop
			break;
		}
		s += n;
	}
	return count;
}

char *mbs_copy(char *dest, char *src, int size)
{
	char *s;
	int n;
	strcopy(dest, src, size);
	s = dest;
	while (*s) {
		n = mblen(s, 4);
		if (n < 0) {
			// invalid char, stop
			*s = 0;
			break;
		}
		s += n;
	}
	return dest;
}

bool mbs_trunc(char *mbs, int n)
{
	int len = mbs_len(mbs);
	if (len <= n) return false;
	int slen = strlen(mbs);
	wchar_t wbuf[n+1];
	wbuf[0] = 0;
	mbstowcs(wbuf, mbs, n);
	wbuf[n] = 0;
	wcstombs(mbs, wbuf, slen+1);
	return true;
}

char *mbs_align(const char *str, int n)
{
	static char strbuf[100];
	if (strlen(str) >= sizeof(strbuf) || n >= sizeof(strbuf)) return (char*)str;
	// fill with space
	memset(strbuf, ' ', sizeof(strbuf));
	// overwrite with str, keeping trailing space
	memcpy(strbuf, str, strlen(str));
	// terminate
	strbuf[sizeof(strbuf)-1] = 0;
	// truncate multibyte string
	mbs_trunc(strbuf, n);
	return strbuf;
}

int mbs_coll(char *a, char *b)
{
	//int lena = strlen(a);
	//int lenb = strlen(b);
	int lena = mbs_len_valid(a);
	int lenb = mbs_len_valid(b);
	wchar_t wa[lena+1];
	wchar_t wb[lenb+1];
	int wlen, i;
	int sa, sb, x;
	wlen = mbstowcs(wa, a, lena);
	wa[wlen>0?wlen:0] = 0;
	wlen = mbstowcs(wb, b, lenb);
	wb[wlen>0?wlen:0] = 0;
	for (i=0; wa[i] || wb[i]; i++) {
		sa = wa[i];
		if ((unsigned)sa < MAX_USORT_MAP) sa = usort_map[sa];
		sb = wb[i];
		if ((unsigned)sb < MAX_USORT_MAP) sb = usort_map[sb];
		x = sa - sb;
		if (x) return x;
	}
	return 0;
}

int con_char_len(int c)
{
	return 1;
	
	/*int cc;
	int len;
	if (c < 512) return 1;
	cc = map_ufont(c);
	if (cc != 0) return 1;
	if (c < 0 || c > 0xFFFF) return 1;
	if (unifont == NULL) return 1;
	len = unifont->index[c] & 0x0F;
	if (len < 1) return 1;
	if (len > 2) return 2;
	return len;*/
}

int con_len(char *s)
{
	int i, len = 0;
	int n = mbs_len(s);
	wchar_t wbuf[n+1];
	wbuf[0] = 0;
	mbstowcs(wbuf, s, n);
	wbuf[n] = 0;
	for (i=0; i<n; i++) {
		len += con_char_len(wbuf[i]);
	}
	return len;
}

bool con_trunc(char *s, int n)
{
	int slen = strlen(s);
	int i, len = 0;
	wchar_t wbuf[n+1];
	wbuf[0] = 0;
	mbstowcs(wbuf, s, n);
	wbuf[n] = 0;
	for (i=0; i<n; i++) {
		len += con_char_len(wbuf[i]);
		if (len > n) break;
	}
	wbuf[i] = 0; // terminate;
	wcstombs(s, wbuf, slen+1);
	return (len > n); // true if truncated
}

char *con_align(const char *str, int n)
{
	static char strbuf[100];
	if (strlen(str) >= sizeof(strbuf) || n >= sizeof(strbuf)) return (char*)str;
	// fill with space
	memset(strbuf, ' ', sizeof(strbuf));
	// overwrite with str, keeping trailing space
	memcpy(strbuf, str, strlen(str));
	// terminate
	strbuf[sizeof(strbuf)-1] = 0;
	// truncate multibyte string
	con_trunc(strbuf, n);
	while (con_len(strbuf) < n) strcat(strbuf, " ");
	return strbuf;
}


/*int map_ufont(int c)
{
	int i;
	if ((unsigned)c < 512) return c;
	for (i=0; ufont_map[i]; i+=2) {
		if (ufont_map[i] == c) return ufont_map[i+1];
	}
	return 0;
}*/

// FFx y AABB
void hex_dump1(void *p, int size)
{
	char *c = p;
	int i;
	for (i=0; i<size; i++) {
		unsigned cc = (unsigned char)c[i];
		if (cc < 32 || cc > 128) {
			printf("%02x", cc);
		} else {
			printf("%c ", cc);
		}
	}	
}

// FF 40 41 AA BB | .xy..
void hex_dump2(void *p, int size)
{
	int i = 0, j, x = 12;
	char *c = p;
	printf("\n");
	while (i<size) {
		printf("%02x ", i);
		for (j=0; j<x && i+j<size; j++) printf("%02x", (int)c[i+j]);
		printf(" |");
		for (j=0; j<x && i+j<size; j++) {
			unsigned cc = (unsigned char)c[i+j];
			if (cc < 32 || cc > 128) cc = '.';
			printf("%c", cc);
		}
		printf("|\n");
		i += x;
	}	
}

// FF4041AABB 
void hex_dump3(void *p, int size)
{
	int i = 0, j, x = 16;
	char *c = p;
	while (i<size) {
		printf_("");
		for (j=0; j<x && i+j<size; j++) printf("%02x", (int)c[i+j]);
		printf("\n");
		i += x;
	}	
}


#if 0

void memstat()
{
	//malloc_stats();
}

void memcheck()
{
	//mallinfo();
}

#endif

/* Copyright 2005 Shaun Jackman
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */
//This code from dirname.c, meant to be part of libgen, modified by Clipper
char* dirname(char *path)
{
	char *p;
	if( path == NULL || *path == '\0' )
		return ".";
	p = path + strlen(path) - 1;
	while( *p == '/' ) {
		if( p == path )
			return path;
		*p-- = '\0';
	}
	while( p >= path && *p != '/' )
		p--;
	return
		p < path ? "." :
		p == path ? "/" :
		*(p-1) == ':' ? "/" :
		(*p = '\0', path);
}
