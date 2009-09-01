/* FreeType 2 and UTF-8 encoding support for
 * DarkPlaces
 */
#include "quakedef.h"

#include "ft2.h"
#include "ft2_defs.h"
#include "ft2_fontdefs.h"

/*
================================================================================
Function definitions. Taken from the freetype2 headers.
================================================================================
*/


FT_EXPORT( FT_Error )
(*qFT_Init_FreeType)( FT_Library  *alibrary );
FT_EXPORT( FT_Error )
(*qFT_Done_FreeType)( FT_Library  library );
/*
FT_EXPORT( FT_Error )
(*qFT_New_Face)( FT_Library   library,
		 const char*  filepathname,
		 FT_Long      face_index,
		 FT_Face     *aface );
*/
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
FT_EXPORT( FT_Error )
(*qFT_Get_Kerning)( FT_Face     face,
		    FT_UInt     left_glyph,
		    FT_UInt     right_glyph,
		    FT_UInt     kern_mode,
		    FT_Vector  *akerning );

/*
================================================================================
Support for dynamically loading the FreeType2 library
================================================================================
*/

static dllfunction_t ft2funcs[] =
{
	{"FT_Init_FreeType",		(void **) &qFT_Init_FreeType},
	{"FT_Done_FreeType",		(void **) &qFT_Done_FreeType},
	//{"FT_New_Face",			(void **) &qFT_New_Face},
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
	{"FT_Get_Kerning",		(void **) &qFT_Get_Kerning},
	{NULL, NULL}
};

/// Handle for FreeType2 DLL
static dllhandle_t ft2_dll = NULL;

/// Memory pool for fonts
static mempool_t *font_mempool= NULL;
static rtexturepool_t *font_texturepool = NULL;

/// FreeType library handle
static FT_Library font_ft2lib = NULL;

/*
====================
Font_CloseLibrary

Unload the FreeType2 DLL
====================
*/
void Font_CloseLibrary (void)
{
	if (font_mempool)
		Mem_FreePool(&font_mempool);
	if (font_texturepool)
		R_FreeTexturePool(&font_texturepool);
	if (font_ft2lib && qFT_Done_FreeType)
	{
		qFT_Done_FreeType(font_ft2lib);
		font_ft2lib = NULL;
	}
	Sys_UnloadLibrary (&ft2_dll);
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
	return true;
}

/*
====================
Font_Init

Initialize the freetype2 font subsystem
====================
*/

static ft2_font_t test_font;

void font_start(void)
{
	if (!Font_OpenLibrary())
		return;

	if (qFT_Init_FreeType(&font_ft2lib))
	{
		Con_Print("ERROR: Failed to initialize the FreeType2 library!\n");
		Font_CloseLibrary();
		return;
	}

	font_mempool = Mem_AllocPool("FONT", 0, NULL);
	if (!font_mempool)
	{
		Con_Print("ERROR: Failed to allocate FONT memory pool!\n");
		Font_CloseLibrary();
		return;
	}

	font_texturepool = R_AllocTexturePool();
	if (!font_texturepool)
	{
		Con_Print("ERROR: Failed to allocate FONT texture pool!\n");
		Font_CloseLibrary();
		return;
	}

	if (!Font_LoadFont("gfx/test", 16, &test_font))
	{
		Con_Print("ERROR: Failed to load test font!\n");
		Font_CloseLibrary();
		return;
	}
}

void font_shutdown(void)
{
	Font_CloseLibrary();
}

void font_newmap(void)
{
}

void Font_Init(void)
{
	// dummy this for now
	// R_RegisterModule("Font_FreeType2", font_start, font_shutdown, font_newmap);
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
	// for a little speedup:
	if ( (*s & 0xE0) == 0xC0 )
	{
		// 2-byte character
		u = ( (s[0] & 0x1F) << 6 ) | (s[1] & 0x3F);
		if (_end)
			*_end = _s + 2;
		return u;
	}
	if ( (*s & 0xF0) == 0xE0 )
	{
		// 3-byte character
		u = ( (s[0] & 0x0F) << 12 ) | ( (s[1] & 0x3F) << 6 ) | (s[2] & 0x3F);
		if (_end)
			*_end = _s + 3;
		return u;
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
	// for a little speedup
	if (w < 0x800)
	{
		if (maxlen < 3)
		{
			to[0] = 0;
			return -1;
		}
		to[2] = 0;
		to[1] = 0x80 | (w & 0x3F); w >>= 6;
		to[0] = 0xC0 | w;
		return 2;
	}
	if (w < 0x10000)
	{
		if (maxlen < 4)
		{
			to[0] = 0;
			return -1;
		}
		to[3] = 0;
		to[2] = 0x80 | (w & 0x3F); w >>= 6;
		to[1] = 0x80 | (w & 0x3F); w >>= 6;
		to[0] = 0xE0 | w;
		return 3;
	}

	// "more general" version:

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

#include "ft2_fontdefs.h"

ft2_font_t *Font_Alloc(void)
{
	return Mem_Alloc(font_mempool, sizeof(ft2_font_t));
}

qboolean Font_LoadFont(const char *name, int size, ft2_font_t *font)
{
	size_t namelen;
	char filename[PATH_MAX];
	int status;

	memset(font, 0, sizeof(*font));

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
		Con_Printf("Failed to load TTF version of font %s\n", name);
		return false;
	}
	Con_Printf("Loading font %s face 0 size %i...\n", filename, size);

	status = qFT_New_Memory_Face(font_ft2lib, (FT_Bytes)font->data, font->datasize, 0, (FT_Face*)&font->face);
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
	font->glyphSize = font->size * 2;
	font->has_kerning = !!(((FT_Face)(font->face))->face_flags & FT_FACE_FLAG_KERNING);
	font->sfx = (1.0/64.0) / (double)font->size;
	font->sfy = (1.0/64.0) / (double)font->size;

	status = qFT_Set_Pixel_Sizes((FT_Face)font->face, size, size);
	if (status)
	{
		Con_Printf("ERROR: can't size pixel sizes for face of font %s\n"
			   "Error %i\n", // TODO: error strings
			   name, status);
		Mem_Free(font->data);
		return false;
	}

	if (!Font_LoadMapForIndex(font, 0, NULL))
	{
		Con_Printf("ERROR: can't load the first character map for %s\n"
			   "This is fatal\n",
			   name);
		Mem_Free(font->data);
		return false;
	}

	// load the default kerning vector:
	if (font->has_kerning)
	{
		Uchar l, r;
		FT_Vector kernvec;
		for (l = 0; l < 256; ++l)
		{
			for (r = 0; r < 256; ++r)
			{
				if (qFT_Get_Kerning(font->face, l, r, FT_KERNING_DEFAULT, &kernvec))
				{
					font->kerning.kerning[l][r][0] = 0;
					font->kerning.kerning[l][r][1] = 0;
				}
				else
				{
					font->kerning.kerning[l][r][0] = kernvec.x * font->sfx;
					font->kerning.kerning[l][r][1] = kernvec.y * font->sfy;
				}
			}
		}
	}

	return true;
}

qboolean Font_GetKerning(ft2_font_t *font, Uchar left, Uchar right, float *outx, float *outy)
{
	if (font->has_kerning)
		return false;
	if (left < 256 && right < 256)
	{
		// quick-kerning
		if (outx) *outx = font->kerning.kerning[left][right][0];
		if (outy) *outy = font->kerning.kerning[left][right][1];
		return true;
	}
	else
	{
		FT_Vector kernvec;
		if (qFT_Get_Kerning(font->face, left, right, FT_KERNING_DEFAULT, &kernvec))
		{
			if (outx) *outx = kernvec.x;
			if (outy) *outy = kernvec.y;
			return true;
		}
		return false;
	}
}

void Font_UnloadFont(ft2_font_t *font)
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

qboolean Font_LoadMapForIndex(ft2_font_t *font, Uchar _ch, ft2_font_map_t **outmap)
{
	char map_identifier[PATH_MAX];
	unsigned long mapidx = _ch / FONT_CHARS_PER_MAP;
	unsigned char *data;
	FT_ULong ch, mapch;
	int status;
	int tp;

	FT_Face face = font->face;

	int pitch;
	int gR, gC; // glyph position: row and column

	ft2_font_map_t *map;

	int bytesPerPixel = 4; // change the conversion loop too if you change this!

	if (outmap)
		*outmap = NULL;

	map = Mem_Alloc(font_mempool, sizeof(ft2_font_map_t));
	if (!map)
	{
		Con_Printf("ERROR: Out of memory when loading fontmap for %s\n", font->name);
		return false;
	}

	pitch = font->glyphSize * FONT_CHARS_PER_LINE * bytesPerPixel;
	data = Mem_Alloc(font_mempool, (FONT_CHAR_LINES * font->glyphSize) * pitch);
	if (!data)
	{
		Mem_Free(map);
		Con_Printf("ERROR: Failed to allocate memory for glyph data for font %s\n", font->name);
		return false;
	}

	// initialize as white texture with zero alpha
	tp = 0;
	while (tp < (FONT_CHAR_LINES * font->glyphSize) * pitch)
	{
		data[tp++] = 0xFF;
		data[tp++] = 0xFF;
		data[tp++] = 0xFF;
		data[tp++] = 0x00;
	}

	map->start = mapidx * FONT_CHARS_PER_MAP;
	if (!font->font_map)
		font->font_map = map;
	else
	{
		// insert the map at the right place
		ft2_font_map_t *next = font->font_map;
		while(next->next && next->next->start < map->start)
			next = next->next;
		map->next = next->next;
		next->next = map;
	}

	gR = 0;
	gC = -1;
	for (ch = map->start;
	     ch < (FT_ULong)map->start + FONT_CHARS_PER_MAP;
	     ++ch)
	{
		FT_ULong glyphIndex;
		int w, h, x, y;
		FT_GlyphSlot glyph;
		FT_Bitmap *bmp;
		unsigned char *imagedata, *dst, *src;
		glyph_slot_t *mapglyph;

		if (developer.integer)
			Con_Print("------------- GLYPH INFO -----------------\n");

		++gC;
		if (gC >= FONT_CHARS_PER_LINE)
		{
			gC -= FONT_CHARS_PER_LINE;
			++gR;
		}

		imagedata = data + gR * pitch * font->glyphSize + gC * font->glyphSize * bytesPerPixel;
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

		glyph = face->glyph;
		bmp = &glyph->bitmap;

		w = bmp->width;
		h = bmp->rows;

		if (w > font->glyphSize || h > font->glyphSize)
			Con_Printf("WARNING: Glyph %lu is too big in font %s\n", ch, font->name);

		switch (bmp->pixel_mode)
		{
		case FT_PIXEL_MODE_MONO:
			if (developer.integer)
				Con_Print("  Pixel Mode: MONO\n");
			break;
		case FT_PIXEL_MODE_GRAY2:
			if (developer.integer)
				Con_Print("  Pixel Mode: GRAY2\n");
			break;
		case FT_PIXEL_MODE_GRAY4:
			if (developer.integer)
				Con_Print("  Pixel Mode: GRAY4\n");
			break;
		case FT_PIXEL_MODE_GRAY:
			if (developer.integer)
				Con_Print("  Pixel Mode: GRAY\n");
			break;
		default:
			if (developer.integer)
				Con_Printf("  Pixel Mode: Unknown: %i\n", bmp->pixel_mode);
			Mem_Free(data);
			return false;
		}
		for (y = 0; y < h; ++y)
		{
			dst = imagedata + y * pitch;
			src = bmp->buffer + y * bmp->pitch;

			switch (bmp->pixel_mode)
			{
			case FT_PIXEL_MODE_MONO:
				for (x = 0; x < bmp->width; x += 8)
				{
					unsigned char ch = *src++;
					dst += 3; // shift to alpha byte
					*dst = 255 * ((ch & 0x80) >> 7); dst += 4;
					*dst = 255 * ((ch & 0x40) >> 6); dst += 4;
					*dst = 255 * ((ch & 0x20) >> 5); dst += 4;
					*dst = 255 * ((ch & 0x10) >> 4); dst += 4;
					*dst = 255 * ((ch & 0x08) >> 3); dst += 4;
					*dst = 255 * ((ch & 0x04) >> 2); dst += 4;
					*dst = 255 * ((ch & 0x02) >> 1); dst += 4;
					*dst = 255 * ((ch & 0x01) >> 0); dst++; // compensate the first += 3
				}
				break;
			case FT_PIXEL_MODE_GRAY2:
				for (x = 0; x < bmp->width; x += 4)
				{
					unsigned char ch = *src++;
					dst += 3; // shift to alpha byte
					*dst = ( ((ch & 0xA0) >> 6) * 0x55 ); ch <<= 2; dst += 4;
					*dst = ( ((ch & 0xA0) >> 6) * 0x55 ); ch <<= 2; dst += 4;
					*dst = ( ((ch & 0xA0) >> 6) * 0x55 ); ch <<= 2; dst += 4;
					*dst = ( ((ch & 0xA0) >> 6) * 0x55 ); ch <<= 2; dst++; // compensate the +=3
				}
				break;
			case FT_PIXEL_MODE_GRAY4:
				for (x = 0; x < bmp->width; x += 2)
				{
					unsigned char ch = *src++;
					dst += 3; // shift to alpha byte
					*dst = ( ((ch & 0xF0) >> 4) * 0x24); dst += 4;
					*dst = ( ((ch & 0x0F) ) * 0x24); dst++; // compensate the += 3
				}
				break;
			case FT_PIXEL_MODE_GRAY:
				// in this case pitch should equal width
				for (tp = 0; tp < bmp->pitch; ++tp)
					dst[3 + tp*4] = src[tp]; // copy the grey value into the alpha bytes

				//memcpy((void*)dst, (void*)src, bmp->pitch);
				//dst += bmp->pitch;
				break;
			default:
				break;
			}
		}

		// now fill map->glyphs[ch - map->start]
		mapch = ch - map->start;
		mapglyph = &map->glyphs[mapch];

		{
			double bearingX = (double)glyph->metrics.horiBearingX * font->sfx;
			double bearingY = (double)glyph->metrics.horiBearingY * font->sfy;
			double advance = (double)glyph->metrics.horiAdvance * font->sfx;
			double mWidth = (double)glyph->metrics.width * font->sfx;
			double mHeight = (double)glyph->metrics.height * font->sfy;
			//double tWidth = bmp->width / (double)font->size;
			//double tHeight = bmp->rows / (double)font->size;

			mapglyph->vxmin = bearingX;
			mapglyph->vxmax = bearingX + mWidth;
			mapglyph->vymin = -bearingY;
			mapglyph->vymax = mHeight - bearingY;
			mapglyph->txmin = ( (double)(gC * font->glyphSize) ) / ( (double)(font->glyphSize * FONT_CHARS_PER_LINE) );
			mapglyph->txmax = mapglyph->txmin + (double)bmp->width / ( (double)(font->glyphSize * FONT_CHARS_PER_LINE) );
			mapglyph->tymin = ( (double)(gR * font->glyphSize) ) / ( (double)(font->glyphSize * FONT_CHAR_LINES) );
			mapglyph->tymax = mapglyph->tymin + (double)bmp->rows / ( (double)(font->glyphSize * FONT_CHAR_LINES) );
			//mapglyph->advance_x = advance * font->size;
			mapglyph->advance_x = advance;
			mapglyph->advance_y = 0;

			if (developer.integer)
			{
				Con_Printf("  Glyph: %lu   at (%i, %i)\n", (unsigned long)ch, gC, gR);
				if (ch >= 32 && ch <= 128)
					Con_Printf("  Character: %c\n", (int)ch);
				Con_Printf("  Vertex info:\n");
				Con_Printf("    X: ( %f  --  %f )\n", mapglyph->vxmin, mapglyph->vxmax);
				Con_Printf("    Y: ( %f  --  %f )\n", mapglyph->vymin, mapglyph->vymax);
				Con_Printf("  Texture info:\n");
				Con_Printf("    S: ( %f  --  %f )\n", mapglyph->txmin, mapglyph->txmax);
				Con_Printf("    T: ( %f  --  %f )\n", mapglyph->tymin, mapglyph->tymax);
				Con_Printf("  Advance: %f, %f\n", mapglyph->advance_x, mapglyph->advance_y);
			}
		}
	}

	// create a texture from the data now

	if (developer.integer)
	{
		// view using `display -depth 8 -size 512x512 name_page.rgba` (be sure to use a correct -size parameter)
		dpsnprintf(map_identifier, sizeof(map_identifier), "%s_%u.rgba", font->name, (unsigned)map->start/FONT_CHARS_PER_MAP);
		FS_WriteFile(map_identifier, data, pitch * FONT_CHAR_LINES * font->glyphSize);
	}
	dpsnprintf(map_identifier, sizeof(map_identifier), "%s_%u", font->name, (unsigned)map->start/FONT_CHARS_PER_MAP);
	map->texture = R_LoadTexture2D(font_texturepool, map_identifier,
				       font->glyphSize * FONT_CHARS_PER_LINE,
				       font->glyphSize * FONT_CHAR_LINES,
				       data, TEXTYPE_RGBA, TEXF_ALPHA | TEXF_ALWAYSPRECACHE/* | TEXF_MIPMAP*/, NULL);
	Mem_Free(data);
	if (!map->texture)
	{
		// if the first try isn't successful, keep it with a broken texture
		// otherwise we retry to load it every single frame where ft2 rendering is used
		// this would be bad...
		// only `data' must be freed
		return false;
	}
	if (outmap)
		*outmap = map;
	return true;
}

#if 0

extern void _DrawQ_Setup(void);

// TODO: If no additional stuff ends up in the following static functions
// use the DrawQ ones!
static void _Font_ProcessDrawFlag(int flags)
{
	_DrawQ_Setup();
	CHECKGLERROR
	if(flags == DRAWFLAG_ADDITIVE)
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
	else if(flags == DRAWFLAG_MODULATE)
		GL_BlendFunc(GL_DST_COLOR, GL_ZERO);
	else if(flags == DRAWFLAG_2XMODULATE)
		GL_BlendFunc(GL_DST_COLOR,GL_SRC_COLOR);
	else if(flags == DRAWFLAG_SCREEN)
		GL_BlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
	else
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

extern cvar_t r_textcontrast, r_textbrightness, r_textshadow;
static const vec4_t string_colors[] =
{
	// Quake3 colors
	// LordHavoc: why on earth is cyan before magenta in Quake3?
	// LordHavoc: note: Doom3 uses white for [0] and [7]
	{0.0, 0.0, 0.0, 1.0}, // black
	{1.0, 0.0, 0.0, 1.0}, // red
	{0.0, 1.0, 0.0, 1.0}, // green
	{1.0, 1.0, 0.0, 1.0}, // yellow
	{0.0, 0.0, 1.0, 1.0}, // blue
	{0.0, 1.0, 1.0, 1.0}, // cyan
	{1.0, 0.0, 1.0, 1.0}, // magenta
	{1.0, 1.0, 1.0, 1.0}, // white
	// [515]'s BX_COLOREDTEXT extension
	{1.0, 1.0, 1.0, 0.5}, // half transparent
	{0.5, 0.5, 0.5, 1.0}  // half brightness
	// Black's color table
	//{1.0, 1.0, 1.0, 1.0},
	//{1.0, 0.0, 0.0, 1.0},
	//{0.0, 1.0, 0.0, 1.0},
	//{0.0, 0.0, 1.0, 1.0},
	//{1.0, 1.0, 0.0, 1.0},
	//{0.0, 1.0, 1.0, 1.0},
	//{1.0, 0.0, 1.0, 1.0},
	//{0.1, 0.1, 0.1, 1.0}
};

static void Font_GetTextColor(float color[4], int colorindex, float r, float g, float b, float a, qboolean shadow)
{
	float C = r_textcontrast.value;
	float B = r_textbrightness.value;
	if (colorindex & 0x10000) // that bit means RGB color
	{
		color[0] = ((colorindex >> 12) & 0xf) / 15.0;
		color[1] = ((colorindex >> 8) & 0xf) / 15.0;
		color[2] = ((colorindex >> 4) & 0xf) / 15.0;
		color[3] = (colorindex & 0xf) / 15.0;
	}
	else
		Vector4Copy(string_colors[colorindex], color);
	Vector4Set(color, color[0] * r * C + B, color[1] * g * C + B, color[2] * b * C + B, color[3] * a);
	if (shadow)
	{
		float shadowalpha = (color[0]+color[1]+color[2]) * 0.8;
		Vector4Set(color, 0, 0, 0, color[3] * bound(0, shadowalpha, 1));
	}
}

float Font_DrawString_Font(float startx, float starty,
		      const char *text, size_t maxlen,
		      float w, float h,
		      float basered, float basegreen, float baseblue, float basealpha,
		      int flags, int *outcolor, qboolean ignorecolorcodes,
		      ft2_font_t *fnt)
{
	int shadow, colorindex = STRING_COLOR_DEFAULT;
	float x = startx, y, thisw;
	float *av, *at, *ac;
	float color[4];
	int batchcount;
	float vertex3f[QUADELEMENTS_MAXQUADS*4*3];
	float texcoord2f[QUADELEMENTS_MAXQUADS*4*2];
	float color4f[QUADELEMENTS_MAXQUADS*4*4];
	Uchar ch, mapch;
	size_t i;
	const char *text_start = text;
	int tempcolorindex;
	ft2_font_map_t *prevmap = NULL;
	ft2_font_map_t *map;

	if (maxlen < 1)
		maxlen = 1<<30;

	_Font_ProcessDrawFlag(flags);

	R_Mesh_ColorPointer(color4f, 0, 0);
	R_Mesh_ResetTextureState();
	R_Mesh_TexCoordPointer(0, 2, texcoord2f, 0, 0);
	R_Mesh_VertexPointer(vertex3f, 0, 0);
	R_SetupGenericShader(true);

	ac = color4f;
	at = texcoord2f;
	av = vertex3f;
	batchcount = 0;

	// We render onto the baseline, so move down by the intended height.
	// Otherwise the text appears too high since the top edge would be the base line.
	starty += (double)h * (5.0/6.0); // don't use the complete height
	// with sane fonts it should be possible to use the font's `ascent` value
	// but then again, is it safe?

	for (shadow = r_textshadow.value != 0 && basealpha > 0;shadow >= 0;shadow--)
	{
		text = text_start;
		if (!outcolor || *outcolor == -1)
			colorindex = STRING_COLOR_DEFAULT;
		else
			colorindex = *outcolor;

		Font_GetTextColor(color, colorindex, basered, basegreen, baseblue, basealpha, shadow);

		x = startx;
		y = starty;
		if (shadow)
		{
			x += r_textshadow.value;
			y += r_textshadow.value;
		}
		for (i = 0;i < maxlen && *text;i++)
		{
			if (*text == STRING_COLOR_TAG && !ignorecolorcodes && i + 1 < maxlen)
			{
				const char *before;
				Uchar chx[3];
				++text;
				ch = *text; // the color tag is an ASCII character!
				if (ch == STRING_COLOR_RGB_TAG_CHAR)
				{
					// we need some preparation here
					before = text;
					chx[2] = 0;
					if (*text) chx[0] = u8_getchar(text, &text);
					if (*text) chx[1] = u8_getchar(text, &text);
					if (*text) chx[2] = u8_getchar(text, &text);
					if ( ( (chx[0] >= 'A' && chx[0] <= 'F') || (chx[0] >= 'a' && chx[0] <= 'f') || (chx[0] >= '0' && chx[0] <= '9') ) &&
					     ( (chx[1] >= 'A' && chx[1] <= 'F') || (chx[1] >= 'a' && chx[1] <= 'f') || (chx[1] >= '0' && chx[1] <= '9') ) &&
					     ( (chx[2] >= 'A' && chx[2] <= 'F') || (chx[2] >= 'a' && chx[2] <= 'f') || (chx[2] >= '0' && chx[2] <= '9') ) )
					{
					}
					else
						chx[2] = 0;
					text = before; // start from the first hex character
				}
				if (ch <= '9' && ch >= '0') // ^[0-9] found
				{
					colorindex = ch - '0';
					Font_GetTextColor(color, colorindex, basered, basegreen, baseblue, basealpha, shadow);
					continue;
				}
				else if (ch == STRING_COLOR_RGB_TAG_CHAR && i + 3 < maxlen && chx[2]) // ^x found
				{
					// building colorindex...
					ch = tolower(text[i+1]);
					tempcolorindex = 0x10000; // binary: 1,0000,0000,0000,0000
					if (ch <= '9' && ch >= '0') tempcolorindex |= (ch - '0') << 12;
					else if (ch >= 'a' && ch <= 'f') tempcolorindex |= (ch - 87) << 12;
					else tempcolorindex = 0;
					if (tempcolorindex)
					{
						ch = tolower(text[i+2]);
						if (ch <= '9' && ch >= '0') tempcolorindex |= (ch - '0') << 8;
						else if (ch >= 'a' && ch <= 'f') tempcolorindex |= (ch - 87) << 8;
						else tempcolorindex = 0;
						if (tempcolorindex)
						{
							ch = tolower(text[i+3]);
							if (ch <= '9' && ch >= '0') tempcolorindex |= (ch - '0') << 4;
							else if (ch >= 'a' && ch <= 'f') tempcolorindex |= (ch - 87) << 4;
							else tempcolorindex = 0;
							if (tempcolorindex)
							{
								colorindex = tempcolorindex | 0xf;
								// ...done! now colorindex has rgba codes (1,rrrr,gggg,bbbb,aaaa)
								//Con_Printf("^1colorindex:^7 %x\n", colorindex);
								Font_GetTextColor(color, colorindex, basered, basegreen, baseblue, basealpha, shadow);
								i+=3;
								continue;
							}
						}
					}
				}
				else if (ch == STRING_COLOR_TAG)
					i++;
				i--;
			}
			ch = u8_getchar(text, &text);

			map = fnt->font_map;
			while(map && map->start + FONT_CHARS_PER_MAP < ch)
				map = map->next;
			if (!map)
			{
				if (!Font_LoadMapForIndex(fnt, ch, &map))
				{
					shadow = -1;
					break;
				}
				if (!map)
				{
					// this shouldn't happen
					shadow = -1;
					break;
				}
			}

			if (map != prevmap && batchcount)
			{
				// we need a different character map, render what we currently have:
				GL_LockArrays(0, batchcount * 4);
				R_Mesh_Draw(0, batchcount * 4, 0, batchcount * 2, NULL, quadelements, 0, 0);
				GL_LockArrays(0, 0);
				batchcount = 0;
				ac = color4f;
				at = texcoord2f;
				av = vertex3f;
			}

			// TODO: don't call Mesh_Draw all the time
			// call it when the texture changes or the batchcount hits the limit

			R_Mesh_TexBind(0, R_GetTexture(map->texture));
			R_SetupGenericShader(true);

			mapch = ch - map->start;
			thisw = map->glyphs[mapch].advance_x;

			ac[ 0] = color[0]; ac[ 1] = color[1]; ac[ 2] = color[2]; ac[ 3] = color[3];
			ac[ 4] = color[0]; ac[ 5] = color[1]; ac[ 6] = color[2]; ac[ 7] = color[3];
			ac[ 8] = color[0]; ac[ 9] = color[1]; ac[10] = color[2]; ac[11] = color[3];
			ac[12] = color[0]; ac[13] = color[1]; ac[14] = color[2]; ac[15] = color[3];
			at[0] = map->glyphs[mapch].txmin; at[1] = map->glyphs[mapch].tymin;
			at[2] = map->glyphs[mapch].txmax; at[3] = map->glyphs[mapch].tymin;
			at[4] = map->glyphs[mapch].txmax; at[5] = map->glyphs[mapch].tymax;
			at[6] = map->glyphs[mapch].txmin; at[7] = map->glyphs[mapch].tymax;
#define PIXEL_X(x) ( (x)/vid_conwidth.value * vid.width )
#define PIXEL_Y(x) ( (x)/vid_conheight.value * vid.height )
			av[ 0] = x + w * PIXEL_X(map->glyphs[mapch].vxmin); av[ 1] = y + h * PIXEL_Y(map->glyphs[mapch].vymin); av[ 2] = 10;
			av[ 3] = x + w * PIXEL_X(map->glyphs[mapch].vxmax); av[ 4] = y + h * PIXEL_Y(map->glyphs[mapch].vymin); av[ 5] = 10;
			av[ 6] = x + w * PIXEL_X(map->glyphs[mapch].vxmax); av[ 7] = y + h * PIXEL_Y(map->glyphs[mapch].vymax); av[ 8] = 10;
			av[ 9] = x + w * PIXEL_X(map->glyphs[mapch].vxmin); av[10] = y + h * PIXEL_Y(map->glyphs[mapch].vymax); av[11] = 10;

			x += PIXEL_X(thisw * w);
			ac += 16;
			at += 8;
			av += 12;
			batchcount++;
			if (batchcount >= QUADELEMENTS_MAXQUADS)
			{
				GL_LockArrays(0, batchcount * 4);
				R_Mesh_Draw(0, batchcount * 4, 0, batchcount * 2, NULL, quadelements, 0, 0);
				GL_LockArrays(0, 0);
				batchcount = 0;
				ac = color4f;
				at = texcoord2f;
				av = vertex3f;
			}
			/*
			GL_LockArrays(0, 4);
			R_Mesh_Draw(0, 4, 0, 2, NULL, quadelements, 0, 0);
			GL_LockArrays(0, 0);
			*/

			prevmap = map;
		}
	}
	if (batchcount > 0)
	{
		GL_LockArrays(0, batchcount * 4);
		R_Mesh_Draw(0, batchcount * 4, 0, batchcount * 2, NULL, quadelements, 0, 0);
		GL_LockArrays(0, 0);
	}

	if (outcolor)
		*outcolor = colorindex;

	// note: this relies on the proper text (not shadow) being drawn last
	return x;
}

float Font_DrawString(float startx, float starty,
		      const char *text, size_t maxlen,
		      float width, float height,
		      float basered, float basegreen, float baseblue, float basealpha,
		      int flags, int *outcolor, qboolean ignorecolorcodes)
{
	return Font_DrawString_Font(startx, starty, text, maxlen,
				    width, height,
				    basered, basegreen, baseblue, basealpha,
				    flags, outcolor, ignorecolorcodes,
				    &test_font);
}
#endif
