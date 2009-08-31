/* FreeType 2 and UTF-8 encoding support for
 * DarkPlaces
 */
#include "quakedef.h"

#include "ft2.h"

#include "ft2_defs.h"

/*
================================================================================
Function definitions. Taken from the freetype2 headers.
================================================================================
*/


FT_EXPORT( FT_Error )
(*qFT_Init_FreeType)( FT_Library  *alibrary );
FT_EXPORT( FT_Error )
(*qFT_Done_FreeType)( FT_Library  library );
FT_EXPORT( FT_Error )
(*qFT_New_Face)( FT_Library   library,
		 const char*  filepathname,
		 FT_Long      face_index,
		 FT_Face     *aface );
FT_EXPORT( FT_Error )
(*qFT_New_Memory_Face)( FT_Library      library,
			const FT_Byte*  file_base,
			FT_Long         file_size,
			FT_Long         face_index,
			FT_Face        *aface );
FT_EXPORT( FT_Error )
(*qFT_Done_Face)( FT_Face  face );
FT_EXPORT( FT_Error )
(*qFT_Select_Size)( FT_Face  face,
		    FT_Int   strike_index );
FT_EXPORT( FT_Error )
(*qFT_Request_Size)( FT_Face          face,
		     FT_Size_Request  req );
FT_EXPORT( FT_Error )
(*qFT_Set_Char_Size)( FT_Face     face,
		      FT_F26Dot6  char_width,
		      FT_F26Dot6  char_height,
		      FT_UInt     horz_resolution,
		      FT_UInt     vert_resolution );
FT_EXPORT( FT_Error )
(*qFT_Set_Pixel_Sizes)( FT_Face  face,
			FT_UInt  pixel_width,
			FT_UInt  pixel_height );
FT_EXPORT( FT_Error )
(*qFT_Load_Glyph)( FT_Face   face,
		   FT_UInt   glyph_index,
		   FT_Int32  load_flags );
FT_EXPORT( FT_Error )
(*qFT_Load_Char)( FT_Face   face,
		  FT_ULong  char_code,
		  FT_Int32  load_flags );
FT_EXPORT( FT_UInt )
(*qFT_Get_Char_Index)( FT_Face   face,
		       FT_ULong  charcode );
FT_EXPORT( FT_Error )
(*qFT_Render_Glyph)( FT_GlyphSlot    slot,
		     FT_Render_Mode  render_mode );

/*
================================================================================
Support for dynamically loading the FreeType2 library
================================================================================
*/

static dllfunction_t ft2funcs[] =
{
	{"FT_Init_FreeType",		(void **) &qFT_Init_FreeType},
	{"FT_Done_FreeType",		(void **) &qFT_Done_FreeType},
	{"FT_New_Face",			(void **) &qFT_New_Face},
	{"FT_New_Memory_Face",		(void **) &qFT_New_Memory_Face},
	{"FT_Done_Face",		(void **) &qFT_Done_Face},
	{"FT_Select_Size",		(void **) &qFT_Select_Size},
	{"FT_Request_Size",		(void **) &qFT_Request_Size},
	{"FT_Set_Char_Size",		(void **) &qFT_Set_Char_Size},
	{"FT_Set_Pixel_Sizes",		(void **) &qFT_Set_Pixel_Sizes},
	{"FT_Load_Glyph",		(void **) &qFT_Load_Glyph},
	{"FT_Load_Char",		(void **) &qFT_Load_Char},
	{"FT_Get_Char_Index",		(void **) &qFT_Get_Char_Index},
	{"FT_Render_Glyph",		(void **) &qFT_Render_Glyph},
	{NULL, NULL}
};

/// Handle for FreeType2 DLL
static dllhandle_t ft2_dll = NULL;

/// Memory pool for fonts
static mempool_t *font_mempool;

/// FreeType library handle
static FT_Library font_ft2lib;

/*
====================
Font_CloseLibrary

Unload the FreeType2 DLL
====================
*/
void Font_CloseLibrary (void)
{
	Sys_UnloadLibrary (&ft2_dll);
	if (font_mempool)
		Mem_FreePool(&font_mempool);
	if (font_ft2lib && qFT_Done_FreeType)
	{
		qFT_Done_FreeType(font_ft2lib);
		font_ft2lib = NULL;
	}

}


/*
====================
Font_OpenLibrary

Try to load the FreeType2 DLL
====================
*/
qboolean Font_OpenLibrary (void)
{
	const char* dllnames [] =
	{
#if defined(WIN64)
		#error path for freetype 2 dll
#elif defined(WIN32)
		#error path for freetype 2 dll
#elif defined(MACOSX)
		"libfreetype.dylib",
#else
		"libfreetype.so.6",
		"libfreetype.so",
#endif
		NULL
	};

	// Already loaded?
	if (ft2_dll)
		return true;

	// Load the DLL
	if (!Sys_LoadLibrary (dllnames, &ft2_dll, ft2funcs))
		return false;

	if (qFT_Init_FreeType(&font_ft2lib))
	{
		Con_Print("Failed to initialize the FreeType2 library!\n");
		Font_CloseLibrary();
		return false;
	}

	font_mempool = Mem_AllocPool("FONT", 0, NULL);
	if (!font_mempool)
	{
		Font_CloseLibrary();
		return false;
	}
	return true;
}

/*
================================================================================
UTF-8 encoding and decoding functions follow.
================================================================================
*/

/** Get the number of characters in in an UTF-8 string.
 * @param _s    An utf-8 encoded null-terminated string.
 * @return      The number of unicode characters in the string.
 */
size_t u8_strlen(const char *_s)
{
	size_t len = 0;
	unsigned char *s = (unsigned char*)_s;
	while (*s)
	{
		// ascii char
		if (*s <= 0x7F)
		{
			++len;
			++s;
			continue;
		}

		// start of a wide character
		if (*s & 0xC0)
		{
			++len;
			for (++s; *s >= 0x80 && *s <= 0xC0; ++s);
			continue;
		}

		// part of a wide character, we ignore that one
		if (*s <= 0xBF) // 10111111
		{
			++s;
			continue;
		}
	}
	return len;
}

/** Fetch a character from an utf-8 encoded string.
 * @param _s      The start of an utf-8 encoded multi-byte character.
 * @param _end    Will point to after the first multi-byte character.
 * @return        The 32-bit integer representation of the first multi-byte character.
 */
Uchar u8_getchar(const char *_s, const char **_end)
{
	const unsigned char *s = (unsigned char*)_s;
	Uchar u;
	unsigned char mask;
	unsigned char v;

	if (*s < 0x80)
	{
		if (_end)
			*_end = _s + 1;
		return (U_int32)*s;
	}

	if (*s < 0xC0)
	{
		// starting within a wide character - skip it and retrieve the one after it
		for (++s; *s >= 0x80 && *s < 0xC0; ++s);
		// or we could return '?' here?
	}

	u = 0;
	mask = 0x7F;
	v = *s & mask;
	for (mask >>= 1; v > (*s & mask); mask >>= 1)
		v = (*s & mask);
	u = (Uchar)(*s & mask);
	for (++s; *s >= 0x80 && *s < 0xC0; ++s)
		u = (u << 6) | (*s & 0x3F);

	if (_end)
		*_end = (const char*)s;

	return u;
}

/** Encode a wide-character into utf-8.
 * @param w        The wide character to encode.
 * @param to       The target buffer the utf-8 encoded string is stored to.
 * @param maxlen   The maximum number of bytes that fit into the target buffer.
 * @return         Number of bytes written to the buffer, or less or equal to 0 if the buffer is too small.
 */
int u8_fromchar(Uchar w, char *to, size_t maxlen)
{
	size_t i, j;
	char bt;
	char tmp[16];

	if (maxlen < 1)
		return -2;

	if (w < 0x80)
	{
		to[0] = (char)w;
		if (maxlen < 2)
			return -1;
		to[1] = 0;
		return 1;
	}

	// check how much space we need and store data into a
	// temp buffer - this is faster than recalculating again
	i = 0;
	bt = 0;
	while (w)
	{
		tmp[i++] = 0x80 | (w & 0x3F);
		bt = (bt >> 1) | 0x80;
		w >>= 6;
		// see if we still fit into the target buffer
		if (i+1 >= maxlen) // +1 for the \0
			return -i;

		// there are no characters which take up that much space yet
		// and there won't be for the next many many years, still... let's be safe
		if (i >= sizeof(tmp))
			return -1;
	}
	tmp[i-1] |= bt;
	for (j = 0; j < i; ++j)
	{
		to[i-j-1] = tmp[j];
	}

	to[i] = 0;
	return i;
}

/** Convert a utf-8 multibyte string to a wide character string.
 * @param wcs       The target wide-character buffer.
 * @param mb        The utf-8 encoded multibyte string to convert.
 * @param maxlen    The maximum number of wide-characters that fit into the target buffer.
 * @return          The number of characters written to the target buffer.
 */
size_t u8_mbstowcs(Uchar *wcs, const char *mb, size_t maxlen)
{
	size_t i;
	for (i = 0; *mb && i < maxlen; ++i)
		*wcs++ = u8_getchar(mb, &mb);
	if (i < maxlen)
		*wcs = 0;
	return i;
}

/** Convert a wide-character string to a utf-8 multibyte string.
 * @param mb      The target buffer the utf-8 string is written to.
 * @param wcs     The wide-character string to convert.
 * @param maxlen  The number bytes that fit into the multibyte target buffer.
 * @return        The number of bytes written, not including the terminating \0
 */
size_t u8_wcstombs(char *mb, const Uchar *wcs, size_t maxlen)
{
	size_t i;
	const char *start = mb;
	for (i = 0; *wcs && i < maxlen; ++i)
	{
		int len;
		if ( (len = u8_fromchar(*wcs++, mb, maxlen - i)) < 0)
			return (mb - start);
		mb += len;
	}
	if (i < maxlen)
		*mb = 0;
	return (mb - start);
}

/*
================================================================================
Implementation of a more or less lazy font loading and rendering code.
================================================================================
*/

static qboolean Font_LoadMapForIndex(font_t *font, Uchar index);
qboolean Font_LoadFont(const char *name, int size, font_t *font)
{
	size_t namelen;
	char filename[PATH_MAX];
	int status;

	if (!Font_OpenLibrary())
	{
		Con_Printf("WARNING: can't open load font %s\n"
			   "You need the FreeType2 DLL to load font files\n",
			   name);
		return false;
	}

	namelen = strlen(name);

	memcpy(filename, name, namelen);
	memcpy(filename + namelen, ".ttf", 5);

	font->data = FS_LoadFile(filename, font_mempool, false, &font->datasize);
	if (!font->data)
	{
		// FS_LoadFile being not-quiet should print an error :)
		return false;
	}


	status = qFT_New_Face(font_ft2lib, filename, 0, (FT_Face*)&font->face);
	if (status)
	{
		Con_Printf("ERROR: can't create face for %s\n"
			   "Error %i\n", // TODO: error strings
			   name, status);
		Mem_Free(font->data);
		return false;
	}

	memcpy(font->name, name, namelen+1);
	font->size = size;

	status = qFT_Set_Pixel_Sizes((FT_Face)font->face, size, size);
	if (status)
	{
		Con_Printf("ERROR: can't size pixel sizes for face of font %s\n"
			   "Error %i\n", // TODO: error strings
			   name, status);
		Mem_Free(font->data);
		return false;
	}

	if (!Font_LoadMapForIndex(font, 0))
	{
		Con_Printf("ERROR: can't load the first character map for %s\n"
			   "This is fatal\n",
			   name);
		Mem_Free(font->data);
		return false;
	}

	return true;
}

void Font_UnloadFont(font_t *font)
{
	if (font->data)
		Mem_Free(font->data);
	if (ft2_dll)
	{
		if (font->face)
		{
			qFT_Done_Face((FT_Face)font->face);
			font->face = NULL;
		}
	}
}

#define FONT_CHARS_PER_LINE 16
#define FONT_CHAR_LINES 16
#define FONT_CHARS_PER_MAP (FONT_CHARS_PER_LINE * FONT_CHAR_LINES)

static qboolean Font_LoadMapForIndex(font_t *font, Uchar _ch)
{
	unsigned long mapidx = _ch / FONT_CHARS_PER_MAP;
	unsigned char *data;
	FT_ULong ch;
	int status;

	FT_Face face = font->face;

	data = Mem_Alloc(font_mempool, FONT_CHARS_PER_MAP * (font->size * font->size));
	if (!data)
	{
		Con_Printf("ERROR: Failed to allocate memory for glyph data for font %s\n", font->name);
		return false;
	}

	for (ch = mapidx * FONT_CHARS_PER_MAP;
	     ch < (mapidx + 1) * FONT_CHARS_PER_MAP;
	     ++ch)
	{
		FT_ULong glyphIndex;

		glyphIndex = qFT_Get_Char_Index(face, ch);

		status = qFT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
		if (status)
		{
			Con_Printf("failed to load glyph %lu for %s\n", glyphIndex, font->name);
			continue;
		}

		status = qFT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if (status)
		{
			Con_Printf("failed to render glyph %lu for %s\n", glyphIndex, font->name);
			continue;
		}
	}

	return true;
}
