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

static void *_Sys_ThreadMem_Free(size_t size)
{
	return Mem_Free(threadmempool, size);
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

void SDL_Mutex_Free (sys_mutex_t *mutex)
{
	if (!threads_available)
		return;
	SDL_DestroyMutex( (SDL_mutex*)mutex );
}

typedef struct {
	sys_mutex_t          *mutex;
	int                   maxThreads;
	int                   maxQueued;
	memexpandablearray_t  threads;
	memexpandablearray_t  queue;
} _sys_threadpool_t;

typedef SDL_thread *_sys_thread_t;

typedef struct {
	SDL_thread        *thread;
	_sys_threadpool_t *pool;
	SDL_sem           *semRun; // increased whenever the thread can run again
	SDL_sem           *semQueue; // increased as soon as the thread accepts a new job - AFTER the function is executed
	qboolean           quit;
	sys_threadentry_t *nextEntry;
	void              *nextUserdata;
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
		SDL_SemWait(self->semRun);
		if (!nextEntry)
			continue;
		entry = self->nextEntry;
		data = self->data;
		self->nextEntry = NULL;
		(*entry)(data);
		SDL_SemPost(self->semQueue);
	}
}

static qboolean ThreadPool_Try(_sys_poolthread_t *thread, sys_threadentry_t *entry, void *data)
{
	if (SDL_SemTryWait(thread->semQueue) == SDL_MUTEX_TIMEOUT)
		return false;
	thread->nextEntry = entry;
	thread->nextData = data;
	SDL_SemPost(thread->semRun);
	return true;
}

static _sys_poolthread_t *_Sys_PoolThread_New (_sys_threadpool_t *pool)
{
	_sys_poolthread_t *thread = _Sys_ThreadMem_Alloc(sizeof(_sys_poolthread_t));
	thread->pool = pool;
	thread->semQueue = SDL_CreateSemaphore(1);
	thread->semRun = SDL_CreateSemaphore(0);
	thread->quit = false;
	thread->nextEntry = NULL;
	thread->nextUserdata = NULL;
	thread->thread = SDL_CreateThread((sys_threadentry_t*)&ThreadPool_Entry, (void*)thread);
	return thread;
}

sys_threadpool_t *Sys_ThreadPool_New (int min, int max, int queueMax)
{
	_sys_threadpool_t *pool;
	if (!threads_available)
		return NULL;
	if (!Sys_ThreadMem_Lock())
		return NULL;

	pool = _Sys_ThreadMem_Alloc(sizeof(_sys_threadpool_t));
	if (!pool)
		return NULL;
	memset(pool, 0, sizeof(pool));
	pool->maxThreads = max;
	pool->maxQueued = queueMax;

	if (!(pool->mutex = Sys_Mutex_New()))
		goto error;

	Mem_ExpandableArray_NewArray(&pool->threads, threadmempool, sizeof(_sys_poolthread_t), 16);
	Mem_ExpandableArray_NewArray(&pool->queue, threadmempool, sizeof(_sys_queueentry_t), 16);

	for (; min > 0; --min) {
		_sys_thread_t *th;
		th = Mem_ExpandableArray_AllocRecord(&pool->threads);
		*th = _Sys_PoolThread_New(pool);
	}
	
	Sys_ThreadMem_Unlock();
	return pool;

error:
	if (pool->mutex)
		_Sys_ThreadMem_Free(pool->mutex);
	_Sys_ThreadMem_Free(pool);
	Sys_ThreadMem_Unlock();
	return NULL;
}
