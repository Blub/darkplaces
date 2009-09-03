/* Header for FreeType 2 and UTF-8 encoding support for
 * DarkPlaces
 */

#ifndef DP_FREETYPE2_H__
#define DP_FREETYPE2_H__

//#include <sys/types.h>

#include "utf8lib.h"

/* 
 * From http://www.unicode.org/Public/UNIDATA/Blocks.txt
 *
 *   E000..F8FF; Private Use Area
 *   F0000..FFFFF; Supplementary Private Use Area-A
 *
 * TODO:
 *   Range E000 - E0FF
 *     Contains the non-FreeType2 version of characters.
 */

typedef struct ft2_font_map_s ft2_font_map_t;
typedef struct ft2_attachment_s ft2_attachment_t;
#define ft2_oldstyle_map ((ft2_font_map_t*)-1)

typedef float ft2_kernvec[2];
typedef struct ft2_kerning_s
{
	ft2_kernvec kerning[256][256]; /* kerning[left char][right char] */
} ft2_kerning_t;

typedef struct ft2_font_s
{
	char            name[64];

	qboolean        has_kerning;

	// TODO: clean this up and do not expose everything.
	
	//unsigned char  *data;
	//fs_offset_t     datasize;
	void           *face;

	// an unordered array of ordered linked lists of glyph maps for a specific size
	ft2_font_map_t *font_maps[MAX_FONT_SIZES];
	int             num_sizes;

	// attachments
	size_t            attachmentcount;
	ft2_attachment_t *attachments;
} ft2_font_t;

void            Font_CloseLibrary(void);
void            Font_Init(void);
qboolean        Font_OpenLibrary(void);
ft2_font_t*     Font_Alloc(void);
void            Font_UnloadFont(ft2_font_t *font);
//qboolean        Font_LoadFont(const char *name, int size, int face, ft2_font_t *font);
int             Font_IndexForSize(ft2_font_t *font, float size);
ft2_font_map_t *Font_MapForIndex(ft2_font_t *font, int index);
qboolean        Font_LoadFont(const char *name, dp_font_t *dpfnt);
qboolean        Font_GetKerningForSize(ft2_font_t *font, float w, float h, Uchar left, Uchar right, float *outx, float *outy);
qboolean        Font_GetKerningForMap(ft2_font_t *font, int map_index, float w, float h, Uchar left, Uchar right, float *outx, float *outy);

#endif // DP_FREETYPE2_H__
