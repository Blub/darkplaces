#include "quakedef.h"

#define SUPPORTDLL

# include <time.h>
#ifndef WIN32
# include <unistd.h>
# include <fcntl.h>
#ifdef SUPPORTDLL
# include <dlfcn.h>
#endif
#endif

static char sys_timestring[128];
char *Sys_TimeString(const char *timeformat)
{
	time_t mytime = time(NULL);
#if _MSC_VER >= 1400
	struct tm mytm;
	localtime_s(&mytm, &mytime);
	strftime(sys_timestring, sizeof(sys_timestring), timeformat, &mytm);
#else
	strftime(sys_timestring, sizeof(sys_timestring), timeformat, localtime(&mytime));
#endif
	return sys_timestring;
}


extern qboolean host_shuttingdown;
void Sys_Quit (int returnvalue)
{
	if (COM_CheckParm("-profilegameonly"))
		Sys_AllowProfiling(false);
	host_shuttingdown = true;
	Host_Shutdown();
	exit(returnvalue);
}

#if defined(__linux__) || defined(__FreeBSD__)
#ifdef __cplusplus
extern "C"
#endif
int moncontrol(int);
#endif

void Sys_AllowProfiling(qboolean enable)
{
#if defined(__linux__) || defined(__FreeBSD__)
	moncontrol(enable);
#endif
}


/*
===============================================================================

DLL MANAGEMENT

===============================================================================
*/

qboolean Sys_LoadLibrary (const char** dllnames, dllhandle_t* handle, const dllfunction_t *fcts)
{
#ifdef SUPPORTDLL
	const dllfunction_t *func;
	dllhandle_t dllhandle = 0;
	unsigned int i;

	if (handle == NULL)
		return false;

#ifndef WIN32
#ifdef PREFER_PRELOAD
	dllhandle = dlopen(NULL, RTLD_LAZY | RTLD_GLOBAL);
	if(dllhandle)
	{
		for (func = fcts; func && func->name != NULL; func++)
			if (!(*func->funcvariable = (void *) Sys_GetProcAddress (dllhandle, func->name)))
			{
				dlclose(dllhandle);
				goto notfound;
			}
		Con_DPrintf ("All of %s's functions were already linked in! Not loading dynamically...\n", dllnames[0]);
		*handle = dllhandle;
		return true;
	}
notfound:
#endif
#endif

	// Initializations
	for (func = fcts; func && func->name != NULL; func++)
		*func->funcvariable = NULL;

	// Try every possible name
	Con_DPrintf ("Trying to load library...");
	for (i = 0; dllnames[i] != NULL; i++)
	{
		Con_DPrintf (" \"%s\"", dllnames[i]);
#ifdef WIN32
		dllhandle = LoadLibrary (dllnames[i]);
#else
		dllhandle = dlopen (dllnames[i], RTLD_LAZY | RTLD_GLOBAL);
#endif
		if (dllhandle)
			break;
	}

	// see if the names can be loaded relative to the executable path
	// (this is for Mac OSX which does not check next to the executable)
	if (!dllhandle && strrchr(com_argv[0], '/'))
	{
		char path[MAX_OSPATH];
		strlcpy(path, com_argv[0], sizeof(path));
		strrchr(path, '/')[1] = 0;
		for (i = 0; dllnames[i] != NULL; i++)
		{
			char temp[MAX_OSPATH];
			strlcpy(temp, path, sizeof(temp));
			strlcat(temp, dllnames[i], sizeof(temp));
			Con_DPrintf (" \"%s\"", temp);
#ifdef WIN32
			dllhandle = LoadLibrary (temp);
#else
			dllhandle = dlopen (temp, RTLD_LAZY | RTLD_GLOBAL);
#endif
			if (dllhandle)
				break;
		}
	}

	// No DLL found
	if (! dllhandle)
	{
		Con_DPrintf(" - failed.\n");
		return false;
	}

	Con_DPrintf(" - loaded.\n");

	// Get the function adresses
	for (func = fcts; func && func->name != NULL; func++)
		if (!(*func->funcvariable = (void *) Sys_GetProcAddress (dllhandle, func->name)))
		{
			Con_DPrintf ("Missing function \"%s\" - broken library!\n", func->name);
			Sys_UnloadLibrary (&dllhandle);
			return false;
		}

	*handle = dllhandle;
	return true;
#else
	return false;
#endif
}

void Sys_UnloadLibrary (dllhandle_t* handle)
{
#ifdef SUPPORTDLL
	if (handle == NULL || *handle == NULL)
		return;

#ifdef WIN32
	FreeLibrary (*handle);
#else
	dlclose (*handle);
#endif

	*handle = NULL;
#endif
}

void* Sys_GetProcAddress (dllhandle_t handle, const char* name)
{
#ifdef SUPPORTDLL
#ifdef WIN32
	return (void *)GetProcAddress (handle, name);
#else
	return (void *)dlsym (handle, name);
#endif
#else
	return NULL;
#endif
}

void *_Sys_ThreadMem_Alloc (size_t);
void _Sys_ThreadMem_Free (void*);

typedef struct {
	sys_mutex_t          *mutex;
	int                   maxThreads;
	int                   maxQueued;
	qboolean              quit;
	memexpandablearray_t  threads;
	memexpandablearray_t  queue;
	sys_semaphore_t      *semEmpty; // not-0 if the queue is empty
	struct _sys_poolthread_s *first;
} _sys_threadpool_t;

typedef struct _sys_poolthread_s {
	sys_thread_t      *thread;
	_sys_threadpool_t *pool;
	sys_semaphore_t   *semRun; // increased whenever the thread can run again
	sys_semaphore_t   *semQueue; // increased as soon as the thread accepts a new job - AFTER the function is executed
	qboolean           quit;
	sys_threadentry_t *nextEntry;
	void              *nextUserdata;
	struct _sys_poolthread_s *nextThread; // we order them so the free ones are first
} _sys_poolthread_t;

typedef struct {
	sys_threadentry_t *entry;
	void *userdata;
} _sys_queueentry_t;

static int ThreadPool_Entry(_sys_poolthread_t *self)
{
	sys_threadentry_t *entry;
	void *data;
	while (!self->quit) {
		Sys_Semaphore_Wait(self->semRun, true);
		/* not the responsibility of this function to detect such a thing
		if (!self->nextEntry)
			continue;
		*/
		entry = self->nextEntry;
		data = self->nextUserdata;
		self->nextEntry = NULL;
		(*entry)(data);
		Sys_Semaphore_Post(self->semQueue);
	}
	return 0;
}

static qboolean ThreadPool_Try(_sys_poolthread_t *thread, sys_threadentry_t *entry, void *data)
{
	if (!Sys_Semaphore_Wait(thread->semQueue, false))
		return false;
	thread->nextEntry = entry;
	thread->nextUserdata = data;
	Sys_Semaphore_Post(thread->semRun);
	return true;
}

static _sys_poolthread_t *_Sys_PoolThread_New (_sys_threadpool_t *pool)
{
	_sys_poolthread_t *thread = _Sys_ThreadMem_Alloc(sizeof(_sys_poolthread_t));
	thread->pool = pool;
	thread->semQueue = Sys_Semaphore_New(1);
	thread->semRun = Sys_Semaphore_New(0);
	thread->quit = false;
	thread->nextEntry = NULL;
	thread->nextUserdata = NULL;
	thread->thread = Sys_Thread_New((sys_threadentry_t*)&ThreadPool_Entry, (void*)thread);
	return thread;
}

static void _Sys_PoolThread_Free(_sys_poolthread_t *thread)
{
	Sys_Semaphore_Free(thread->semQueue);
	Sys_Semaphore_Free(thread->semRun);
}

sys_threadpool_t *Sys_ThreadPool_New (int min, int max, int queueMax)
{
	_sys_threadpool_t *pool;
	if (!Sys_ThreadsAvailable())
		return NULL;
	if (!Sys_ThreadMem_Lock())
		return NULL;

	if (min > max)
		min = max;
	if (min < 1)
		min = 1;

	pool = _Sys_ThreadMem_Alloc(sizeof(_sys_threadpool_t));
	if (!pool)
		return NULL;
	memset(pool, 0, sizeof(pool));
	pool->quit = false;
	pool->maxThreads = max;
	pool->maxQueued = queueMax;
	pool->semEmpty = Sys_Semaphore_New(1);

	if (!(pool->mutex = Sys_Mutex_New()))
		goto error;

	Mem_ExpandableArray_NewArray(&pool->threads, threadmempool, sizeof(_sys_poolthread_t), 16);
	Mem_ExpandableArray_NewArray(&pool->queue, threadmempool, sizeof(_sys_queueentry_t), 16);

	for (; min > 0; --min) {
		_sys_poolthread_t **th;
		th = Mem_ExpandableArray_AllocRecord(&pool->threads);
		*th = _Sys_PoolThread_New(pool);
	}
	
	Sys_ThreadMem_Unlock();
	return (sys_threadpool_t*)pool;

error:
	if (pool->mutex)
		Sys_Mutex_Free(pool->mutex);
	_Sys_ThreadMem_Free(pool);
	Sys_ThreadMem_Unlock();
	return NULL;
}

void Sys_ThreadPool_Join (sys_threadpool_t *_pool, qboolean kill)
{
	size_t i;
	size_t nThreads;
	_sys_threadpool_t *pool = (_sys_threadpool_t*)_pool;

	// here we need to check if the mutex actually locks, if so
	// we check if this threadpool is being destroyd already, and if so, we return
	// if not, we set the flag that it is being destroyed, and wait for it to die.
	// In the end, we free the mutex instead of unlocking it, so that every locking
	// operation fails, so that the other threads know the pool is dead.
	// every other function has to check if the mutex can actually be locked, if not,
	// it needs to leave the pool alone
	if (!Sys_Mutex_Lock(pool->mutex))
		return;
	if (pool->quit)
		return;
	pool->quit = true; // stop accepting jobs
	pool->maxThreads = 0;
	pool->maxQueued = 0;
	Sys_Mutex_Unlock(pool->mutex);

	Sys_Semaphore_Wait(pool->semEmpty, true);

	Sys_Mutex_Lock(pool->mutex);
	nThreads = Mem_ExpandableArray_IndexRange(&pool->threads);
	for (i = 0; i < nThreads; ++i) {
		_sys_poolthread_t *t = (_sys_poolthread_t*) Mem_ExpandableArray_RecordAtIndex(&pool->threads, i);
		t->quit = true;
		Sys_Mutex_Unlock(pool->mutex);
		if (kill)
			Thread_Cancel(t->thread);
		else
			Sys_Thread_Join(t->thread);
		_Sys_PoolThread_Free(t);
		Sys_Mutex_Lock(pool->mutex);
	}
	Sys_Mutex_Free(pool->mutex);
	Mem_ExpandableArray_FreeArray(&pool->threads);
	Mem_ExpandableArray_FreeArray(&pool->queue);
	
	// Now die:
	Sys_ThreadMem_Free(pool);
}
