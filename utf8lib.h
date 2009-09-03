/*
 * UTF-8 utility functions for DarkPlaces
 */
#ifndef UTF8LIB_H__
#define UTF8LIB_H__

#include "qtypes.h"

// types for unicode strings
// let them be 32 bit for now
// normally, whcar_t is 16 or 32 bit, 16 on linux I think, 32 on haiku and maybe windows
#ifdef _MSC_VER
#include <stdint.h>
typedef __int32 U_int32;
#else
#include <sys/types.h>
typedef int32_t U_int32;
#endif

// Uchar, a wide character
typedef U_int32 Uchar;

size_t u8_strlen(const char*);
int    u8_byteofs(const char*, size_t, size_t*);
int    u8_charidx(const char*, size_t, size_t*);
size_t u8_bytelen(const char*, size_t);
Uchar  u8_getchar(const char*, const char**);
int    u8_fromchar(Uchar, char*, size_t);
size_t u8_wcstombs(char*, const Uchar*, size_t);
size_t u8_COM_StringLengthNoColors(const char *s, size_t size_s, qboolean *valid);

// returns a static buffer, use this for inlining
char  *u8_encodech(Uchar ch);

#endif // UTF8LIB_H__
