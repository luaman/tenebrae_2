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

PENTA: the whole file is freakin penta...

Stencil clear / fillrate saving by using opengl scisorring.

The basic idea is that we seach for lights whose scissor rects do not overlap,
if we find such lights we don't clear the stencil buffer between passes.

*/

#include "quakedef.h"

/*
Copy one rectangle to another.
*/
void R_RectCopy(screenrect_t *src, screenrect_t *dst) {
	int i;
	for (i=0; i<4; i++) {
		dst->coords[i] = src->coords[i];
	}
}

/*
Copy one rectangle to another.
*/
void R_RectPosCopy(screenrect_t *src, screenrect_t *dst) {
	int i;
	for (i=0; i<4; i++) {
		dst->coords[i] = src->coords[i]+50000;
	}
}

/*
Extend the rectangle in res by the rectangle in add
(make it contain both rectangles)
*/

// <AWE> added following macros
#if defined (__APPLE__) || defined (MACOSX)
#define min(A,B)	((A) < (B) ? (A) : (B))
#define max(A,B)	((A) > (B) ? (A) : (B))
#endif /* __APPLE__ || MACOSX */

void R_RectsAdd(screenrect_t *add, screenrect_t *res) {
	res->coords[0] = min (res->coords[0],add->coords[0]);
	res->coords[1] = min (res->coords[1],add->coords[1]);
	res->coords[2] = max (res->coords[2],add->coords[2]);
	res->coords[3] = max (res->coords[3],add->coords[3]);
}

/*
Calculate the surface of the given rect
*/
int R_RectSurf(screenrect_t *rect) {
	return (rect->coords[2]-rect->coords[0])*(rect->coords[3]-rect->coords[1]);
}

/*
qboolean R_RectOverlap(screenrect_t *r1, screenrect_t *r2) {
	screenrect_t rr1,rr2;
	R_RectPosCopy(r1,&rr1);
	R_RectPosCopy(r2,&rr2);
	return R_RectOverlap_Broken(&rr1,&rr2);
}

*/
/*
Check if rectangles overlap
returns true if they overlap
*/
qboolean R_RectOverlap(screenrect_t *r1, screenrect_t *r2) {

	if (r1->coords[0] > r2->coords[2]) return false;
	if (r1->coords[1] > r2->coords[3]) return false;

 
	if (r1->coords[2] < r2->coords[0]) return false;
	if (r1->coords[3] < r2->coords[1]) return false;

	return true;
	//r1 is rightof r2
	/*if (r1->coords[0] > r2->coords[0]) {
		//overlap in x dir?
		if ((r1->coords[0] - r2->coords[2]) < 0)	{
			//check y dir

			//r1 below r2
			if (r1->coords[1] > r2->coords[1]) {
				if ((r1->coords[1]-r2->coords[3]) < 0)
					return true;
				else
					return false;
			//r1 above r2
			} else {
				if ((r2->coords[1]-r1->coords[3]) < 0) 
					return true;
				else 
					return false;
			}
		//no overlap in x- they don't overlap at all
		} else {
			return false;
		}
	//r2 is rightof r1
	} else {
		//overlap in x dir
		if ((r2->coords[0] - r1->coords[2]) < 0)	{
			//check y dir

			//r1 below r2
			if (r1->coords[1] > r2->coords[1]) {
				if ((r1->coords[1]-r2->coords[3]) < 0)
					return true;
				else
					return false;
			//r1 above r2
			} else {
				if ((r2->coords[1]-r1->coords[3]) < 0) 
					return true;
				else 
					return false;
			}
		//no overlap in x- they don't overlap at all
		} else {
			return false;
		}
	}*/
}

screenrect_t *recList;	//first rectangle of the list
screenrect_t totalRect;	//rectangle that holds all rectangles in the list
int recsUsed;


void R_ClearRectList() {
	recList = NULL;
	recsUsed = 0;
}

qboolean R_CheckRectList(screenrect_t *rec) {
	screenrect_t *r;
	r = recList;
	while (r) {
		if (R_RectOverlap(rec,r)) return false;
		r = r->next;
	}
	return true;
}

void R_AddRectList(screenrect_t *rec) {
	
	//Extend bounding rectangle
	if (!recList) {
		R_RectCopy(rec,&totalRect);
	} else {
		R_RectsAdd(rec,&totalRect);
	}
	//Add it to the list
	rec->next = recList;
	recList = rec;
}

void R_SetTotalRect() {
	glScissor(totalRect.coords[0], totalRect.coords[1],
		totalRect.coords[2]-totalRect.coords[0],  totalRect.coords[3]-totalRect.coords[1]);	
}

/*

Some bounding box stuff for the new boxlights support

*/

/**
	Returns true if both boxes intersect
*/
aabox_t emptyBox (void) {
	aabox_t result;
	VectorCopy(vec3_origin, result.maxs);
	VectorCopy(vec3_origin, result.mins);
	return result;
}

aabox_t constructBox (vec3_t mins, vec3_t maxs) {
	aabox_t result;
	VectorCopy(mins, result.mins);
	VectorCopy(maxs, result.maxs);
	return result;
}

qboolean intersectsBox(aabox_t *b1, aabox_t *b2) {
 
	if (b1->mins[0] > b2->maxs[0]) return false;
	if (b1->mins[1] > b2->maxs[1]) return false;
	if (b1->mins[2] > b2->maxs[2]) return false;
 
	if (b1->maxs[0] < b2->mins[0]) return false;
	if (b1->maxs[1] < b2->mins[1]) return false;
	if (b1->maxs[2] < b2->mins[2]) return false;

	return true;
}

qboolean intersectsBoxPoint(aabox_t *b1, vec3_t p) {
 
	if (p[0] > b1->maxs[0]) return false;
	if (p[1] > b1->maxs[1]) return false;
	if (p[2] > b1->maxs[2]) return false;
 
	if (p[0] < b1->mins[0]) return false;
	if (p[1] < b1->mins[1]) return false;
	if (p[2] < b1->mins[2]) return false;

	return true;
}

qboolean intersectsMinsMaxs(aabox_t *b1, vec3_t mins, vec3_t maxs) {
 
	if (b1->mins[0] > maxs[0]) return false;
	if (b1->mins[1] > maxs[1]) return false;
	if (b1->mins[2] > maxs[2]) return false;
 
	if (b1->maxs[0] < mins[0]) return false;
	if (b1->maxs[1] < mins[1]) return false;
	if (b1->maxs[2] < mins[2]) return false;

	return true;
}

/*
void VectorMax(vec3_t i1, vec3_t i2, vec3_t r) {
	r[0] = max(i1[0],i2[0]);
	r[1] = max(i1[1],i2[1]);
	r[2] = max(i1[2],i2[2]);
}


void VectorMin(vec3_t i1, vec3_t i2, vec3_t r) {
	r[0] = min(i1[0],i2[0]);
	r[1] = min(i1[1],i2[1]);
	r[2] = min(i1[2],i2[2]);
}
*/

aabox_t intersectBoxes(aabox_t *b1, aabox_t *b2) {
	aabox_t result;
	float temp;
	int i;

	VectorMax(b1->mins,b2->mins,result.mins);
	VectorMin(b1->maxs,b2->maxs,result.maxs);

	for (i=0;i<3;i++) {
		if (result.mins[i]>result.maxs[i]) {
			result = emptyBox();
			return result;
			/*
			temp = result.maxs[i];
			result.maxs[i] = result.mins[i];
			result.mins[i] = temp;
			*/
		}
	}
	return result;
}

aabox_t addBoxes(aabox_t *b1, aabox_t *b2) {
	aabox_t result;
	VectorMin(b1->mins,b2->mins,result.mins);
	VectorMax(b1->maxs,b2->maxs,result.maxs);
	return result;
}

void boxScreenSpaceRect(aabox_t *b, int *rect);

void drawBoxWireframe(aabox_t *b) {
	
	glBegin(GL_LINES);

		//Top plane

		glVertex3f(b->maxs[0],b->maxs[1],b->maxs[2]);
		glVertex3f(b->mins[0],b->maxs[1],b->maxs[2]);

		glVertex3f(b->maxs[0],b->maxs[1],b->maxs[2]);
		glVertex3f(b->maxs[0],b->mins[1],b->maxs[2]);

		glVertex3f(b->mins[0],b->mins[1],b->maxs[2]);
		glVertex3f(b->mins[0],b->maxs[1],b->maxs[2]);

		glVertex3f(b->mins[0],b->mins[1],b->maxs[2]);
		glVertex3f(b->maxs[0],b->mins[1],b->maxs[2]);

		//Bottom plane

		glVertex3f(b->maxs[0],b->maxs[1],b->mins[2]);
		glVertex3f(b->mins[0],b->maxs[1],b->mins[2]);

		glVertex3f(b->maxs[0],b->maxs[1],b->mins[2]);
		glVertex3f(b->maxs[0],b->mins[1],b->mins[2]);

		glVertex3f(b->mins[0],b->mins[1],b->mins[2]);
		glVertex3f(b->mins[0],b->maxs[1],b->mins[2]);

		glVertex3f(b->mins[0],b->mins[1],b->mins[2]);
		glVertex3f(b->maxs[0],b->mins[1],b->mins[2]);

		//Sides

		glVertex3f(b->mins[0],b->maxs[1],b->mins[2]);
		glVertex3f(b->mins[0],b->maxs[1],b->maxs[2]);

		glVertex3f(b->maxs[0],b->maxs[1],b->mins[2]);
		glVertex3f(b->maxs[0],b->maxs[1],b->maxs[2]);

		glVertex3f(b->mins[0],b->mins[1],b->mins[2]);
		glVertex3f(b->mins[0],b->mins[1],b->maxs[2]);

		glVertex3f(b->maxs[0],b->mins[1],b->mins[2]);
		glVertex3f(b->maxs[0],b->mins[1],b->maxs[2]);
	glEnd();
}

void IntersectRayPlane(vec3_t v1, vec3_t v2, plane_t *plane, vec3_t res) {
	vec3_t v;
	float sect;
	VectorSubtract (v1, v2, v);
	sect = -(DotProduct (plane->normal, v1)-plane->dist) / DotProduct (plane->normal, v);
	VectorScale (v,sect,v);
	VectorAdd (v1, v, res);
}

void addPoint(matrix_4x4 modelviewproj, vec3_t v1, int *rect) {

	matrix_1x4 point, res;
	float px, py;

	point[0] = v1[0];
	point[1] = v1[1];
	point[2] = v1[2];
	point[3] = 1.0f;
	Mat_Mul_1x4_4x4(point, modelviewproj, res);

	px = (res[0]*(1/res[3])+1.0) * 0.5;
	py = (res[1]*(1/res[3])+1.0) * 0.5;

	px = px * r_Iviewport[2] + r_Iviewport[0];
	py = py * r_Iviewport[3] + r_Iviewport[1];

	if (px > rect[2]) rect[2] = (int)px;
	if (px < rect[0]) rect[0] = (int)px;
	if (py > rect[3]) rect[3] = (int)py;
	if (py < rect[1]) rect[1] = (int)py;
}

void addEdge(matrix_4x4 modelviewproj, vec3_t v1, vec3_t v2, int *rect) {
	
	vec3_t intersect;
	plane_t plane;
	qboolean side1, side2;

	plane.normal[0] = vpn[0];
	plane.normal[1] = vpn[1];
	plane.normal[2] = vpn[2];
	plane.dist = DotProduct(r_refdef.vieworg,vpn) + 5.0;

	//Check edge to frustrum near plane
	side1 = ((DotProduct(plane.normal, v1) - plane.dist) >= 0.0);
	side2 = ((DotProduct(plane.normal, v2) - plane.dist) >= 0.0);

	if (!side1 && !side2) return; //edge behind near plane

	if (!side1 || !side2)
		IntersectRayPlane(v1,v2,&plane,intersect);

	if (!side1) {
		VectorCopy(intersect,v1);
	} else if (!side2) {
		VectorCopy(intersect,v2);
	}

	addPoint(modelviewproj, v1, rect);
	addPoint(modelviewproj, v2, rect);

}

/**
	Recturns the screen space rectangle taken by the box.
	(Clips the box to the near plane to have correct results even if the box intersects the near plane)
*/
void boxScreenSpaceRect(aabox_t *b, int *rect) {

	matrix_4x4 modelviewproj, world, proj;
	vec3_t v1, v2;

	glGetFloatv (GL_MODELVIEW_MATRIX, &world[0][0]);
	glGetFloatv (GL_PROJECTION_MATRIX, &proj[0][0]);
	Mat_Mul_4x4_4x4(world, proj, modelviewproj);

	rect[0] = 100000000;
	rect[1] = 100000000;
	rect[2] = -100000000;
	rect[3] = -100000000;

	VectorConstruct(b->maxs[0],b->maxs[1],b->maxs[2],v1);
	VectorConstruct(b->mins[0],b->maxs[1],b->maxs[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->maxs[0],b->maxs[1],b->maxs[2],v1);
	VectorConstruct(b->maxs[0],b->mins[1],b->maxs[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->mins[0],b->mins[1],b->maxs[2],v1);
	VectorConstruct(b->mins[0],b->maxs[1],b->maxs[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->mins[0],b->mins[1],b->maxs[2],v1);
	VectorConstruct(b->maxs[0],b->mins[1],b->maxs[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	//Bottom plane

	VectorConstruct(b->maxs[0],b->maxs[1],b->mins[2],v1);
	VectorConstruct(b->mins[0],b->maxs[1],b->mins[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->maxs[0],b->maxs[1],b->mins[2],v1);
	VectorConstruct(b->maxs[0],b->mins[1],b->mins[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->mins[0],b->mins[1],b->mins[2],v1);
	VectorConstruct(b->mins[0],b->maxs[1],b->mins[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->mins[0],b->mins[1],b->mins[2],v1);
	VectorConstruct(b->maxs[0],b->mins[1],b->mins[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	//Sides

	VectorConstruct(b->mins[0],b->maxs[1],b->mins[2],v1);
	VectorConstruct(b->mins[0],b->maxs[1],b->maxs[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->maxs[0],b->maxs[1],b->mins[2],v1);
	VectorConstruct(b->maxs[0],b->maxs[1],b->maxs[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->mins[0],b->mins[1],b->mins[2],v1);
	VectorConstruct(b->mins[0],b->mins[1],b->maxs[2],v2);
	addEdge(modelviewproj, v1, v2, rect);

	VectorConstruct(b->maxs[0],b->mins[1],b->mins[2],v1);
	VectorConstruct(b->maxs[0],b->mins[1],b->maxs[2],v2);
	addEdge(modelviewproj, v1, v2, rect);
}
