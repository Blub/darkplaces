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
FT_EXPORT( FT_Error )
(*qFT_Attach_Stream)( FT_Face        face,
		      FT_Open_Args*  parameters );
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
	{"FT_Attach_Stream",		(void **) &qFT_Attach_Stream},
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

	/*
	if (!Font_LoadFont("gfx/test", 16, &test_font))
	{
		Con_Print("ERROR: Failed to load test font!\n");
		Font_CloseLibrary();
		return;
	}
	*/
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

qboolean Font_Attach(ft2_font_t *font, ft2_attachment_t *attachment)
{
	ft2_attachment_t *na;

	font->attachmentcount++;
	na = (ft2_attachment_t*)Mem_Alloc(font_mempool, sizeof(font->attachments[0]) * font->attachmentcount);
	if (na == NULL)
		return false;
	if (font->attachments && font->attachmentcount > 1)
	{
		memcpy(na, font->attachments, sizeof(font->attachments[0]) * (font->attachmentcount - 1));
		Mem_Free(font->attachments);
	}
	memcpy(na + sizeof(font->attachments[0]) * (font->attachmentcount - 1), attachment, sizeof(*attachment));
	font->attachments = na;
	return true;
}

qboolean Font_LoadFont(const char *name, int size, ft2_font_t *font)
{
	size_t namelen;
	char filename[PATH_MAX];
	int status;
	size_t i;

	memset(font, 0, sizeof(*font));

	if (size <= 0) // default size requested
		size = 24;

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
		ft2_attachment_t afm;

		memcpy(filename + namelen, ".pfb", 5);
		font->data = FS_LoadFile(filename, font_mempool, false, &font->datasize);

		if (font->data)
		{
			memcpy(filename + namelen, ".afm", 5);
			afm.data = FS_LoadFile(filename, font_mempool, false, &afm.size);

			if (afm.data)
				Font_Attach(font, &afm);
		}
	}

	if (!font->data)
	{
		// FS_LoadFile being not-quiet should print an error :)
		Con_Printf("Failed to load font-file for '%s', it will not support as many characters.\n", name);
		return false;
	}
	Con_Printf("Loading font %s face 0 size %i...\n", filename, size);

	status = qFT_New_Memory_Face(font_ft2lib, (FT_Bytes)font->data, font->datasize, 0, (FT_Face*)&font->face);
	if (status)
	{
		Con_Printf("ERROR: can't create face for %s\n"
			   "Error %i\n", // TODO: error strings
			   name, status);
		// Mem_Free(font->data);
		Font_UnloadFont(font);
		return false;
	}

	// add the attachments
	for (i = 0; i < font->attachmentcount; ++i)
	{
		FT_Open_Args args;
		memset(&args, 0, sizeof(args));
		args.flags = FT_OPEN_MEMORY;
		args.memory_base = (const FT_Byte*)font->attachments[i].data;
		args.memory_size = font->attachments[i].size;
		if (qFT_Attach_Stream(font->face, &args))
			Con_Printf("Failed to add attachment %u to %s\n", (unsigned)i, font->name);
	}

	memcpy(font->name, name, namelen+1);
	font->size = size;
	font->glyphSize = font->size * 2;
	font->has_kerning = !!(((FT_Face)(font->face))->face_flags & FT_FACE_FLAG_KERNING);
	font->sfx = (1.0/64.0) / (double)font->size;
	font->sfy = (1.0/64.0) / (double)font->size;

	status = qFT_Set_Pixel_Sizes((FT_Face)font->face, /*size*/0, size);
	if (status)
	{
		Con_Printf("ERROR: can't size pixel sizes for face of font %s\n"
			   "Error %i\n", // TODO: error strings
			   name, status);
		//Mem_Free(font->data);
		Font_UnloadFont(font);
		return false;
	}

	if (!Font_LoadMapForIndex(font, 0, NULL))
	{
		Con_Printf("ERROR: can't load the first character map for %s\n"
			   "This is fatal\n",
			   name);
		//Mem_Free(font->data);
		Font_UnloadFont(font);
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
				FT_ULong ul, ur;
				ul = qFT_Get_Char_Index(font->face, l);
				ur = qFT_Get_Char_Index(font->face, r);
				if (qFT_Get_Kerning(font->face, ul, ur, FT_KERNING_DEFAULT, &kernvec))
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
	if (!font->has_kerning)
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
		FT_ULong ul, ur;
		ul = qFT_Get_Char_Index(font->face, left);
		ur = qFT_Get_Char_Index(font->face, right);
		if (qFT_Get_Kerning(font->face, ul, ur, FT_KERNING_DEFAULT, &kernvec))
		{
			if (outx) *outx = kernvec.x * font->sfx;
			if (outy) *outy = kernvec.y * font->sfy;
			return true;
		}
		return false;
	}
}

void Font_UnloadFont(ft2_font_t *font)
{
	if (font->data)
		Mem_Free(font->data);
	if (font->attachments && font->attachmentcount)
	{
		Mem_Free(font->attachments);
		font->attachmentcount = 0;
		font->attachments = NULL;
	}
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
		//FT_ULong glyphIndex;
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
		//glyphIndex = qFT_Get_Char_Index(face, ch);
		//status = qFT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER);
		status = qFT_Load_Char(face, ch, FT_LOAD_RENDER);
		if (status)
		{
			//Con_Printf("failed to load glyph %lu for %s\n", glyphIndex, font->name);
			Con_Printf("failed to load glyph for char %x from font %s\n", ch, font->name);
			continue;
		}

		/* obsolete when using FT_LOAD_RENDER
		if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
		{
			status = qFT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
			if (status)
			{
				Con_Printf("failed to render glyph %lu for %s\n", glyphIndex, font->name);
				continue;
			}
		}
		*/

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
			//double advance = glyph->advance.x >> 6;
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
