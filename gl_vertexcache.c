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

---

Vertex memory cache, this is used for dynamically changing stuff, like
skinned meshes or interpolated keyframes.  (Wich are all done on the
cpu because of shadow volumes)
Oh, and this code is based on the Q1 surface cache... (A rover that
throws away old stuff untill it has enough space)

*/

#include "quakedef.h"

typedef struct vertexcacheitem_s
{
	struct vertexcacheitem_s *next;
	DriverPtr				 *owner;// NULL is an empty chunk of memory
	size_t					 size;	 // of data part
	DriverPtr				 buffer; //Actual memory
} vertexcacheitem_t;

//A single vertex cache, a cache is never bigger than 65k verts so we may have
//multiple caches
typedef struct {
	DriverPtr	buffer; //Actual memory this cache is controlling
	vertexcacheitem_t *items;
	vertexcacheitem_t *rover;
	size_t	size; //Size in bytes of this cache
	size_t	free; //Free memory in this cache, it may be fragmented tough
} vertexcache_t;

#define VERTEXCACHE_DEBUG 1
#define GUARDSIZE       4

//These are used to detect if we are thrasing the cache
static qboolean gl_roverwrapped;
static size_t gl_initial_offset;

static vertexcacheitem_t *AllocCacheItem(void) {
	return malloc(sizeof(vertexcacheitem_t));
}

static void FreeCacheItem(vertexcacheitem_t *i) {
	free(i);
}

/**
	This may be dead slow, if it's uncached mem:  debug only
*/
static void VC_CheckCacheGuard (vertexcache_t *c)
{
	int i;
	byte *s = GL_MapToUserSpace(c->buffer);

	s += c->size;
	for (i=0 ; i<GUARDSIZE ; i++) {
		if (s[i] != (byte)i)
			Sys_Error ("GL_CheckCacheGuard failed: Vertex cache is corrupted");
	}

	GL_UnmapFromUserSpace(c->buffer);
}

/**
	This may be dead slow, if it's uncached mem:  debug only
*/
static void VC_ClearCacheGuard (vertexcache_t *c)
{
	int i;
	byte *s = GL_MapToUserSpace(c->buffer);

	s += c->size;
	for (i=0 ; i<GUARDSIZE ; i++) {
		s[i] = (byte)i;
	}

	GL_UnmapFromUserSpace(c->buffer);
}

static void VC_InitVertexCache(vertexcache_t *c, size_t size)
{
	vertexcacheitem_t *i;

	Con_Printf ("%ik vertex cache\n", size/1024);

	c->buffer = GL_DynamicAlloc(size + GUARDSIZE, NULL);
	c->size = size;
	c->free = size;
	c->items = i = AllocCacheItem();
	c->rover = i;

	i->next = NULL;
	i->owner = NULL;
	i->size = size;
	i->buffer = c->buffer;

	VC_ClearCacheGuard(c);
}

static void VC_FlushVertexCache(vertexcache_t *c)
{
	vertexcacheitem_t *i, *n;

	i=c->items;
	while(i)
	{
		if (i->owner)
			*i->owner = DRVNULL;
		n=i->next;
		FreeCacheItem(i);
		i = n;
	}

	c->items = i = AllocCacheItem();
	i->next = NULL;
	i->owner = NULL;
	i->size = c->size;
	i->buffer = c->buffer;
	c->free = c->size;
	c->rover = i;
}

static void VC_FreeVertexCache(vertexcache_t *c)
{
	VC_FlushVertexCache(c);
	FreeCacheItem(c->items);
	c->items = NULL;
}

static vertexcacheitem_t *VC_VertecCacheAlloc (vertexcache_t *c, size_t size)
{
	vertexcacheitem_t           *new;
	qboolean                wrapped_this_time;

	if ((size <= 0) || (size > 0x400000)) //4 megabytes santity check
		Sys_Error ("GL_VertecCacheAlloc: bad cache size %d\n", size);

	size = (size + 3) & ~3;
	if (size > c->size)
		Sys_Error ("GL_VertecCacheAlloc: %i > cache size",size);

	if (size > c->free)
		Sys_Error ("GL_VertecCacheAlloc: %i > cache free",size);

	// if there is not size bytes after the rover, reset to the start
	wrapped_this_time = false;

	if ( !c->rover || c->rover->buffer.offset > c->size - size)
	{
		if (c->rover)
		{
			wrapped_this_time = true;
		}
		c->rover = c->items;
	}

	// colect and free surfcache_t blocks until the rover block is large enough
	new = c->rover;
	if (c->rover->owner)
		*c->rover->owner = DRVNULL;

	while (new->size < size)
	{
		// free another
		vertexcacheitem_t *old = c->rover;
		c->rover = c->rover->next;
		FreeCacheItem(old);
		if (!c->rover)
			Sys_Error ("GL_VertecCacheAlloc: hit the end of memory");
		if (c->rover->owner)
			*c->rover->owner = DRVNULL;

		new->size += c->rover->size;
		new->next = c->rover->next;
	}

	// create a fragment out of any leftovers
	if (new->size - size > 256)
	{
		c->rover = AllocCacheItem();
		c->rover->size = new->size - size;
		c->rover->next = new->next;
		c->rover->owner = NULL;
		c->rover->buffer.segment = new->buffer.segment;
		c->rover->buffer.offset = new->buffer.offset+size;
		new->next = c->rover;
		new->size = size;
	}
	else
		c->rover = new->next;

	new->owner = NULL;              // should be set properly after return

	if (gl_roverwrapped)
	{
		if (wrapped_this_time || (c->rover->buffer.offset >= gl_initial_offset))
			r_cache_thrash = true;
	}
	else if (wrapped_this_time)
	{       
		gl_roverwrapped = true;
	}

#ifdef VERTEXCACHE_DEBUG
	//Only in debug as this can be very slow due to the driver mem being uncached
	VC_CheckCacheGuard (c);
#endif
	return new;
}

static void VC_VertecCacheDump (vertexcache_t *c)
{
	vertexcacheitem_t *test;

	for (test = c->items; test; test=test->next) {
		if (test == c->rover)
			Con_Printf("ROVER:\n");
		Con_Printf("%p : segment(%i) offset(%i) bytes(%i) owner(%p)\n",test, test->buffer.segment, test->buffer.offset, test->size, test->owner);
	}
}

/*************************

	Public interface

*************************/

#define VERTEX_CACHE_SIZE (1024*1024*8)
vertexcache_t cache;

void GL_InitVertexCache(void) {
	VC_InitVertexCache(&cache, VERTEX_CACHE_SIZE);
}

void GL_FreeVertexCache(void) {
	VC_FreeVertexCache(&cache);
}

/**
	Owner will we overwritten with the pointer, if the cache later decides to free the allocated
	cache spot, owner will be overwritten with NULL
	So owner should point to "stable" memory, no temporaries on the stack please!
*/
void GL_AllocVertexCache(const size_t size, DriverPtr *owner) {
	vertexcacheitem_t *r = VC_VertecCacheAlloc(&cache, size);
	*owner = r->buffer;
	r->owner = owner;
}

void GL_FlushVertexCache(void) {
	VC_FlushVertexCache(&cache);
}