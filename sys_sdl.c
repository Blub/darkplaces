#include "quakedef.h"

#ifdef WIN32
#include <io.h>
#include "conio.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#endif

#include <signal.h>

#include <SDL.h>

// =======================================================================
// General routines
// =======================================================================

void Sys_Shutdown (void)
{
#ifndef WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif
	fflush(stdout);
	SDL_Quit();
}


void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];

// change stdin to non blocking
#ifndef WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif

	va_start (argptr,error);
	dpvsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);

	Con_Printf ("Quake Error: %s\n", string);

	Host_Shutdown ();
	exit (1);
}

void Sys_PrintToTerminal(const char *text)
{
#ifndef WIN32
	// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
	int origflags = fcntl (1, F_GETFL, 0);
	fcntl (1, F_SETFL, origflags & ~FNDELAY);
#else
#define write _write
#endif
	while(*text)
	{
		int written = (int)write(1, text, (int)strlen(text));
		if(written <= 0)
			break; // sorry, I cannot do anything about this error - without an output
		text += written;
	}
#ifndef WIN32
	fcntl (1, F_SETFL, origflags);
#endif
	//fprintf(stdout, "%s", text);
}

double Sys_DoubleTime (void)
{
	static int first = true;
	static double oldtime = 0.0, curtime = 0.0;
	double newtime;
	newtime = (double) SDL_GetTicks() / 1000.0;


	if (first)
	{
		first = false;
		oldtime = newtime;
	}

	if (newtime < oldtime)
	{
		// warn if it's significant
		if (newtime - oldtime < -0.01)
			Con_Printf("Sys_DoubleTime: time stepped backwards (went from %f to %f, difference %f)\n", oldtime, newtime, newtime - oldtime);
	}
	else if (newtime > oldtime + 1800)
	{
		Con_Printf("Sys_DoubleTime: time stepped forward (went from %f to %f, difference %f)\n", oldtime, newtime, newtime - oldtime);
	}
	else
		curtime += newtime - oldtime;
	oldtime = newtime;

	return curtime;
}

char *Sys_ConsoleInput(void)
{
	if (cls.state == ca_dedicated)
	{
		static char text[MAX_INPUTLINE];
		int len = 0;
#ifdef WIN32
		int c;

		// read a line out
		while (_kbhit ())
		{
			c = _getch ();
			_putch (c);
			if (c == '\r')
			{
				text[len] = 0;
				_putch ('\n');
				len = 0;
				return text;
			}
			if (c == 8)
			{
				if (len)
				{
					_putch (' ');
					_putch (c);
					len--;
					text[len] = 0;
				}
				continue;
			}
			text[len] = c;
			len++;
			text[len] = 0;
			if (len == sizeof (text))
				len = 0;
		}
#else
		fd_set fdset;
		struct timeval timeout;
		FD_ZERO(&fdset);
		FD_SET(0, &fdset); // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (select (1, &fdset, NULL, NULL, &timeout) != -1 && FD_ISSET(0, &fdset))
		{
			len = read (0, text, sizeof(text));
			if (len >= 1)
			{
				// rip off the \n and terminate
				text[len-1] = 0;
				return text;
			}
		}
#endif
	}
	return NULL;
}

void Sys_Sleep(int microseconds)
{
	SDL_Delay(microseconds / 1000);
}

char *Sys_GetClipboardData (void)
{
#ifdef WIN32
	char *data = NULL;
	char *cliptext;

	if (OpenClipboard (NULL) != 0)
	{
		HANDLE hClipboardData;

		if ((hClipboardData = GetClipboardData (CF_TEXT)) != 0)
		{
			if ((cliptext = (char *)GlobalLock (hClipboardData)) != 0)
			{
				size_t allocsize;
				allocsize = GlobalSize (hClipboardData) + 1;
				data = (char *)Z_Malloc (allocsize);
				strlcpy (data, cliptext, allocsize);
				GlobalUnlock (hClipboardData);
			}
		}
		CloseClipboard ();
	}
	return data;
#else
	return NULL;
#endif
}

void Sys_InitConsole (void)
{
}

void Sys_Init_Commands (void)
{
}

int main (int argc, char *argv[])
{
	signal(SIGFPE, SIG_IGN);

	com_argc = argc;
	com_argv = (const char **)argv;

#ifndef WIN32
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif

	// we don't know which systems we'll want to init, yet...
	SDL_Init(0);

	Host_Main();

	return 0;
}

#include "zone.h"
#include <SDL/SDL_thread.h>

static SDL_mutex *mem_mutex = NULL;
static qboolean threads_available = false;

qboolean Sys_InitThreads (void)
{
	mem_mutex = SDL_CreateMutex();
	if (!mem_mutex)
		return false;

	threads_available = true;
	return true;
}

qboolean Sys_ThreadMem_Lock (void)
{
	return (SDL_mutexP(mem_mutex) != 0);
}

qboolean Sys_ThreadMem_Unlock (void)
{
	return (SDL_mutexV(mem_mutex) != 0);
}

static void *_Sys_ThreadMem_Alloc(size_t size)
{
	return Mem_Alloc(threadmempool, size);
}

void *Sys_ThreadMem_Alloc (size_t size)
{
	void *data;
	if (!threads_available)
		return NULL;
	Sys_ThreadMem_Lock();
	data = _Sys_ThreadMem_Alloc(size);
	Sys_ThreadMem_Unlock();
	return data;
}

static void _Sys_ThreadMem_Free (void *data)
{
	Mem_Free(data);
}

void Sys_ThreadMem_Free (void *data)
{
	if (threads_available)
		Sys_ThreadMem_Lock();
	_Sys_ThreadMem_Free(data);
	if (threads_available)
		Sys_ThreadMem_Unlock();
}

sys_mutex_t *Sys_Mutex_New (void)
{
	if (!threads_available)
		return NULL;
	return (sys_mutex_t*)SDL_CreateMutex();
}

qboolean Sys_Mutex_Lock (sys_mutex_t *mutex)
{
	if (!threads_available)
		return false;
	return (SDL_mutexP((SDL_mutex*)mutex) == 0);
}

qboolean Sys_Mutex_Unlock (sys_mutex_t *mutex)
{
	if (!threads_available)
		return false;
	return (SDL_mutexV((SDL_mutex*)mutex) == 0);
}

void Sys_Mutex_Free (sys_mutex_t *mutex)
{
	if (!threads_available)
		return;
	SDL_DestroyMutex( (SDL_mutex*)mutex );
}

sys_semaphore_t *Sys_Semaphore_New (int value)
{
	return (sys_semaphore_t*)SDL_CreateSemaphore(value);
}

void Sys_Semaphore_Free (sys_semaphore_t *sem)
{
	SDL_DestroySemaphore((SDL_sem*)sem);
}

qboolean Sys_Semaphore_Wait (sys_semaphore_t *sem, qboolean block)
{
	if (block)
		return (SDL_SemWait((SDL_sem*)sem) == 0);
	else
		return (SDL_SemTryWait((SDL_sem*)sem) == 0);
}

qboolean Sys_Semaphore_Post (sys_semaphore_t *sem)
{
	return (SDL_SemPost((SDL_sem*)sem) == 0);
}

int Sys_Semaphore_Value (sys_semaphore_t *sem)
{
	return SDL_SemValue((SDL_sem*)sem);
}

typedef SDL_Thread _sys_thread_t;

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

sys_thread_t *Sys_Thread_New(sys_threadentry_t *entry, void *userdata)
{
	return (sys_thread_t*)SDL_CreateThread((int (*)(void*))entry, userdata);
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
	if (!threads_available)
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

int Sys_Thread_Join (sys_thread_t *thread)
{
	int status;
	SDL_WaitThread((SDL_Thread*)thread, &status);
	return status;
}

void Thread_Cancel (sys_thread_t *thread)
{
	SDL_KillThread((SDL_Thread*)thread);
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

/*! Spawn a thread in a specified threadpool. If block is true, and the job-queue isn't maxed out, the function will block until
 * the function is actually executed. If the queue is maxed out nothing happens and 0 is returned.
 * It returns 0 on error, 1 if the job is queued successfully, and 2 if the job is also executed immediately.

int               *Sys_Thread_Spawn (sys_threadpool_t*, sys_threadentry_t*, void *userdata, qboolean block)
{
}
*/
