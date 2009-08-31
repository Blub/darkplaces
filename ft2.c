/* FreeType 2 and UTF-8 encoding support for
 * DarkPlaces
 */
#include "quakedef.h"

#include "ft2.h"

#ifdef _MSC_VER
typedef __int32 FT_Int32;
#else
typedef int32_t FT_Int32;
#endif

typedef int FT_Error;

typedef signed char FT_Char;
typedef unsigned char FT_Byte;
typedef const FT_Byte *FT_Bytes;
typedef char FT_String;
typedef signed int FT_Int;
typedef unsigned int FT_UInt;
typedef signed long FT_Long;
typedef unsigned long FT_ULong;
typedef void *FT_Pointer;
typedef size_t FT_Offset;
typedef signed long FT_F26Dot6;

typedef void *FT_Stream;
typedef void *FT_Module;
typedef void *FT_Library;
typedef void *FT_Face;

typedef void *FT_GlyphSlot;

// Taken from the freetype headers:
typedef signed long FT_Pos;
typedef struct FT_Vector_
{
	FT_Pos x;
	FT_Pos y;
} FT_Vector;

typedef struct  FT_BBox_
{
	FT_Pos  xMin, yMin;
	FT_Pos  xMax, yMax;
} FT_BBox;

typedef enum  FT_Pixel_Mode_
{
	FT_PIXEL_MODE_NONE = 0,
	FT_PIXEL_MODE_MONO,
	FT_PIXEL_MODE_GRAY,
	FT_PIXEL_MODE_GRAY2,
	FT_PIXEL_MODE_GRAY4,
	FT_PIXEL_MODE_LCD,
	FT_PIXEL_MODE_LCD_V,
	FT_PIXEL_MODE_MAX      /* do not remove */
} FT_Pixel_Mode;
typedef enum  FT_Render_Mode_
{
	FT_RENDER_MODE_NORMAL = 0,
	FT_RENDER_MODE_LIGHT,
	FT_RENDER_MODE_MONO,
	FT_RENDER_MODE_LCD,
	FT_RENDER_MODE_LCD_V,

	FT_RENDER_MODE_MAX
} FT_Render_Mode;

#define ft_pixel_mode_none   FT_PIXEL_MODE_NONE
#define ft_pixel_mode_mono   FT_PIXEL_MODE_MONO
#define ft_pixel_mode_grays  FT_PIXEL_MODE_GRAY
#define ft_pixel_mode_pal2   FT_PIXEL_MODE_GRAY2
#define ft_pixel_mode_pal4   FT_PIXEL_MODE_GRAY4

typedef struct  FT_Bitmap_
{
	int             rows;
	int             width;
	int             pitch;
	unsigned char*  buffer;
	short           num_grays;
	char            pixel_mode;
	char            palette_mode;
	void*           palette;
} FT_Bitmap;

typedef struct  FT_Outline_
{
	short       n_contours;      /* number of contours in glyph        */
	short       n_points;        /* number of points in the glyph      */

	FT_Vector*  points;          /* the outline's points               */
	char*       tags;            /* the points flags                   */
	short*      contours;        /* the contour end points             */

	int         flags;           /* outline masks                      */
} FT_Outline;

#define FT_OUTLINE_NONE             0x0
#define FT_OUTLINE_OWNER            0x1
#define FT_OUTLINE_EVEN_ODD_FILL    0x2
#define FT_OUTLINE_REVERSE_FILL     0x4
#define FT_OUTLINE_IGNORE_DROPOUTS  0x8
#define FT_OUTLINE_SMART_DROPOUTS   0x10
#define FT_OUTLINE_INCLUDE_STUBS    0x20

#define FT_OUTLINE_HIGH_PRECISION   0x100
#define FT_OUTLINE_SINGLE_PASS      0x200

#define ft_outline_none             FT_OUTLINE_NONE
#define ft_outline_owner            FT_OUTLINE_OWNER
#define ft_outline_even_odd_fill    FT_OUTLINE_EVEN_ODD_FILL
#define ft_outline_reverse_fill     FT_OUTLINE_REVERSE_FILL
#define ft_outline_ignore_dropouts  FT_OUTLINE_IGNORE_DROPOUTS
#define ft_outline_high_precision   FT_OUTLINE_HIGH_PRECISION
#define ft_outline_single_pass      FT_OUTLINE_SINGLE_PASS

#define FT_CURVE_TAG( flag )  ( flag & 3 )

#define FT_CURVE_TAG_ON           1
#define FT_CURVE_TAG_CONIC        0
#define FT_CURVE_TAG_CUBIC        2

#define FT_CURVE_TAG_TOUCH_X      8  /* reserved for the TrueType hinter */
#define FT_CURVE_TAG_TOUCH_Y     16  /* reserved for the TrueType hinter */

#define FT_CURVE_TAG_TOUCH_BOTH  ( FT_CURVE_TAG_TOUCH_X | \
                                   FT_CURVE_TAG_TOUCH_Y )

#define FT_Curve_Tag_On       FT_CURVE_TAG_ON
#define FT_Curve_Tag_Conic    FT_CURVE_TAG_CONIC
#define FT_Curve_Tag_Cubic    FT_CURVE_TAG_CUBIC
#define FT_Curve_Tag_Touch_X  FT_CURVE_TAG_TOUCH_X
#define FT_Curve_Tag_Touch_Y  FT_CURVE_TAG_TOUCH_Y

typedef int
(*FT_Outline_MoveToFunc)( const FT_Vector*  to,
			  void*             user );
#define FT_Outline_MoveTo_Func  FT_Outline_MoveToFunc

typedef int
(*FT_Outline_LineToFunc)( const FT_Vector*  to,
			  void*             user );
#define FT_Outline_LineTo_Func  FT_Outline_LineToFunc

typedef int
(*FT_Outline_ConicToFunc)( const FT_Vector*  control,
			   const FT_Vector*  to,
			   void*             user );
#define FT_Outline_ConicTo_Func  FT_Outline_ConicToFunc

typedef int
(*FT_Outline_CubicToFunc)( const FT_Vector*  control1,
			   const FT_Vector*  control2,
			   const FT_Vector*  to,
			   void*             user );
#define FT_Outline_CubicTo_Func  FT_Outline_CubicToFunc

typedef struct  FT_Outline_Funcs_
{
	FT_Outline_MoveToFunc   move_to;
	FT_Outline_LineToFunc   line_to;
	FT_Outline_ConicToFunc  conic_to;
	FT_Outline_CubicToFunc  cubic_to;

	int                     shift;
	FT_Pos                  delta;
} FT_Outline_Funcs;

#ifndef FT_IMAGE_TAG
#define FT_IMAGE_TAG( value, _x1, _x2, _x3, _x4 )  \
          value = ( ( (unsigned long)_x1 << 24 ) | \
                    ( (unsigned long)_x2 << 16 ) | \
                    ( (unsigned long)_x3 << 8  ) | \
                      (unsigned long)_x4         )
#endif /* FT_IMAGE_TAG */

typedef enum  FT_Glyph_Format_
{
	FT_IMAGE_TAG( FT_GLYPH_FORMAT_NONE, 0, 0, 0, 0 ),

	FT_IMAGE_TAG( FT_GLYPH_FORMAT_COMPOSITE, 'c', 'o', 'm', 'p' ),
	FT_IMAGE_TAG( FT_GLYPH_FORMAT_BITMAP,    'b', 'i', 't', 's' ),
	FT_IMAGE_TAG( FT_GLYPH_FORMAT_OUTLINE,   'o', 'u', 't', 'l' ),
	FT_IMAGE_TAG( FT_GLYPH_FORMAT_PLOTTER,   'p', 'l', 'o', 't' )
} FT_Glyph_Format;
#define ft_glyph_format_none       FT_GLYPH_FORMAT_NONE
#define ft_glyph_format_composite  FT_GLYPH_FORMAT_COMPOSITE
#define ft_glyph_format_bitmap     FT_GLYPH_FORMAT_BITMAP
#define ft_glyph_format_outline    FT_GLYPH_FORMAT_OUTLINE
#define ft_glyph_format_plotter    FT_GLYPH_FORMAT_PLOTTER

typedef struct  FT_Glyph_Metrics_
{
	FT_Pos  width;
	FT_Pos  height;

	FT_Pos  horiBearingX;
	FT_Pos  horiBearingY;
	FT_Pos  horiAdvance;

	FT_Pos  vertBearingX;
	FT_Pos  vertBearingY;
	FT_Pos  vertAdvance;
} FT_Glyph_Metrics;

#define FT_EXPORT( x )  x

#define FT_OPEN_MEMORY    0x1
#define FT_OPEN_STREAM    0x2
#define FT_OPEN_PATHNAME  0x4
#define FT_OPEN_DRIVER    0x8
#define FT_OPEN_PARAMS    0x10

#define ft_open_memory    FT_OPEN_MEMORY     /* deprecated */
#define ft_open_stream    FT_OPEN_STREAM     /* deprecated */
#define ft_open_pathname  FT_OPEN_PATHNAME   /* deprecated */
#define ft_open_driver    FT_OPEN_DRIVER     /* deprecated */
#define ft_open_params    FT_OPEN_PARAMS     /* deprecated */

typedef struct  FT_Parameter_
{
	FT_ULong    tag;
	FT_Pointer  data;
} FT_Parameter;

typedef struct  FT_Open_Args_
{
	FT_UInt         flags;
	const FT_Byte*  memory_base;
	FT_Long         memory_size;
	FT_String*      pathname;
	FT_Stream       stream;
	FT_Module       driver;
	FT_Int          num_params;
	FT_Parameter*   params;
} FT_Open_Args;
typedef enum  FT_Size_Request_Type_
{
	FT_SIZE_REQUEST_TYPE_NOMINAL,
	FT_SIZE_REQUEST_TYPE_REAL_DIM,
	FT_SIZE_REQUEST_TYPE_BBOX,
	FT_SIZE_REQUEST_TYPE_CELL,
	FT_SIZE_REQUEST_TYPE_SCALES,

	FT_SIZE_REQUEST_TYPE_MAX

} FT_Size_Request_Type;
typedef struct  FT_Size_RequestRec_
{
	FT_Size_Request_Type  type;
	FT_Long               width;
	FT_Long               height;
	FT_UInt               horiResolution;
	FT_UInt               vertResolution;
} FT_Size_RequestRec;
typedef struct FT_Size_RequestRec_  *FT_Size_Request;

#define FT_LOAD_DEFAULT                      0x0
#define FT_LOAD_NO_SCALE                     0x1
#define FT_LOAD_NO_HINTING                   0x2
#define FT_LOAD_RENDER                       0x4
#define FT_LOAD_NO_BITMAP                    0x8
#define FT_LOAD_VERTICAL_LAYOUT              0x10
#define FT_LOAD_FORCE_AUTOHINT               0x20
#define FT_LOAD_CROP_BITMAP                  0x40
#define FT_LOAD_PEDANTIC                     0x80
#define FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH  0x200
#define FT_LOAD_NO_RECURSE                   0x400
#define FT_LOAD_IGNORE_TRANSFORM             0x800
#define FT_LOAD_MONOCHROME                   0x1000
#define FT_LOAD_LINEAR_DESIGN                0x2000
#define FT_LOAD_NO_AUTOHINT                  0x8000U

#define FT_LOAD_TARGET_( x )   ( (FT_Int32)( (x) & 15 ) << 16 )

#define FT_LOAD_TARGET_NORMAL  FT_LOAD_TARGET_( FT_RENDER_MODE_NORMAL )
#define FT_LOAD_TARGET_LIGHT   FT_LOAD_TARGET_( FT_RENDER_MODE_LIGHT  )
#define FT_LOAD_TARGET_MONO    FT_LOAD_TARGET_( FT_RENDER_MODE_MONO   )
#define FT_LOAD_TARGET_LCD     FT_LOAD_TARGET_( FT_RENDER_MODE_LCD    )
#define FT_LOAD_TARGET_LCD_V   FT_LOAD_TARGET_( FT_RENDER_MODE_LCD_V  )

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
(*FT_Request_Size)( FT_Face          face,
		    FT_Size_Request  req );
FT_EXPORT( FT_Error )
(*FT_Set_Char_Size)( FT_Face     face,
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
