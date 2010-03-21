#include "quakedef.h"

#define SUPPORTDLL

#ifdef WIN32
# include <windows.h>
# include <mmsystem.h> // timeGetTime
# include <time.h> // localtime
#else
# include <unistd.h>
# include <fcntl.h>
# include <sys/time.h>
# include <time.h>
# ifdef SUPPORTDLL
#  include <dlfcn.h>
# endif
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

typedef struct _threadlist_s {
	struct _sys_poolthread_s *thread;
	struct _threadlist_s *next;
} _threadlist_t;

typedef struct _joblist_s {
	sys_threadentry_t *entry;
	void *data;
	struct _joblist_s *next;
} _joblist_t;

typedef struct {
	sys_mutex_t          *mutex;
	qboolean              quit;
	int                   maxThreads;
	int                   maxQueued;
	sys_semaphore_t       jobs; // increased whenever a job is added
	sys_semaphore_t       available; // # of idle threads
	sys_semaphore_t       empty; // 1 if empty

	size_t                threadcount;
	_threadlist_t        *threadlist;
	size_t                jobcount;
	_joblist_t           *joblist;
	_joblist_t           *jobend;
} _sys_threadpool_t;

typedef struct _sys_poolthread_s {
	sys_thread_t      *thread;
	_sys_threadpool_t *pool;
	qboolean           quit;
} _sys_poolthread_t;

typedef struct {
	sys_threadentry_t *entry;
	void *userdata;
} _sys_queueentry_t;

static qboolean _Sys_ThreadPool_GetJob(_sys_threadpool_t *pool, sys_threadentry_t **entry, void **data)
{
	_joblist_t *job;
	if (!pool->jobcount)
		return false;
	pool->jobcount--;
	job = pool->joblist;
	pool->joblist = pool->joblist->next;
	if (pool->jobend == job)
		pool->jobend = pool->joblist;
	
	*entry = job->entry;
	*data = job->data;
	Sys_ThreadMem_Free(job);
	return true;
}

static qboolean _Sys_ThreadPool_AddJob(_sys_threadpool_t *pool, sys_threadentry_t *entry, void *data)
{
	_joblist_t *job = (_joblist_t*)Sys_ThreadMem_Alloc(sizeof(_joblist_t));
	if (!job)
		return false;
	job->next = NULL;
	job->entry = entry;
	job->data = data;
	if (!pool->joblist)
		pool->joblist = pool->jobend = job;
	else {
		pool->jobend->next = job;
		pool->jobend = job;
	}
	pool->jobcount++;
	return true;
}

static int ThreadPool_Entry(_sys_poolthread_t *self)
{
	sys_threadentry_t *entry;
	void *data;
	Sys_Semaphore_Post(self->pool->available);
	while (!self->quit) {
		if (!Sys_Semaphore_Wait(self->pool->jobs, true))
			return 1;
		if (!Sys_Mutex_Lock(self->pool->mutex))
			return 1;
		if (!_Sys_ThreadPool_GetJob(self->pool, &entry, &data)) {
			Sys_Mutex_Unlock(self->pool->mutex);
			return 1;
		}
		Sys_Mutex_Unlock(self->pool->mutex);
		if (entry)
		{
			Sys_Semaphore_Wait(self->pool->available, true);
			(*entry)(data);
			Sys_Semaphore_Post(self->pool->available);
		}
	}
	return 0;
}

/*
static qboolean ThreadPool_Try(_sys_poolthread_t *thread, sys_threadentry_t *entry, void *data)
{
	if (!Sys_Semaphore_Wait(thread->semQueue, false))
		return false;
	thread->nextEntry = entry;
	thread->nextUserdata = data;
	Sys_Semaphore_Post(thread->semRun);
	return true;
}
*/

static _sys_poolthread_t *_Sys_PoolThread_New (_sys_threadpool_t *pool)
{
	_sys_poolthread_t *thread = _Sys_ThreadMem_Alloc(sizeof(_sys_poolthread_t));
	thread->pool = pool;
	thread->quit = false;
	thread->thread = Sys_Thread_New((sys_threadentry_t*)&ThreadPool_Entry, (void*)thread);
	return thread;
}

sys_threadpool_t *Sys_ThreadPool_New (int min, int max, int queueMax)
{
	_threadlist_t *tlend;
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
	if (!(pool->mutex = Sys_Mutex_New()))
		goto error;
	pool->quit = false;
	pool->maxThreads = max;
	pool->maxQueued = queueMax;
	pool->jobs = Sys_Semaphore_New(0);
	pool->available = Sys_Semaphore_New(0);
	pool->empty = Sys_Semaphore_New(1);

	pool->threadlist = tlend = (_threadlist_t*)_Sys_ThreadMem_Alloc(sizeof(_threadlist_t));
	pool->threadlist->thread = _Sys_PoolThread_New(pool);
	pool->threadlist->next = NULL;
	for (--min; min > 0; --min) {
		_threadlist_t *list = (_threadlist_t*)_Sys_ThreadMem_Alloc(sizeof(_threadlist_t));
		list->thread = _Sys_PoolThread_New(pool);
		list->next = NULL;
		tlend->next = list;
		tlend = list;
	}
	
	Sys_ThreadMem_Unlock();
	return (sys_threadpool_t*)pool;

error:
	_Sys_ThreadMem_Free(pool);
	Sys_ThreadMem_Unlock();
	return NULL;
}

void Sys_ThreadPool_Join (sys_threadpool_t *_pool, qboolean kill)
{
	_sys_threadpool_t *pool = (_sys_threadpool_t*)_pool;
	_threadlist_t *tlist;
	_joblist_t *jlist;
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

	Sys_Semaphore_Wait(pool->empty, true);

	Sys_Mutex_Lock(pool->mutex);
	for (tlist = pool->threadlist; tlist != NULL; tlist = tlist->next) {
		_sys_poolthread_t *t = tlist->thread;
		t->quit = true;
		Sys_Mutex_Unlock(pool->mutex);
		if (kill)
			Thread_Cancel(t->thread);
		else
			Sys_Thread_Join(t->thread);
		Sys_Mutex_Lock(pool->mutex);
	}
	Sys_Mutex_Free(pool->mutex);

	Sys_ThreadMem_Lock();
	for (tlist = pool->threadlist; tlist != NULL; tlist = tlist->next)
		_Sys_ThreadMem_Free(tlist);
	
	pool->threadlist = NULL;
	if (pool->joblist) {
		Con_Print("^1ERROR: Threadpool contains jobs after being destroyed!\n");
		for (jlist = pool->joblist; jlist != NULL; jlist = jlist->next)
			_Sys_ThreadMem_Free(jlist);
	}
	Sys_ThreadMem_Unlock();
	Sys_Semaphore_Free(pool->available);
	Sys_Semaphore_Free(pool->empty);
	Sys_Semaphore_Free(pool->jobs);
	
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

#ifdef WIN32
# define HAVE_TIMEGETTIME 1
# define HAVE_QUERYPERFORMANCECOUNTER 1
# define HAVE_Sleep 1
#endif

#if defined(CLOCK_MONOTONIC) || defined(CLOCK_HIRES)
# define HAVE_CLOCKGETTIME 1
#endif

#ifndef WIN32
// FIXME improve this check, manpage hints to DST_NONE
# define HAVE_GETTIMEOFDAY 1
#endif

#ifdef FD_SET
# define HAVE_SELECT 1
#endif

#ifndef WIN32
// FIXME improve this check
# define HAVE_USLEEP 1
#endif

// this one is referenced elsewhere
cvar_t sys_usenoclockbutbenchmark = {CVAR_SAVE, "sys_usenoclockbutbenchmark", "0", "don't use ANY real timing, and simulate a clock (for benchmarking); the game then runs as fast as possible. Run a QC mod with bots that does some stuff, then does a quit at the end, to benchmark a server. NEVER do this on a public server."};

// these are not
static cvar_t sys_debugsleep = {0, "sys_debugsleep", "0", "write requested and attained sleep times to standard output, to be used with gnuplot"};
static cvar_t sys_usesdlgetticks = {CVAR_SAVE, "sys_usesdlgetticks", "0", "use SDL_GetTicks() timer (less accurate, for debugging)"};
static cvar_t sys_usesdldelay = {CVAR_SAVE, "sys_usesdldelay", "0", "use SDL_Delay() (less accurate, for debugging)"};
#if HAVE_QUERYPERFORMANCECOUNTER
static cvar_t sys_usequeryperformancecounter = {CVAR_SAVE, "sys_usequeryperformancecounter", "0", "use windows QueryPerformanceCounter timer (which has issues on multicore/multiprocessor machines and processors which are designed to conserve power) for timing rather than timeGetTime function (which has issues on some motherboards)"};
#endif
#if HAVE_CLOCKGETTIME
static cvar_t sys_useclockgettime = {CVAR_SAVE, "sys_useclockgettime", "0", "use POSIX clock_gettime function (which has issues if the system clock speed is far off, as it can't get fixed by NTP) for timing rather than gettimeofday (which has issues if the system time is stepped by ntpdate, or apparently on some Xen installations)"};
#endif

static unsigned long benchmark_time;

void Sys_Init_Commands (void)
{
	Cvar_RegisterVariable(&sys_debugsleep);
	Cvar_RegisterVariable(&sys_usenoclockbutbenchmark);
#if HAVE_TIMEGETTIME || HAVE_QUERYPERFORMANCECOUNTER || HAVE_CLOCKGETTIME || HAVE_GETTIMEOFDAY
	if(sys_supportsdlgetticks)
	{
		Cvar_RegisterVariable(&sys_usesdlgetticks);
		Cvar_RegisterVariable(&sys_usesdldelay);
	}
#endif
#if HAVE_QUERYPERFORMANCECOUNTER
	Cvar_RegisterVariable(&sys_usequeryperformancecounter);
#endif
#if HAVE_CLOCKGETTIME
	Cvar_RegisterVariable(&sys_useclockgettime);
#endif
}

double Sys_DoubleTime(void)
{
	static int first = true;
	static double oldtime = 0.0, curtime = 0.0;
	double newtime;
	if(sys_usenoclockbutbenchmark.integer)
	{
		benchmark_time += 1;
		return ((double) benchmark_time) / 1e6;
	}

	// first all the OPTIONAL timers

#if HAVE_QUERYPERFORMANCECOUNTER
	else if (sys_usequeryperformancecounter.integer)
	{
		// LordHavoc: note to people modifying this code, DWORD is specifically defined as an unsigned 32bit number, therefore the 65536.0 * 65536.0 is fine.
		// QueryPerformanceCounter
		// platform:
		// Windows 95/98/ME/NT/2000/XP
		// features:
		// very accurate (CPU cycles)
		// known issues:
		// does not necessarily match realtime too well (tends to get faster and faster in win98)
		// wraps around occasionally on some platforms (depends on CPU speed and probably other unknown factors)
		double timescale;
		LARGE_INTEGER PerformanceFreq;
		LARGE_INTEGER PerformanceCount;

		if (!QueryPerformanceFrequency (&PerformanceFreq))
		{
			Con_Printf ("No hardware timer available\n");
			// fall back to timeGetTime
			Cvar_SetValueQuick(&sys_usequeryperformancecounter, false);
			return Sys_DoubleTime();
		}
		QueryPerformanceCounter (&PerformanceCount);

		#ifdef __BORLANDC__
		timescale = 1.0 / ((double) PerformanceFreq.u.LowPart + (double) PerformanceFreq.u.HighPart * 65536.0 * 65536.0);
		newtime = ((double) PerformanceCount.u.LowPart + (double) PerformanceCount.u.HighPart * 65536.0 * 65536.0) * timescale;
		#else
		timescale = 1.0 / ((double) PerformanceFreq.LowPart + (double) PerformanceFreq.HighPart * 65536.0 * 65536.0);
		newtime = ((double) PerformanceCount.LowPart + (double) PerformanceCount.HighPart * 65536.0 * 65536.0) * timescale;
		#endif
	}
#endif

#if HAVE_CLOCKGETTIME
	else if (sys_useclockgettime.integer)
	{
		struct timespec ts;
#  ifdef CLOCK_MONOTONIC
		// linux
		clock_gettime(CLOCK_MONOTONIC, &ts);
#  else
		// sunos
		clock_gettime(CLOCK_HIGHRES, &ts);
#  endif
		newtime = (double) ts.tv_sec + ts.tv_nsec / 1000000000.0;
	}
#endif

	// now all the FALLBACK timers
	else if(sys_supportsdlgetticks && sys_usesdlgetticks.integer)
	{
		newtime = (double) Sys_SDL_GetTicks() / 1000.0;
	}
#if HAVE_GETTIMEOFDAY
	else
	{
		struct timeval tp;
		gettimeofday(&tp, NULL);
		newtime = (double) tp.tv_sec + tp.tv_usec / 1000000.0;
	}
#elif HAVE_TIMEGETTIME
	else
	{
		static int firsttimegettime = true;
		// timeGetTime
		// platform:
		// Windows 95/98/ME/NT/2000/XP
		// features:
		// reasonable accuracy (millisecond)
		// issues:
		// wraps around every 47 days or so (but this is non-fatal to us, odd times are rejected, only causes a one frame stutter)

		// make sure the timer is high precision, otherwise different versions of windows have varying accuracy
		if (firsttimegettime)
		{
			timeBeginPeriod (1);
			firsttimegettime = false;
		}

		newtime = (double) timeGetTime () / 1000.0;
	}
#else
	// fallback for using the SDL timer if no other timer is available
	else
	{
		newtime = (double) Sys_SDL_GetTicks() / 1000.0;
		// this calls Sys_Error() if not linking against SDL
	}
#endif

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

void Sys_Sleep(int microseconds)
{
	double t = 0;
	if(sys_usenoclockbutbenchmark.integer)
	{
		benchmark_time += microseconds;
		return;
	}
	if(sys_debugsleep.integer)
	{
		t = Sys_DoubleTime();
	}
	if(sys_supportsdlgetticks && sys_usesdldelay.integer)
	{
		Sys_SDL_Delay(microseconds / 1000);
	}
#if HAVE_SELECT
	else
	{
		struct timeval tv;
		tv.tv_sec = microseconds / 1000000;
		tv.tv_usec = microseconds % 1000000;
		select(0, NULL, NULL, NULL, &tv);
	}
#elif HAVE_USLEEP
	else
	{
		usleep(microseconds);
	}
#elif HAVE_Sleep
	else
	{
		Sleep(microseconds / 1000);
	}
#else
	else
	{
		Sys_SDL_Delay(microseconds / 1000);
	}
#endif
	if(sys_debugsleep.integer)
	{
		t = Sys_DoubleTime() - t;
		printf("%d %d # debugsleep\n", microseconds, (unsigned int)(t * 1000000));
	}
}

const char *Sys_FindInPATH(const char *name, char namesep, const char *PATH, char pathsep, char *buf, size_t bufsize)
{
	const char *p = PATH;
	const char *q;
	if(p && name)
	{
		while((q = strchr(p, ':')))
		{
			dpsnprintf(buf, bufsize, "%.*s%c%s", (int)(q-p), p, namesep, name);
			if(FS_SysFileExists(buf))
				return buf;
			p = q + 1;
		}
		if(!q) // none found - try the last item
		{
			dpsnprintf(buf, bufsize, "%s%c%s", p, namesep, name);
			if(FS_SysFileExists(buf))
				return buf;
		}
	}
	return name;
}

const char *Sys_FindExecutableName(void)
{
#if defined(WIN32)
	return com_argv[0];
#else
	static char exenamebuf[MAX_OSPATH+1];
	ssize_t n = -1;
#if defined(__FreeBSD__)
	n = readlink("/proc/curproc/file", exenamebuf, sizeof(exenamebuf)-1);
#elif defined(__linux__)
	n = readlink("/proc/self/exe", exenamebuf, sizeof(exenamebuf)-1);
#endif
	if(n > 0 && (size_t)(n) < sizeof(exenamebuf))
	{
		exenamebuf[n] = 0;
		return exenamebuf;
	}
	if(strchr(com_argv[0], '/'))
		return com_argv[0]; // possibly a relative path
	else
		return Sys_FindInPATH(com_argv[0], '/', getenv("PATH"), ':', exenamebuf, sizeof(exenamebuf));
#endif
}

void Sys_ProvideSelfFD(void)
{
	if(com_selffd != -1)
		return;
	com_selffd = FS_SysOpenFD(Sys_FindExecutableName(), "rb", false);
}
