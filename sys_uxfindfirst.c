/*
Copyright (C) 2003 Tenebrae Team

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
   Sys_Find* implementation for UNIX like systems
   system conformance :

   GLIBC < 2.2 based system

       ISO/IEC 9945-2 (fnmatch)
       BSD 4.3        (dirent syscalls)

   OTHER :
  
       POSIX.2        (glob)

*/

/* glibc < 2.3 */

#include "quakedef.h"
#include <dirent.h>
#include <fnmatch.h>

#if defined(__GLIBC__) && !(__GLIBC_PREREQ(2,3))

#define UXDATA_GRANULARITY 10

typedef struct {
	size_t count;
	size_t lsize;
	char **list;
} uxdirdata_t;

int uxdata_free (uxdata_t *ud)
{
	int i;
	// elements	
	for (i=0;i<ud->lsize;i++)
		Z_Free (ud->list[i]);
	// table
	if (ud->lsize % UXDATA_GRANULARITY)
		ud->lsize += UXDATA_GRANULARITY - (ud->lsize % UXDATA_GRANULARITY);
	Z_Free (ud->list);
}


int direntp_compare (struct dirent **p1, struct dirent **p2)
{
	return strncmp ((*p1)->d_name, (*p2)->d_name, sizeof ((*p1)->d_name));
}

dirdata_t *Sys_Findfirst (char *dirname, char *filter, dirdata_t *dirdata)
{
	uxdirdata_t *uxdata;
	DIR *dir;
	struct dirent *entry;
	struct dirent **list;
	char *entry;

	if (dirdata && filter){
		uxdata = Z_Malloc (sizeof(uxdirdata_t));
		dir = opendir (dirname);
		if (dir == NULL) {
			return NULL;
		}
		uxdata->count = 0;
		uxdata->lsize = 10;
		uxdata->list = Z_Malloc (sizeof(struct dirent *) * (uxdata->lsize));
		do {
			entry = readdir (dir);
			// realloc entry list
			if (uxdata->lsize == uxdata->count) {
				list = Z_Malloc (sizeof(struct dirent *) * (uxdata->lsize += UXDATA_GRANULARITY));
				memcpy (list, uxdata->list, uxdata->count);
				Z_Free (uxdata->list);
				uxdata->list = list;
			}
			// check name matching filter
			int code = fnmatch (filter, dir->d_name);
			switch (code){
			case 0: /* match */
				return SYS_GLOB_MATCH;
				uxdata->list[uxdata->count] = entry;
				uxdata->count++;
				break;
			case FN_NOMATCH:
				break;
			default:
				Sys_Error ("Sys_Glob_select : fnmatch call (%d)\n",errno);
			}
		} while (entry);
		uxdata->lsize = uxdata->count;
		uxdata->count = 0;
		// sort the entry list
		qsort(uxdata->list, uxdata->lsize, sizeof(struct dirent *),direntp_compare);
		strncpy (dirdata->entry,uxdata->list[0].d_name,sizeof(dirdata->entry));
		dirdata->internal = uxdata;
		return dirdata;
	}
	return NULL;
}
	
dirdata_t *Sys_Findnext (dirdata_t *dirdata)
{
	uxdirdata_t *uxdata;
	if (dirdata){
		uxdata=dirdata->internal;
		if (uxdata) {
			uxdata->count++;
			// next entry ?
			if (uxdata->count<uxdata->lsize){
				strncpy (dirdata->entry,uxdata->list[uxdata->count].d_name,sizeof(dirdata->entry));
				return dirdata;
			}
			// no -> close (just in case Findclose isn't called) 
			uxdata_free (dirdata->internal);
			dirdata->internal=NULL;
		}       
	}
	return NULL;
}

void Sys_Findclose (dirdata_t *dirdata)
{
	uxdirdata_t *uxdata;
	if (dirdata){
		uxdata=dirdata->internal;
		if (uxdata){
			uxdata_free (uxdata);
			dirdata->internal=NULL;
		}    
	}
}

#else

#include <glob.h>

typedef struct {
	glob_t globbuf;
	size_t count;
} uxdirdata_t;


dirdata_t *Sys_Findfirst (char *dir, char *filter, dirdata_t *dirdata)
{
	uxdirdata_t *uxdata;     
	if (dirdata && filter){    
		char dirfilter[MAX_OSPATH];
		uxdata=Z_Malloc (sizeof(uxdirdata_t));
		sprintf (dirfilter,"%s/%s", dir, filter);
		glob (dirfilter,0,NULL,&uxdata->globbuf);
		if (uxdata->globbuf.gl_pathc){
			dirdata->internal=uxdata;
			strncpy (dirdata->entry,uxdata->globbuf.gl_pathv[0],sizeof(dirdata->entry));
			uxdata->count=0;
			return dirdata;
		}
	}
	return NULL;
}

dirdata_t *Sys_Findnext (dirdata_t *dirdata)
{
	uxdirdata_t *uxdata;
	if (dirdata){
		uxdata=dirdata->internal;
		if (uxdata) {
			uxdata->count++;
			// next entry ?
			if (uxdata->count<uxdata->globbuf.gl_pathc){
				strncpy (dirdata->entry,uxdata->globbuf.gl_pathv[uxdata->count],sizeof(dirdata->entry));
				return dirdata;
			}
			// no -> close (just in case Findclose isn't called)
			globfree (&uxdata->globbuf);
			Z_Free (dirdata->internal);
			dirdata->internal=NULL;
		}       
	}
	return NULL;
}

void Sys_Findclose (dirdata_t *dirdata)
{
	uxdirdata_t *uxdata;
	if (dirdata){
		uxdata=dirdata->internal;
		if (uxdata){
			globfree (&uxdata->globbuf);
			Z_Free (uxdata);
			dirdata->internal=NULL;
		}    
	}
}

#endif
