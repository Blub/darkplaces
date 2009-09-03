/* UIM input support for
 * DarkPlaces
 */
#include "quakedef.h"

#include "uim_dp.h"

/*
================================================================================
CVars introduced with the UIM extension
================================================================================
*/

cvar_t im_method = {CVAR_SAVE, "im_method", "anthy", "which input method to use if uim is supported and active"};
cvar_t im_enabled = {CVAR_SAVE, "im_enabled", "0", "use UIM input"};

/*
================================================================================
Library imports. Taken from the uim headers.
================================================================================
*/

struct uim_code_converter *quim_iconv;

int           (*quim_init)(void);
void          (*quim_quit)(void);
uim_candidate (*quim_get_candidate)(uim_context, int index, int accel_enum_hint);
void          (*quim_candidate_free)(uim_candidate);
int           (*quim_get_candidate_index)(uim_context);
const char*   (*quim_candidate_get_cand_str)(uim_candidate);
const char*   (*quim_get_default_im_name)(const char *localename);
uim_context   (*quim_create_context)(void *cookie,
				     const char *encoding,
				     const char *im, const char *lang,
				     const char *engine,
				     struct uim_code_converter *conv,
				     void (*commit_cb)(void *ptr, const char *str));
void          (*quim_release_context)(uim_context);
int           (*quim_helper_init_client_fd)(void (*disconnect_cb)(void));
void          (*quim_close_client_fd)(int);
void          (*quim_set_preedit_cb)(uim_context,
				     void (*clear_cb)(void*),
				     void (*pushback_cb)(void*, int attr, const char *str),
				     void (*update_cb)(void*));
//void          (*quim_set_prop_list_update_cb)(uim_context,
//					      void (*update_cb)(void*, const char *longstr));
void          (*quim_set_candidate_selector_cb)(uim_context,
						void (*activate_cb)(void*, int nr, int display_limit),
						void (*select_cb)(void*, int index),
						void (*shift_page_cb)(void*, int direction),
						void (*deactivate_cb)(void*));
void          (*quim_prop_list_update)(uim_context);
void          (*quim_press_key)(uim_context, int key, int mods);
void          (*quim_release_key)(uim_context, int key, int mods);

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
	{"uim_release_context",		(void **) &quim_release_context},
	{"uim_helper_init_client_fd",	(void **) &quim_helper_init_client_fd},
	{"uim_close_client_fd",		(void **) &quim_close_client_fd},
	{"uim_set_preedit_cb",		(void **) &quim_set_preedit_cb},
	{"uim_set_candidate_selector_cb",		(void **) &quim_set_candidate_selector_cb},
	{"uim_prop_list_update",	(void **) &quim_prop_list_update},
	{"uim_press_key",		(void **) &quim_press_key},
	{"uim_release_key",		(void **) &quim_release_key},
	{"uim_iconv",			(void **) &quim_iconv},
	
	{NULL, NULL}
};

/// Handle for UIM dll
static dllhandle_t uim_dll = NULL;

/// Memory pool for uim
static mempool_t *uim_mempool= NULL;

/*
====================
UIM_CloseLibrary

Unload the UIM DLL
====================
*/
void UIM_CloseLibrary (void)
{
	if (uim_mempool)
		Mem_FreePool(&uim_mempool);
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
