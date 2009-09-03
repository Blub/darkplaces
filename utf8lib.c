#include "quakedef.h"
#include "utf8lib.h"

/*
================================================================================
UTF-8 encoding and decoding functions follow.
================================================================================
*/

/** Validate that this strings starts with a valid utf8 character.
 * @param _s    An utf-8 encoded null-terminated string.
 * @return
 */
static inline qboolean u8_validate(const char *_s)
{
	const unsigned char *s = (const unsigned char*)_s;
	if (*s < 0x80) // ascii
		return true;
	if (*s < 0xC0) // in-between
		return false;
	if (*s < 0xC2) // overlong encoding, not allowed
		return false;
	if (*s < 0xF5) // valid start of a sequence
		return true;
	// anything else is restricted since RFC 3629, November 2003
	return false;
}

/** Get the number of characters in an UTF-8 string.
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
		if (*s < 0x80)
		{
			++len;
			++s;
			continue;
		}

		// part of a wide character, we ignore that one
		if (*s < 0xC0) // 10111111
		{
			++s;
			continue;
		}

		// start of a wide character
		if (u8_validate((const char*)s))
			++len;
		for (++s; *s >= 0x80 && *s <= 0xC0; ++s);
	}
	return len;
}

/** Get the number of bytes used in a string to represent an amount of characters.
 * @param _s    An utf-8 encoded null-terminated string.
 * @param n     The number of characters we want to know the byte-size for.
 * @return      The number of bytes used to represent n characters.
 */
size_t u8_bytelen(const char *_s, size_t n)
{
	size_t len = 0;
	unsigned char *s = (unsigned char*)_s;
	while (*s && n)
	{
		// ascii char
		if (*s < 0x80)
		{
			++len;
			++s;
			--n;
			continue;
		}

		// part of a wide character, this time we cannot ignore it
		if (*s < 0xC0) // 10111111
		{
			++s;
			++len;
			continue;
		}

		// start of a wide character
		for (++len, ++s; *s >= 0x80 && *s < 0xC0; ++s, ++len);
		--n;
	}
	return len;
}

/** Get the byte-index for a character-index.
 * @param _s      An utf-8 encoded string.
 * @param i       The character-index for which you want the byte offset.
 * @param len     If not null, character's length will be stored in there.
 * @return        The byte-index at which the character begins, or -1 if the string is too short.
 */
int u8_byteofs(const char *_s, size_t i, size_t *len)
{
	size_t ofs = 0;
	unsigned char *s = (unsigned char*)_s;
	while (i > 0 && s[ofs])
	{
		// ascii character
		if (s[ofs] < 0x80)
		{
			++ofs;
			--i;
			continue;
		}

		// part of a wide character, weignore that one
		if (s[ofs] < 0xC0)
		{
			++ofs;
			continue;
		}

		// start of a wide character
		if (u8_validate((const char*)s))
			--i;
		for (++ofs; s[ofs] >= 0x80 && s[ofs] <= 0xC0; ++ofs);
	}
	if (!s[ofs])
		return -1;
	if (len) {
		if (s[ofs] < 0x80)
			*len = 1;
		else if (s[ofs] & 0xC0)
		{
			size_t i;
			for (i = 1; s[ofs+i] >= 0x80 && s[ofs+i] <= 0xC0; ++i);
			*len = i;
		}
		else if (s[ofs] < 0xC0)
			*len = 0;
	}
	return ofs;
}

/** Get the char-index for a byte-index.
 * @param _s      An utf-8 encoded string.
 * @param i       The byte offset for which you want the character index.
 * @param len     If not null, the offset within the character is stored here.
 * @return        The character-index, or -1 if the string is too short.
 */
int u8_charidx(const char *_s, size_t i, size_t *len)
{
	size_t ofs = 0;
	int idx = 0;
	unsigned char *s = (unsigned char*)_s;
	
	while (ofs < i && s[ofs])
	{
		// ascii character
		if (s[ofs] < 0x80)
		{
			++idx;
			++ofs;
			continue;
		}

		// part of a wide character, weignore that one
		if (s[ofs] < 0xC0)
		{
			++ofs;
			continue;
		}

		// start of a wide character
		if (s[ofs] & 0xC0)
		{
			size_t start = ofs;
			if (!u8_validate((const char*)s))
			{
				// invalid byte
				++ofs;
				continue;
			}
			for (++ofs; s[ofs] >= 0x80 && s[ofs] <= 0xC0 && ofs < i; ++ofs);
			if (s[ofs] >= 0x80 && s[ofs] < 0xC0)
			{
				// it ends within this character
				if (len)
					*len = ofs - start;
				return idx;
			}
			++idx;
			continue;
		}
	}
	if (len) *len = 0;
	return idx;
}

/** Fetch a character from an utf-8 encoded string.
 * @param _s      The start of an utf-8 encoded multi-byte character.
 * @param _end    Will point to after the first multi-byte character.
 * @return        The 32-bit integer representation of the first multi-byte character or 0 for invalid characters.
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

	while (!u8_validate((const char*)s))
	{
		// skip invalid characters
		for (++s; *s >= 0x80 && *s < 0xC0; ++s);
		if (!*s)
			return 0;
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

/** uses u8_fromchar on a static buffer
 * @param ch        The unicode character to convert to encode
 * @return          A statically allocated buffer containing the character's utf8 representation, or NULL if it fails.
 */
char *u8_encodech(Uchar ch)
{
	static char buf[16];
	if (u8_fromchar(ch, buf, sizeof(buf)) > 0)
		return buf;
	return NULL;
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
	Uchar ch;
	for (i = 0; *mb && i < maxlen; ++i)
	{
		ch = u8_getchar(mb, &mb);
		if (!ch)
			break;
		*wcs++ = ch;
	}
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
============
UTF-8 aware COM_StringLengthNoColors

calculates the visible width of a color coded string.

*valid is filled with TRUE if the string is a valid colored string (that is, if
it does not end with an unfinished color code). If it gets filled with FALSE, a
fix would be adding a STRING_COLOR_TAG at the end of the string.

valid can be set to NULL if the caller doesn't care.

For size_s, specify the maximum number of characters from s to use, or 0 to use
all characters until the zero terminator.
============
*/
size_t
u8_COM_StringLengthNoColors(const char *s, size_t size_s, qboolean *valid)
{
	const char *end = size_s ? (s + size_s) : NULL;
	size_t len = 0;

	for(;;)
	{
		switch((s == end) ? 0 : *s)
		{
			case 0:
				if(valid)
					*valid = TRUE;
				return len;
			case STRING_COLOR_TAG:
				++s;
				switch((s == end) ? 0 : *s)
				{
					case STRING_COLOR_RGB_TAG_CHAR:
						if (s+1 != end && isxdigit(s[1]) &&
							s+2 != end && isxdigit(s[2]) &&
							s+3 != end && isxdigit(s[3]) )
						{
							s+=3;
							break;
						}
						++len; // STRING_COLOR_TAG
						++len; // STRING_COLOR_RGB_TAG_CHAR
						break;
					case 0: // ends with unfinished color code!
						++len;
						if(valid)
							*valid = FALSE;
						return len;
					case STRING_COLOR_TAG: // escaped ^
						++len;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9': // color code
						break;
					default: // not a color code
						++len; // STRING_COLOR_TAG
						++len; // the character
						break;
				}
				break;
			default:
				++len;
				break;
		}
		
		// start of a wide character
		if (*s & 0xC0)
		{
			for (++s; *s >= 0x80 && *s <= 0xC0; ++s);
			continue;
		}
		// part of a wide character, we ignore that one
		if (*s <= 0xBF)
			--len;
		++s;
	}
	// never get here
}
