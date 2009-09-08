/* UIM input support for
 * DarkPlaces
 */
#include "quakedef.h"

#include "keys.h"
#include "uim_dp.h"

//#include <locale.h>

/*
================================================================================
CVars introduced with the UIM extension
================================================================================
*/

cvar_t im_enabled = {CVAR_SAVE, "im_enabled", "1", "use UIM input"};
cvar_t im_engine = {CVAR_SAVE, "im_engine", "anthy", "which input method to use if uim is supported and active"};
cvar_t im_language = {CVAR_SAVE, "im_language", "ja", "which language should be used for the input editor (en for english, ja for japanese, etc.)"};

cvar_t im_cursor = {CVAR_SAVE, "im_cursor", "", "the cursor to append to what you type when not selecting candidates - you don't want this for the chatbox"};
cvar_t im_cursor_start = {CVAR_SAVE, "im_cursor_start", "^3[^x888", "how to mark the beginning of the input cursor"};
cvar_t im_cursor_end = {CVAR_SAVE, "im_cursor_end", "^3]", "how to mark the end of the input cursor"};
cvar_t im_selection_start = {CVAR_SAVE, "im_selection_start", "^xf80", "how to mark the beginning of the current selection (as in, what UIM would display underlined)"};
cvar_t im_selection_end = {CVAR_SAVE, "im_selection_end", "", "how to mark the end of the current selection (as in, what UIM would display underlined)"};
cvar_t im_separator = {CVAR_SAVE, "im_separator", "|", "separator to use..."};

/*
================================================================================
Library imports. Taken from the uim headers.
================================================================================
*/
struct uim_code_converter **quim_iconv;

int           (*quim_init)(void);
void          (*quim_quit)(void);
uim_candidate (*quim_get_candidate)(uim_context, int index, int accel_enum_hint);
void          (*quim_candidate_free)(uim_candidate);
int           (*quim_get_candidate_index)(uim_context);
const char*   (*quim_candidate_get_cand_str)(uim_candidate);
const char*   (*quim_get_default_im_name)(const char *localename);
uim_context   (*quim_create_context)(void *cookie,
				     const char *encoding,
				     const char *lang,
				     const char *engine,
				     struct uim_code_converter *conv,
				     void (*commit_cb)(void *ptr, const char *str));
void          (*quim_reset_context)(uim_context);
void          (*quim_release_context)(uim_context);
int           (*quim_helper_init_client_fd)(void (*disconnect_cb)(void));
void          (*quim_helper_close_client_fd)(int);
void          (*quim_helper_send_message)(int, const char*);
void          (*quim_set_preedit_cb)(uim_context,
				     void (*clear_cb)(void*),
				     void (*pushback_cb)(void*, int attr, const char *str),
				     void (*update_cb)(void*));
void          (*quim_set_candidate_selector_cb)(uim_context,
						void (*activate_cb)(void*, int nr, int display_limit),
						void (*select_cb)(void*, int index),
						void (*shift_page_cb)(void*, int direction),
						void (*deactivate_cb)(void*));
void          (*quim_prop_list_update)(uim_context);
int           (*quim_press_key)(uim_context, int key, int mods);
int           (*quim_release_key)(uim_context, int key, int mods);
void          (*quim_set_prop_list_update_cb)(uim_context, void (*update_cb)(void*, const char *str));
void          (*quim_set_configuration_changed_cb)(uim_context, void (*changed_cb)(void*));

/*
================================================================================
Support for dynamically loading the UIM library
================================================================================
*/

static dllfunction_t uimfuncs[] =
{
	{"uim_init",			(void **) &quim_init},
	{"uim_quit",			(void **) &quim_quit},
	{"uim_get_candidate",		(void **) &quim_get_candidate},
	{"uim_candidate_free",		(void **) &quim_candidate_free},
	{"uim_get_candidate_index",	(void **) &quim_get_candidate_index},
	{"uim_candidate_get_cand_str",	(void **) &quim_candidate_get_cand_str},
	{"uim_get_default_im_name",	(void **) &quim_get_default_im_name},
	{"uim_create_context",		(void **) &quim_create_context},
	{"uim_reset_context",		(void **) &quim_reset_context},
	{"uim_release_context",		(void **) &quim_release_context},
	{"uim_helper_init_client_fd",	(void **) &quim_helper_init_client_fd},
	{"uim_helper_close_client_fd",	(void **) &quim_helper_close_client_fd},
	{"uim_helper_send_message",	(void **) &quim_helper_send_message},
	{"uim_set_preedit_cb",		(void **) &quim_set_preedit_cb},
	{"uim_set_candidate_selector_cb",		(void **) &quim_set_candidate_selector_cb},
	{"uim_prop_list_update",	(void **) &quim_prop_list_update},
	{"uim_press_key",		(void **) &quim_press_key},
	{"uim_release_key",		(void **) &quim_release_key},
	{"uim_iconv",			(void **) &quim_iconv},
	{"uim_set_prop_list_update_cb", (void **) &quim_set_prop_list_update_cb},
	{"uim_set_configuration_changed_cb", (void **) &quim_set_configuration_changed_cb},

	{NULL, NULL}
};

/// Handle for UIM dll
static dllhandle_t uim_dll = NULL;

/*
====================
UIM_CloseLibrary

Unload the UIM DLL
====================
*/
void UIM_CloseLibrary (void)
{
	Sys_UnloadLibrary (&uim_dll);
}

/*
====================
UIM_OpenLibrary

Try to load the UIM DLL
====================
*/
qboolean UIM_OpenLibrary (void)
{
	const char* dllnames [] =
	{
#if defined(WIN64)
		#error path for freetype 2 dll
#elif defined(WIN32)
		#error path for freetype 2 dll
#elif defined(MACOSX)
		"libuim.dylib",
#else
		"libuim.so.6",
		"libuim.so",
#endif
		NULL
	};

	// Already loaded?
	if (uim_dll)
		return true;

	// Load the DLL
	if (!Sys_LoadLibrary (dllnames, &uim_dll, uimfuncs))
		return false;
	return true;
}

/*
================================================================================
UIM input method implementation.
================================================================================
*/

typedef struct
{
	uim_context    ctx;
	int            fd;
	//mempool_t     *mempool;

	char          *buffer;
	size_t         buffer_pos;
	size_t         buffer_size;
	size_t         length;
	size_t         edit_start;
	size_t         edit_pos;
	size_t         edit_length;
	size_t         tail_length;
	size_t         cursor_pos;
	size_t         cursor_length;
	size_t         cursor_inpos;
	size_t         cursor_inlength;

	// where can we find the previous color code?
	size_t         pushcount;
	const char    *pc;
	size_t         pc_len;

	qboolean       active;
	int            active_count;
	int            active_limit;

	/*
	char          *copybuffer;
	size_t         copylen;
	size_t         copypos;
	*/

	qUIM_SetCursor setcursor;

	// this is the only way K_RETURN actually sends a chat message
	// when using stuff like anthy
	int            actions;
	int            pushed;
} quim_state;

static quim_state quim;

static void UIM_PropListUpdate(void*, const char *str);
static void UIM_Commit(void*, const char *str);
static void UIM_HelperDisconnected(void);
static void UIM_Clear(void*);
static void UIM_Push(void*, int attr, const char *str);
static void UIM_Update(void*);
static void UIM_Activate(void*, int nr, int display_limit);
static void UIM_Select(void*, int index);
static void UIM_Shift(void*, int dir);
static void UIM_Deactivate(void*);
static void UIM_ConfigChanged(void*);
static void UIM_Restart_f(void);

/*
====================
UIM_Init

Load UIM support and register commands / cvars
====================
*/
void UIM_Start(void);
static qboolean UIM_InitConverter(void);
void UIM_Init(void)
{
	// register the cvars in any case so they're at least saved,
	// after all, they're for the user

	Cvar_RegisterVariable(&im_engine);
	Cvar_RegisterVariable(&im_language);
	Cvar_RegisterVariable(&im_enabled);
	Cvar_RegisterVariable(&im_cursor);
	Cvar_RegisterVariable(&im_cursor_start);
	Cvar_RegisterVariable(&im_cursor_end);
	Cvar_RegisterVariable(&im_selection_start);
	Cvar_RegisterVariable(&im_selection_end);
	Cvar_RegisterVariable(&im_separator);
	Cmd_AddCommand ("im_restart", UIM_Restart_f, "restart the input editor");

	//quim.mempool = Mem_AllocPool("UIM", 0, NULL);
	//UIM_Start();
}

static void UIM_Restart_f(void)
{
	UIM_Shutdown();
	UIM_Start();
}

static struct uim_code_converter *dp_converter;

static qboolean UIM_InitConverter(void)
{
	/*
	dp_converter = &dp_uim_conv;
	dp_uim_conv.is_convertible = &conv_IsConvertible;
	dp_uim_conv.create = &conv_Create;
	dp_uim_conv.convert = &conv_Convert;
	dp_uim_conv.release = &conv_Release;
	*/
	dp_converter = *quim_iconv;
	return true;
}

/*
====================
UIM_Shutdown

Unload all UIM resources
====================
*/
void UIM_Shutdown(void)
{
	if (UIM_Available())
	{
		int fd = quim.fd;
		quim.fd = 0;
		UIM_CancelBuffer();
		quim_release_context(quim.ctx);
		quim_helper_close_client_fd(fd);
		quim.ctx = NULL;
	}
	//if (quim.mempool)
	//	Mem_FreePool(&quim.mempool);
}

/*
====================
UIM_Start

Try starting up UIM, this should be made available as console command maybe?
====================
*/
void UIM_Start(void)
{
	if (!UIM_OpenLibrary())
		return;

	if (!UIM_InitConverter())
		return;

	//setlocale(LC_CTYPE, NULL);
	if (quim_init() != 0)
	{
		Con_Print("Failed to initialize UIM support\n");
		UIM_Shutdown();
		return;
	}

	quim.ctx = quim_create_context(NULL, "UTF-8", im_language.string, im_engine.string, dp_converter, &UIM_Commit);
	if (quim.ctx == NULL)
	{
		Con_Print("Failed to create UIM context\n");
		UIM_Shutdown();
		return;
	}

	quim.fd = quim_helper_init_client_fd(&UIM_HelperDisconnected);
	if (quim.fd < 2) // well, in and out aren't exactly ... good
	{
		Con_Print("Failed to initialize UIM helper client fd\n");
		UIM_Shutdown();
		return;
	}
	Con_Print("UIM Helper connected\n");

	quim_set_preedit_cb(quim.ctx, &UIM_Clear, &UIM_Push, &UIM_Update);
	quim_set_candidate_selector_cb(quim.ctx, &UIM_Activate, &UIM_Select, &UIM_Shift, &UIM_Deactivate);
	quim_set_prop_list_update_cb(quim.ctx, &UIM_PropListUpdate);
	quim_set_configuration_changed_cb(quim.ctx, &UIM_ConfigChanged);
	quim_prop_list_update(quim.ctx);
}

// api entry, must check for UIM availability *ahem*
qboolean UIM_Available(void)
{
	if (!im_enabled.integer)
		return false;
	return (!!uim_dll && quim.ctx);
}

// api entry, must check for UIM availability
qboolean UIM_Direct(void)
{
	if (!uim_dll || !quim.buffer || !quim.ctx || quim.fd <= 0 || !quim.buffer_size)
		return true;
	// FIXME: direct!
	return false;
}

static int UIM_GetKeyMod(void)
{
	int mod = 0;
	if (Key_IsPressed(K_SHIFT)) mod |= UMod_Shift;
	if (Key_IsPressed(K_ALT))   mod |= UMod_Alt;
	if (Key_IsPressed(K_CTRL))  mod |= UMod_Control;
	if (Key_IsPressed(K_SUPER)) mod |= UMod_Super;
	return mod;
}

static int UIM_KeyToUKey(int key, Uchar unicode)
{
	int ofs = 0;
	switch (key)
	{
	case K_TAB:
		return UKey_Tab;
	case K_ENTER:
		return UKey_Return;
	case K_ESCAPE:
		return UKey_Escape;
	case K_SPACE:
		return ' ';

	case K_BACKSPACE:
		return UKey_Backspace;
	case K_UPARROW:
		return UKey_Up;
	case K_DOWNARROW:
		return UKey_Down;
	case K_LEFTARROW:
		return UKey_Left;
	case K_RIGHTARROW:
		return UKey_Right;

	case K_ALT:
		return UKey_Alt;
	case K_CTRL:
		return UKey_Control;
	case K_SHIFT:
		return UKey_Shift;

	case K_F1: ++ofs;
	case K_F2: ++ofs;
	case K_F3: ++ofs;
	case K_F4: ++ofs;
	case K_F5: ++ofs;
	case K_F6: ++ofs;
	case K_F7: ++ofs;
	case K_F8: ++ofs;
	case K_F9: ++ofs;
	case K_F10: ++ofs;
	case K_F11: ++ofs;
	case K_F12: ++ofs;
		return UKey_F1 + ofs;

	case K_INS:
		return UKey_Insert;
	case K_DEL:
	case K_KP_DEL:
		return UKey_Delete;
	case K_PGUP:
		//case K_KP_PGUP:
		return UKey_Next;
	case K_PGDN:
		//case K_KP_PGDN:
		return UKey_Prior;
	case K_HOME:
		return UKey_Home;
	case K_END:
		return UKey_End;

	case K_NUMLOCK:
		return UKey_Num_Lock;
	case K_CAPSLOCK:
		return UKey_Caps_Lock;
	case K_SCROLLOCK:
		return UKey_Scroll_Lock;

	case K_KP_0:
		return '0';
	case K_KP_1:
		return '1';
	case K_KP_2:
		return '2';
	case K_KP_3:
		return '3';
	case K_KP_4:
		return '4';
	case K_KP_5:
		return '5';
	case K_KP_6:
		return '6';
	case K_KP_7:
		return '7';
	case K_KP_8:
		return '8';
	case K_KP_9:
		return '9';

	default:
		if (unicode < 0x7E)
			return unicode;
		return 0;
	}
}

// api entry, must check for UIM availability
qboolean UIM_KeyUp(int key, Uchar unicode)
{
	if (!UIM_Available())
		return false;
	quim.actions = 0;
	return (quim_release_key(quim.ctx, UIM_KeyToUKey(key, unicode), UIM_GetKeyMod()) == 0) && !!quim.actions;
}

// api entry, must check for UIM availability
qboolean UIM_KeyDown(int key, Uchar unicode)
{
	if (!UIM_Available())
		return false;
	quim.actions = 0;
	return (quim_press_key(quim.ctx, UIM_KeyToUKey(key, unicode), UIM_GetKeyMod()) == 0) && !!quim.actions;
}

// api entry, must check for UIM availability
qboolean UIM_Key(int key, Uchar unicode)
{
	qboolean handled = false;
	int mod, ukey;
	if (!UIM_Available())
		return false;
	quim.actions = 0;
	mod = UIM_GetKeyMod();
	ukey = UIM_KeyToUKey(key, unicode);
	//Con_Printf("uim handling key: %i (mod: %i) char: %c\n", ukey, mod, (ukey >= 32 && ukey < 0x7F) ? ukey : '.');
	if (quim_press_key(quim.ctx, ukey, mod) == 0)
		handled = true;
	if (quim_release_key(quim.ctx, ukey, mod) == 0)
		handled = true;
	return handled && !!quim.actions;
}

// api entry, must check for UIM availability
static char defcolor[3];
qboolean UIM_EnterBuffer(char *buffer, size_t bufsize, size_t pos, qUIM_SetCursor setcursor_cb)
{
	if (!UIM_Available())
		return false;
	if (quim.buffer)
		UIM_CancelBuffer();
	if (!buffer)
		return false;

	quim.actions = 0;
	quim.pushcount = 0;
	quim.active = false;
	quim.buffer = buffer;
	quim.buffer_size = bufsize;
	quim.buffer_pos = pos;
	quim.edit_start = pos;
	quim.edit_pos = pos;
	quim.edit_length = 0;
	quim.length = strlen(buffer);
	quim.tail_length = quim.length - quim.edit_start;
	
	quim.setcursor = setcursor_cb;

	defcolor[0] = STRING_COLOR_TAG;
	defcolor[1] = '7';
	defcolor[2] = 0;
	quim.pc = defcolor;
	quim.pc_len = 2;

	// search for the previous color code
	if (pos >= 2)
	{
		do
		{
			--pos;
			if (pos >= 1 && buffer[pos-1] == STRING_COLOR_TAG && isdigit(buffer[pos]))
			{
				quim.pc = &buffer[pos-1];
				quim.pc_len = 2;
				break;
			}
			else if (pos >= 4 &&
				 buffer[pos-4] == STRING_COLOR_TAG &&
				 buffer[pos-3] == STRING_COLOR_RGB_TAG_CHAR &&
				 isxdigit(buffer[pos-2]) &&
				 isxdigit(buffer[pos-1]) &&
				 isxdigit(buffer[pos]))
			{
				quim.pc = &buffer[pos-4];
				quim.pc_len = 5;
				break;
			}
		} while(pos);
	}
	return true;
}

// api entry, must check for UIM availability
void UIM_CancelBuffer(void)
{
	if (!UIM_Available())
		return;
	quim_reset_context(quim.ctx);
	quim.buffer = NULL;
	quim.setcursor = NULL;
	quim.pc = defcolor;
	quim.pc_len = 2;
}

// api entry, must check for UIM availability
void UIM_SetCursor(int pos)
{
	UIM_EnterBuffer(quim.buffer, quim.buffer_size, pos, quim.setcursor);
}

static qboolean UIM_Insert2(const char *str, size_t _slen)
{
	size_t slen;
	if (!*str)
		return true;
	if (_slen)
		slen = _slen;
	else
		slen = strlen(str);
	if (quim.edit_pos + slen + quim.tail_length >= quim.buffer_size - 1) {
		Con_Print("UIM: Insertion failed: not enough space!\n");
		return false;
	}
	//Con_Printf("Inserting [%s]\n", str);
	//Con_Printf("Insertion point: %lu\n", (unsigned long)quim.edit_pos);
	memmove(quim.buffer + quim.edit_pos + slen,
		quim.buffer + quim.edit_pos,
		quim.buffer_size - quim.edit_pos - slen + 1);
	memcpy(quim.buffer + quim.edit_pos, str, slen);
	if (quim.edit_pos >= quim.length)
		quim.buffer[quim.edit_pos + slen] = 0;
	quim.edit_pos += slen;
	quim.edit_length += slen;
	return true;
}

static inline qboolean UIM_Insert(const char *str)
{
	return UIM_Insert2(str, 0);
}

static void UIM_Commit(void *cookie, const char *str)
{
	++quim.actions;
	quim.pushed = 0;
	//Con_Printf("UIM_Commit: %s\n", str);
	UIM_Clear(cookie);
	if (!UIM_Insert(str))
	{
		Con_Print("UIM: Failed to commit buffer\n");
		return;
	}
	quim.buffer_pos = quim.edit_pos;
	quim.edit_start = quim.edit_pos;
	quim.edit_length = 0;
	quim.pushcount = 0;
	if (quim.setcursor)
		quim.setcursor(quim.buffer_pos);
}

static void UIM_HelperDisconnected(void)
{
	++quim.actions;
	//Con_Print("UIM Helper disconnected\n");
	if (quim.fd > 0)
		UIM_Shutdown();
}

static void UIM_Clear(void *cookie)
{
	++quim.actions;
	quim.pushed = 0;
	//Con_Print("UIM_Clear\n");
	memmove(quim.buffer + quim.buffer_pos,
		quim.buffer + quim.edit_pos,
		quim.buffer_size - quim.edit_pos + 1);
	quim.edit_pos = quim.buffer_pos;
	quim.edit_length = 0;
	quim.cursor_pos = quim.edit_pos;
	quim.cursor_length = 0;
	quim.cursor_inpos = quim.edit_pos;
	quim.cursor_inlength = 0;
	quim.pushcount = 0;
	if (quim.setcursor)
		quim.setcursor(quim.edit_pos);
}

// we have BUFSTART and QUIM_CURSOR
static void UIM_Push(void *cookie, int attr, const char *str)
{
	++quim.actions;
	++quim.pushed;
	//Con_Printf("UIM_Push: (%i) %s\n", attr, str);
	if ((attr & (UPreeditAttr_Cursor | UPreeditAttr_Reverse)) == (UPreeditAttr_Cursor | UPreeditAttr_Reverse))
	{
		quim.cursor_pos = quim.edit_pos;
		quim.cursor_length = 0;
		quim.cursor_inpos = quim.edit_pos;
		quim.cursor_inlength = 0;
		if (!UIM_Insert(im_cursor_start.string))
			return;
		quim.cursor_inpos = quim.edit_pos;
		quim.cursor_length = strlen(im_cursor_start.string);
	}
	if (attr & UPreeditAttr_UnderLine)
	{
		if (!UIM_Insert(im_selection_start.string))
			return;
	}
	if (str[0])
	{
		if (!UIM_Insert(str))
			return;
	}
	if (attr & UPreeditAttr_UnderLine)
	{
		if (!UIM_Insert(im_selection_end.string))
			return;
	}
	if (attr & UPreeditAttr_Cursor)
	{
		if (attr & UPreeditAttr_Reverse)
		{
			size_t slen = strlen(str);
			size_t cl = quim.cursor_length;
			quim.cursor_length += slen;
			quim.cursor_inlength += slen;
			if (!UIM_Insert(im_cursor_end.string))
				return;
			quim.cursor_length += cl;
		}
		else
		{
			/*
			size_t epos = quim.edit_pos;
			size_t elen = quim.edit_length;
			*/
			if (!UIM_Insert(im_cursor.string))
				return;
			/*
			quim.edit_pos = epos;
			quim.edit_length = elen;
			*/
		}
	}
}

static void UIM_Update(void *cookie)
{
	++quim.actions;
	if (quim.pushed) // do not append color to commits, only to pushed objects
		UIM_Insert2(quim.pc, quim.pc_len);
	if (quim.setcursor)
		quim.setcursor(quim.edit_pos);
	//Con_Print("UIM_Update\n");
	// well, of course
	// we could allocate a buffer in which we work
	// and only update on "update"
	// but who cares, seriously?
}

static void UIM_Activate(void *cookie, int nr, int display_limit)
{
	++quim.actions;
	//Con_Print("UIM_Activate\n");
	quim.active = true;
	quim.active_count = nr;
	quim.active_limit = display_limit;
}

static void UIM_Select(void *cookie, int index)
{
	uim_candidate cd;
	const char *str;
	size_t slen;
	++quim.actions;

	//Con_Print("UIM_Select\n");
	cd = quim_get_candidate(quim.ctx, index, quim.active_count);
	str = quim_candidate_get_cand_str(cd);
	if (str)
		slen = strlen(str);

	// check if we even fit into that buffer
	if (!str ||
	    quim.edit_pos + quim.tail_length + slen - quim.cursor_inlength >= quim.buffer_size - 1)
	{
		// erase the part in the cursor:
		memmove(quim.buffer + quim.cursor_inpos,
			quim.buffer + quim.cursor_inpos + quim.cursor_inlength,
			quim.buffer_size - quim.cursor_inpos - quim.cursor_inlength + 1);
		quim.cursor_length -= quim.cursor_inlength;
		quim.cursor_inpos -= quim.cursor_inlength;
		quim.edit_length -= quim.cursor_inlength;
		quim.cursor_inlength = 0;
		if (str)
			Con_Print("Failed to select entry: buffer too short\n");
		return;
	}

	// we can insert this stuff
	// move the cursor-end + tail
	memmove(quim.buffer + quim.cursor_inpos + quim.cursor_inlength,
		quim.buffer + quim.cursor_pos + quim.cursor_length,
		quim.buffer_size - quim.cursor_pos - quim.cursor_length + 1);
	
	memcpy(quim.buffer + quim.cursor_inpos, str, slen);

	quim.edit_length = quim.edit_length + slen - quim.cursor_inlength;
	quim.cursor_length = quim.cursor_length + slen - quim.cursor_inlength;
	quim.cursor_inlength = slen;

	quim_candidate_free(cd);
}

static void UIM_Shift(void *cookie, int dir)
{
	++quim.actions;
	Con_Print("^1ERROR: ^7UIM_Shift: Not implemented\n");
}

static void UIM_Deactivate(void *cookie)
{
	++quim.actions;
	quim.active = false;
	//Con_Print("^3UIM: make UIM_Deactivate move the cursor...\n");
}

static void UIM_ConfigChanged(void *cookie)
{
	++quim.actions;
	//Con_Print("UIM_ConfigChanged\n");
}

static void UIM_PropListUpdate(void *cookie, const char *str)
{
	++quim.actions;
	if (quim.fd > 0)
	{
		char buffer[1024<<2];
		//Con_Printf("UIM_PropListUpdate\n%s\n", str);
		dpsnprintf(buffer, sizeof(buffer), "prop_list_update\ncharset=UTF-8\n%s", str);
		quim_helper_send_message(quim.fd, buffer);
	}
}
