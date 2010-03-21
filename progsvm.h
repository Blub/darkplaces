/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
This is a try to make the vm more generic, it is mainly based on the progs.h file.
For the license refer to progs.h.

Generic means, less as possible hard-coded links with the other parts of the engine.
This means no edict_engineprivate struct usage, etc.
The code uses void pointers instead.
*/

#ifndef PROGSVM_H
#define PROGSVM_H

#include "pr_comp.h"			// defs shared with qcc
#include "progdefs.h"			// generated by program cdefs
#include "clprogdefs.h"			// generated by program cdefs

#ifndef DP_SMALLMEMORY
#define PROFILING
#endif

// forward declaration of clgecko_t
struct clgecko_s;

typedef struct prvm_stack_s
{
	int				s;
	mfunction_t		*f;
	double			profile_acc;
	double			builtinsprofile_acc;
} prvm_stack_t;


typedef union prvm_eval_s
{
	string_t		string;
	float			_float;
	float			vector[3];
	func_t			function;
	int				ivector[3];
	int				_int;
	int				edict;
} prvm_eval_t;

typedef struct prvm_required_field_s
{
	int type;
	const char *name;
} prvm_required_field_t;


// AK: I dont call it engine private cause it doesnt really belongs to the engine
//     it belongs to prvm.
typedef struct prvm_edict_private_s
{
	qboolean free;
	float freetime;
	int mark;
	const char *allocation_origin;
} prvm_edict_private_t;

typedef struct prvm_edict_s
{
	// engine-private fields (stored in dynamically resized array)
	//edict_engineprivate_t *e;
	union
	{
		prvm_edict_private_t *required;
		vec_t *vp;
		// FIXME: this server pointer really means world, not server
		// (it is used by both server qc and client qc, but not menu qc)
		edict_engineprivate_t *server;
		// add other private structs as you desire
		// new structs have to start with the elements of prvm_edit_private_t
		// e.g. a new struct has to either look like this:
		//	typedef struct server_edict_private_s {
		//		prvm_edict_private_t base;
		//		vec3_t moved_from;
		//      vec3_t moved_fromangles;
		//		... } server_edict_private_t;
		// or:
		//	typedef struct server_edict_private_s {
		//		qboolean free;
		//		float freetime;
		//		vec3_t moved_from;
		//      vec3_t moved_fromangles;
		//		... } server_edict_private_t;
		// However, the first one should be preferred.
	} priv;
	// QuakeC fields (stored in dynamically resized array)
	union
	{
		vec_t *vp;
		entvars_t		*server;
		cl_entvars_t	*client;
	} fields;
} prvm_edict_t;

#define PRVM_EDICTFIELDVALUE(ed, fieldoffset) (fieldoffset >= 0 ? (prvm_eval_t *)((int *)ed->fields.vp + fieldoffset) : NULL)
#define PRVM_GLOBALFIELDVALUE(fieldoffset) (fieldoffset >= 0 ? (prvm_eval_t *)((int *)prog->globals.generic + fieldoffset) : NULL)

//============================================================================
#define PRVM_OP_STATE		1

#ifdef DP_SMALLMEMORY
#define	PRVM_MAX_STACK_DEPTH		128
#define	PRVM_LOCALSTACK_SIZE		2048

#define PRVM_MAX_OPENFILES 16
#define PRVM_MAX_OPENSEARCHES 8
#define PRVM_MAX_GECKOINSTANCES 1
#else
#define	PRVM_MAX_STACK_DEPTH		1024
#define	PRVM_LOCALSTACK_SIZE		16384

#define PRVM_MAX_OPENFILES 256
#define PRVM_MAX_OPENSEARCHES 128
#define PRVM_MAX_GECKOINSTANCES 32
#endif

typedef void (*prvm_builtin_t) (void);

// NOTE: field offsets use -1 for NULL
typedef struct prvm_prog_fieldoffsets_s
{
	// server and client use a lot of similar fields, so this is combined
	int SendEntity; // ssqc
	int SendFlags; // ssqc
	int Version; // ssqc (legacy)
	int alpha; // ssqc / csqc
	int ammo_cells1; // ssqc - Dissolution of Eternity mission pack
	int ammo_lava_nails; // ssqc - Dissolution of Eternity mission pack
	int ammo_multi_rockets; // ssqc - Dissolution of Eternity mission pack
	int ammo_nails1; // ssqc - Dissolution of Eternity mission pack
	int ammo_plasma; // ssqc - Dissolution of Eternity mission pack
	int ammo_rockets1; // ssqc - Dissolution of Eternity mission pack
	int ammo_shells1; // ssqc - Dissolution of Eternity mission pack
	int angles; // common - used by changeyaw/changepitch
	int button3; // ssqc
	int button4; // ssqc
	int button5; // ssqc
	int button6; // ssqc
	int button7; // ssqc
	int button8; // ssqc
	int button9; // ssqc
	int button10; // ssqc
	int button11; // ssqc
	int button12; // ssqc
	int button13; // ssqc
	int button14; // ssqc
	int button15; // ssqc
	int button16; // ssqc
	int buttonchat; // ssqc
	int buttonuse; // ssqc
	int chain; // common - used by find builtins
	int classname; // common
	int clientcamera; // ssqc
	int clientcolors; // ssqc
	int clientstatus; // ssqc
	int color; // ssqc
	int colormod; // ssqc / csqc
	int contentstransition; // ssqc
	int cursor_active; // ssqc
	int cursor_screen; // ssqc
	int cursor_trace_endpos; // ssqc
	int cursor_trace_ent; // ssqc
	int cursor_trace_start; // ssqc
	int customizeentityforclient; // ssqc
	int dimension_hit; // ssqc / csqc
	int dimension_solid; // ssqc / csqc
	int disableclientprediction; // ssqc
	int dphitcontentsmask; // ssqc / csqc
	int drawonlytoclient; // ssqc
	int effects; // ssqc / csqc
	int exteriormodeltoclient; // ssqc
	int fatness; // ssqc / csqc
	int forceshader; // csqc
	int frame1time; // csqc
	int frame2; // csqc
	int frame2time; // csqc
	int frame3; // csqc
	int frame3time; // csqc
	int frame4; // csqc
	int frame4time; // csqc
	int frame; // common - used by OP_STATE
	int fullbright; // ssqc - Nehahra support
	int glow_color; // ssqc
	int glow_size; // ssqc
	int glow_trail; // ssqc
	int glowmod; // ssqc / csqc
	int gravity; // ssqc
	int groundentity; // ssqc / csqc
	int hull; // ssqc / csqc
	int ideal_yaw; // ssqc / csqc
	int idealpitch; // ssqc / csqc
	int items2; // ssqc
	int lerpfrac3; // csqc
	int lerpfrac4; // csqc
	int lerpfrac; // csqc
	int light_lev; // ssqc
	int message; // csqc
	int modelflags; // ssqc
	int movement; // ssqc
	int movetypesteplandevent; // ssqc
	int netaddress; // ssqc
	int nextthink; // common - used by OP_STATE
	int nodrawtoclient; // ssqc
	int pflags; // ssqc
	int ping; // ssqc
	int packetloss; // ssqc
	int movementloss; // ssqc
	int pitch_speed; // ssqc / csqc
	int playermodel; // ssqc
	int playerskin; // ssqc
	int pmodel; // ssqc
	int punchvector; // ssqc
	int renderamt; // ssqc - HalfLife support
	int renderflags; // csqc
	int rendermode; // ssqc - HalfLife support
	int scale; // ssqc / csqc
	int shadertime; // csqc
	int skeletonindex; // csqc / ssqc FTE_CSQC_SKELETONOBJECTS / DP_SKELETONOBJECTS
	int style; // ssqc
	int tag_entity; // ssqc / csqc
	int tag_index; // ssqc / csqc
	int think; // common - used by OP_STATE
	int viewmodelforclient; // ssqc
	int viewzoom; // ssqc
	int yaw_speed; // ssqc / csqc
	int bouncefactor; // ssqc
	int bouncestop; // ssqc

	int solid; // ssqc / csqc (physics)
	int movetype; // ssqc / csqc (physics)
	int modelindex; // ssqc / csqc (physics)
	int mins; // ssqc / csqc (physics)
	int maxs; // ssqc / csqc (physics)
	int mass; // ssqc / csqc (physics)
	int origin; // ssqc / csqc (physics)
	int velocity; // ssqc / csqc (physics)
	//int axis_forward; // ssqc / csqc (physics)
	//int axis_left; // ssqc / csqc (physics)
	//int axis_up; // ssqc / csqc (physics)
	//int spinvelocity; // ssqc / csqc (physics)
	//int angles; // ssqc / csqc (physics)
	int avelocity; // ssqc / csqc (physics)
	int jointtype; // ssqc / csqc (physics)
	int enemy; // ssqc / csqc (physics)
	int aiment; // ssqc / csqc (physics)
	int movedir; // ssqc / csqc (physics)

	int camera_transform; // csqc (warpzones)
}
prvm_prog_fieldoffsets_t;

// NOTE: global offsets use -1 for NULL
typedef struct prvm_prog_globaloffsets_s
{
	// server and client use a lot of similar globals, so this is combined
	int SV_InitCmd; // ssqc
	int self; // common
	int time; // ssqc / csqc
	int v_forward; // ssqc / csqc
	int v_right; // ssqc / csqc
	int v_up; // ssqc / csqc
	int view_angles; // csqc
	int trace_allsolid; // ssqc / csqc
	int trace_startsolid; // ssqc / csqc
	int trace_fraction; // ssqc / csqc
	int trace_inwater; // ssqc / csqc
	int trace_inopen; // ssqc / csqc
	int trace_endpos; // ssqc / csqc
	int trace_plane_normal; // ssqc / csqc
	int trace_plane_dist; // ssqc / csqc
	int trace_ent; // ssqc / csqc
	int trace_networkentity; // csqc
	int trace_dphitcontents; // ssqc / csqc
	int trace_dphitq3surfaceflags; // ssqc / csqc
	int trace_dphittexturename; // ssqc / csqc
	int trace_dpstartcontents; // ssqc / csqc
	int intermission; // csqc
	int coop; // csqc
	int deathmatch; // csqc
	int dmg_take; // csqc
	int dmg_save; // csqc
	int dmg_origin; // csqc
	int sb_showscores; // csqc
	int drawfont; // csqc / menu
	int drawfontscale; // csqc / menu
	int require_spawnfunc_prefix; // ssqc
	int worldstatus; // ssqc
	int servertime; // csqc
	int serverprevtime; // csqc
	int serverdeltatime; // csqc
	int gettaginfo_name; // ssqc / csqc
	int gettaginfo_parent; // ssqc / csqc
	int gettaginfo_offset; // ssqc / csqc
	int gettaginfo_forward; // ssqc / csqc
	int gettaginfo_right; // ssqc / csqc
	int gettaginfo_up; // ssqc / csqc
	int transparent_offset; // csqc
}
prvm_prog_globaloffsets_t;

// these are initialized using PRVM_ED_FindFunction
// NOTE: function offsets use 0 for NULL
typedef struct prvm_prog_funcoffsets_s
{
	func_t CSQC_ConsoleCommand; // csqc
	func_t CSQC_Ent_Remove; // csqc
	func_t CSQC_Ent_Spawn; // csqc DP_CSQC_ENT_SPAWN extension (BlackHC - TODO: needs to be added to dpextensions.qc)
	func_t CSQC_Ent_Update; // csqc
	func_t CSQC_Event; // csqc [515]: engine call this for its own needs so csqc can do some things according to what engine it's running on.  example: to say about edicts increase, whatever...
	func_t CSQC_Event_Sound; // csqc : called by engine when an incoming sound packet arrives so CSQC can act on it
	func_t CSQC_Init; // csqc
	func_t CSQC_InputEvent; // csqc
	func_t CSQC_Parse_CenterPrint; // csqc
	func_t CSQC_Parse_Print; // csqc
	func_t CSQC_Parse_StuffCmd; // csqc
	func_t CSQC_Parse_TempEntity; // csqc [515]: very helpfull when you want to create your own particles/decals/etc for effects that already exist
	func_t CSQC_Shutdown; // csqc
	func_t CSQC_UpdateView; // csqc
	func_t Gecko_Query; // csqc, mqc
	func_t EndFrame; // ssqc
	func_t RestoreGame; // ssqc
	func_t SV_ChangeTeam; // ssqc
	func_t SV_ParseClientCommand; // ssqc
	func_t SV_PlayerPhysics; // ssqc
	func_t SV_OnEntityPreSpawnFunction; // ssqc
	func_t SV_OnEntityNoSpawnFunction; // ssqc
	func_t SV_OnEntityPostSpawnFunction; // ssqc
	func_t GameCommand; // any
	func_t SV_Shutdown; // ssqc
	func_t URI_Get_Callback; // any
	func_t SV_PausedTic; //ssqc

	// menu qc only uses some functions, nothing else
	func_t m_draw; // mqc
	func_t m_init; // mqc
	func_t m_keydown; // mqc
	func_t m_keyup; // mqc
	func_t m_shutdown; // mqc
	func_t m_toggle; // mqc
}
prvm_prog_funcoffsets_t;

// stringbuffer flags
#define STRINGBUFFER_SAVED	1 // saved in savegames

typedef struct prvm_stringbuffer_s
{
	int max_strings;
	int num_strings;
	char **strings;
	const char *origin;
	unsigned char flags;
}
prvm_stringbuffer_t;

// [INIT] variables flagged with this token can be initialized by 'you'
// NOTE: external code has to create and free the mempools but everything else is done by prvm !
typedef struct prvm_prog_s
{
	double              starttime;
	unsigned int		id; // increasing unique id of progs instance
	dprograms_t			*progs;
	mfunction_t			*functions;
	char				*strings;
	int					stringssize;
	ddef_t				*fielddefs;
	ddef_t				*globaldefs;
	dstatement_t		*statements;
	int					entityfields;			// number of vec_t fields in progs (some variables are 3)
	int					entityfieldsarea;		// LordHavoc: equal to max_edicts * entityfields (for bounds checking)

	int					*statement_linenums; // NULL if not available

	double				*statement_profile; // only incremented if prvm_statementprofiling is on

	union {
		vec_t *generic;
		globalvars_t *server;
		cl_globalvars_t *client;
	} globals;

	int					maxknownstrings;
	int					numknownstrings;
	// this is updated whenever a string is removed or added
	// (simple optimization of the free string search)
	int					firstfreeknownstring;
	const char			**knownstrings;
	unsigned char		*knownstrings_freeable;
	const char          **knownstrings_origin;
	const char			***stringshash;

	memexpandablearray_t	stringbuffersarray;

	// all memory allocations related to this vm_prog (code, edicts, strings)
	mempool_t			*progs_mempool; // [INIT]

	prvm_builtin_t		*builtins; // [INIT]
	int					numbuiltins; // [INIT]

	int					argc;

	int					trace;
	mfunction_t			*xfunction;
	int					xstatement;

	// stacktrace writes into stack[MAX_STACK_DEPTH]
	// thus increase the array, so depth wont be overwritten
	prvm_stack_t		stack[PRVM_MAX_STACK_DEPTH+1];
	int					depth;

	int					localstack[PRVM_LOCALSTACK_SIZE];
	int					localstack_used;

	unsigned short		headercrc; // [INIT]
	unsigned short		headercrc2; // [INIT] alternate CRC for tenebrae progs.dat

	unsigned short		filecrc;

	//============================================================================
	// until this point everything also exists (with the pr_ prefix) in the old vm

	qfile_t				*openfiles[PRVM_MAX_OPENFILES];
	const char *         openfiles_origin[PRVM_MAX_OPENFILES];
	fssearch_t			*opensearches[PRVM_MAX_OPENSEARCHES];
	const char *         opensearches_origin[PRVM_MAX_OPENSEARCHES];
	struct clgecko_s		*opengeckoinstances[PRVM_MAX_GECKOINSTANCES];
	skeleton_t			*skeletons[MAX_EDICTS];

	// copies of some vars that were former read from sv
	int					num_edicts;
	// number of edicts for which space has been (should be) allocated
	int					max_edicts; // [INIT]
	// used instead of the constant MAX_EDICTS
	int					limit_edicts; // [INIT]

	// number of reserved edicts (allocated from 1)
	int					reserved_edicts; // [INIT]

	prvm_edict_t		*edicts;
	vec_t				*edictsfields;
	void					*edictprivate;

	// size of the engine private struct
	int					edictprivate_size; // [INIT]

	prvm_prog_fieldoffsets_t	fieldoffsets;
	prvm_prog_globaloffsets_t	globaloffsets;
	prvm_prog_funcoffsets_t		funcoffsets;

	// allow writing to world entity fields, this is set by server init and
	// cleared before first server frame
	qboolean			allowworldwrites;

	// name of the prog, e.g. "Server", "Client" or "Menu" (used for text output)
	char				*name; // [INIT]

	// flag - used to store general flags like PRVM_GE_SELF, etc.
	int				flag;

	char				*extensionstring; // [INIT]

	qboolean			loadintoworld; // [INIT]

	// used to indicate whether a prog is loaded
	qboolean			loaded;
	qboolean			leaktest_active;

	// translation buffer (only needs to be freed on unloading progs, type is private to prvm_edict.c)
	void *po;

	// printed together with backtraces
	const char *statestring;

//	prvm_builtin_mem_t  *mem_list;

// now passed as parameter of PRVM_LoadProgs
//	char				**required_func;
//	int					numrequiredfunc;

	//============================================================================

	ddef_t				*self; // if self != 0 then there is a global self

	//============================================================================
	// function pointers

	void				(*begin_increase_edicts)(void); // [INIT] used by PRVM_MEM_Increase_Edicts
	void				(*end_increase_edicts)(void); // [INIT]

	void				(*init_edict)(prvm_edict_t *edict); // [INIT] used by PRVM_ED_ClearEdict
	void				(*free_edict)(prvm_edict_t *ed); // [INIT] used by PRVM_ED_Free

	void				(*count_edicts)(void); // [INIT] used by PRVM_ED_Count_f

	qboolean			(*load_edict)(prvm_edict_t *ent); // [INIT] used by PRVM_ED_LoadFromFile

	void				(*init_cmd)(void); // [INIT] used by PRVM_InitProg
	void				(*reset_cmd)(void); // [INIT] used by PRVM_ResetProg

	void				(*error_cmd)(const char *format, ...) DP_FUNC_PRINTF(1); // [INIT]

	void				(*ExecuteProgram)(func_t fnum, const char *errormessage); // pointer to one of the *VM_ExecuteProgram functions
} prvm_prog_t;

extern prvm_prog_t * prog;

#define PRVM_MAXPROGS 3
#define PRVM_SERVERPROG 0 // actually not used at the moment
#define PRVM_CLIENTPROG 1
#define PRVM_MENUPROG	2

extern prvm_prog_t prvm_prog_list[PRVM_MAXPROGS];

//============================================================================
// prvm_cmds part

extern prvm_builtin_t vm_sv_builtins[];
extern prvm_builtin_t vm_cl_builtins[];
extern prvm_builtin_t vm_m_builtins[];

extern const int vm_sv_numbuiltins;
extern const int vm_cl_numbuiltins;
extern const int vm_m_numbuiltins;

extern char * vm_sv_extensions; // client also uses this
extern char * vm_m_extensions;

void VM_SV_Cmd_Init(void);
void VM_SV_Cmd_Reset(void);

void VM_CL_Cmd_Init(void);
void VM_CL_Cmd_Reset(void);

void VM_M_Cmd_Init(void);
void VM_M_Cmd_Reset(void);

void VM_Cmd_Init(void);
void VM_Cmd_Reset(void);
//============================================================================

void PRVM_Init (void);

#ifdef PROFILING
void MVM_ExecuteProgram (func_t fnum, const char *errormessage);
void CLVM_ExecuteProgram (func_t fnum, const char *errormessage);
void SVVM_ExecuteProgram (func_t fnum, const char *errormessage);
#else
#define MVM_ExecuteProgram SVVM_ExecuteProgram
#define CLVM_ExecuteProgram SVVM_ExecuteProgram
void SVVM_ExecuteProgram (func_t fnum, const char *errormessage);
#endif
#define PRVM_ExecuteProgram prog->ExecuteProgram

#define PRVM_Alloc(buffersize) _PRVM_Alloc(buffersize, __FILE__, __LINE__)
#define PRVM_Free(buffer) _PRVM_Free(buffer, __FILE__, __LINE__)
#define PRVM_FreeAll() _PRVM_FreeAll(__FILE__, __LINE__)
void *_PRVM_Alloc (size_t buffersize, const char *filename, int fileline);
void _PRVM_Free (void *buffer, const char *filename, int fileline);
void _PRVM_FreeAll (const char *filename, int fileline);

void PRVM_Profile (int maxfunctions, int mininstructions, int sortby);
void PRVM_Profile_f (void);
void PRVM_ChildProfile_f (void);
void PRVM_CallProfile_f (void);
void PRVM_PrintFunction_f (void);

void PRVM_PrintState(void);
void PRVM_CrashAll (void);
void PRVM_Crash (void);
void PRVM_ShortStackTrace(char *buf, size_t bufsize);
const char *PRVM_AllocationOrigin(void);

ddef_t *PRVM_ED_FindField(const char *name);
ddef_t *PRVM_ED_FindGlobal(const char *name);
mfunction_t *PRVM_ED_FindFunction(const char *name);

int PRVM_ED_FindFieldOffset(const char *name);
int PRVM_ED_FindGlobalOffset(const char *name);
func_t PRVM_ED_FindFunctionOffset(const char *name);
#define PRVM_ED_FindFieldOffset_FromStruct(st, field) prog->fieldoffsets . field = ((int *)(&((st *)NULL)-> field ) - ((int *)NULL))
#define PRVM_ED_FindGlobalOffset_FromStruct(st, field) prog->globaloffsets . field = ((int *)(&((st *)NULL)-> field ) - ((int *)NULL))

void PRVM_MEM_IncreaseEdicts(void);

qboolean PRVM_ED_CanAlloc(prvm_edict_t *e);
prvm_edict_t *PRVM_ED_Alloc (void);
void PRVM_ED_Free (prvm_edict_t *ed);
void PRVM_ED_ClearEdict (prvm_edict_t *e);

void PRVM_PrintFunctionStatements (const char *name);
void PRVM_ED_Print(prvm_edict_t *ed, const char *wildcard_fieldname);
void PRVM_ED_Write (qfile_t *f, prvm_edict_t *ed);
const char *PRVM_ED_ParseEdict (const char *data, prvm_edict_t *ent);

void PRVM_ED_WriteGlobals (qfile_t *f);
void PRVM_ED_ParseGlobals (const char *data);

void PRVM_ED_LoadFromFile (const char *data);

unsigned int PRVM_EDICT_NUM_ERROR(unsigned int n, char *filename, int fileline);
#define	PRVM_EDICT(n) (((unsigned)(n) < (unsigned int)prog->max_edicts) ? (unsigned int)(n) : PRVM_EDICT_NUM_ERROR((unsigned int)(n), __FILE__, __LINE__))
#define	PRVM_EDICT_NUM(n) (prog->edicts + PRVM_EDICT(n))

//int NUM_FOR_EDICT_ERROR(prvm_edict_t *e);
#define PRVM_NUM_FOR_EDICT(e) ((int)((prvm_edict_t *)(e) - prog->edicts))
//int PRVM_NUM_FOR_EDICT(prvm_edict_t *e);

#define	PRVM_NEXT_EDICT(e) ((e) + 1)

#define PRVM_EDICT_TO_PROG(e) (PRVM_NUM_FOR_EDICT(e))
//int PRVM_EDICT_TO_PROG(prvm_edict_t *e);
#define PRVM_PROG_TO_EDICT(n) (PRVM_EDICT_NUM(n))
//prvm_edict_t *PRVM_PROG_TO_EDICT(int n);

//============================================================================

#define	PRVM_G_FLOAT(o) (prog->globals.generic[o])
#define	PRVM_G_INT(o) (*(int *)&prog->globals.generic[o])
#define	PRVM_G_EDICT(o) (PRVM_PROG_TO_EDICT(*(int *)&prog->globals.generic[o]))
#define PRVM_G_EDICTNUM(o) PRVM_NUM_FOR_EDICT(PRVM_G_EDICT(o))
#define	PRVM_G_VECTOR(o) (&prog->globals.generic[o])
#define	PRVM_G_STRING(o) (PRVM_GetString(*(string_t *)&prog->globals.generic[o]))
//#define	PRVM_G_FUNCTION(o) (*(func_t *)&prog->globals.generic[o])

// FIXME: make these go away?
#define	PRVM_E_FLOAT(e,o) (((float*)e->fields.vp)[o])
#define	PRVM_E_INT(e,o) (((int*)e->fields.vp)[o])
//#define	PRVM_E_VECTOR(e,o) (&((float*)e->fields.vp)[o])
#define	PRVM_E_STRING(e,o) (PRVM_GetString(*(string_t *)&((float*)e->fields.vp)[o]))

extern	int		prvm_type_size[8]; // for consistency : I think a goal of this sub-project is to
// make the new vm mostly independent from the old one, thus if it's necessary, I copy everything

void PRVM_Init_Exec(void);

void PRVM_ED_PrintEdicts_f (void);
void PRVM_ED_PrintNum (int ent, const char *wildcard_fieldname);

const char *PRVM_GetString(int num);
int PRVM_SetEngineString(const char *s);
const char *PRVM_ChangeEngineString(int i, const char *s);
int PRVM_SetTempString(const char *s);
int PRVM_AllocString(size_t bufferlength, char **pointer);
void PRVM_FreeString(int num);

//============================================================================

// used as replacement for a prog stack
//#define PRVM_DEBUGPRSTACK

#ifdef PRVM_DEBUGPRSTACK
#define PRVM_Begin  if(prog != 0) Con_Printf("prog not 0(prog = %i) in file: %s line: %i!\n", PRVM_GetProgNr(), __FILE__, __LINE__)
#define PRVM_End	prog = 0
#else
#define PRVM_Begin
#define PRVM_End	prog = 0
#endif

//#define PRVM_SAFENAME
#ifndef PRVM_SAFENAME
#	define PRVM_NAME	(prog->name)
#else
#	define PRVM_NAME	(prog->name ? prog->name : "Unknown prog name")
#endif

// helper macro to make function pointer calls easier
#define PRVM_GCALL(func)	if(prog->func) prog->func

#define PRVM_ERROR		prog->error_cmd

// other prog handling functions
qboolean PRVM_SetProgFromString(const char *str);
void PRVM_SetProg(int prognr);

/*
Initializing a vm:
Call InitProg with the num
Set up the fields marked with [INIT] in the prog struct
Load a program with LoadProgs
*/
void PRVM_InitProg(int prognr);
// LoadProgs expects to be called right after InitProg
void PRVM_LoadProgs (const char *filename, int numrequiredfunc, char **required_func, int numrequiredfields, prvm_required_field_t *required_field, int numrequiredglobals, char **required_global);
void PRVM_ResetProg(void);

qboolean PRVM_ProgLoaded(int prognr);

int	PRVM_GetProgNr(void);

void VM_Warning(const char *fmt, ...) DP_FUNC_PRINTF(1);

// TODO: fill in the params
//void PRVM_Create();

void VM_GenerateFrameGroupBlend(framegroupblend_t *framegroupblend, const prvm_edict_t *ed);
void VM_FrameBlendFromFrameGroupBlend(frameblend_t *frameblend, const framegroupblend_t *framegroupblend, const dp_model_t *model);
void VM_UpdateEdictSkeleton(prvm_edict_t *ed, const dp_model_t *edmodel, const frameblend_t *frameblend);
void VM_RemoveEdictSkeleton(prvm_edict_t *ed);

#endif
