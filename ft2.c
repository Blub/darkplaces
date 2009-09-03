/* FreeType 2 and UTF-8 encoding support for
 * DarkPlaces
 */
#include "quakedef.h"

#include "ft2.h"
#include "ft2_defs.h"
#include "ft2_fontdefs.h"

/*
================================================================================
CVars introduced with the freetype extension
================================================================================
*/

cvar_t r_font_disable_freetype = {CVAR_SAVE, "r_font_disable_freetype", "0", "disable freetype support for fonts entirely"};
cvar_t r_font_use_alpha_textures = {CVAR_SAVE, "r_font_use_alpha_textures", "0", "use alpha-textures for font rendering, this should safe memory"};

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

	if (r_font_disable_freetype.integer)
		return false;

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
	int i;
	for (i = 0; i < MAX_FONTS; ++i)
	{
		if (dp_fonts[i].ft2)
		{
			Font_UnloadFont(dp_fonts[i].ft2);
			dp_fonts[i].ft2 = NULL;
		}
	}
	Font_CloseLibrary();
}

void font_newmap(void)
{
}

void Font_Init(void)
{
	Cvar_RegisterVariable(&r_font_disable_freetype);
	Cvar_RegisterVariable(&r_font_use_alpha_textures);
}

/*
================================================================================
Implementation of a more or less lazy font loading and rendering code.
================================================================================
*/

#include "ft2_fontdefs.h"

ft2_font_t *Font_Alloc(void)
{
	if (!ft2_dll)
		return NULL;
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

static qboolean Font_LoadFile(const char *name, int _face, ft2_font_t *font);
static qboolean Font_LoadSize(ft2_font_t *font, float size);
qboolean Font_LoadFont(const char *name, dp_font_t *dpfnt)
{
	int s, count;
	ft2_font_t *ft2;

	ft2 = Font_Alloc();
	if (!ft2)
	{
		dpfnt->ft2 = NULL;
		return false;
	}

	if (!Font_LoadFile(name, dpfnt->req_face, ft2))
	{
		dpfnt->ft2 = NULL;
		Mem_Free(ft2);
		return false;
	}

	count = 0;
	for (s = 0; s < MAX_FONT_SIZES; ++s)
	{
		if (Font_LoadSize(ft2, dpfnt->req_sizes[s]))
			++count;
	}
	if (!count)
	{
		// loading failed for every requested size
		Font_UnloadFont(ft2);
		Mem_Free(ft2);
		dpfnt->ft2 = NULL;
		return false;
	}
	//Con_Printf("%i sizes loaded\n", count);
	dpfnt->ft2 = ft2;
	return true;
}

static qboolean Font_LoadFile(const char *name, int _face, ft2_font_t *font)
{
	size_t namelen;
	char filename[PATH_MAX];
	int status;
	size_t i;
	unsigned char *data;
	fs_offset_t datasize;

	memset(font, 0, sizeof(*font));

	if (!Font_OpenLibrary())
	{
		if (!r_font_disable_freetype.integer)
		{
			Con_Printf("WARNING: can't open load font %s\n"
				   "You need the FreeType2 DLL to load font files\n",
				   name);
		}
		return false;
	}

	namelen = strlen(name);

	memcpy(filename, name, namelen);
	memcpy(filename + namelen, ".ttf", 5);
	data = FS_LoadFile(filename, font_mempool, false, &datasize);
	if (!data)
	{
		memcpy(filename + namelen, ".otf", 5);
		data = FS_LoadFile(filename, font_mempool, false, &datasize);
	}
	if (!data)
	{
		ft2_attachment_t afm;

		memcpy(filename + namelen, ".pfb", 5);
		data = FS_LoadFile(filename, font_mempool, false, &datasize);

		if (data)
		{
			memcpy(filename + namelen, ".afm", 5);
			afm.data = FS_LoadFile(filename, font_mempool, false, &afm.size);

			if (afm.data)
				Font_Attach(font, &afm);
		}
	}

	if (!data)
	{
		// FS_LoadFile being not-quiet should print an error :)
		return false;
	}
	Con_Printf("Loading font %s face %i...\n", filename, _face);

	status = qFT_New_Memory_Face(font_ft2lib, (FT_Bytes)data, datasize, _face, (FT_Face*)&font->face);
	if (status && _face != 0)
	{
		Con_Printf("Failed to load face %i of %s. Falling back to face 0\n", _face, name);
		_face = 0;
		status = qFT_New_Memory_Face(font_ft2lib, (FT_Bytes)data, datasize, 0, (FT_Face*)&font->face);
	}
	if (status)
	{
		Con_Printf("ERROR: can't create face for %s\n"
			   "Error %i\n", // TODO: error strings
			   name, status);
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
	//font->size = size;
	//font->glyphSize = font->size * 2;
	font->has_kerning = !!(((FT_Face)(font->face))->face_flags & FT_FACE_FLAG_KERNING);
	//font->sfx = (1.0/64.0) / (double)font->size;
	//font->sfy = (1.0/64.0) / (double)font->size;
	return true;
}

static qboolean Font_LoadMap(ft2_font_t *font, ft2_font_map_t *mapstart, Uchar _ch, ft2_font_map_t **outmap);
static qboolean Font_LoadSize(ft2_font_t *font, float size)
{
	int map_index;
	ft2_font_map_t *fmap, temp;

	if (size < 0)
		return false;
	if (!size)
		size = 16;

	for (map_index = 0; map_index < MAX_FONT_SIZES; ++map_index)
	{
		if (!font->font_maps[map_index])
			break;
		// if a similar size has already been loaded, ignore this one
		//abs(font->font_maps[map_index]->size - size) < 4
		if (font->font_maps[map_index]->size == size)
			return true;
	}
	
	if (map_index >= MAX_FONT_SIZES)
		return false;

	memset(&temp, 0, sizeof(temp));
	temp.size = size;
	temp.glyphSize = size*2;
	temp.sfx = (1.0/64.0)/(double)size;
	temp.sfy = (1.0/64.0)/(double)size;
	if (!Font_LoadMap(font, &temp, 0, &fmap))
	{
		Con_Printf("ERROR: can't load the first character map for %s\n"
			   "This is fatal\n",
			   font->name);
		Font_UnloadFont(font);
		return false;
	}
	font->font_maps[map_index] = temp.next;

	fmap->sfx = temp.sfx;
	fmap->sfy = temp.sfy;

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
					fmap->kerning.kerning[l][r][0] = 0;
					fmap->kerning.kerning[l][r][1] = 0;
				}
				else
				{
					fmap->kerning.kerning[l][r][0] = kernvec.x * fmap->sfx;
					fmap->kerning.kerning[l][r][1] = kernvec.y * fmap->sfy;
				}
			}
		}
	}

	return true;
}

int Font_IndexForSize(ft2_font_t *font, float fsize)
{
	int match = -1;
	int value = 1000000;
	int nval;
	int matchsize = -10000;
	int m;
	int size;
	ft2_font_map_t **maps = font->font_maps;

	fsize = fsize * vid.height / vid_conheight.value;
	
	if (fsize < 0)
		size = 16;
	else
	{
		// round up
		size = (int)fsize;
		if (fsize - (float)size >= 0.49)
			++size;
	}
	
	for (m = 0; m < MAX_FONT_SIZES; ++m)
	{
		if (!maps[m])
			continue;
		// "round up" to the bigger size if two equally-valued matches exist
		nval = abs(maps[m]->size - size);
		if (match == -1 || nval < value || (nval == value && matchsize < maps[m]->size))
		{
			value = nval;
			match = m;
			matchsize = maps[m]->size;
			if (value == 0) // there is no better match
			{
				/*
				if (fsize != -1 && (fsize < 4.591 || fsize > 4.5914))
					Con_Printf(" %f -> %i [%i]\n", fsize, match, matchsize);
				*/
				return match;
			}
		}
	}
	/*
	if (fsize != -1 && (fsize < 4.591 || fsize > 4.5914))
		Con_Printf(" %f -> %i [%i]\n", fsize, match, matchsize);
	*/
	return match;
}

ft2_font_map_t *Font_MapForIndex(ft2_font_t *font, int index)
{
	if (index < 0 || index >= MAX_FONT_SIZES)
		return NULL;
	return font->font_maps[index];
}

static qboolean Font_SetSize(ft2_font_t *font, float w, float h)
{
	if (qFT_Set_Char_Size((FT_Face)font->face, w*64, h*64, 72, 72))
		return false;
	return true;
}

qboolean Font_GetKerningForMap(ft2_font_t *font, int map_index, float w, float h, Uchar left, Uchar right, float *outx, float *outy)
{
	ft2_font_map_t *fmap;
	if (!font->has_kerning)
		return false;
	if (map_index < 0 || map_index >= MAX_FONT_SIZES)
		return false;
	fmap = font->font_maps[map_index];
	if (!fmap)
		return false;
	if (left < 256 && right < 256)
	{
		// quick-kerning, be aware of the size: scale it
		if (outx) *outx = fmap->kerning.kerning[left][right][0] * w / (float)fmap->size;
		if (outy) *outy = fmap->kerning.kerning[left][right][1] * h / (float)fmap->size;
		return true;
	}
	else
	{
		FT_Vector kernvec;
		FT_ULong ul, ur;

		//if (qFT_Set_Pixel_Sizes((FT_Face)font->face, 0, fmap->size))
		if (!Font_SetSize(font, w, h))
		{
			// this deserves an error message
			Con_Printf("Failed to get kerning for %s\n", font->name);
			return false;
		}
		ul = qFT_Get_Char_Index(font->face, left);
		ur = qFT_Get_Char_Index(font->face, right);
		if (qFT_Get_Kerning(font->face, ul, ur, FT_KERNING_DEFAULT, &kernvec))
		{
			if (outx) *outx = kernvec.x * fmap->sfx;
			if (outy) *outy = kernvec.y * fmap->sfy;
			return true;
		}
		return false;
	}
}

qboolean Font_GetKerningForSize(ft2_font_t *font, float w, float h, Uchar left, Uchar right, float *outx, float *outy)
{
	return Font_GetKerningForMap(font, Font_IndexForSize(font, h), w, h, left, right, outx, outy);
}

static void UnloadMapRec(ft2_font_map_t *map)
{
	if (map->texture)
	{
		R_FreeTexture(map->texture);
		map->texture = NULL;
	}
	if (map->next)
		UnloadMapRec(map->next);
	Mem_Free(map);
}

void Font_UnloadFont(ft2_font_t *font)
{
	int i;
	/*
	if (font->data)
		Mem_Free(font->data);
	*/
	if (font->attachments && font->attachmentcount)
	{
		Mem_Free(font->attachments);
		font->attachmentcount = 0;
		font->attachments = NULL;
	}
	for (i = 0; i < MAX_FONT_SIZES; ++i)
	{
		if (font->font_maps[i])
		{
			UnloadMapRec(font->font_maps[i]);
			font->font_maps[i] = NULL;
		}
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

static qboolean Font_LoadMap(ft2_font_t *font, ft2_font_map_t *mapstart, Uchar _ch, ft2_font_map_t **outmap)
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

	ft2_font_map_t *map, *next;

	int bytesPerPixel = 4; // change the conversion loop too if you change this!

	if (outmap)
		*outmap = NULL;

	if (r_font_use_alpha_textures.integer)
		bytesPerPixel = 1;

	//status = qFT_Set_Pixel_Sizes((FT_Face)font->face, /*size*/0, mapstart->size);
	//if (status)
	if (!Font_SetSize(font, mapstart->size, mapstart->size))
	{
		Con_Printf("ERROR: can't set pixel sizes for font %s\n", font->name);
		return false;
	}

	map = Mem_Alloc(font_mempool, sizeof(ft2_font_map_t));
	if (!map)
	{
		Con_Printf("ERROR: Out of memory when loading fontmap for %s\n", font->name);
		return false;
	}

	// copy over the information
	map->size = mapstart->size;
	map->glyphSize = mapstart->glyphSize;
	map->sfx = mapstart->sfx;
	map->sfy = mapstart->sfy;

	pitch = map->glyphSize * FONT_CHARS_PER_LINE * bytesPerPixel;
	data = Mem_Alloc(font_mempool, (FONT_CHAR_LINES * map->glyphSize) * pitch);
	if (!data)
	{
		Con_Printf("ERROR: Failed to allocate memory for font %s size %i\n", font->name, map->size);
		Mem_Free(map);
		return false;
	}

	// initialize as white texture with zero alpha
	tp = 0;
	while (tp < (FONT_CHAR_LINES * map->glyphSize) * pitch)
	{
		if (bytesPerPixel == 4)
		{
			data[tp++] = 0xFF;
			data[tp++] = 0xFF;
			data[tp++] = 0xFF;
		}
		data[tp++] = 0x00;
	}

	// insert the map
	map->start = mapidx * FONT_CHARS_PER_MAP;
	next = mapstart;
	while(next->next && next->next->start < map->start)
		next = next->next;
	map->next = next->next;
	next->next = map;

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
			Con_Print("glyphinfo: ------------- GLYPH INFO -----------------\n");

		++gC;
		if (gC >= FONT_CHARS_PER_LINE)
		{
			gC -= FONT_CHARS_PER_LINE;
			++gR;
		}

		imagedata = data + gR * pitch * map->glyphSize + gC * map->glyphSize * bytesPerPixel;
		//glyphIndex = qFT_Get_Char_Index(face, ch);
		//status = qFT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER);
		status = qFT_Load_Char(face, ch, FT_LOAD_RENDER);
		if (status)
		{
			//Con_Printf("failed to load glyph %lu for %s\n", glyphIndex, font->name);
			Con_Printf("failed to load glyph for char %lx from font %s\n", (unsigned long)ch, font->name);
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

		if (w > map->glyphSize || h > map->glyphSize)
			Con_Printf("WARNING: Glyph %lu is too big in font %s, size %i\n", ch, font->name, map->size);

		switch (bmp->pixel_mode)
		{
		case FT_PIXEL_MODE_MONO:
			if (developer.integer)
				Con_Print("glyphinfo:   Pixel Mode: MONO\n");
			break;
		case FT_PIXEL_MODE_GRAY2:
			if (developer.integer)
				Con_Print("glyphinfo:   Pixel Mode: GRAY2\n");
			break;
		case FT_PIXEL_MODE_GRAY4:
			if (developer.integer)
				Con_Print("glyphinfo:   Pixel Mode: GRAY4\n");
			break;
		case FT_PIXEL_MODE_GRAY:
			if (developer.integer)
				Con_Print("glyphinfo:   Pixel Mode: GRAY\n");
			break;
		default:
			if (developer.integer)
				Con_Printf("glyphinfo:   Pixel Mode: Unknown: %i\n", bmp->pixel_mode);
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
				dst += bytesPerPixel - 1; // shift to alpha byte
				for (x = 0; x < bmp->width; x += 8)
				{
					unsigned char ch = *src++;
					*dst = 255 * ((ch & 0x80) >> 7); dst += bytesPerPixel;
					*dst = 255 * ((ch & 0x40) >> 6); dst += bytesPerPixel;
					*dst = 255 * ((ch & 0x20) >> 5); dst += bytesPerPixel;
					*dst = 255 * ((ch & 0x10) >> 4); dst += bytesPerPixel;
					*dst = 255 * ((ch & 0x08) >> 3); dst += bytesPerPixel;
					*dst = 255 * ((ch & 0x04) >> 2); dst += bytesPerPixel;
					*dst = 255 * ((ch & 0x02) >> 1); dst += bytesPerPixel;
					*dst = 255 * ((ch & 0x01) >> 0); dst += bytesPerPixel;
				}
				break;
			case FT_PIXEL_MODE_GRAY2:
				dst += bytesPerPixel - 1; // shift to alpha byte
				for (x = 0; x < bmp->width; x += 4)
				{
					unsigned char ch = *src++;
					*dst = ( ((ch & 0xA0) >> 6) * 0x55 ); ch <<= 2; dst += bytesPerPixel;
					*dst = ( ((ch & 0xA0) >> 6) * 0x55 ); ch <<= 2; dst += bytesPerPixel;
					*dst = ( ((ch & 0xA0) >> 6) * 0x55 ); ch <<= 2; dst += bytesPerPixel;
					*dst = ( ((ch & 0xA0) >> 6) * 0x55 ); ch <<= 2; dst += bytesPerPixel;
				}
				break;
			case FT_PIXEL_MODE_GRAY4:
				dst += bytesPerPixel - 1; // shift to alpha byte
				for (x = 0; x < bmp->width; x += 2)
				{
					unsigned char ch = *src++;
					*dst = ( ((ch & 0xF0) >> 4) * 0x24); dst += bytesPerPixel;
					*dst = ( ((ch & 0x0F) ) * 0x24); dst += bytesPerPixel;
				}
				break;
			case FT_PIXEL_MODE_GRAY:
				// in this case pitch should equal width
				for (tp = 0; tp < bmp->pitch; ++tp)
					dst[(bytesPerPixel - 1) + tp*bytesPerPixel] = src[tp]; // copy the grey value into the alpha bytes

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
			double bearingX = (double)glyph->metrics.horiBearingX * map->sfx;
			double bearingY = (double)glyph->metrics.horiBearingY * map->sfy;
			double advance = (double)glyph->metrics.horiAdvance * map->sfx;
			//double advance = glyph->advance.x >> 6;
			double mWidth = (double)glyph->metrics.width * map->sfx;
			double mHeight = (double)glyph->metrics.height * map->sfy;
			//double tWidth = bmp->width / (double)font->size;
			//double tHeight = bmp->rows / (double)font->size;

			mapglyph->vxmin = bearingX;
			mapglyph->vxmax = bearingX + mWidth;
			mapglyph->vymin = -bearingY;
			mapglyph->vymax = mHeight - bearingY;
			mapglyph->txmin = ( (double)(gC * map->glyphSize) ) / ( (double)(map->glyphSize * FONT_CHARS_PER_LINE) );
			mapglyph->txmax = mapglyph->txmin + (double)bmp->width / ( (double)(map->glyphSize * FONT_CHARS_PER_LINE) );
			mapglyph->tymin = ( (double)(gR * map->glyphSize) ) / ( (double)(map->glyphSize * FONT_CHAR_LINES) );
			mapglyph->tymax = mapglyph->tymin + (double)bmp->rows / ( (double)(map->glyphSize * FONT_CHAR_LINES) );
			//mapglyph->advance_x = advance * font->size;
			mapglyph->advance_x = advance;
			mapglyph->advance_y = 0;

			if (developer.integer)
			{
				Con_Printf("glyphinfo:   Glyph: %lu   at (%i, %i)\n", (unsigned long)ch, gC, gR);
				Con_Printf("glyphinfo:   %f, %f, %lu\n", bearingX, map->sfx, (unsigned long)glyph->metrics.horiBearingX);
				if (ch >= 32 && ch <= 128)
					Con_Printf("glyphinfo:   Character: %c\n", (int)ch);
				Con_Printf("glyphinfo:   Vertex info:\n");
				Con_Printf("glyphinfo:     X: ( %f  --  %f )\n", mapglyph->vxmin, mapglyph->vxmax);
				Con_Printf("glyphinfo:     Y: ( %f  --  %f )\n", mapglyph->vymin, mapglyph->vymax);
				Con_Printf("glyphinfo:   Texture info:\n");
				Con_Printf("glyphinfo:     S: ( %f  --  %f )\n", mapglyph->txmin, mapglyph->txmax);
				Con_Printf("glyphinfo:     T: ( %f  --  %f )\n", mapglyph->tymin, mapglyph->tymax);
				Con_Printf("glyphinfo:   Advance: %f, %f\n", mapglyph->advance_x, mapglyph->advance_y);
			}
		}
	}

	// create a texture from the data now

	if (developer.integer)
	{
		// view using `display -depth 8 -size 512x512 name_page.rgba` (be sure to use a correct -size parameter)
		dpsnprintf(map_identifier, sizeof(map_identifier), "%s_%u.rgba", font->name, (unsigned)map->start/FONT_CHARS_PER_MAP);
		FS_WriteFile(map_identifier, data, pitch * FONT_CHAR_LINES * map->glyphSize);
	}
	dpsnprintf(map_identifier, sizeof(map_identifier), "%s_%u", font->name, (unsigned)map->start/FONT_CHARS_PER_MAP);

	// probably use bytesPerPixel here instead?
	if (r_font_use_alpha_textures.integer)
	{
		map->texture = R_LoadTexture2D(font_texturepool, map_identifier,
					       map->glyphSize * FONT_CHARS_PER_LINE,
					       map->glyphSize * FONT_CHAR_LINES,
					       data, TEXTYPE_ALPHA, TEXF_ALPHA | TEXF_ALWAYSPRECACHE/* | TEXF_MIPMAP*/, NULL);
	} else {
		map->texture = R_LoadTexture2D(font_texturepool, map_identifier,
					       map->glyphSize * FONT_CHARS_PER_LINE,
					       map->glyphSize * FONT_CHAR_LINES,
					       data, TEXTYPE_RGBA, TEXF_ALPHA | TEXF_ALWAYSPRECACHE/* | TEXF_MIPMAP*/, NULL);
	}

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

qboolean Font_LoadMapForIndex(ft2_font_t *font, int map_index, Uchar _ch, ft2_font_map_t **outmap)
{
	if (map_index < 0 || map_index >= MAX_FONT_SIZES)
		return false;
	// the first map must have been loaded already
	if (!font->font_maps[map_index])
		return false;
	return Font_LoadMap(font, font->font_maps[map_index], _ch, outmap);
}
