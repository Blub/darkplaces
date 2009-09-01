#ifndef FT2_PRIVATE_H__
#define FT2_PRIVATE_H__

// anything should work, but I recommend multiples of 8
// since the texture size should be a power of 2
#define FONT_CHARS_PER_LINE 16
#define FONT_CHAR_LINES 16
#define FONT_CHARS_PER_MAP (FONT_CHARS_PER_LINE * FONT_CHAR_LINES)

typedef struct glyph_slot_s
{
	// we keep the quad coords here only currently
	// if you need other info, make Font_LoadMapForIndex fill it into this slot
	double txmin; // texture coordinate in [0,1]
	double txmax;
	double tymin;
	double tymax;
	float vxmin;
	float vxmax;
	float vymin;
	float vymax;
	float advance_x;
	float advance_y;
} glyph_slot_t;

struct ft2_font_map_s
{
	Uchar start;
	struct ft2_font_map_s *next;

	rtexture_t *texture;
	glyph_slot_t glyphs[FONT_CHARS_PER_MAP];
};

qboolean Font_LoadMapForIndex(ft2_font_t *font, Uchar _ch, ft2_font_map_t **outmap);

#endif // FT2_PRIVATE_H__
