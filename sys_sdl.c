#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#ifndef __WIN32__
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#endif

#include "quakedef.h"

qboolean			isDedicated;

int noconinput = 0;


#if defined (USERPREF_DIR)
char *prefdir=  ".quake";
#endif

#if defined (BASEDIR)
/* - DC - won't work without this  */
#define macro_string(x) _macro_string(x)
#define _macro_string(x) #x
char *basedir = macro_string(BASEDIR);
#undef macro_string
#undef _macro_string
#else
char *basedir = ".";
#endif

char *cachedir = "/tmp";

cvar_t  sys_linerefresh = {"sys_linerefresh","0"};// set for entity display
cvar_t  sys_nostdout = {"sys_nostdout","0"};

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int y, int val)
{
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[4096];
	
	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);
	fprintf(stderr, "%s", text);
	
	//Con_Print (text);
}

void Sys_Quit (void)
{
	Host_Shutdown();
	exit(0);
}

void Sys_Init(void)
{
#if id386
	Sys_SetFPCW();
#endif
}

#if !id386

/*
================
Sys_LowFPPrecision
================
*/
void Sys_LowFPPrecision (void)
{
// causes weird problems on Nextstep
}


/*
================
Sys_HighFPPrecision
================
*/
void Sys_HighFPPrecision (void)
{
// causes weird problems on Nextstep
}

#endif	// !id386


void Sys_Error (char *error, ...)
{ 
    va_list     argptr;
    char        string[4096];

    va_start (argptr,error);
    vsprintf (string,error,argptr);
    va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	Host_Shutdown ();
	exit (1);

} 

void Sys_Warn (char *warning, ...)
{ 
    va_list     argptr;
    char        string[4096];
    
    va_start (argptr,warning);
    vsprintf (string,warning,argptr);
    va_end (argptr);
	fprintf(stderr, "Warning: %s", string);
} 

/*
===============================================================================

FILE IO

===============================================================================
*/

#define	MAX_HANDLES		10
FILE	*sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
Qfilelength
================
*/
static int Qfilelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE	*f;
	int		i;
	
	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;
	
	return Qfilelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int		i;
	
	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;
	
	return i;
}

void Sys_FileClose (int handle)
{
	if ( handle >= 0 ) {
		fclose (sys_handles[handle]);
		sys_handles[handle] = NULL;
	}
}

void Sys_FileSeek (int handle, int position)
{
	if ( handle >= 0 ) {
		fseek (sys_handles[handle], position, SEEK_SET);
	}
}

int Sys_FileRead (int handle, void *dst, int count)
{
	char *data;
	int size, done;

	size = 0;
	if ( handle >= 0 ) {
		data = dst;
		while ( count > 0 ) {
			done = fread (data, 1, count, sys_handles[handle]);
			if ( done == 0 ) {
				break;
			}
			data += done;
			count -= done;
			size += done;
		}
	}
	return size;
		
}

int Sys_FileWrite (int handle, void *src, int count)
{
	char *data;
	int size, done;

	size = 0;
	if ( handle >= 0 ) {
		data = src;
		while ( count > 0 ) {
			done = fread (data, 1, count, sys_handles[handle]);
			if ( done == 0 ) {
				break;
			}
			data += done;
			count -= done;
			size += done;
		}
	}
	return size;
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	
	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}
	
	return -1;
}

void Sys_mkdir (char *path)
{
#ifdef __WIN32__
    mkdir (path);
#else
    mkdir (path, 0777);
#endif
}


void Sys_Strtime(char *buf)
{
  struct tm *tm_;
  static time_t t_;
  
  time(&t_);
  tm_=gmtime(&t_);
  
  sprintf(buf,"%02d:%02d:%02d",tm_->tm_hour,tm_->tm_min,tm_->tm_sec);
  
}

// directory entry list internal data
// POSIX stuff...
// FIX-ME : is there a more portable way to do that ?
#include <glob.h>

typedef struct {
  glob_t globbuf;
  size_t count;
} uxdirdata_t;

static uxdirdata_t uxdata;


dirdata_t *Sys_Findfirst (char *dir, char *filter, dirdata_t *dirdata)
{
  char dirfilter[MAX_OSPATH];
  if (!filter || !dirdata)
    return NULL;
  sprintf(dirfilter,"%s/%s", dir, filter);
  glob(dirfilter,0,NULL,&uxdata.globbuf);
  if (uxdata.globbuf.gl_pathc){
    dirdata->internal=&uxdata;
    strncpy(dirdata->entry,uxdata.globbuf.gl_pathv[0],sizeof(dirdata->entry));
    uxdata.count=0;
    return dirdata;
  }
  return NULL;
}

dirdata_t *Sys_Findnext (dirdata_t *dirdata)
{
  uxdirdata_t *uxdata;
  if (dirdata){
    uxdata=dirdata->internal;
    uxdata->count++;
    // next entry ?
    if (uxdata->count<uxdata->globbuf.gl_pathc){
      strncpy(dirdata->entry,uxdata->globbuf.gl_pathv[uxdata->count],sizeof(dirdata->entry));
      return dirdata;
    }
    // no -> close
    globfree(&uxdata->globbuf);
    dirdata->internal=NULL;
  }
  return NULL;
}


void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr; 
    static char data[4096];
    FILE *fp;
    
    va_start(argptr, fmt);
    vsprintf(data, fmt, argptr);
    va_end(argptr);
    fp = fopen(file, "a");
    fwrite(data, strlen(data), 1, fp);
    fclose(fp);
}

double Sys_FloatTime (void)
{
#ifdef __WIN32__

	static int starttime = 0;

	if ( ! starttime )
		starttime = clock();

	return (clock()-starttime)*1.0/1024;

#else

    struct timeval tp;
    struct timezone tzp; 
    static int      secbase; 
    
    gettimeofday(&tp, &tzp);  

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec/1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;

#endif
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

static volatile int oktogo;

void alarm_handler(int x)
{
	oktogo=1;
}

byte *Sys_ZoneBase (int *size)
{

	char *QUAKEOPT = getenv("QUAKEOPT");

	*size = 0xc00000;
	if (QUAKEOPT)
	{
		while (*QUAKEOPT)
			if (tolower(*QUAKEOPT++) == 'm')
			{
				*size = atof(QUAKEOPT) * 1024*1024;
				break;
			}
	}
	return malloc (*size);

}

void Sys_LineRefresh(void)
{
}

void Sys_Sleep(void)
{
	SDL_Delay(1);
}

void floating_point_exception_handler(int whatever)
{
//	Sys_Warn("floating point exception\n");
	signal(SIGFPE, floating_point_exception_handler);
}

void moncontrol(int x)
{
}


// - DC - create user pref directory
#if defined (USERPREF_DIR)
char *Sys_InitUserDir(void)
{
  char *home;
  char *userdir;
  struct stat stat_buf;

  home = getenv("HOME");
  if (!home){
    Sys_Error("Environment variable HOME not found.\n");
  }
  userdir = (char *)malloc(strlen(home)+strlen(prefdir)+1);
  strcpy(userdir,home);
  if (home[strlen(home)-1]!='/')
    strcat(userdir,"/");

  if (stat(userdir,&stat_buf)){
    Sys_Error("could not stat %s\n",userdir);
  }
  
  strcat(userdir,prefdir);
  if (stat(userdir,&stat_buf)){
    if (errno == ENOENT)
      Sys_mkdir(userdir);
    else
      Sys_Error("error while stating %s\n",userdir);  
  }
  if (!S_ISDIR(stat_buf.st_mode))
    Sys_Error("%s is not a directory\n",userdir);

  return userdir;
}
#endif


int main (int c, char **v)
{

	double		time, oldtime, newtime;
	quakeparms_t parms;
	extern int vcrFile;
	extern int recording;
	static int frame;
	int mb_mem_size=50;
	int j;
	moncontrol(0);

//	signal(SIGFPE, floating_point_exception_handler);
	signal(SIGFPE, SIG_IGN);


	COM_InitArgv(c, v);
	parms.argc = com_argc;
	parms.argv = com_argv;


	j = COM_CheckParm ("-heapsize");
	if (j)
		mb_mem_size =
			(int) (Q_atof (com_argv[j + 1]));

	parms.memsize = mb_mem_size*1024*1024;
	parms.membase = malloc (parms.memsize);
	// not enough memory !!
	if (!parms.membase){
	  Sys_Error("Not enough memory - asked for %d - change with -heapsize <value in Mb> \n",mb_mem_size);
	}

#if defined (USERPREF_DIR)
	parms.userdir = Sys_InitUserDir ();
#endif
	parms.basedir = basedir;

	// caching is disabled by default, use -cachedir to enable
	parms.cachedir = "";

	Sys_Init();

	Host_Init(&parms);

	Cvar_RegisterVariable (&sys_nostdout);

    oldtime = Sys_FloatTime () - 0.1;
    while (1)
    {
// find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;

        if (cls.state == ca_dedicated)
        {   // play vcrfiles at max speed
            if (time < sys_ticrate.value && (vcrFile == -1 || recording) )
            {
                SDL_Delay (1);
                continue;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }

        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        if (++frame > 10)
            moncontrol(1);      // profile only while we do each Quake frame
        Host_Frame (time);
        moncontrol(0);

// graphic debugging aids
        if (sys_linerefresh.value)
            Sys_LineRefresh ();
    }

}


/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{

	int r;
	unsigned long addr;
	int psize = getpagesize();

	fprintf(stderr, "writable code %lx-%lx\n", startaddr, startaddr+length);

	addr = startaddr & ~(psize-1);

	r = mprotect((char*)addr, length + startaddr - addr, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");

}

