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
// sys.h -- non-portable functions

#ifndef SYS_H
#define SYS_H


//
// DLL management
//

// Win32 specific
#ifdef WIN32
# include <windows.h>
typedef HMODULE dllhandle_t;

// Other platforms
#else
  typedef void* dllhandle_t;
#endif

typedef struct dllfunction_s
{
	const char *name;
	void **funcvariable;
}
dllfunction_t;

/*! Loads a library. 
 * \param dllnames a NULL terminated array of possible names for the DLL you want to load.
 * \param handle
 * \param fcts
 */
qboolean Sys_LoadLibrary (const char** dllnames, dllhandle_t* handle, const dllfunction_t *fcts);
void Sys_UnloadLibrary (dllhandle_t* handle);
void* Sys_GetProcAddress (dllhandle_t handle, const char* name);

/// called early in Host_Init
void Sys_InitConsole (void);
/// called after command system is initialized but before first Con_Print
void Sys_Init_Commands (void);


/// \returns current timestamp
char *Sys_TimeString(const char *timeformat);

//
// system IO interface (these are the sys functions that need to be implemented in a new driver atm)
//

/// an error will cause the entire program to exit
void Sys_Error (const char *error, ...) DP_FUNC_PRINTF(1);

/// (may) output text to terminal which launched program
void Sys_PrintToTerminal(const char *text);

/// INFO: This is only called by Host_Shutdown so we dont need testing for recursion
void Sys_Shutdown (void);
void Sys_Quit (int returnvalue);

/*! on some build/platform combinations (such as Linux gcc with the -pg
 * profiling option) this can turn on/off profiling, used primarily to limit
 * profiling to certain areas of the code, such as ingame performance without
 * regard for loading/shutdown performance (-profilegameonly on commandline)
 */
void Sys_AllowProfiling (qboolean enable);

double Sys_DoubleTime (void);

char *Sys_ConsoleInput (void);

/// called to yield for a little bit so as not to hog cpu when paused or debugging
void Sys_Sleep(int microseconds);

/// Perform Key_Event () callbacks until the input que is empty
void Sys_SendKeyEvents (void);

char *Sys_GetClipboardData (void);

typedef void *sys_mutex_t;
//typedef void *sys_semaphore_t;
typedef void *sys_thread_t;
typedef void *sys_threadpool_t;
typedef int sys_threadentry_t (void*);

// Sys_ThreadMem_ - thread-safe memory allocation routines
// Sys_Thread_    - system function to deal with threads
// Thread_        - functions used by threads, for example to exit a thread

void              *Sys_ThreadMem_Alloc (size_t);
void               Sys_ThreadMem_Free (void*);
sys_mutex_t       *Sys_Mutex_New (void);
qboolean           Sys_Mutex_Lock (sys_mutex_t*);
qboolean           Sys_Mutex_Unlock (sys_mutex_t*);
void               Sys_Mutex_Free (sys_mutex_t*);
sys_threadpool_t  *Sys_ThreadPool_New (int min, int max, int queueMax); // max -1 for no limit
/*! This spawns a new thread which does not belong to a threadpool.
 * There is no hardcoded limit here. It returns NULL if the call fails.
 */
sys_thread_t      *Thread_New (sys_threadentry_t*, void *userdata);
/*! Cancel a running thread. This may not be used with threads from a threadpool.
 * In case the thread belonged to a threadpool, nothing happens and false is returned.
 * Otherwise the thread is cancelled and true is returned.
 */
qboolean           Thread_Cancel (sys_thread_t*);
/*! (Try to) Spawn a thread. If there is still space in the threadpool, the function is executed in a new thread.
 * Otherwise, if block is false, the call fails and returns NULL, or if block is true, the call blocks until a
 * thread is available.
 */
sys_thread_t      *Sys_Thread_Spawn (sys_threadpool_t*, sys_threadentry_t*, void *userdata, qboolean block);
/*! (Try to) Queue a function for execution in the threadpool.
 * As soon as a thread is available in the threadpool, the function will be executed.
 * In case too many functions are queued already, this function fails and returns -1.
 * Otherwise, the function returns whether or not there was a thread already available.
 */
int                Sys_Thread_Queue (sys_threadpool_t*, sys_threadentry_t*, void *userdata);

void               Thread_Exit (int);
//void               Thread_Yield();

/*
sys_semaphore_t   *Sys_Semaphore_New (int);
qboolean           Sys_Semaphore_Acquire (sys_semaphore_t*);
qboolean           Sys_Semaphore_AcquireCount (sys_semaphore_t*, int);
qboolean           Sys_Semaphore_Release (sys_semaphore_t*);
qboolean           Sys_Semaphore_ReleaseCount (sys_semaphore_t*, int);
void               Sys_Semaphore_Free (sys_semaphore_t*);
*/

#endif

