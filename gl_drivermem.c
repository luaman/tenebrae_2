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

Driver managed vertex memory

The all routines return DriverPtr's that can be translated and used trough
the use of these routines.
A segment is just a single opengl VBO object, this should abstract the memory management
spanning difference VBO objects because of driver maximum VBO sizes.
*/

#include "quakedef.h"

//number of segments to keep track of
#define MAX_SEGMENTS 256
//maximum size of a segment (this maybe driver dependent)
#define MAX_SEGMENTSIZE (1024*1024*16)
//size of the allocated segments
#define OPTIMAL_SEGMENTSIZE (1024*1024*4)

DriverPtr nullDriver = {0, 0}; //Segment 

typedef struct {
	int size;
	int freeOffset;
	int	mapCount;
	qboolean isSystemMem;	//This segment is system memory and not vbo mem (not supported or no free vbo space left)
	void *nonVboData;		//only nonull if isSystemMem is true
	void *mapData;		//only nonull if isSystemMem is true
} segmentdescriptor_t;

static segmentdescriptor_t segmentDescriptors[MAX_SEGMENTS];
static int numSegments = 0;

/*******************************************************************

	Our own little abstraction layer that abstracts vbo and system mem
	if vbo is not available.

*******************************************************************/

static int GL_CreateSegment(size_t size, void *data, int usage) {
	void *nonVboData;
	qboolean isSystemMem = false;

	if (numSegments >= MAX_SEGMENTS) 
		Sys_Error("GL_CreateSegment: No segments left\n");

	if (gl_vbo) {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, numSegments);
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, size, data,usage);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		nonVboData = NULL;
		//fixme check for OUT_OF_MEMORY error
	} else {
		nonVboData = malloc(size);
		isSystemMem = true;
		if (data) {
			memcpy(nonVboData,data, size);
		}
	}

	segmentDescriptors[numSegments].mapCount = 0;
	segmentDescriptors[numSegments].nonVboData = nonVboData;
	segmentDescriptors[numSegments].isSystemMem = isSystemMem;
	segmentDescriptors[numSegments].size = size;
	segmentDescriptors[numSegments].freeOffset = 0;
	numSegments++;
	return numSegments-1;
}


static void GL_FreeSegment(int segment) {
	segmentdescriptor_t *s;

	if (segment != (numSegments-1)) 
		Sys_Error("GL_FreeSegment: Only last created segment can be freed\n");

	s = &segmentDescriptors[segment];
	if (s->isSystemMem) {
		if (s->nonVboData)
			free(s->nonVboData);
	} else {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, segment);
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, 0, NULL, GL_STREAM_DRAW_ARB);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}

	numSegments--;
}
/*
void *GL_MapToUserSpace(DriverPtr p) {
	byte *data;
	segmentdescriptor_t *s;

	if ((p.segment < 0) || (p.segment >= numSegments)) Sys_Error("GL_MapToUserSpace: Invalid segment");
	s = &segmentDescriptors[p.segment];

	if (s->isSystemMem) {
		return ((byte *)s->nonVboData)+p.offset;
	} else {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, p.segment);
		data = qglMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		return data+p.offset;
	}
}

void GL_UnmapFromUserSpace(DriverPtr p) {
	segmentdescriptor_t *s;

	if ((p.segment < 0) || (p.segment >= numSegments)) Sys_Error("GL_UnmapFromUserSpace: Invalid segment");
	s = &segmentDescriptors[p.segment];
	if (!s->isSystemMem) {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, p.segment);
		if (!qglUnmapBufferARB(GL_ARRAY_BUFFER_ARB)) {
			Sys_Error("GL_UnmapFromUserSpace: Buffer data store was corrupted\n");
		}
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
}
*/
/**
Maps the driver mem pointer to the proces's adress space
*/
void *GL_MapToUserSpace(DriverPtr p) {
	byte *data;
	segmentdescriptor_t *s;

	if ((p.segment < 0) || (p.segment >= numSegments)) Sys_Error("GL_MapToUserSpace: Invalid segment");
	s = &segmentDescriptors[p.segment];

	if (s->isSystemMem) {
		return ((byte *)s->nonVboData)+p.offset;
	} else {
		if (s->mapCount == 0) {
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, p.segment);
			s->mapData = qglMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB);
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		}
		s->mapCount++;
		return (byte *)s->mapData+p.offset;
	}
}

/**
Unmaps the driver mem pointer from the adress space
This MUST! be called before the driver can use the memory
*/
void GL_UnmapFromUserSpace(DriverPtr p) {
	segmentdescriptor_t *s;

	if ((p.segment < 0) || (p.segment >= numSegments)) Sys_Error("GL_UnmapFromUserSpace: Invalid segment");
	s = &segmentDescriptors[p.segment];
	if (!s->isSystemMem) {
		s->mapCount--;
		if (s->mapCount == 0) {
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, p.segment);
			if (!qglUnmapBufferARB(GL_ARRAY_BUFFER_ARB)) {
				Sys_Error("GL_UnmapFromUserSpace: Buffer data store was corrupted\n");
			}
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		}
	}
}

/**
Converts a normal pointer to a VBO compatible pointer (used for some of the geometry that is still in system mem)
*/
DriverPtr GL_WrapUserPointer(void *p) {
	DriverPtr dr;
	dr.segment = 0;
	dr.offset = (byte *)p-(byte*)NULL;
	return dr;
}

/**
This is probably faster than doing a Map and then a memcpy and then an unmap
*/
void drivermemcpy(DriverPtr dest, void *src, size_t size) {
	segmentdescriptor_t *s;

	if ((dest.segment < 0) || (dest.segment >= numSegments)) Sys_Error("GL_UnmapFromUserSpace: Invalid segment");	
	s = &segmentDescriptors[dest.segment];
	if (!s->isSystemMem) {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, dest.segment);
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, dest.offset, size, src);
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	} else {
		memcpy((byte *)s->nonVboData+dest.offset, src, size);
	}
}

//This will have less vbo state changes, but the spec mentions binding another is cheap anyway
//it's the pointer changing that is expensive
#define RESET_VBO 1

void GL_TexCoordPointer(GLint size, GLenum type, GLsizei stride, DriverPtr p) {
	segmentdescriptor_t *s;
	if ((p.segment < 0) || (p.segment >= numSegments)) Sys_Error("GL_MapToUserSpace: Invalid segment");
	s = &segmentDescriptors[p.segment];

	if (s->isSystemMem) {
		if (gl_vbo) qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);	
		glTexCoordPointer(size, type, stride, (byte *)s->nonVboData+p.offset);
	} else {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, p.segment);	
		glTexCoordPointer(size, type, stride, (byte *)s->nonVboData+p.offset);
#ifdef RESET_VBO
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
#endif
	}
}

void GL_VertexPointer(GLint size, GLenum type, GLsizei stride, DriverPtr p) {
	segmentdescriptor_t *s;
	if ((p.segment < 0) || (p.segment >= numSegments)) Sys_Error("GL_MapToUserSpace: Invalid segment");
	s = &segmentDescriptors[p.segment];

	if (s->isSystemMem) {
		if (gl_vbo) qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);	
		glVertexPointer(size, type, stride, (byte *)s->nonVboData+p.offset);
	} else {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, p.segment);	
		glVertexPointer(size, type, stride, (byte *)s->nonVboData+p.offset);
#ifdef RESET_VBO
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
#endif
	}
}

void GL_NormalPointer(GLenum type, GLsizei stride, DriverPtr p) {
	segmentdescriptor_t *s;
	if ((p.segment < 0) || (p.segment >= numSegments)) Sys_Error("GL_MapToUserSpace: Invalid segment");
	s = &segmentDescriptors[p.segment];

	if (s->isSystemMem) {
		if (gl_vbo) qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);	
		glNormalPointer(type, stride, (byte *)s->nonVboData+p.offset);
	} else {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, p.segment);	
		glNormalPointer(type, stride, (byte *)s->nonVboData+p.offset);
#ifdef RESET_VBO
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
#endif
	}
}

void GL_ColorPointer(GLint size, GLenum type, GLsizei stride, DriverPtr p) {
	segmentdescriptor_t *s;
	if ((p.segment < 0) || (p.segment >= numSegments)) Sys_Error("GL_MapToUserSpace: Invalid segment");
	s = &segmentDescriptors[p.segment];

	if (s->isSystemMem) {
		if (gl_vbo) qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);	
		glColorPointer(size, type, stride, (byte *)s->nonVboData+p.offset);
	} else {
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, p.segment);	
		glColorPointer(size, type, stride, (byte *)s->nonVboData+p.offset);
#ifdef RESET_VBO
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
#endif
	}
}

DriverPtr GL_OffsetDriverPtr (DriverPtr p, int offset) {
	p.offset+=offset;
	return p;
}


/*******************************************************************

	Driver memory public interface

*******************************************************************/

/**
	Allocates size bytes of memory and copies data to the block.
	Returns a pointer to driver memory.
	This memory is "slow" if you use the map routines, it should
	not be written or read often.
	This memory cannot be freed, it is freed automatically when
	GL_FreeAll is called.
*/
DriverPtr GL_StaticAlloc(size_t size, void *data) {
	segmentdescriptor_t *s;
	int i, segment;
	DriverPtr p;

	if (size > MAX_SEGMENTSIZE) 
		Sys_Error("GL_StaticAlloc: Tried to allocate more than MAX_SEGMENTSIZE vertex buffer memory\n");

	//If it's to big put it in a separate segment
	if (size > OPTIMAL_SEGMENTSIZE-(OPTIMAL_SEGMENTSIZE/4)) {
		int segment = GL_CreateSegment(size, data, GL_STATIC_DRAW_ARB);
		segmentDescriptors[segment].freeOffset = size;
		p.segment = segment;
		p.offset = 0;
		return p;
	}

	//Find space in an existing segment
	for (i=0, s=segmentDescriptors; i<numSegments; i++, s++) {
		if (size < (s->size - s->freeOffset)) {
			p.segment = i;
			p.offset = s->freeOffset;
			s->freeOffset += size;
			drivermemcpy(p, data, size);
			return p;
		}
	}

	//Allocate a new segment
	segment = GL_CreateSegment(OPTIMAL_SEGMENTSIZE, NULL, GL_STATIC_DRAW_ARB);
	segmentDescriptors[segment].freeOffset = size;
	p.segment = segment;
	p.offset = 0;
	drivermemcpy(p, data, size);
	return p;
}

/**
	Allocates size bytes of memory and copies data to the block.
	Returns a pointer to driver memory.
	This is write once every few frames use multiple times every frame
	sort of memory, so reasonably fast to write.
	This memory cannot be freed, it is freed automatically when
	GL_FreeAll is called (Use the vertex cache for dynamically changing
	memory)
*/
DriverPtr GL_DynamicAlloc(size_t size, void *data) {
	//Allocate a new segment
	int segment = GL_CreateSegment(size, data, GL_DYNAMIC_DRAW_ARB);
	DriverPtr p;
	segmentDescriptors[segment].freeOffset = size;
	p.segment = segment;
	p.offset = 0;
	return p;
}

/**
	Frees all driver memory.
	(This should be called at level changes for example)
*/
void GL_FreeAll(void) {
	int i;
	for (i=numSegments-1; i>=0; i--) {
		//free this segment
		GL_FreeSegment(i);
	}
}

void GL_InitDriverMem(void) {
	//vbo with id 0 is a special case in opengl make a fake entry in our segment table
	segmentDescriptors[0].freeOffset = 0;
	segmentDescriptors[0].size = 0;
	segmentDescriptors[0].nonVboData = 0;
	segmentDescriptors[0].isSystemMem = true;
	numSegments = 1;
}

void GL_FreeDriverMem(void) {
	GL_FreeAll();
}
