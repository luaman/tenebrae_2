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

void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
	vec3_t pVec, vec;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize(vec);
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
}

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

	for (i=0; i<in->width*in->height; i++) {
		//put the vertices in the global vertex table
		if (i==0)
			out->firstvertex = R_AllocateVertexInTemp(clean[i].position, clean[i].texture, clean[i].lightmap, clean[i].color);
		else
			R_AllocateVertexInTemp(clean[i].position, clean[i].texture, clean[i].lightmap, clean[i].color);
	}

	free(expand);
}

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


			sharedDeg = degenerateDist(globalVertexTable[mesh->firstvertex+li1].position,globalVertexTable[mesh->firstvertex+i2].position);
			if (!sharedDeg &&
				!degenerateDist(globalVertexTable[mesh->firstvertex+li2].position,globalVertexTable[mesh->firstvertex+i2].position) &&
				!degenerateDist(globalVertexTable[mesh->firstvertex+li1].position,globalVertexTable[mesh->firstvertex+li2].position))
			{
				mesh->indecies[index++] = li2;
				mesh->indecies[index++] = li1;
				mesh->indecies[index++] = i2;
			} else 
				degRemove++;
			

			if (!sharedDeg &&
				!degenerateDist(globalVertexTable[mesh->firstvertex+li1].position,globalVertexTable[mesh->firstvertex+i1].position) &&
				!degenerateDist(globalVertexTable[mesh->firstvertex+i2].position,globalVertexTable[mesh->firstvertex+i1].position))
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

	addIndecies = (mesh->isExploded) ? mesh->unexplodedIndecies : mesh->indecies;
	Q_memset(num,0,sizeof(int)*mesh->numvertices);
	mesh->tangents = Hunk_Alloc(sizeof(vec3_t)*mesh->numvertices);
	mesh->binormals = Hunk_Alloc(sizeof(vec3_t)*mesh->numvertices);
	mesh->normals = Hunk_Alloc(sizeof(vec3_t)*mesh->numvertices);
	mesh->triplanes = Hunk_Alloc(sizeof(plane_t)*mesh->numtriangles);

	for (i=0; i<mesh->numtriangles; i++) {
		TangentForPoly(&mesh->indecies[i*3],&tempVertices[mesh->firstvertex],tang,bin);
		NormalForPoly(&mesh->indecies[i*3],&tempVertices[mesh->firstvertex],norm);

		//per triangle normal for shadow volume
		VectorCopy(norm,mesh->triplanes[i].normal);
		mesh->triplanes[i].dist = DotProduct(tempVertices[mesh->firstvertex+mesh->indecies[i*3]].position,norm);
		
		//smooth tangent space basis
		for (j=0; j<3; j++) {
			VectorAdd(mesh->tangents[addIndecies[i*3+j]],tang,mesh->tangents[addIndecies[i*3+j]]);
			VectorAdd(mesh->binormals[addIndecies[i*3+j]],bin,mesh->binormals[addIndecies[i*3+j]]);
			VectorAdd(mesh->normals[addIndecies[i*3+j]],norm,mesh->normals[addIndecies[i*3+j]]);
			num[addIndecies[i*3+j]]++;
		}
	}

	for (i=0; i<mesh->numvertices; i++) {
		if (num[i] != 0) {
			VectorScale(mesh->tangents[i],1.0f/num[i],mesh->tangents[i]);
			VectorNormalize(mesh->tangents[i]);

			VectorScale(mesh->binormals[i],1.0f/num[i],mesh->binormals[i]);
			VectorNormalize(mesh->binormals[i]);

			VectorScale(mesh->normals[i],1.0f/num[i],mesh->normals[i]);
			VectorNormalize(mesh->normals[i]);

			//CrossProduct(mesh->binormals[i], mesh->tangents[i], mesh->normals[i]);
		} /*else Con_Printf("num == 0\n");*/
	}

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

	int i, j;

	for (i=0; i<mesh->numindecies; i++) {
		mmvertex_t vert = tempVertices[mesh->firstvertex+mesh->indecies[i]];
		mesh->unexplodedIndecies[i] = -1;
		for (j=0; j<mesh->numvertices; j++) {
			if (compareVert(&vert,&tempVertices[mesh->firstvertex+j])) {
				mesh->unexplodedIndecies[i] = j;
				break;
			}
		}
		//
		if (mesh->unexplodedIndecies[i] == -1) {
			mesh->unexplodedIndecies[i]	= mesh->indecies[i];
		}
	}
}

/**
* Smooth ormals are calculated for the unexplodedVertices, copy the smooth ones ove to the individual
* vertices of the triangles.
*/
void DistribueUnexplodedNormals(mesh_t *mesh) {

	int i, j;

	for (i=0; i<mesh->numindecies; i++) {
		VectorCopy(mesh->normals[mesh->unexplodedIndecies[i]],mesh->normals[mesh->indecies[i]]);
		VectorCopy(mesh->tangents[mesh->unexplodedIndecies[i]],mesh->tangents[mesh->indecies[i]]);
		VectorCopy(mesh->binormals[mesh->unexplodedIndecies[i]],mesh->binormals[mesh->indecies[i]]);
	}
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
		VectorMax(m->maxs, tempVertices[m->firstvertex+i].position ,m->maxs);
		VectorMin(m->mins, tempVertices[m->firstvertex+i].position ,m->mins);
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

	curve.controlwidth = LittleLong(in->patchOrder[0]);
	curve.controlheight = LittleLong(in->patchOrder[1]);
	curve.firstcontrol = LittleLong(in->firstvertex);
	
	//just use the control points as vertices
	curve.firstvertex = LittleLong(in->firstmeshvertex);

	mesh->isExploded = false;

	//evaluate the mesh vertices
	if (gl_mesherror.value > 0)
		SubdivideCurve(&curve, mesh, &tempVertices[curve.firstcontrol], gl_mesherror.value);

	//setup rest of the mesh
	mesh->shader = shader;
	mesh->lightmapIndex = LittleLong(in->lightofs);

	CreateCurveIndecies(&curve, mesh);
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
	int i;

	Con_Printf("Inline model\n");
	//setup stuff of mesh that was stored in the bsp file
	//note: endiannes is important here as it's from the file!

	mesh->firstvertex = LittleLong(in->firstvertex);
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

	SetupUnexplodedIndecies(mesh);

	//setup rest of the mesh
	mesh->shader = shader;
	mesh->lightmapIndex = in->lightofs;

	CreateTangentSpace(mesh);
	SetupMeshConnectivity(mesh);
	SetupMeshBox(mesh);

	DistribueUnexplodedNormals(mesh);
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

	for (i=0; i<mesh->numvertices; i++) {
		globalVertexTable[i+mesh->firstvertex].color[0] = (int)(globalVertexTable[i+mesh->firstvertex].color[0]*sh_lightmapbright.value);
		globalVertexTable[i+mesh->firstvertex].color[1] = (int)(globalVertexTable[i+mesh->firstvertex].color[1]*sh_lightmapbright.value);
		globalVertexTable[i+mesh->firstvertex].color[2] = (int)(globalVertexTable[i+mesh->firstvertex].color[2]*sh_lightmapbright.value);
	}
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