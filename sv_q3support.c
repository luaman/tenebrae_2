/*
Copyright (C) 2002-2003 Charles Hollemeersch

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

PENTA:

Quake 3 collision/areaportals code
Heavily based on the public quake2 source

*/
#include "quakedef.h"

mplane_t	*box_planes;
int			box_firstbrush;
mbrush_t	*box_brush;

//#define M sv.worldmodel

//timestamp variables
int c_pointcontents = 0;
int c_brush_traces = 0;
int	checkcount = 0;
int c_traces = 0;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (model_t *M)
{
	int			i;
	int			side;
	mplane_t	*p;
	mbrushside_t	*s;

	box_planes = &M->planes[M->numplanes];
	box_firstbrush = M->numbrushes;
	box_brush = &M->brushes[M->numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = M->numbrushsides;
	box_brush->contents = CONTENTS_SOLID;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &M->brushsides[M->numbrushsides+i];
		s->plane = 	M->planes + (M->numplanes+i*2+side);
		s->shader = NULL;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
	}
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*//*
int	CM_HeadnodeForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}
*/
/*
===================
CM_BrushForBox
===================
*/
int	CM_BrushForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_firstbrush;
}


/*
==================
CM_PointLeafnum_r

==================
*/
int CM_PointLeafnum_r (model_t *M, vec3_t p, int num)
{
	float		d;
	mnode_t		*node;
	mplane_t	*plane;

	while (num >= 0)
	{
		node = M->nodes + num;
		plane = node->plane;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->ichildren[1];
		else
			num = node->ichildren[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

int CM_PointLeafnum (model_t *M, vec3_t p)
{
	if (!M->numplanes)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (M, p, 0);
}



/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int		leaf_count, leaf_maxcount;
int		*leaf_list;
float	*leaf_mins, *leaf_maxs;
int		leaf_topnode;

void CM_BoxLeafnums_r (model_t *M, int nodenum)
{
	mplane_t	*plane;
	mnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}
	
		node = &M->nodes[nodenum];
		plane = node->plane;
//		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
		if (s == 1)
			nodenum = node->ichildren[0];
		else if (s == 2)
			nodenum = node->ichildren[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r (M, node->ichildren[0]);
			nodenum = node->ichildren[1];
		}

	}
}

int	CM_BoxLeafnums_headnode (model_t *M, vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (M, headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int	CM_BoxLeafnums (model_t *M,vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode)
{
	return CM_BoxLeafnums_headnode (M,mins, maxs, list,
		listsize, 0 /*headnode??*/, topnode);
}



/*
==================
CM_PointContents

==================
*/
int CM_PointContents (model_t *M, vec3_t p, int headnode)
{
	int		l, contents, i, j;
	mleaf_t			*leaf;
	mbrush_t		*brush;
	mbrushside_t	*brushside;

	if (!M->numnodes)	// map not loaded
		return 0;

	l = CM_PointLeafnum_r (M, p, 0 /*headnode*/);

	//PENTA: Based on qfusion contents soucre
	leaf = &M->leafs[l];
	if ( leaf->contents & CONTENTS_NODROP ) {
		contents = CONTENTS_NODROP;
	} else {
		contents = 0;
	}

	for (i = 0; i < leaf->numbrushes; i++)
	{
		brush = &M->brushes[M->leafbrushes[leaf->firstbrush + i]];

		// check if brush actually adds something to contents
		if ( (contents & brush->contents) == brush->contents ) {
			continue;
		}
		
		brushside = &M->brushsides[brush->firstbrushside];
		for ( j = 0; j < brush->numsides; j++, brushside++ )
		{
			if ((DotProduct(p, brushside->plane->normal) -brushside->plane->dist) > 0)
				break;
		}

		if (j == brush->numsides) 
			contents |= brush->contents;
	}

	return contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents (model_t *M, vec3_t p, int headnode, vec3_t origin, vec3_t angles)
{
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;
	int			l;

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (angles[0] || angles[1] || angles[2])
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	l = CM_PointLeafnum_r (M, p_l, headnode);

	return M->leafs[l].contents;
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

vec3_t	trace_start, trace_end;
vec3_t	trace_mins, trace_maxs;
vec3_t	trace_extents;

trace_t	trace_trace;
int		trace_contents;
qboolean	trace_ispoint;		// optimized case

/*
================
CM_ClipBoxToBrush
================
*/
void CM_ClipBoxToBrush (model_t *M, vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
					  trace_t *trace, mbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	mbrushside_t	*side, *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &M->brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	
	/*if (getout || startout)*/ trace->inopen = true;
	

	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1 && enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			//trace->plane = *clipplane;
			//trace->surface = &(leadside->surface->c);
			VectorCopy(clipplane->normal,trace->plane.normal);
			trace->plane.dist = clipplane->dist;
			//trace->contents = brush->contents;
		}
	}
}

/*
================
CM_TestBoxInBrush
================
*/
void CM_TestBoxInBrush (model_t *M, vec3_t mins, vec3_t maxs, vec3_t p1,
					  trace_t *trace, mbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	mbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &M->brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = DotProduct (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0) {
			trace->inopen = true;
			return;
		}

	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	//trace->contents = brush->contents;


}


/*
================
CM_TraceToLeaf
================
*/
void CM_TraceToLeaf (model_t *M, int leafnum)
{
	int			k;
	int			brushnum;
	mleaf_t		*leaf;
	mbrush_t	*b;

	leaf = &M->leafs[leafnum];
	//if ( !(leaf->contents & trace_contents))
	//	return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numbrushes ; k++)
	{
		brushnum = M->leafbrushes[leaf->firstbrush+k];
		b = &M->brushes[brushnum];
		/*
		Con_Printf("Checkbrush %i",brushnum);

		if (brushnum == 1800) {
			int j;
			for (j=0; j<b->numsides; j++) {
				Con_Printf("Side: %f %f %f %f\n",M->brushsides[j+b->firstbrushside].plane->normal[0],
											M->brushsides[j+b->firstbrushside].plane->normal[1],
											M->brushsides[j+b->firstbrushside].plane->normal[2],
											M->brushsides[j+b->firstbrushside].plane->dist);
			}

		}
		*/

		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;

		CM_ClipBoxToBrush (M, trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}

/*
================
CM_TraceToBrushModel

PENTA: Clips to the given brush model
Q3 doesn't have bsp's associated with map models anymore, just a list of brushes
================
*/
void CM_TraceToBrushModel (model_t *M, int firstbrush, int numbrushes, vec3_t mins, vec3_t maxs,
						   vec3_t start, vec3_t end, trace_t *trace, int brushmask)
{
	int		i, brushnum;
	mbrush_t *b;

	//Con_Printf("CM_TraceToBrushModel");

	if (!M->numnodes)	// map not loaded
		return;
	trace_contents = brushmask;
	trace_ispoint = false;

	for (i=0; i<numbrushes; i++)
	{
		//brushnum = M->leafbrushes[firstbrush+i];
		b = &M->brushes[firstbrush+i];

		if ( !(b->contents & trace_contents))
			continue;

		CM_ClipBoxToBrush (M, mins, maxs, start, end, trace, b);
		if (!trace->fraction)
			return;
	}

	if (trace->fraction == 1)
	{
		VectorCopy (end, trace->endpos);
	}
	else
	{
		for (i=0 ; i<3 ; i++)
			trace->endpos[i] = start[i] + trace->fraction * (end[i] - start[i]);
	}
}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf (model_t *M, int leafnum)
{
	int			k;
	int			brushnum;
	mleaf_t		*leaf;
	mbrush_t	*b;

	leaf = &M->leafs[leafnum];
	//if ( !(leaf->contents & trace_contents))
	//	return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numbrushes ; k++)
	{
		brushnum = M->leafbrushes[leaf->firstbrush+k];
		b = &M->brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_TestBoxInBrush (M, trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
==================
CM_RecursiveHullCheck

==================
*/
void CM_RecursiveHullCheck (model_t *M, int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	mnode_t		*node;
	mplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (M, -1-num);
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = M->nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs(trace_extents[0]*plane->normal[0]) +
				fabs(trace_extents[1]*plane->normal[1]) +
				fabs(trace_extents[2]*plane->normal[2]);
	}


#if 0
CM_RecursiveHullCheck (M, node->ichildren[0], p1f, p2f, p1, p2);
CM_RecursiveHullCheck (M, node->ichildren[1], p1f, p2f, p1, p2);
return;
#endif

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		CM_RecursiveHullCheck (M, node->ichildren[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		CM_RecursiveHullCheck (M, node->ichildren[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;
		
	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (M, node->ichildren[side], p1f, midf, p1, mid);


	// go past the node
	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;
		
	midf = p1f + (p2f - p1f)*frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (M, node->ichildren[side^1], midf, p2f, mid, p2);
}



//======================================================================

/*
==================
CM_BoxTrace
==================
*/
trace_t		CM_BoxTrace (model_t *M, vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask)
{
	int		i;

//	Con_Printf("CM_BoxTrace");
	checkcount++;		// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	//trace_trace.surface = &(nullsurface.c);

	if (!M->numnodes)	// map not loaded
		return trace_trace;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int		leafs[1024];
		int		i, numleafs;
		vec3_t	c1, c2;
		int		topnode;

		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		numleafs = CM_BoxLeafnums_headnode (M, c1, c2, leafs, 1024, headnode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
			CM_TestInLeaf (M, leafs[i]);
			if (trace_trace.allsolid)
				break;
		}
		VectorCopy (start, trace_trace.endpos);
		return trace_trace;
	}

	//
	// check for point special case
	//
	if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0
		&& maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
	{
		trace_ispoint = true;
		VectorClear (trace_extents);
	}
	else
	{
		trace_ispoint = false;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	//
	// general sweeping through world
	//
	CM_RecursiveHullCheck (M, headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1)
	{
		VectorCopy (end, trace_trace.endpos);
	}
	else
	{
		for (i=0 ; i<3 ; i++)
			trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}
	return trace_trace;
}


/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
#ifdef _WIN32
#pragma optimize( "", off )
#endif


trace_t		CM_TransformedBoxTrace (model_t *M, vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask,
						  vec3_t origin, vec3_t angles)
{
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		a;
	vec3_t		forward, right, up;
	vec3_t		temp;
	qboolean	rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (angles[0] || angles[1] || angles[2] )
		rotated = true;
	else
		rotated = false;

	if (rotated)
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	trace = CM_BoxTrace (M, start_l, end_l, mins, maxs, headnode, brushmask);

	if (rotated && trace.fraction != 1.0)
	{
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = -DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	return trace;
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif



/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
/*
void CM_DecompressVis (byte *in, byte *out)
{
	int		c;
	byte	*out_p;
	int		row;

	row = (numclusters+7)>>3;	
	out_p = out;

	if (!in || !numvisibility)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;		
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			Com_DPrintf ("warning: Vis decompression overrun\n");
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

byte	pvsrow[MAX_MAP_LEAFS/8];
byte	phsrow[MAX_MAP_LEAFS/8];

byte	*CM_ClusterPVS (int cluster)
{
	if (cluster == -1)
		memset (pvsrow, 0, (numclusters+7)>>3);
	else
		CM_DecompressVis (map_visibility + map_vis->bitofs[cluster][DVIS_PVS], pvsrow);
	return pvsrow;
}

byte	*CM_ClusterPHS (int cluster)
{
	if (cluster == -1)
		memset (phsrow, 0, (numclusters+7)>>3);
	else
		CM_DecompressVis (map_visibility + map_vis->bitofs[cluster][DVIS_PHS], phsrow);
	return phsrow;
}

*/
/*
===============================================================================

AREAPORTALS

PENTA: Areaportals seem to have changed somewhat from quake2
this is based on the qfusion source who supports quake3 style areaportals
===============================================================================
*/

/*
====================
FloodAreaConnections


====================
*/
void	FloodAreaConnections (model_t *M)
{
	int		i, j;

	// area 0 is not used
	for (i=1 ; i<M->numareas ; i++)
	{
		for (  j = 1; j < M->numareas; j++ ) {
			M->areas[i].numareaportals[j] = ( j == i );
		}
	}
}

void	CM_SetAreaPortalState (model_t *M, int area1, int area2, qboolean open)
{
	if (area1 > M->numareas || area2 > M->numareas) {
		Con_Printf ("CM_SetAreaPortalState: area > M->numareas\n");
		return;
	}
	if ( open ) {
		M->areas[area1].numareaportals[area2]++;
		M->areas[area2].numareaportals[area1]++;
		Con_Printf("Open portal between %i and %i\n", area1, area2);
	} else {
		M->areas[area1].numareaportals[area2]--;
		//shouln't really happen
		if (M->areas[area1].numareaportals[area2] < 0)
			M->areas[area1].numareaportals[area2] = 0;

		M->areas[area2].numareaportals[area1]--;
		//shouln't really happen
		if (M->areas[area2].numareaportals[area1] < 0)
			M->areas[area2].numareaportals[area1] = 0;

		Con_Printf("Close portal between %i and %i\n", area1, area2);
	}
}

qboolean	CM_AreasConnected (model_t *M, int area1, int area2)
{
	int		i;
/*
	if (cm_noAreas->value)
		return true;
*/
	if (area1 > M->numareas || area2 > M->numareas)
		Sys_Error ("CM_AreasConnected: area > numareas");

	if ( M->areas[area1].numareaportals[area2] )
		return true;

	// area 0 is not used
	for (i=1 ; i<M->numareas ; i++)
	{
		if ( M->areas[i].numareaportals[area1] &&
			M->areas[i].numareaportals[area2] )
			return true;
	}

	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits (model_t *M, byte *buffer, int area)
{
	int		i;
	int		bytes;

	bytes = (M->numareas+7)>>3;
/*
	if (cm_noAreas->value)
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
*/
	{
		memset (buffer, 0, bytes);

		for (i=1 ; i<M->numareas ; i++)
		{
			if (!area || CM_AreasConnected (M, i, area ) || i == area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	buffer[area>>3] |= 1<<(area&7);

	return bytes;
}

/*
=================
CM_MergeAreaBits
=================
*/
void CM_MergeAreaBits (model_t *M, byte *buffer, int area)
{
	int		i;

	for (i=1 ; i<M->numareas ; i++)
	{
		if ( CM_AreasConnected (M, i, area ) || i == area)
			buffer[i>>3] |= 1<<(i&7);
	}
}

/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void	CM_WritePortalState (model_t *M, FILE *f)
{
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void	CM_ReadPortalState (model_t *M, FILE *f)
{
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
/*
qboolean CM_HeadnodeVisible (int nodenum, byte *visbits)
{
	int		leafnum;
	int		cluster;
	mnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = map_leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &map_nodes[nodenum];
	if (CM_HeadnodeVisible(node->ichildren[0], visbits))
		return true;
	return CM_HeadnodeVisible(node->ichildren[1], visbits);
}
*/