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

Bezier curve code...
We evaluate curves at load time based on the user's precision preferences.
No dynamic lod...
*/
#include "quakedef.h"

int			numleafbrushes;

void TangentForPoly(int *index, mmvertex_t *vertices,vec3_t Tangent, vec3_t Binormal);
void NormalForPoly(int *index, mmvertex_t *vertices,vec3_t Normal);

//these are just utility structures
typedef struct {
	int firstcontrol;
	int firstvertex;
	int controlwidth, controlheight;
	int width, height;
} curve_t;

#define MAX_BIN 10
int binomials[MAX_BIN][MAX_BIN];

/**
* If this returns true consider the points "degenerate" producing a zero area traingle.
*/
qboolean degenerateDist(vec3_t v1, vec3_t v2) {
	vec3_t s;
	float d;
	VectorSubtract(v1,v2,s);
	d = DotProduct(s,s);
	return (d < 0.01);
}

/*
We roll or own Bezier code...
Dunno how id is supposed to do it but we just evaluate the Bernstein polynomials....
It's not particulary efficient but we pre-evaluate them so it's not a problem...
*/

int fac(int n) {
	int i;
	int rez = 1;
	for (i=2;i<=n;i++) {
		rez*=i;
	}
	return rez;
}

int binomial(int n, int k) {
	return fac(n)/fac(k)/fac(n-k);
}

//Make a lookup table ...
void CS_FillBinomials(void) {
	int i,j;
	for (i=0; i<MAX_BIN; i++) {
		for (j=0; j<MAX_BIN; j++) {
			binomials[i][j] = binomial(i,j);
		}
	}
}

//Evaluates the bernstein polynomial
float Bernstein(int k, int n, float u) {
	return (float)binomials[n][k]*(float)pow(1.0-u,n-k)*(float)pow(u,k);
}

/*
=================
EvaluateBezier

Evaluates the bezier surface with given control points at the u,v parameters
=================
*/
void EvaluateBezier(mmvertex_t *controlpoints,int ofsw, int ofsh, int width, int height, float u, float v,mmvertex_t *result) {

	int i,j;
	float scale;
	float color[4];
	int	n=3;
	int	m=3;
	mmvertex_t *controlpoint, *controlpoint2;
	vec3_t temp;

	for (i=0; i<4; i++) {
		color[i] = 0.0f;
	}

	for (i=0; i<3; i++) {
		result->position[i] = 0.0;
	}

	for (i=0; i<2; i++) {
		result->texture[i] = 0.0;
		result->lightmap[i] = 0.0;
	}

	//Calculate vertices & texture coords
	for (i=0; i<n; i++) {
		for (j=0; j<m; j++) {
			scale = Bernstein(i,n-1,u)*Bernstein(j,m-1,v);

			controlpoint = &controlpoints[(ofsw+i)+(ofsh+j)*width];
			result->position[0]+=(scale*controlpoint->position[0]);
			result->position[1]+=(scale*controlpoint->position[1]);
			result->position[2]+=(scale*controlpoint->position[2]);

			result->texture[0]+=(scale*controlpoint->texture[0]);
			result->texture[1]+=(scale*controlpoint->texture[1]);

			result->lightmap[0]+=(scale*controlpoint->lightmap[0]);
			result->lightmap[1]+=(scale*controlpoint->lightmap[1]);

			color[0]+=(scale*controlpoint->color[0]);
			color[1]+=(scale*controlpoint->color[1]);
			color[2]+=(scale*controlpoint->color[2]);
			color[3]+=(scale*controlpoint->color[3]);
		}
	}

	//Yeah parametric tangent space! (done by deriving the function to u or v)
/*
	//tangent
	for (i=0; i<n; i++) {
		for (j=0; j<m-1; j++) {
			scale = Bernstein(i,n-1,u)*Bernstein(j,m-2,v);

			controlpoint = &controlpoints[(ofsw+i)+(ofsh+j+1)*width];
			controlpoint2 = &controlpoints[(ofsw+i)+(ofsh+j)*width];
			VectorSubtract(controlpoint->position,controlpoint2->position, temp);
			result->tangent[0] +=  scale*temp[0];
			result->tangent[1] +=  scale*temp[1];
			result->tangent[2] +=  scale*temp[2];
		}
	}
	VectorScale(result->tangent,m-1,result->tangent);
	VectorNormalize(result->tangent); //needed?

	//binormal
	for (i=0; i<n-1; i++) {
		for (j=0; j<m; j++) {
			scale = Bernstein(i,n-2,u)*Bernstein(j,m-1,v);

			controlpoint = &controlpoints[(ofsw+i+1)+(ofsh+j)*width];
			controlpoint2 = &controlpoints[(ofsw+i)+(ofsh+j)*width];
			VectorSubtract(controlpoint->position,controlpoint2->position, temp);
			result->binormal[0] +=  scale*temp[0];
			result->binormal[1] +=  scale*temp[1];
			result->binormal[2] +=  scale*temp[2];
		}
	}
	VectorScale(result->binormal,n-1,result->binormal);
	VectorNormalize(result->binormal); //needed?

	//normal
	CrossProduct(result->binormal, result->tangent, result->normal);
	VectorNormalize(result->normal); //needed?
*/
	/*
	VectorCopy(result->binormal, temp);
	VectorCopy(result->tangent, result->binormal);
	VectorCopy(result->tangent, temp);
	*/
	//VectorScale(result->tangent,-1,result->tangent);

	for (i=0; i<4; i++) {
		result->color[i] = (byte)color[i];
	}
}

/**
* Quake3 beziers, are made up out of one or more 3x3 bezier patches
*/
void EvaluateBiquadraticBeziers(mmvertex_t *controlpoints, int width, int height, float u, float v,mmvertex_t *result) {


//	EvaluateBezier(controlpoints,0,0,width,height,u,v,result);

	//calculate number of patches in curve
	int numpatchx = (width- 1) / 2;
	int	numpatchy = (height- 1) / 2;

	float invx = 1.0f / numpatchx;
	float invy = 1.0f / numpatchy;

	//caclucate patch given u/v is on
	int ofsx = floor(u*numpatchx)*2;
	int ofsy = floor(v*numpatchy)*2;

	if (ofsx >= (width-1)) ofsx-=2;
	if (ofsy >= (height-1)) ofsy-=2;

	//calculate u/v relative to patch
	u = (u-(ofsx/2)*invx)*numpatchx;
	v = (v-(ofsy/2)*invy)*numpatchy;

	EvaluateBezier(controlpoints,ofsx,ofsy,width,height,u,v,result);
}

void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
	vec3_t pVec, vec;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize(vec);
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
}

/*********************
	Quad tree subdivision
**********************/

void InterpolateMemVertex(mmvertex_t *v1, mmvertex_t *v2, float i, mmvertex_t *result) {
	float ii = 1.0f-i;
	result->position[0] = v1->position[0]*ii + v2->position[0]*i;
	result->position[1] = v1->position[1]*ii + v2->position[1]*i;
	result->position[2] = v1->position[2]*ii + v2->position[2]*i;

	result->texture[0] = v1->texture[0]*ii + v2->texture[0]*i;
	result->texture[1] = v1->texture[1]*ii + v2->texture[1]*i;

	result->lightmap[0] = v1->lightmap[0]*ii + v2->lightmap[0]*i;
	result->lightmap[1] = v1->lightmap[1]*ii + v2->lightmap[1]*i;

	result->color[0] = (byte)(v1->color[0]*ii + v2->color[0]*i);
	result->color[1] = (byte)(v1->color[1]*ii + v2->color[1]*i);
	result->color[2] = (byte)(v1->color[2]*ii + v2->color[2]*i);
	result->color[3] = (byte)(v1->color[3]*ii + v2->color[3]*i);
}

//curve vertex
typedef struct { 
	mmvertex_t	p;
	float		u;
	float		v;
} cvertex_t;

void AverageCurveVertexParams(cvertex_t *v1, cvertex_t *v2, cvertex_t *result) {
	result->u = (v1->u + v2->u)*0.5f;
	result->v = (v1->v + v2->v)*0.5f;
}

void InterpolateCurveVertex(cvertex_t *v1, cvertex_t *v2, float i, cvertex_t *result) {
	float ii = 1.0f-i;
	InterpolateMemVertex(&v1->p, &v2->p, i, &result->p);
	result->u = v1->u*ii + v2->u*i;
	result->v = v1->v*ii + v2->v*i;
}


qboolean UnderThresholdSimple(mmvertex_t *v1, mmvertex_t *v2) {
	vec3_t temp;
	VectorSubtract(v1->position, v2->position, temp);
	return (Length(temp) < gl_mesherror.value);
}


qboolean UnderThreshold(mmvertex_t *p1, mmvertex_t *p2, mmvertex_t *t) {
	vec3_t proj, dir;
	float len;
	ProjectPointOntoVector(t->position, p1->position, p2->position, proj);
	VectorSubtract(t->position, proj, dir);
	len = Length(dir);
	return (len < gl_mesherror.value);
}

/**
	The first three are vertices, the third is the center point
*/
void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
qboolean UnderThresholdTri(mmvertex_t *v1, mmvertex_t *v2, mmvertex_t *v3, mmvertex_t *m) {
	vec3_t proj, normal, t1, t2, dir;
	float len, dist, d;

	VectorSubtract(v1->position, v2->position, t1);
	VectorSubtract(v3->position, v2->position, t2);
	CrossProduct(t2,t1, normal);
	VectorNormalize(normal);
	dist = DotProduct(normal, v1->position);
	d = DotProduct(normal, m->position) - dist;

	return (d < gl_mesherror.value);
	/*ProjectPointOnPlane(proj, v1->position, normal);
	VectorSubtract(m->position, proj, dir);
	len = Length(dir);
	return (len < gl_mesherror.value);
	*/

//	return true;
}

void EvaluateCurve(curve_t *in, mmvertex_t *control, float u, float v, mmvertex_t *result) {
	EvaluateBiquadraticBeziers(control, in->controlwidth, in->controlheight, u, v, result);
}

/**
	Evaluates the curve for the parameters stored in the vertex
*/
void EvaluateCurveVertex(cvertex_t *v1, curve_t *in, mmvertex_t *control) {
	//clamp all u/v's to 65k boundaries to avoid sparklies where 2 curves meet
	//in one curve this is not really a problem due to vertex sharing.
	int u = (int)(((double)(v1->u))*65535.0);
	int v = (int)(((double)(v1->v))*65535.0);
	EvaluateCurve(in, control, u/65535.0f, v/65535.f, &v1->p);
}


static int * subdivIndices = NULL;
static unsigned int * subdivVertHash = NULL;
static mmvertex_t *subdivVerts = NULL;
static int subdivNumIndices = 0;
static int subdivNumVerts = 0;
static int subdivMaxIndices = 0;
static int subdivMaxVerts = 0;

static void ClearEmit(void) {
	if (subdivIndices) free(subdivIndices);
	if (subdivVerts) free(subdivVerts);
	if (subdivVertHash) free(subdivVertHash);
	subdivIndices = NULL;
	subdivVerts = NULL;
	subdivVertHash = NULL;
	subdivNumIndices =  0;
	subdivNumVerts =  0;
	subdivMaxIndices = 0;
	subdivMaxVerts = 0;
}

static unsigned int HashCurveVertex(cvertex_t *v) {
	return (((unsigned int)(((double)(v->u))*65535.0))<<16) + ((unsigned int)(((double)(v->v))*65535.0));
}

static int AllocateCurveVertex(cvertex_t *v) {
	unsigned int hash;
	int i;

	if (subdivNumVerts+1 >= subdivMaxVerts) {
		subdivMaxVerts += 64;
		subdivVerts = realloc(subdivVerts, subdivMaxVerts*sizeof(mmvertex_t));
		subdivVertHash = realloc(subdivVertHash, subdivMaxVerts*sizeof(int));
	}

	hash =  HashCurveVertex(v);
	//Con_Printf("Hash %i\n", hash);
	for (i=0; i<subdivNumVerts; i++) {
		if (hash == subdivVertHash[i])
			return i;
	}

	subdivVerts[subdivNumVerts] = v->p;
	subdivVertHash[subdivNumVerts] = hash;
	subdivNumVerts ++;
	return subdivNumVerts-1;
}

static void EmitQuad(cvertex_t *corners) {
	int vertInds[4];
	int i;
	//enough space for indices
	if (subdivNumIndices+6 >= subdivMaxIndices) {
		subdivMaxIndices += 64;
		subdivIndices = realloc(subdivIndices, subdivMaxIndices*sizeof(int));
	}

	//emit it
	for (i=0; i<4; i++) {
		vertInds[i] = AllocateCurveVertex(&corners[i]);
	}

	subdivIndices[subdivNumIndices+0] = vertInds[2];
	subdivIndices[subdivNumIndices+1] = vertInds[1];
	subdivIndices[subdivNumIndices+2] = vertInds[0];
	subdivIndices[subdivNumIndices+3] = vertInds[0];
	subdivIndices[subdivNumIndices+4] = vertInds[3];
	subdivIndices[subdivNumIndices+5] = vertInds[2];

	subdivNumIndices += 6;
}

static qboolean DegenerateEdge(cvertex_t *v1, cvertex_t *v2) {

	return (degenerateDist(v1->p.position, v2->p.position));
}

/**
	Returns the index of the degenerate edge, or -1 if none
*/
static int DegenerateQuad(cvertex_t *corners) {
	int i;
	for (i=0; i<4; i++) {
		int ii = (i+1)%4;
		if (DegenerateEdge(&corners[i], &corners[ii])) {
			return i;
		}
	}
	return -1;
}

/**
Returns true if the triangle is degenerate
	(simple degenerate 3 verts identical it does not detect the 3 linear verts case)
*/
static int DegenerateTri(cvertex_t *p, cvertex_t *q, cvertex_t *r) {
	return (DegenerateEdge(p,q) || DegenerateEdge(q,r) || DegenerateEdge(r,p));
}

static void EmitTri(cvertex_t *p1, cvertex_t *p2 ,cvertex_t *p3) {

	//Degenerate triangle? return immediately
	if (DegenerateTri(p1,p2,p3)) {
		return;
	}

	//enough space for indices
	if (subdivNumIndices+3 >= subdivMaxIndices) {
		subdivMaxIndices += 64;
		subdivIndices = realloc(subdivIndices, subdivMaxIndices*sizeof(int));
	}

	//emit it
	subdivIndices[subdivNumIndices+2] = AllocateCurveVertex(p1);
	subdivIndices[subdivNumIndices+1] = AllocateCurveVertex(p2);
	subdivIndices[subdivNumIndices+0] = AllocateCurveVertex(p3);

	subdivNumIndices += 3;
}

#define MAX_SUBDIV_DEPTH 10
static void AdaptiveSubdivideTri(cvertex_t *p, cvertex_t *q, cvertex_t *r,
								 int e1t, int e2t, int e3t,
								 curve_t *in, mmvertex_t *control, int depth);

static void SubdivideQuad(cvertex_t *corners, qboolean *parentLock, curve_t *in, mmvertex_t *control, int depth) {
	qboolean lock[4];
	qboolean allLock = true;
	cvertex_t midp[4], evalp[4], arg[4], center;
	float centeru, centerv;
	int i, degInd;

	//Degenerate quads continue as triangles
	//Don't degenerate depth 0 quads as they can be legitimately degenerate
	//as is the case with a bezier where to eges touch (a droplet like shape)
	if (depth > 0) {
		degInd = DegenerateQuad(corners);
		if (degInd >= 0) {
			int indexes[3];
			int numInd = 0;
			for (i=0; i<4; i++) {
				if (i!= degInd)
				indexes[numInd++] = i;
			}
			AdaptiveSubdivideTri(&corners[indexes[0]], &corners[indexes[1]], &corners[indexes[2]], 0, 0, 0, in, control, depth+1);
		}
	}

	if (depth >= MAX_SUBDIV_DEPTH) {
		EmitQuad(corners);
		return;
	}

	for (i=0; i<4; i++) {
		int ni = (i+1)%4;
		InterpolateCurveVertex(&corners[i],&corners[ni],0.5, &midp[i]);
		AverageCurveVertexParams(&corners[i],&corners[ni], &evalp[i]);
		EvaluateCurveVertex(&evalp[i], in, control);
		lock[i] = parentLock[i] || UnderThreshold(&corners[i].p,&corners[ni].p,&evalp[i].p);
		if (!lock[i]) allLock = false;
		//lock[i] = false;
		//allLock = false;
	}

	if (allLock) {
		EmitQuad(corners);
		return;
	}

	//InterpolateMemVertex(&midp[0],&midp[2],0.5, &center);
	center.u = (corners[0].u+corners[1].u+corners[2].u+corners[3].u)*0.25;
	center.v = (corners[0].v+corners[1].v+corners[2].v+corners[3].v)*0.25;
	EvaluateCurveVertex(&center, in, control);

	//create 4 subquads
	/*arg[0] = corners[0];
	arg[1] = (lock[0]) ? midp[0] : evalp[0];
	arg[2] = center;
	arg[3] = (lock[3]) ? midp[3] : evalp[3];
	argu[0] = cornu[0];
	argv[0] = cornv[0];
	argu[1] = (cornu[0]+cornu[1])*0.5;
	argv[1] = (cornv[0]+cornv[1])*0.5;
	argu[2] = centeru;
	argv[2] = centerv;
	argu[3] = (cornu[3]+cornu[0])*0.5;
	argv[3] = (cornv[3]+cornv[0])*0.5;
	SubdivideQuad(arg, argu, argv, lock, in, control);*/
/*
	//only one edge remains unlocked split in two triangles
	if ((lock[0] && lock[1] && lock[2] && !lock[3]) ||
		(lock[1] && lock[2] && lock[3] && !lock[0]) ||
		(lock[0] && lock[2] && lock[3] && !lock[1]) ||
		(lock[0] && lock[1] && lock[3] && !lock[2]))
	{
		cvertex_t ccorners[4];

		for (i=0; i<4; i++) {
			ccorners[i].u = cornu[i];
			ccorners[i].v = cornv[i];
			ccorners[i].p = corners[i];
		}

		AdaptiveSubdivideTri(&ccorners[0], &ccorners[1], &ccorners[2], in, control, depth+1);
		AdaptiveSubdivideTri(&ccorners[0], &ccorners[2], &ccorners[3], in, control, depth+1);	

		return;
	}
*/
	//only split in two horizontal quads
	if (lock[1] && lock[3] && !lock[0] && !lock[2]) {
		arg[0] = corners[0];
		arg[1] = (lock[0]) ? midp[0] : evalp[0];
		arg[2] = (lock[2]) ? midp[2] : evalp[2];
		arg[3] = corners[3];
		SubdivideQuad(arg, lock, in, control, depth+1);

		arg[0] = (lock[0]) ? midp[0] : evalp[0];
		arg[1] = corners[1];
		arg[2] = corners[2];
		arg[3] = (lock[2]) ? midp[2] : evalp[2];
		SubdivideQuad(arg, lock, in, control, depth+1);

		return;
	}

	//only split in two horizontal quads
	if (lock[0] && lock[2] && !lock[1] && !lock[3]) {
		arg[0] = corners[0];
		arg[1] = corners[1];
		arg[2] = (lock[1]) ? midp[1] : evalp[1];
		arg[3] = (lock[3]) ? midp[3] : evalp[3];
		SubdivideQuad(arg, lock, in, control, depth+1);

		arg[0] = (lock[3]) ? midp[3] : evalp[3];
		arg[1] = (lock[1]) ? midp[1] : evalp[1];
		arg[2] = corners[2];
		arg[3] = corners[3];
		SubdivideQuad(arg, lock, in, control, depth+1);

		return;
	}

	//All unlocked make 4 quads
	if (!lock[0] && !lock[2] && !lock[1] && !lock[3]) {
		for (i=0; i<4; i++) {
			int i1 = (i+1)%4;
			int i2 = (i+2)%4;
			int i3 = (i+3)%4;

			arg[i] = corners[i];
			arg[i1] = (lock[i]) ? midp[i] : evalp[i];
			arg[i2] = center;
			arg[i3] = (lock[i3]) ? midp[i3] : evalp[i3];
			SubdivideQuad(arg, lock, in, control, depth+1);
		}
	//three edges locked or corner locks, make 2 triangles
	} else {
		AdaptiveSubdivideTri(&corners[0], &corners[1], &corners[2], 0, 0, 1, in, control, depth+1);
		AdaptiveSubdivideTri(&corners[0], &corners[2], &corners[3], 1, 0, 0, in, control, depth+1);	
	}

	//edges sharing a corner are locked, also split in tris
	/*if ((lock[0] && lock[1]) ||
	(lock[1] && lock[2]) ||
	(lock[2] && lock[3]) ||
	(lock[3] && lock[0]))
	{*/
}

void Curve2MeshQuadTree(curve_t *in, mmvertex_t *verts, mesh_t *out) {
	int		i, j, k, l, w, h;
	float	prev, next;
	float	du, dv, u ,v;
	qboolean lock[4] = {false, false, false, false};
	cvertex_t args[4];
	du = 1.0f/(in->controlwidth-1);
	dv = 1.0f/(in->controlheight-1);

	ClearEmit();

	args[0].u = 0.0f;
	args[0].v = 0.0f;
	args[1].u = 1.0f;
	args[1].v = 0.0f;
	args[2].u = 1.0f;
	args[2].v = 1.0f;
	args[3].u = 0.0f;
	args[3].v = 1.0f;

	for (k=0; k<4; k++) {
		EvaluateCurveVertex(&args[k], in, verts);
	}
	SubdivideQuad(args, lock, in, verts, 0);
/*	
	for (i=0, u=0; i<in->controlwidth-1; i++, u+=du) {
		for (j=0, v=0; j<in->controlheight-1; j++, v+=dv) {
			//controle punten evalueeren!!
			//args[0] = verts[i+j*in->controlwidth];
			//args[1] = verts[i+1+j*in->controlwidth];
			//args[2] = verts[i+1+(j+1)*in->controlwidth];
			//args[3] = verts[i+(j+1)*in->controlwidth];
			argsu[0] = u;
			argsu[1] = u+du;
			argsu[2] = u+du;
			argsu[3] = u;
			argsv[0] = v;
			argsv[1] = v;
			argsv[2] = v+dv;
			argsv[3] = v+dv;
			for (k=0; k<4; k++) {
				EvaluateCurve(in, verts, argsu[k], argsv[k], &args[k]);
			}
			SubdivideQuad(args, argsu, argsv, lock, in, verts, 0);
		}
	}
*/	

	if (!subdivNumVerts) {
		out->numvertices = 0;
		out->numindecies = 0;
		out->indecies = NULL;
		out->numtriangles = 0;
		return;
	}

	//allocate vertices
	out->numvertices = subdivNumVerts;

	out->vertices = GL_StaticAlloc(subdivNumVerts*sizeof(mmvertex_t), subdivVerts);
	out->userVerts = Hunk_Alloc(subdivNumVerts*sizeof(vec3_t));
	for (i=0; i<subdivNumVerts; i++) {
		VectorCopy(subdivVerts[i].position,out->userVerts[i]);
	}
	/*
	out->firstvertex = R_AllocateVertexInTemp(subdivVerts[0].position, subdivVerts[0].texture, subdivVerts[0].lightmap, subdivVerts[0].color);
	for (i=1; i<subdivNumVerts; i++) {
		R_AllocateVertexInTemp(subdivVerts[i].position, subdivVerts[i].texture, subdivVerts[i].lightmap, subdivVerts[i].color);
	}*/

	//allocate indices
	out->numindecies = subdivNumIndices;
	out->indecies = Hunk_Alloc(subdivNumIndices*sizeof(int));
	out->numtriangles = out->numindecies/3;
	memcpy(out->indecies, subdivIndices, subdivNumIndices*sizeof(int));
	ClearEmit();
}

static void AdaptiveSubdivideTri(cvertex_t *p, cvertex_t *q, cvertex_t *r, 
								 int e1t, int e2t, int e3t,
								 curve_t *in, mmvertex_t *control, int depth) {
	qboolean tessPQ, tessQR, tessRP;
	cvertex_t tPQ, tQR, tPR, tPQR;

//Terminate prematurely?
	if (depth >= MAX_SUBDIV_DEPTH) {
		EmitTri(p,q,r);
		return;
	}

//Calculate midpoints in parameter space, and evaluate the curve
	AverageCurveVertexParams(p, q, &tPQ);
	AverageCurveVertexParams(q, r, &tQR);
	AverageCurveVertexParams(p, r, &tPR);
	EvaluateCurveVertex(&tPQ, in, control);
	EvaluateCurveVertex(&tQR, in, control);
	EvaluateCurveVertex(&tPR, in, control);

	//optimize, not always needed
	tPQR.u = (p->u + q->u + r->u) / 3.0f;
	tPQR.v = (p->v + q->v + r->v) / 3.0f;
	EvaluateCurveVertex(&tPQR, in, control);

	tessPQ = !((e1t) ? UnderThresholdTri(&p->p,&q->p, &r->p, &tPQ.p) : UnderThreshold(&p->p,&q->p, &tPQ.p));
	tessQR = !((e2t) ? UnderThresholdTri(&p->p,&q->p, &r->p, &tQR.p) : UnderThreshold(&q->p,&r->p, &tQR.p));
	tessRP = !((e3t) ? UnderThresholdTri(&p->p,&q->p, &r->p, &tPR.p) : UnderThreshold(&r->p,&p->p, &tPR.p));
/*
	tessPQ = !UnderThreshold(&p->p,&q->p, &tPQ.p);
	tessQR = !UnderThreshold(&q->p,&r->p, &tQR.p);
	tessRP = !UnderThreshold(&r->p,&p->p, &tPR.p);
*/
//Subdivide all edges
	if (tessPQ && tessQR && tessRP) {
        //Subdivide into 4 triangles
		AdaptiveSubdivideTri(p   , &tPQ, &tPR, e1t,   1, e3t, in, control, depth+1);
		AdaptiveSubdivideTri(q   , &tQR, &tPQ, e2t,   1, e1t, in, control, depth+1);
		AdaptiveSubdivideTri(r   , &tPR, &tQR, e3t,   1, e2t, in, control, depth+1);;
		AdaptiveSubdivideTri(&tPQ, &tQR, &tPR, 1, 1, 1, in, control, depth+1);;
//Two edge cases
	} else if (tessPQ && tessQR) {
		AdaptiveSubdivideTri(p   , &tPQ, r   , e1t,   1, e3t, in, control, depth+1);
		AdaptiveSubdivideTri(&tPQ, &tQR, r   ,   1, e2t,   1, in, control, depth+1);
		AdaptiveSubdivideTri(&tPQ, q   , &tQR, e1t, e2t,   1, in, control, depth+1);
	} else if (tessPQ && tessRP) {
		AdaptiveSubdivideTri(p   , &tPQ, &tPR, e1t,   1, e3t, in, control, depth+1);
		AdaptiveSubdivideTri(&tPQ, q   , &tPR, e1t,   1,   1, in, control, depth+1);
		AdaptiveSubdivideTri(&tPR, q   , r   ,   1, e2t, e3t, in, control, depth+1);
	} else if (tessQR && tessRP) {
		AdaptiveSubdivideTri(p   , q   , &tQR, e1t, e2t,   1, in, control, depth+1);
		AdaptiveSubdivideTri(p   , &tQR, &tPR,   1,   1, e3t, in, control, depth+1);
		AdaptiveSubdivideTri(&tPR, &tQR, r   ,   1, e2t, e3t, in, control, depth+1);
//One edge cases
	} else if (tessPQ) {
		AdaptiveSubdivideTri(p   , &tPQ, r   , e1t,   1, e3t, in, control, depth+1);
		AdaptiveSubdivideTri(q   , r   , &tPQ, e2t,   1, e1t, in, control, depth+1);
	} else if (tessQR) {
		AdaptiveSubdivideTri(p   , q   , &tQR, e1t, e2t,   1, in, control, depth+1);
		AdaptiveSubdivideTri(p   , &tQR, r   ,   1, e2t, e3t, in, control, depth+1);
	} else if (tessRP) {
		AdaptiveSubdivideTri(p   , q   , &tPR, e1t,   1, e3t, in, control, depth+1);
		AdaptiveSubdivideTri(&tPR, q   , r   ,   1, e2t, e3t, in, control, depth+1);
//Center point case
/*	} else if (!UnderThresholdTri(&p->p, &q->p, &r->p, &tPQR.p)) {
		AdaptiveSubdivideTri(&tPQR, p, q, in, control, depth+1);
		AdaptiveSubdivideTri(&tPQR, q, r, in, control, depth+1);
		AdaptiveSubdivideTri(&tPQR, r, p, in, control, depth+1);*/
//Terminating case
	} else {
		EmitTri(p, q, r);
	}
}

void Curve2MeshTriSubdiv(curve_t *in, mmvertex_t *verts, mesh_t *out) {
	int		i;
	cvertex_t corners[4];

	ClearEmit();

	//Evaluate at the 4 bezier corners
	corners[0].u = 0.0f;
	corners[0].v = 0.0f;
	corners[1].u = 1.0f;
	corners[1].v = 0.0f;
	corners[2].u = 1.0f;
	corners[2].v = 1.0f;
	corners[3].u = 0.0f;
	corners[3].v = 1.0f;

	EvaluateCurveVertex(&corners[0], in, verts);
	EvaluateCurveVertex(&corners[1], in, verts);
	EvaluateCurveVertex(&corners[2], in, verts);
	EvaluateCurveVertex(&corners[3], in, verts);

	//Subdivide two triangles
	AdaptiveSubdivideTri(&corners[0], &corners[1], &corners[2], 0, 0, 0, in, verts, 0);
	AdaptiveSubdivideTri(&corners[0], &corners[2], &corners[3], 0, 0, 0, in, verts, 0);

	//allocate vertices
	out->numvertices = subdivNumVerts;
	out->vertices = GL_StaticAlloc(subdivNumVerts*sizeof(mmvertex_t), subdivVerts);
	out->userVerts = Hunk_Alloc(subdivNumVerts*sizeof(vec3_t));
	for (i=0; i<subdivNumVerts; i++) {
		VectorCopy(subdivVerts[i].position,out->userVerts[i]);
	}
/*
	out->firstvertex = R_AllocateVertexInTemp(subdivVerts[0].position, subdivVerts[0].texture, subdivVerts[0].lightmap, subdivVerts[0].color);
	for (i=1; i<subdivNumVerts; i++) {
		R_AllocateVertexInTemp(subdivVerts[i].position, subdivVerts[i].texture, subdivVerts[i].lightmap, subdivVerts[i].color);
	}
*/
	//allocate indices
	out->numindecies = subdivNumIndices;
	out->indecies = Hunk_Alloc(subdivNumIndices*sizeof(int));
	out->numtriangles = out->numindecies/3;
	memcpy(out->indecies, subdivIndices, subdivNumIndices*sizeof(int));
	ClearEmit();
}

/**
* "Evaluates the controlpoints"
*/
void PutMeshOnCurve(curve_t in, mmvertex_t *verts) {
	int		i, j, l, w, h;
	float	prev, next;
	float	du, dv, u ,v;
	mmvertex_t results[128*128];
	du = 1.0f/(in.width-1);
	dv = 1.0f/(in.height-1);

	for (i=0, u=0; i<in.width; i++, u+=du) {
		for (j=0, v=0; j<in.height; j++, v+=dv) {
			EvaluateBiquadraticBeziers(verts,in.width,in.height,u,v,&results[i+j*in.width]);
		}
	}

	for (i=0; i<in.width*in.height; i++) {
		VectorCopy(results[i].position,verts[i].position);
	}
}

#define MAX_EXPANDED_AXIS 128

int	originalWidths[MAX_EXPANDED_AXIS];
int	originalHeights[MAX_EXPANDED_AXIS];

/**
* Removes colinear rows and colums, this reduces the triangle count if you have
* lots of nice and flat curves.
*/
mmvertex_t *RemoveLinearMeshColumnsRows(curve_t *inc, mmvertex_t *inverts) {
	int i, j, k;
	float len, maxLength;
	vec3_t proj, dir;
	static mmvertex_t	expand[MAX_EXPANDED_AXIS][MAX_EXPANDED_AXIS];
	mmvertex_t *verts;
	int width, height;

	width = inc->width;
	height = inc->height;

	for (i=0; i<inc->width; i++) {
		for (j=0; j<inc->height; j++) {
			expand[j][i] = inverts[i+j*width];
		}
	}

	//columns
	for (j=1; j<width - 1; j++) {
		maxLength = 0;
		for (i=0; i<height; i++) {
			ProjectPointOntoVector(expand[i][j].position, expand[i][j-1].position, expand[i][j+1].position, proj);
			VectorSubtract(expand[i][j].position, proj, dir);
			len = Length(dir);
			if (len > maxLength) {
				maxLength = len;
			}
		}
		if (maxLength < 0.1)
		{
			width--;
			for (i=0; i<height; i++) {
				for (k=j; k<width; k++) {
					expand[i][k] = expand[i][k+1];
				}
			}
			for (k=j; k<width; k++) {
				originalWidths[k] = originalWidths[k+1];
			}
			j--;
		}
	}

	//rows
	for (j=1; j<height - 1; j++) {
		maxLength = 0;
		for (i=0; i<width ; i++) {
			ProjectPointOntoVector(expand[j][i].position, expand[j-1][i].position, expand[j+1][i].position, proj);
			VectorSubtract(expand[j][i].position, proj, dir);
			len = Length(dir);
			if (len > maxLength) {
				maxLength = len;
			}
		}
		if (maxLength < 0.1)
		{
			height--;
			for (i=0; i<width; i++) {
				for (k = j; k < height; k++) {
					expand[k][i] = expand[k+1][i];
				}
			}
			for (k=j; k<height; k++) {
				originalHeights[k] = originalHeights[k+1];
			}
			j--;
		}
	}

	//verts are still in 128*128 array, convert to a with*height array
	verts = &expand[0][0];
	for (i=1; i<height; i++) {
		memmove( &verts[i*width], expand[i], width * sizeof(mmvertex_t) );
	}

	inc->width = width;
	inc->height = height;
	return verts;
}

/**
* Evaluate the mesh, subdivide the control grid amount times.
* Copies the resulting vertices to the out mesh.
*/
void SubdivideCurve(curve_t *in, mesh_t *out, mmvertex_t *verts, int amount) {
	int		i, j, l, w, h, newwidth, newheight;
	float	prev, next;
	float	du, dv, u ,v;
	mmvertex_t *expand;
	mmvertex_t *clean;

	newwidth = in->controlwidth*amount;
	newheight = in->controlheight*amount;

	//only a temporaly buffer
	expand = malloc(sizeof(mmvertex_t)*newwidth*newheight);

	if (!expand) Sys_Error("No more memory\n");

	du = 1.0f/(newwidth-1);
	dv = 1.0f/(newheight-1);

	for (i=0, u=0; i<newwidth; i++, u+=du) {
		for (j=0, v=0; j<newheight; j++, v+=dv) {
			EvaluateBiquadraticBeziers(verts,in->controlwidth,in->controlheight,u,v,&expand[i+j*newwidth]);
		}
	}

	in->width = newwidth;
	in->height = newheight;

	clean = RemoveLinearMeshColumnsRows(in, expand);
	out->numvertices = in->width*in->height;

	/*
	for (i=0; i<in->width*in->height; i++) {
		//put the vertices in the global vertex table
		if (i==0)
			out->firstvertex = R_AllocateVertexInTemp(clean[i].position, clean[i].texture, clean[i].lightmap, clean[i].color);
		else
			R_AllocateVertexInTemp(clean[i].position, clean[i].texture, clean[i].lightmap, clean[i].color);
	}*/
	out->vertices = GL_StaticAlloc(out->numvertices*sizeof(mmvertex_t), clean);
	out->userVerts = Hunk_Alloc(out->numvertices*sizeof(vec3_t));
	for (i=0; i<out->numvertices; i++) {
		VectorCopy(clean[i].position,out->userVerts[i]);
	}

	free(expand);
}

/**
* Setup de index table (vertices are already calculated we just setup the indexes here)
*/
void CreateCurveIndecies(curve_t *curve, mesh_t *mesh)
{
	int i,j, i1, i2, li1, li2;
	int w,h, index;
	qboolean sharedDeg;
	int degRemove = 0;
	vec3_t norm;

	h = curve->width;
	w = curve->height;

	mesh->numtriangles = (curve->width-1)*(curve->height-1)*2;

	mesh->numindecies = mesh->numtriangles*3;
	mesh->indecies = (int *)Hunk_Alloc(sizeof(int)*mesh->numindecies);

	li1 = h;
	li2 = 0;
	index = 0;
	for (i=0; i<w-1; i++) {

		li1 = (i+1)*h;
		li2 = i*h;
		for (j=1; j<h; j++) {
			i1 = j+(i+1)*h;
			i2 = j+i*h;


			sharedDeg = degenerateDist(mesh->userVerts[li1],mesh->userVerts[i2]);
			if (!sharedDeg &&
				!degenerateDist(mesh->userVerts[li2],mesh->userVerts[i2]) &&
				!degenerateDist(mesh->userVerts[li1],mesh->userVerts[li2]))
			{
				mesh->indecies[index++] = li2;
				mesh->indecies[index++] = li1;
				mesh->indecies[index++] = i2;
			} else
				degRemove++;


			if (!sharedDeg &&
				!degenerateDist(mesh->userVerts[li1],mesh->userVerts[i1]) &&
				!degenerateDist(mesh->userVerts[i2],mesh->userVerts[i1]))
			{
				mesh->indecies[index++] = i2;
				mesh->indecies[index++] = li1;
				mesh->indecies[index++] = i1;
			} else
				degRemove++;

			li1 = i1;
			li2 = i2;
		}
	}
	mesh->numtriangles-=degRemove;

	/*if (degRemove) {
		Con_Printf("Removed %i degenerate triangles\n",degRemove);
	}*/
}

/**
* Setup the tangentspace for the mesh
*   (also sets up the per triangle plane eq's for the shadow volume calculations)
*/
void CreateTangentSpace(mesh_t *mesh) {

	int i,j;
	int *num = malloc(sizeof(int)*mesh->numvertices);
	int *addIndecies;
	vec3_t tang, bin, v1, v2, norm;
	vec3_t *tangents, *binormals, *normals;
	mmvertex_t *vertices;

	addIndecies = (mesh->isExploded) ? mesh->unexplodedIndecies : mesh->indecies;
	Q_memset(num,0,sizeof(int)*mesh->numvertices);
	tangents = malloc(sizeof(vec3_t)*mesh->numvertices);
	Q_memset(tangents,0,sizeof(vec3_t)*mesh->numvertices);
	binormals = malloc(sizeof(vec3_t)*mesh->numvertices);
	Q_memset(binormals,0,sizeof(vec3_t)*mesh->numvertices);
	normals = malloc(sizeof(vec3_t)*mesh->numvertices);
	Q_memset(normals,0,sizeof(vec3_t)*mesh->numvertices);
	mesh->triplanes = Hunk_Alloc(sizeof(plane_t)*mesh->numtriangles);

	vertices = GL_MapToUserSpace(mesh->vertices);
	for (i=0; i<mesh->numtriangles; i++) {

		//FIXME: This needs texture coords so we read them from the vbo mem
		TangentForPoly(&mesh->indecies[i*3],vertices,tang,bin);
		NormalForPoly(&mesh->indecies[i*3],vertices,norm);

		//per triangle normal for shadow volume
		VectorCopy(norm,mesh->triplanes[i].normal);
		mesh->triplanes[i].dist = DotProduct(mesh->userVerts[mesh->indecies[i*3]],norm);

		//smooth tangent space basis
		for (j=0; j<3; j++) {
			VectorAdd(tangents[addIndecies[i*3+j]],tang,tangents[addIndecies[i*3+j]]);
			VectorAdd(binormals[addIndecies[i*3+j]],bin,binormals[addIndecies[i*3+j]]);
			VectorAdd(normals[addIndecies[i*3+j]],norm,normals[addIndecies[i*3+j]]);
			num[addIndecies[i*3+j]]++;
		}
	}
	GL_UnmapFromUserSpace(mesh->vertices);

	//Scale and normalize tangents
	for (i=0; i<mesh->numvertices; i++) {
		if (num[i] != 0) {
			VectorScale(tangents[i],1.0f/num[i],tangents[i]);
			VectorNormalize(tangents[i]);

			VectorScale(binormals[i],1.0f/num[i],binormals[i]);
			VectorNormalize(binormals[i]);

			VectorScale(normals[i],1.0f/num[i],normals[i]);
			VectorNormalize(normals[i]);

			//CrossProduct(mesh->binormals[i], mesh->tangents[i], mesh->normals[i]);
		} /*else Con_Printf("num == 0\n");*/
	}

	mesh->tangents = GL_StaticAlloc(sizeof(vec3_t)*mesh->numvertices, tangents);
	mesh->binormals = GL_StaticAlloc(sizeof(vec3_t)*mesh->numvertices, binormals);
	mesh->normals = GL_StaticAlloc(sizeof(vec3_t)*mesh->numvertices, normals);
	free(tangents);
	free(binormals);
	free(normals);
	free(num);
}

/**
* Setup neighbour pointers for the given triangle
* triangles points to a listf of numTris*3 indecies;
*/
int FindNeighbourMesh(int triIndex, int edgeIndex, int numTris, int *triangles, int *neighbours) {
	int i, j, v1, v0, found,foundj = 0;
	int *current = &triangles[triIndex*3];
	int *t;
	qboolean	dup;

	v0 = current[edgeIndex];
	v1 = current[(edgeIndex+1)%3];

	//XYZ
	found = -1;
	dup = false;
	for (i=0; i<numTris*3; i+=3) {
		if (i == triIndex*3) continue;
		t = &triangles[i];

		for (j=0; j<3; j++) {
			if (((current[edgeIndex] == triangles[i+j])
			    && (current[(edgeIndex+1)%3] == triangles[i+(j+1)%3]))
				||
			   ((current[edgeIndex] == triangles[i+(j+1)%3])
			    && (current[(edgeIndex+1)%3] == triangles[i+j])))
			{
				//no edge for this model found yet?
				if (found == -1) {
					found = i;
					foundj = j;
				}
				//the three edges story
				else
					dup = true;
			}

		}
	}

	//normal edge, setup neighbour pointers
	if (!dup) {
		if (found != -1)
			neighbours[found+foundj] = triIndex;
		if (found >= 0)
			return found/3;
		return found;
	}
	//naughty egde let no-one have the neighbour
	//Con_Printf("%s: warning: open edge added\n",loadname);
	return -1;
}

/**
* Setup neghbour pointers for all triangles (needed by shadow volumes)
*/
void SetupMeshConnectivity(mesh_t *m) {
	int i, j;
	int *indecies;

	m->neighbours = Hunk_Alloc(sizeof(int)*m->numtriangles*3);

	for (i=0; i<m->numtriangles*3; i++) {
		m->neighbours[i] = -1;
	}

	indecies = (m->isExploded) ? m->unexplodedIndecies : m->indecies;

	//Setup connectivity
	for (i=0; i<m->numtriangles; i++)
		for (j=0 ; j<3 ; j++) {
			//none found yet
			if (m->neighbours[i*3+j] == -1) {
				m->neighbours[i*3+j] = FindNeighbourMesh(i, j, m->numtriangles, indecies, m->neighbours);
			}
		}
}

/**
* Check if 2 vertices are equal
*	This just checks the position currently, it's used by the smooth normal calculations so we
*   may want to consider angle between the tris or texture coords in the future.
*/
qboolean compareVert(mmvertex_t *v1, mmvertex_t *v2) {
	return (v1->position[0] == v2->position[0]) &&
		   (v1->position[1] == v2->position[1]) &&
		   (v1->position[2] == v2->position[2]) &&
		   (v1->texture[0] == v2->texture[0]) &&
		   (v1->texture[1] == v2->texture[1]);

}

/**
* Sometimes q3map produces unique vertices for every triangle (if they have lightmap coords)
* so the smooth normal calculations will always produce per triangle normals.
* To solve this we create an extra index table unexplodedIndecies that points to the shared vertices
* for every triangle (so it will have wrong texture coords for some of the tris)
*/
void SetupUnexplodedIndecies(mesh_t *mesh) {
	mmvertex_t *vertices;
	int i, j;

	vertices = GL_MapToUserSpace(mesh->vertices);
	for (i=0; i<mesh->numindecies; i++) {
		mmvertex_t vert = vertices[mesh->indecies[i]];
		mesh->unexplodedIndecies[i] = -1;
		for (j=0; j<mesh->numvertices; j++) {
			if (compareVert(&vert,&vertices[j])) {
				mesh->unexplodedIndecies[i] = j;
				break;
			}
		}
		//
		if (mesh->unexplodedIndecies[i] == -1) {
			mesh->unexplodedIndecies[i]	= mesh->indecies[i];
		}
	}
	GL_UnmapFromUserSpace(mesh->vertices);
}

/**
* Smooth ormals are calculated for the unexplodedVertices, copy the smooth ones ove to the individual
* vertices of the triangles.
*/

void DistributeUnexplodedNormals(mesh_t *mesh) {

	int i, j;
	vec3_t *tangents;
	vec3_t *normals;
	vec3_t *binormals;

	tangents = GL_MapToUserSpace(mesh->tangents);
	binormals = GL_MapToUserSpace(mesh->binormals);
	normals = GL_MapToUserSpace(mesh->normals);
	for (i=0; i<mesh->numindecies; i++) {
		VectorCopy(normals[mesh->unexplodedIndecies[i]],normals[mesh->indecies[i]]);
		VectorCopy(tangents[mesh->unexplodedIndecies[i]],tangents[mesh->indecies[i]]);
		VectorCopy(binormals[mesh->unexplodedIndecies[i]],binormals[mesh->indecies[i]]);
	}
	GL_UnmapFromUserSpace(mesh->tangents);
	GL_UnmapFromUserSpace(mesh->binormals);
	GL_UnmapFromUserSpace(mesh->normals);
}


/**
* Setup this mesh's bounding box
*/
void SetupMeshBox(mesh_t *m) {

	int i;

	m->mins[0] = 10e10f;
	m->mins[1] = 10e10f;
	m->mins[2] = 10e10f;


	m->maxs[0] = -10e10f;
	m->maxs[1] = -10e10f;
	m->maxs[2] = -10e10f;

	for (i=0; i<m->numvertices; i++) {
		VectorMax(m->maxs, m->userVerts[i], m->maxs);
		VectorMin(m->mins, m->userVerts[i], m->mins);
	}
}



/*
=================
CurveCreate

Creates a curve from the given surface

=================
*/
void MESH_CreateCurve(dq3face_t *in, mesh_t *mesh, mapshader_t *shader)
{
	curve_t curve;
	memset(mesh,0,sizeof(mesh_t));
	curve.controlwidth = LittleLong(in->patchOrder[0]);
	curve.controlheight = LittleLong(in->patchOrder[1]);
	curve.firstcontrol = LittleLong(in->firstvertex);

	//just use the control points as vertices
	curve.firstvertex = LittleLong(in->firstmeshvertex);

	mesh->isExploded = false;

	//evaluate the mesh vertices
	//if (gl_mesherror.value > 0)
	//	SubdivideCurve(&curve, mesh, &tempVertices[curve.firstcontrol], gl_mesherror.value);
	//Curve2MeshTriSubdiv(&curve, &tempVertices[curve.firstcontrol], mesh);
	Curve2MeshQuadTree(&curve, &tempVertices[curve.firstcontrol], mesh);

	//Con_Printf("%i vertices\n", mesh->numvertices);
	//setup rest of the mesh
	mesh->shader = shader;
	mesh->lightmapIndex = ((LittleLong(in->lightofs)/2)/PACKED_LIGHTMAP_COUNT)*2;

	//CreateCurveIndecies(&curve, mesh);
	CreateTangentSpace(mesh);
	SetupMeshConnectivity(mesh);
	SetupMeshBox(mesh);

	mesh->trans.origin[0] = mesh->trans.origin[1] = mesh->trans.origin[2] = 0.0f;
	mesh->trans.angles[0] = mesh->trans.angles[1] = mesh->trans.angles[2] = 0.0f;
	mesh->trans.scale[0] = mesh->trans.scale[1] = mesh->trans.scale[2] = 1.0f;

	//PutMeshOnCurve(*curve,&tempVertices[curve->firstcontrol]);
	//SubdivideMesh(curve,gl_mesherror.value,1000,&tempVertices[curve->firstcontrol]);
//	Con_Printf("MeshCurve %i %i %i\n",curve->firstcontrol,curve->controlwidth,curve->controlheight);
}

void MESH_CreateInlineModel(dq3face_t *in, mesh_t *mesh, int *indecies, mapshader_t *shader)
{
	int i, firstVert = LittleLong(in->firstvertex);
	memset(mesh,0,sizeof(mesh_t));
	Con_Printf("Inline model\n");
	//setup stuff of mesh that was stored in the bsp file
	//note: endiannes is important here as it's from the file!

	mesh->numvertices = LittleLong(in->numvertices);
	mesh->numindecies = LittleLong(in->nummeshvertices);
	mesh->numtriangles = mesh->numindecies/3;

	Con_Printf("Triangles(%i) Vertices(%i) Indecies(%i)\n",mesh->numtriangles,mesh->numvertices,mesh->numindecies);

	mesh->isExploded = (mesh->numindecies == mesh->numvertices);

	mesh->indecies = (int *)Hunk_Alloc(sizeof(int)*mesh->numindecies);
	mesh->unexplodedIndecies = malloc(sizeof(int)*mesh->numindecies);

	for (i=0; i<mesh->numindecies; i++) {
		mesh->indecies[i] = LittleLong(indecies[i]);
	}

	mesh->vertices = GL_StaticAlloc(mesh->numvertices*sizeof(mmvertex_t),&tempVertices[firstVert]);
	mesh->userVerts = Hunk_Alloc(mesh->numvertices*sizeof(vec3_t));
	for (i=0; i<mesh->numvertices; i++) {
		VectorCopy(tempVertices[firstVert+i].position,mesh->userVerts[i]);
	}

	SetupUnexplodedIndecies(mesh);

	//setup rest of the mesh
	mesh->shader = shader;
	mesh->lightmapIndex = ((LittleLong(in->lightofs)/2)/PACKED_LIGHTMAP_COUNT)*2;

	CreateTangentSpace(mesh);
	SetupMeshConnectivity(mesh);
	SetupMeshBox(mesh);

	DistributeUnexplodedNormals(mesh);
	free(mesh->unexplodedIndecies);

	mesh->trans.origin[0] = mesh->trans.origin[1] = mesh->trans.origin[2] = 0.0f;
	mesh->trans.angles[0] = mesh->trans.angles[1] = mesh->trans.angles[2] = 0.0f;
	mesh->trans.scale[0] = mesh->trans.scale[1] = mesh->trans.scale[2] = 1.0f;
}

/**
*  Multiplies the curve's color with the current lightmap brightness.
*/
void MESH_SetupMeshColors(mesh_t *mesh)
{
	int i;
	mmvertex_t *vertices = GL_MapToUserSpace(mesh->vertices);
	for (i=0; i<mesh->numvertices; i++) {
		vertices[i].color[0] = (int)(vertices[i].color[0]*sh_lightmapbright.value);
		vertices[i].color[1] = (int)(vertices[i].color[1]*sh_lightmapbright.value);
		vertices[i].color[2] = (int)(vertices[i].color[2]*sh_lightmapbright.value);
	}
	GL_UnmapFromUserSpace(mesh->vertices);
}

/*
void CS_DrawAmbient(mcurve_t *curve)
{
	int i,j, i1, i2;
	int w,h;

	//GL_Bind(curve->texture->gl_texturenum);
	glShadeModel (GL_SMOOTH);
	//Con_Printf("Drawcurve %i %i %i\n",curve->firstvertex,curve->width,curve->height);
	h = curve->width;
	w = curve->height;
	for (i=0; i<w-1; i++) {
		c_brush_polys+= 2*(h-1);
		glBegin(GL_TRIANGLE_STRIP);
		for (j=0; j<h; j++) {
			i1 = curve->firstvertex+j+(i+1)*h;
			i2 = curve->firstvertex+j+i*h;

			glColor3ubv((byte *)&((float *)&globalVertexTable[i2])[7]);
			glTexCoord2fv(&((float *)&globalVertexTable[i2])[3]);
			glVertex3fv((float *)&globalVertexTable[i2]);

			glColor3ubv((byte *)&((float *)&globalVertexTable[i1])[7]);
			glTexCoord2fv(&((float *)&globalVertexTable[i1])[3]);
			glVertex3fv((float *)&globalVertexTable[i1]);
		}
		glEnd();
	}
}
*/
