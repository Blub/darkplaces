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

cvar_t im_enabled = {CVAR_SAVE, "im_enabled", "0", "use UIM input"};
cvar_t im_engine = {CVAR_SAVE, "im_engine", "anthy", "which input method to use if uim is supported and active"};
cvar_t im_language = {CVAR_SAVE, "im_language", "ja", "which language should be used for the input editor (en for english, ja for japanese, etc.)"};

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
				     const char *lang,
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

/*
================================================================================
UIM input method implementation.
================================================================================
*/

typedef struct
{
	uim_context ctx;
	int         fd;
} quim_state;

static quim_state quim;

static void UIM_Commit(void*, const char *str);
static void UIM_HelperDisconnected(void);
static void UIM_Clear(void*);
static void UIM_Push(void*, int attr, const char *str);
static void UIM_Update(void*);
static void UIM_Activate(void*, int nr, int display_limit);
static void UIM_Select(void*, int index);
static void UIM_Shift(void*, int dir);
static void UIM_Deactivate(void*);

static void UIM_Start(void);
/*
====================
UIM_Init

Load UIM support and register commands / cvars
====================
*/
void UIM_Init(void)
{
	// register the cvars in any case so they're at least saved,
	// after all, they're for the user

	Cvar_RegisterVariable(&im_engine);
	Cvar_RegisterVariable(&im_language);
	Cvar_RegisterVariable(&im_enabled);

	UIM_Start();
}

/*
====================
UIM_Start

Try starting up UIM, this should be made available as console command maybe?
====================
*/
static void UIM_Start(void)
{
	if (!UIM_OpenLibrary())
		return;

	if (quim_init() != 0)
	{
		Con_Print("Failed to initialize UIM support\n");
		return;
	}

	quim.ctx = quim_create_context(NULL, "UTF-8", im_language.string, im_engine.string, quim_iconv, &UIM_Commit);
	if (quim.ctx == NULL)
	{
		Con_Print("Failed to create UIM context\n");
		UIM_CloseLibrary();
		return;
	}

	quim.fd = quim_helper_init_client_fd(&UIM_HelperDisconnected);
	if (quim.fd < 0)
	{
		quim_release_context(quim.ctx);
		quim.ctx = NULL;
		UIM_CloseLibrary();
		return;
	}

	quim_set_preedit_cb(quim.ctx, &UIM_Clear, &UIM_Push, &UIM_Update);
	quim_set_candidate_selector_cb(quim.ctx, &UIM_Activate, &UIM_Select, &UIM_Shift, &UIM_Deactivate);
	quim_prop_list_update(quim.ctx);
}

static void UIM_Commit(void *cookie, const char *str)
{
}

static void UIM_HelperDisconnected(void)
{
}

static void UIM_Clear(void *cookie)
{
}

static void UIM_Push(void *cookie, int attr, const char *str)
{
}

static void UIM_Update(void *cookie)
{
}

static void UIM_Activate(void *cookie, int nr, int display_limit)
{
}

static void UIM_Select(void *cookie, int index)
{
}

static void UIM_Shift(void *cookie, int dir)
{
}

static void UIM_Deactivate(void *cookie)
{
}

