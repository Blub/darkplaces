/*
  This contains parts of the original header and thus contains its copyright notice:

  Copyright (c) 2003-2009 uim Project http://code.google.com/p/uim/

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of authors nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/
#ifndef UIM_DP__H__
#define UIM_DP_H__

#include "utf8lib.h"

typedef enum
{
	uim_false = 0,
	uim_true = 1
} uim_bool;

typedef struct uim_context_   *uim_context;
typedef struct uim_candidate_ *uim_candidate;

void UIM_Init(void);
qboolean UIM_Available(void);
qboolean UIM_Direct(void);
void UIM_Key(int key, Uchar unicode);
void UIM_KeyUp(int key, Uchar unicode);
void UIM_KeyDown(int key, Uchar unicode);

// exiting functions have a qUIM_ prefix
typedef void (*qUIM_SetCursor)(int pos);

void UIM_EnterBuffer(char *buffer, size_t bufsize, int pos, qUIM_SetCursor setcursor_cb);
void UIM_CancelBuffer(void);

// from the original uim.h:
enum UKey {
  UKey_0 = 48,
  UKey_1 = 49,
  UKey_2 = 50,
  UKey_3 = 51,
  UKey_4 = 52,
  UKey_5 = 53,
  UKey_6 = 54,
  UKey_7 = 55,
  UKey_8 = 56,
  UKey_9 = 57,
  UKey_Yen = 165,

  UKey_Escape = 256,
  UKey_Tab,
  UKey_Backspace,
  UKey_Delete,
  UKey_Insert,
  UKey_Return,
  UKey_Left,
  UKey_Up ,
  UKey_Right ,
  UKey_Down ,
  UKey_Prior , /* page up */
  UKey_Next , /* page down */
  UKey_Home,
  UKey_End,

  UKey_Multi_key, /* multi-key character compose */
  UKey_Codeinput,
  UKey_SingleCandidate,
  UKey_MultipleCandidate,
  UKey_PreviousCandidate,
  UKey_Mode_switch, /* charcter set switch */

  /* Japanese keyboard */
  UKey_Kanji, /* kanji, kanji convert */
  UKey_Muhenkan, /* cancel conversion */
  UKey_Henkan_Mode, /* start/stop conversion */
  UKey_Henkan = UKey_Henkan_Mode, /* alias for Henkan_Mode */
  UKey_Romaji,
  UKey_Hiragana,
  UKey_Katakana,
  UKey_Hiragana_Katakana, /* hiragana/katakana toggle */
  UKey_Zenkaku,
  UKey_Hankaku,
  UKey_Zenkaku_Hankaku, /* zenkaku/hankaku toggle */
  UKey_Touroku,
  UKey_Massyo,
  UKey_Kana_Lock,
  UKey_Kana_Shift,
  UKey_Eisu_Shift, /* alphanumeric shift */
  UKey_Eisu_toggle, /* alphanumeric toggle */

  /* Korean keyboard */
  UKey_Hangul,
  UKey_Hangul_Start,
  UKey_Hangul_End,
  UKey_Hangul_Hanja,
  UKey_Hangul_Jamo,
  UKey_Hangul_Romaja,
  UKey_Hangul_Codeinput,
  UKey_Hangul_Jeonja,
  UKey_Hangul_Banja,
  UKey_Hangul_PreHanja,
  UKey_Hangul_PostHanja,
  UKey_Hangul_SingleCandidate,
  UKey_Hangul_MultipleCandidate,
  UKey_Hangul_PreviousCandidate,
  UKey_Hangul_Special,

  /* function keys */
  UKey_F1,
  UKey_F2,
  UKey_F3,
  UKey_F4,
  UKey_F5,
  UKey_F6,
  UKey_F7,
  UKey_F8,
  UKey_F9,
  UKey_F10,
  UKey_F11,
  UKey_F12,
  UKey_F13,
  UKey_F14,
  UKey_F15,
  UKey_F16,
  UKey_F17,
  UKey_F18,
  UKey_F19,
  UKey_F20,
  UKey_F21,
  UKey_F22,
  UKey_F23,
  UKey_F24,
  UKey_F25,
  UKey_F26,
  UKey_F27,
  UKey_F28,
  UKey_F29,
  UKey_F30,
  UKey_F31,
  UKey_F32,
  UKey_F33,
  UKey_F34,
  UKey_F35, /* X, Gtk and Qt supports up to F35 */

  /* dead keys */
  UKey_Dead_Grave,
  UKey_Dead_Acute,
  UKey_Dead_Circumflex,
  UKey_Dead_Tilde,
  UKey_Dead_Macron,
  UKey_Dead_Breve,
  UKey_Dead_Abovedot,
  UKey_Dead_Diaeresis,
  UKey_Dead_Abovering,
  UKey_Dead_Doubleacute,
  UKey_Dead_Caron,
  UKey_Dead_Cedilla,
  UKey_Dead_Ogonek,
  UKey_Dead_Iota,
  UKey_Dead_VoicedSound,
  UKey_Dead_SemivoicedSound,
  UKey_Dead_Belowdot,
  UKey_Dead_Hook,
  UKey_Dead_Horn,

  /* Japanese Kana keys */
  UKey_Kana_Fullstop,
  UKey_Kana_OpeningBracket,
  UKey_Kana_ClosingBracket,
  UKey_Kana_Comma,
  UKey_Kana_Conjunctive,
  UKey_Kana_WO,
  UKey_Kana_a,
  UKey_Kana_i,
  UKey_Kana_u,
  UKey_Kana_e,
  UKey_Kana_o,
  UKey_Kana_ya,
  UKey_Kana_yu,
  UKey_Kana_yo,
  UKey_Kana_tsu,
  UKey_Kana_ProlongedSound,
  UKey_Kana_A,
  UKey_Kana_I,
  UKey_Kana_U,
  UKey_Kana_E,
  UKey_Kana_O,
  UKey_Kana_KA,
  UKey_Kana_KI,
  UKey_Kana_KU,
  UKey_Kana_KE,
  UKey_Kana_KO,
  UKey_Kana_SA,
  UKey_Kana_SHI,
  UKey_Kana_SU,
  UKey_Kana_SE,
  UKey_Kana_SO,
  UKey_Kana_TA,
  UKey_Kana_CHI,
  UKey_Kana_TSU,
  UKey_Kana_TE,
  UKey_Kana_TO,
  UKey_Kana_NA,
  UKey_Kana_NI,
  UKey_Kana_NU,
  UKey_Kana_NE,
  UKey_Kana_NO,
  UKey_Kana_HA,
  UKey_Kana_HI,
  UKey_Kana_FU,
  UKey_Kana_HE,
  UKey_Kana_HO,
  UKey_Kana_MA,
  UKey_Kana_MI,
  UKey_Kana_MU,
  UKey_Kana_ME,
  UKey_Kana_MO,
  UKey_Kana_YA,
  UKey_Kana_YU,
  UKey_Kana_YO,
  UKey_Kana_RA,
  UKey_Kana_RI,
  UKey_Kana_RU,
  UKey_Kana_RE,
  UKey_Kana_RO,
  UKey_Kana_WA,
  UKey_Kana_N,
  UKey_Kana_VoicedSound,
  UKey_Kana_SemivoicedSound,

  /* non-standard platform specific keys (e.g. Zaurus PDA) */
  UKey_Private1,
  UKey_Private2,
  UKey_Private3,
  UKey_Private4,
  UKey_Private5,
  UKey_Private6,
  UKey_Private7,
  UKey_Private8,
  UKey_Private9,
  UKey_Private10,
  UKey_Private11,
  UKey_Private12,
  UKey_Private13,
  UKey_Private14,
  UKey_Private15,
  UKey_Private16,
  UKey_Private17,
  UKey_Private18,
  UKey_Private19,
  UKey_Private20,
  UKey_Private21,
  UKey_Private22,
  UKey_Private23,
  UKey_Private24,
  UKey_Private25,
  UKey_Private26,
  UKey_Private27,
  UKey_Private28,
  UKey_Private29,
  UKey_Private30,

  /* modifier keys */
  UKey_Shift = 0x8000,
  UKey_Control,
  UKey_Alt,
  UKey_Meta,
  UKey_Super,
  UKey_Hyper,

  /* lock modifier keys: unstable */
  UKey_Caps_Lock = 0x9000,
  UKey_Num_Lock,
  UKey_Scroll_Lock,

  UKey_Other = 0x10000
};

enum UKeyModifier {
  UMod_Shift = 1,
  UMod_Control = 2,
  UMod_Alt = 4,
  UMod_Meta = 8,
  UMod_Pseudo0 = 16,
  UMod_Pseudo1 = 32,
  UMod_Super = 64,
  UMod_Hyper = 128
};

enum UPreeditAttr {
  UPreeditAttr_None = 0,
  UPreeditAttr_UnderLine = 1,
  UPreeditAttr_Reverse = 2,
  UPreeditAttr_Cursor = 4,
  UPreeditAttr_Separator = 8
};

/* Cursor of clipboard text is always positioned at end. */
enum UTextArea {
  UTextArea_Unspecified = 0,
  UTextArea_Primary     = 1,  /* primary text area which IM commits to */
  UTextArea_Selection   = 2,  /* user-selected region of primary text area */
  UTextArea_Clipboard   = 4   /* clipboard text */
};

enum UTextOrigin {
  UTextOrigin_Unspecified = 0,
  UTextOrigin_Cursor      = 1,  /* current position of the cursor */
  UTextOrigin_Beginning   = 2,  /* beginning of the text */
  UTextOrigin_End         = 3   /* end of the text */
};

enum UTextExtent {
  UTextExtent_Unspecified = -1,  /* invalid */

  /* logical extents */
  UTextExtent_Full      = -2,   /* beginning or end of the whole text */
  UTextExtent_Paragraph = -3,   /* the paragraph which the origin is included */
  UTextExtent_Sentence  = -5,   /* the sentence which the origin is included */
  UTextExtent_Word      = -9,   /* the word which the origin is included */
  UTextExtent_CharFrags = -17,  /* character fragments around the origin */

  /* physical extents */
  UTextExtent_DispRect  = -33,  /* the text region displayed in the widget */
  UTextExtent_DispLine  = -65,  /* displayed line (eol: linebreak) */
  UTextExtent_Line      = -129  /* real line      (eol: newline char) */
};

#endif // UIM_DPH_H__
