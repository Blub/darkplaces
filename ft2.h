/* Header for FreeType 2 and UTF-8 encoding support for
 * DarkPlaces
 */

#ifndef DP_FREETYPE2_H__
#define DP_FREETYPE2_H__

#include <sys/types.h>

// types for unicode strings
// let them be 32 bit for now
// normally, whcar_t is 16 or 32 bit, 16 on linux I think, 32 on haiku and maybe windows
#ifdef _MSC_VER
#include <stdint.h>
typedef __int32 U_int32;
#else
typedef int32_t U_int32;
#endif

// Uchar, a wide character
typedef U_int32 Uchar;

size_t u8_strlen(const char*);
Uchar  u8_getchar(const char*, const char**);
int    u8_fromchar(Uchar, char*, size_t);
size_t u8_wcstombs(char*, const Uchar*, size_t);

typedef struct font_map_s font_map_t;

typedef struct
{
	char           name[64];
	int            size;
	int            glyphSize;

	qboolean       has_kerning;

	unsigned char *data;
	fs_offset_t    datasize;
	void          *face;

	// an ordered linked list of glyph maps
	font_map_t    *font_map;
} font_t;

void Font_CloseLibrary(void);
void Font_Init(void);
qboolean Font_OpenLibrary(void);
qboolean Font_LoadFont(const char *name, int size, font_t *font);

#endif // DP_FREETYPE2_H__
