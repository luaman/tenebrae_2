/*
Copyright (C) 2001-2002 Charles Hollemeersch

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

A passtrough bumpmap driver that collects statistics.
This uses the original bumpmap driver for the actual drawing
*/

#include "quakedef.h"

static bumpdriver_t original;
static qboolean gl_bumpstatistics;

static int ambientSurfaces;
static int bumpSurfaces;
static int ambientSurfaceTris;
static int bumpSurfaceTris;

static int ambientTriangles;
static int bumpTriangles;

static double averageTris;
static int numFrames = 0;

void STAT_drawSurfaceListBase(vertexdef_t *verts, msurface_t **surfs, int numSurfaces, shader_t *shader) {
	int i;
	original.drawSurfaceListBase(verts, surfs, numSurfaces, shader);
	ambientSurfaces += numSurfaces;
	for (i=0; i<numSurfaces; i++) {
		ambientSurfaceTris += (surfs[i]->polys->numindecies/3);
	}
}

void STAT_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs, int numSurfaces, const transform_t *tr, const lightobject_t *lo) {
	int i;
	original.drawSurfaceListBump(verts, surfs, numSurfaces, tr, lo);
	bumpSurfaces += numSurfaces;
	for (i=0; i<numSurfaces; i++) {
		bumpSurfaceTris += (surfs[i]->polys->numindecies/3);
	}
}

void STAT_drawTriangleListBase (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader, int lightmapIndex) {
	original.drawTriangleListBase(verts, indecies, numIndecies, shader, lightmapIndex);
	ambientTriangles += (numIndecies/3);
}

void STAT_drawTriangleListBump (const vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader, const transform_t *tr, const lightobject_t *lo) {
	original.drawTriangleListBump(verts, indecies, numIndecies, shader, tr, lo);
	bumpTriangles += (numIndecies/3);
}

void STAT_freeDriver (void) {
	original.freeDriver();
}

/**
	This installs the statistics driver
*/
void BUMP_StartStat(void) {
	original = gl_bumpdriver;
	gl_bumpstatistics = true;

	gl_bumpdriver.drawSurfaceListBase = STAT_drawSurfaceListBase;
	gl_bumpdriver.drawSurfaceListBump = STAT_drawSurfaceListBump;
	gl_bumpdriver.drawTriangleListBase = STAT_drawTriangleListBase;
	gl_bumpdriver.drawTriangleListBump = STAT_drawTriangleListBump;
	gl_bumpdriver.freeDriver = STAT_freeDriver;
}

/**
	This uninstals the statistics driver and reinstalls the old driver
*/
void BUMP_StopStat(void) {
	gl_bumpstatistics = false;
	gl_bumpdriver = original;
}

/**
	This prints the current statistics
*/
void BUMP_PrintStats(void) {
	if (!gl_bumpstatistics) return;
/*
	ambientSurfaces = 0;
	bumpSurfaces = 0;
	ambientSurfaceTris = 0;
	bumpSurfaceTris = 0;
	ambientTriangles = 0;
	bumpTriangles = 0;
*/
	Con_Printf("--- Stats ---\n");
	Con_Printf("Surfaces\n");
	Con_Printf("  Ambient: %i Bump %i Total %i\n",ambientSurfaces, bumpSurfaces, ambientSurfaces+bumpSurfaces);
	Con_Printf("SurfaceTriangles\n");
	Con_Printf("  Ambient: %i Bump %i Total %i\n",ambientSurfaceTris, bumpSurfaceTris, ambientSurfaceTris+bumpSurfaceTris);
	Con_Printf("Triangles\n");
	Con_Printf("  Ambient: %i Bump %i Total %i\n",ambientTriangles, bumpTriangles, ambientTriangles+bumpTriangles);

	averageTris = (averageTris*numFrames + (ambientTriangles+bumpTriangles+ambientSurfaceTris+bumpSurfaceTris)) / (numFrames+1);
	Con_Printf("Average\n");
	Con_Printf(" Triangles: %i\n", (int)(averageTris));
	numFrames++;
}

/**
	This resets the internal counters, should be called every frame
*/
void BUMP_ResetStats(void) {
	if (!gl_bumpstatistics) return;

	ambientSurfaces = 0;
	bumpSurfaces = 0;
	ambientSurfaceTris = 0;
	bumpSurfaceTris = 0;
	ambientTriangles = 0;
	bumpTriangles = 0;
}

void ToggleStatistics_f(void) {
	if (gl_bumpstatistics) {
		BUMP_StopStat();
	} else {
		BUMP_StartStat();
	}
}