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

Quake 3 bsp loading code...
Similar to md3 loading we convert the model at load time to a hybrid quake1/quake3 format...

*/

#include "quakedef.h"
  


model_t	*loadmodel;
byte	*mod_base;
qboolean is_q3map;
int		*meshVerts;
int		ExtraPlanes;

marea_t		map_areas[MAX_MAP_AREAS];

/*
=================
ModQ3_LoadTextures

Touches all the shaders, this will make sure all textures are effectively loaded.
=================
*/
void ModQ3_LoadTextures (lump_t *l)
{
	int		i, j, pixels, num, max, altmax;
	dq3texture_t 	*in;
	mapshader_t	*tx;

	if (!l->filelen)
	{
		loadmodel->mapshaders = NULL;
		return;
	}
	
	if (l->filelen % sizeof(dq3texture_t))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
		
	loadmodel->nummapshaders = l->filelen/sizeof(dq3texture_t);
	loadmodel->mapshaders = Hunk_AllocName (loadmodel->nummapshaders * sizeof(mapshader_t) , loadname);

	in = (dq3texture_t *)(mod_base + l->fileofs);
	for (i=0 ; i<loadmodel->nummapshaders ; i++, in++)
	{
		tx = &loadmodel->mapshaders[i];
		tx->shader = GL_ShaderForName(in->name);
		tx->texturechain = NULL;
		tx->numindecies = 0;
		tx->shader->flags |=  LittleLong(in->flags);
		tx->shader->contents |= LittleLong(in->contents);
	}
}

/*
=================
ModQ3_LoadLighting
=================
*/
void ModQ3_LoadLighting (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		loadmodel->numlightmaps = 0;
		return;
	}

	if (l->filelen % sizeof(dq3lightmap_t))
		Sys_Error ("MOD_LoadLighting: funny lump size in %s",loadmodel->name);

	if (!COM_CheckParm("-externallight")) {
		loadmodel->lightdata = Hunk_AllocName ( l->filelen, loadname);	
		memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
	}

	loadmodel->numlightmaps =  l->filelen/sizeof(dq3lightmap_t);
}


/*
=================
ModQ3_LoadVisibility
=================
*/
void ModQ3_LoadVisibility (lump_t *l)
{
	int	*in;
	int	numvecs;
	int	veclen;

	if (l->filelen < 2*sizeof(int))
	{
		loadmodel->visdata = NULL;
		return;
	}

	in = (int *)(mod_base + l->fileofs);

	numvecs = (*in);
	in++;
	veclen = (*in);
	in++;

	loadmodel->visdata = Hunk_AllocName ( numvecs*veclen, loadname);	
	memcpy (loadmodel->visdata, in, numvecs*veclen);
	loadmodel->numclusters = numvecs;
	loadmodel->clusterlen = veclen;
}


/*
=================
ModQ3_LoadEntities

No changes with quake1
=================
*/
void ModQ3_LoadEntities (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}
	loadmodel->entities = Hunk_AllocName ( l->filelen, loadname);	
	memcpy (loadmodel->entities, mod_base + l->fileofs, l->filelen);
}


/*
=================
ModQ3_LoadVertexes

Put the vertexes in the global vertex table.
=================
*/
void ModQ3_LoadVertexes (lump_t *l)
{
	dq3vertex_t	*in;
	int			i, count;
	vec3_t		pos;
	float		tex[2], lightmap[2];
	byte		color[4];

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	
	Con_DPrintf("%i vertexes in map\n", count);
	loadmodel->numvertexes = count;
	/*
	Not needed all vertices are fetched from global vertex table
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;
	*/
	
	loadmodel->userVerts = Hunk_AllocName (count*sizeof(vec3_t), loadname);	

	for ( i=0 ; i<count ; i++, in++)
	{
		pos[0] = LittleFloat (in->point[0]);
		pos[1] = LittleFloat (in->point[1]);
		pos[2] = LittleFloat (in->point[2]);
		
		tex[0] = LittleFloat (in->texCoord[0]); 
		tex[1] = LittleFloat (in->texCoord[1]); 
		
		lightmap[0] = LittleFloat (in->lightCoord[0]); 
		lightmap[1] = LittleFloat (in->lightCoord[1]); 
		
		color[0] = in->color[0];
		color[1] = in->color[1];
		color[2] = in->color[2];
		color[3] = in->color[3];

		R_AllocateVertexInTemp(pos, tex, lightmap, color);
		VectorCopy(pos,loadmodel->userVerts[i]);
	}
}

/*
=================
ModQ3_LoadSubmodels

Not in current q3 bsp's
=================
*/
void ModQ3_LoadSubmodels (lump_t *l)
{
	dq3model_t	*in;
	dmodel_t	*out;
	int			i, j, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = 0.0;
		}
		for (j=0 ; j<MAX_MAP_HULLS ; j++)
			out->headnode[j] = -1;
		out->visleafs = 0;
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
		out->firstbrush = LittleLong(in->firstbrush);
		out->numbrushes = LittleLong(in->numbrushes);
	}
}

qboolean vertexEqual(mmvertex_t *v1, mmvertex_t *v2) {
	vec3_t r;
	//VectorSubtract(v1->position,v2->position,r);
	//return (DotProduct(r,r) < 0.01);
	return (v1->position[0] == v2->position[0]) &&
		   (v1->position[1] == v2->position[1]) &&
		   (v1->position[2] == v2->position[2]);
}

int numnaughty;
int numnormal;
int noneighbour;
/**
* Find a polygon who shares an edge with the given edge.
* Note: Q3 maps seem to share edges between 3 poly's sometimes, we use the same
* solution as with meshes and let noone have the edge.
*/
void findNeighbourForEdge(glpoly_t *poly, int neighindex) {
	int i,j, eindex2, eindex;
	glpoly_t *p, *found = NULL;
	int foundj;
	qboolean dup = false;

	if (poly->numverts == 0) return;

	eindex = poly->neighbours[neighindex].p1;
	eindex2 = poly->neighbours[neighindex].p2;

	if (poly->neighbours[neighindex].n != NULL) return;
	for (i=0; i<loadmodel->numsurfaces; i++) {
		p = loadmodel->surfaces[i].polys;

		if(!p) continue; //surface is a curve or a md3
		if(loadmodel->surfaces[i].shader->shader->flags & SURF_NOSHADOW) continue; //no neighbours without shadows

		//check if it has a shared vertex
		for (j=0; j<p->numneighbours; j++) {
			int np1, np2;
		
			np1 = p->neighbours[j].p1;
			np2 = p->neighbours[j].p2;

			if ((vertexEqual(&tempVertices[np1],&tempVertices[eindex]) 
				&& vertexEqual(&tempVertices[np2],&tempVertices[eindex2]))
				||
			   (vertexEqual(&tempVertices[np2],&tempVertices[eindex])
			   && vertexEqual(&tempVertices[np1],&tempVertices[eindex2])))
			{

				if (poly == p) {
					if (j != neighindex) {
						Con_Printf("Panic! polygons has self matching edges\n");
					}
		
					continue;
				}

				/*
				if (p->neighbours[j]) {
					int k;
					//p already has a neighbour!
					//a 3 edger this is, kill it


					//take it away from it's old mate
					found = p->neighbours[j];

					if( p->neighbours[j] != &threeEdge) {
						for (k=0; k<found->numverts; k++) {
							if (found->neighbours[k] == p) {
								found->neighbours[k] = &threeEdge;
								break;
							}
						}
					}

					//take it away from p
					p->neighbours[j] = &threeEdge;

					//take it away from the new mate
					poly->neighbours[eindex] = &threeEdge;
				}

				poly->neighbours[eindex] = p;
				p->neighbours[j] = poly;
				return;
				*/
				
				//no edge for this model found yet?
				if (found == NULL) {
					found = p;
					foundj = j;
				}
				//the three edges story
				else
					dup = true;
					break;
				
			}
		}

		if (dup) break;
	}

	
	//normal edge, setup neighbour pointers
	if (!dup) {
		numnormal++;
		if (found) {
			found->neighbours[foundj].n = poly;
		} else {
			noneighbour++;
		}
		poly->neighbours[neighindex].n = found;

	} else {
		//naughty egde let no-one have the neighbour
		poly->neighbours[neighindex].n = NULL;
		numnaughty++;
	}
	
}

void findNeighbours(glpoly_t *poly) {
	int j;

	if (poly->numverts == 0) return;

	for (j=0; j<poly->numneighbours; j++) {
		findNeighbourForEdge(poly,j);
	}

}

/**
* Q3map sometimes adds a center vertex when the polygon can't be
* drawn as a tristrip (see SurfaceAsTriFan in surface.c of the q3 map source)
* we detect this (since we always draw as a fan) and fix it...
*//*
void checkHasCenterSplit(glpoly_t *poly) {
	int i, firstindex;
	mmvertex_t newfirst;
	if (poly->numverts < 5) return;

	//if the first vertex of every triangle is the same => we have a fan
	firstindex = poly->indecies[0];
	for (i=3; i<poly->numindecies; i+=3) {
		if (poly->indecies[i] != firstindex)
			//no fan, do nothing
			return;
	}

	poly->numverts--;
	//a fan q3map added a center vertex then, and it's the last one
	//move it to the first and move the rest

//	newfirst = tempVertices[poly->firstvertex+poly->numverts-1];
//	for (i=poly->numverts-2; i>=0; i--) {
//		tempVertices[poly->firstvertex+i+1] = tempVertices[poly->firstvertex+i];
//	}
//	tempVertices[poly->firstvertex] = newfirst;

}*/

float absf(float f) {
	
	if (f < 0)
		return -f;
	else
		return f;

}

/*
=================
ModQ3_LoadMeshVerts

These are intexes for the surfaces, we don't actually load them since they are copied to
the glpolygon_t structure when we load the faces...
FIXME: endianness
=================
*/
void ModQ3_LoadMeshVerts (lump_t *l)
{
	meshVerts = (int *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*meshVerts))
		Sys_Error ("MOD_LoadMeshVerts: funny lump size in %s",loadmodel->name);	
}

int FindPlane(vec3_t normal, vec3_t vertex) {
	int i, bits;
	float dist = DotProduct(vertex,normal);

	for (i=0; i<loadmodel->numplanes; i++) {
		//float d = DotProduct(normal,loadmodel->planes[i].normal);
		vec3_t diff;
		VectorSubtract(normal,loadmodel->planes[i].normal,diff);
		if ((Length(diff) < 0.001) &&
			(absf(dist - loadmodel->planes[i].dist) < 0.001))
		{
			return i;
		}	
	}

	if (ExtraPlanes <= 0) {
		Con_Printf("*** No planes left, ask your dealer for more planes ***\n");
		return 0;
	}
	ExtraPlanes--;
	//Con_Printf("Plane added: %f %f %f %f\n",normal[0],normal[1],normal[2],dist);
	VectorCopy(normal,loadmodel->planes[loadmodel->numplanes].normal);
	loadmodel->planes[loadmodel->numplanes].dist = dist;
	loadmodel->planes[loadmodel->numplanes].type = calcPlaneType(normal);
	bits = 0;
	for (i=0 ; i<3 ; i++)
	{
		if (normal[i] < 0)
			bits |= 1<<i;
	}
	loadmodel->planes[loadmodel->numplanes].signbits = bits;

	i = loadmodel->numplanes;
	loadmodel->numplanes++;
	return i;
}



void TangentForPoly(int *index, mmvertex_t *vertices,vec3_t Tangent, vec3_t Binormal) {
//see:
//http://members.rogers.com/deseric/tangentspace.htm
	vec3_t stvecs [3];
	float *v0, *v1, *v2;
	float *st0, *st1, *st2;
	vec3_t vec1, vec2;
	vec3_t planes[3];
	int i;

	v0 = &vertices[index[0]].position[0];
	v1 = &vertices[index[1]].position[0];
	v2 = &vertices[index[2]].position[0];
	st0 = &vertices[index[0]].texture[0];
	st1 = &vertices[index[1]].texture[0];
	st2 = &vertices[index[2]].texture[0];

	for (i=0; i<3; i++) {
		vec1[0] = v1[i]-v0[i];
		vec1[1] = st1[0]-st0[0];
		vec1[2] = st1[1]-st0[1];
		vec2[0] = v2[i]-v0[i];
		vec2[1] = st2[0]-st0[0];
		vec2[2] = st2[1]-st0[1];
		VectorNormalize(vec1);
		VectorNormalize(vec2);
		CrossProduct(vec1,vec2,planes[i]);
	}

	//Tangent = (-planes[B][x]/plane[A][x], -planes[B][y]/planes[A][y], - planes[B][z]/planes[A][z] )
	//Binormal = (-planes[C][x]/planes[A][x], -planes[C][y]/planes[A][y], -planes[C][z]/planes[A][z] )
	Tangent[0] = -planes[0][1]/planes[0][0];
	Tangent[1] = -planes[1][1]/planes[1][0];
	Tangent[2] = -planes[2][1]/planes[2][0];
	Binormal[0] = -planes[0][2]/planes[0][0];
	Binormal[1] = -planes[1][2]/planes[1][0];
	Binormal[2] = -planes[2][2]/planes[2][0];
	VectorNormalize(Tangent); //is this needed?
	VectorNormalize(Binormal);
}

void NormalForPoly(int *index, mmvertex_t *vertices,vec3_t Normal) {

	float *v0, *v1, *v2;
	vec3_t vec1, vec2;


	v0 = &vertices[index[0]].position[0];
	v1 = &vertices[index[1]].position[0];
	v2 = &vertices[index[2]].position[0];

	VectorSubtract(v0, v1, vec1);
	VectorSubtract(v2, v1, vec2);
	CrossProduct(vec1, vec2, Normal);
	VectorNormalize(Normal);
}

void setupNeighbours(glpoly_t *poly) {

	int i,j,k,m,p1,p2,p3,p4;
	int found;

	poly->numneighbours = 0;
	//for every triangle in the surface
	for (i=0; i<poly->numindecies; i+=3) {
		//for every edge of this triangle
		for (j=0; j<3; j++) {
			p1 = poly->indecies[i+j];
			p2 = poly->indecies[i+(j+1)%3];
			found = 0;
			//see if there is an triangle that has an edge matching this edge
			for (k=0; k<poly->numindecies; k+=3) {
				if (k==i) continue;
				for (m=0; m<3; m++) {
					p3 = poly->indecies[k+m];
					p4 = poly->indecies[k+(m+1)%3];
					if(((p1 == p3) && (p2 == p4)) || ((p1 == p4) && (p2 == p3))) {
						//found one!
						found = 1;
						//Con_Printf("Found internal edge\n");
						break;
					}
				}
				if (found) break;
			}
			if (!found) {
				//create a new neighbour structure
				if (poly->numneighbours >= poly->numverts) {
					Con_Printf("Bizar incident: Not enough neighbours\n");
					return;
				}
				poly->neighbours[poly->numneighbours].p1 = p1;
				poly->neighbours[poly->numneighbours].p2 = p2;
				poly->neighbours[poly->numneighbours].n = NULL;
				poly->numneighbours++;
			}
		}
	}
	//Con_Printf("Numneighbours: %i, NumVerts: %i | ",poly->numneighbours,poly->numverts);
}

/*
=================
ModQ3_LoadFaces

We load the faces and for every face we also fill in the mtexinfo field, thus
this also does some stuff that was done in LoadTexInfo...
We also already generate the glpolys for the surfaces...
=================
*/
void ModQ3_LoadFaces (lump_t *l)
{
	dq3face_t		*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;
	glpoly_t	*poly;
	vec3_t		cross;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	
	
	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		mmvertex_t *v;
		float lmScale, lmXOffset, lmYOffset;
		int firstVert, numVerts;

		out->firstedge = -1;
		out->numedges = 0;
		out->flags = 0;

		if (in->type == 1) {
			planenum = FindPlane(in->normal,tempVertices[in->firstvertex].position); //ENDIAN bug!! flip in->normal	
		} else {
			planenum = 0; //just any valid plane pointer...
		}
		out->plane = loadmodel->planes + planenum;
				
		// lighting info
		out->samples = NULL;
		out->lightmaptexturenum = LittleLong(in->lightofs);
			
		if ((out->lightmaptexturenum % 2) == 1) {
			//odd lightmap found disable dexlux.
			Cvar_Set ("sh_delux", "0");
		}

		out->shader = &loadmodel->mapshaders[LittleLong(in->texinfo)];
		out->flags = out->shader->shader->flags;
		out->visframe = -1;

		//Convert the lightmap texture coords to packed lightmap coords
		lmScale = 1/(float)PACKED_LIGHTMAP_COLUMS;
		lmXOffset = LIGHTMAP_COLUMN(out->lightmaptexturenum/2)*lmScale;
		lmYOffset = LIGHTMAP_ROW(out->lightmaptexturenum/2)*lmScale;
		numVerts = LittleLong(in->numvertices);
		firstVert = LittleLong(in->firstvertex);
		out->lightmaptexturenum = ((out->lightmaptexturenum/2)/PACKED_LIGHTMAP_COUNT)*2;

		for (i=0; i<numVerts; i++) {
			v = &tempVertices[firstVert+i];
			v->lightmap[0] = v->lightmap[0]*lmScale+lmXOffset;
			v->lightmap[1] = v->lightmap[1]*lmScale+lmYOffset;
		}

		// normal surface
		if (in->type == 1) {

			//fill in the glpoly
			poly = Hunk_Alloc (sizeof(glpoly_t)+(LittleLong(in->nummeshvertices)-1)*sizeof(int));
			poly->numverts = LittleLong(in->numvertices);
			poly->numindecies = LittleLong(in->nummeshvertices);
			poly->firstvertex = LittleLong(in->firstvertex);
			poly->neighbours = (mneighbour_t *)Hunk_Alloc (poly->numverts*sizeof(mneighbour_t));
			poly->next = NULL;
			poly->flags = out->flags;

			//fill in the index lists
			for (i=0; i<LittleLong(in->nummeshvertices); i++) {
				poly->indecies[i] = poly->firstvertex+LittleLong(meshVerts[i+LittleLong(in->firstmeshvertex)]);
			}

			out->polys = poly;
			setupNeighbours(out->polys);
			out->numedges = poly->numverts;
		// a bezier
		} else if (in->type == 2) {
			if (loadmodel->nummeshes < MAX_MAP_MESHES) {
				MESH_CreateCurve(in,&loadmodel->meshes[loadmodel->nummeshes],out->shader);
				//HACK HACK: So when we try all surfaces in the leafs we know the number of the curve
				//we added here...
				out->visframe = loadmodel->nummeshes;
				loadmodel->nummeshes++;
			} else Con_Printf("Warning: MAX_MAP_MESHES exceeded");
			in->numvertices = 0;
			in->nummeshvertices = 0;
			out->polys = NULL;
		//a md3
		} if (in->type == 3) {
			if (loadmodel->nummeshes < MAX_MAP_MESHES) {
				MESH_CreateInlineModel(in,&loadmodel->meshes[loadmodel->nummeshes],&meshVerts[LittleLong(in->firstmeshvertex)],out->shader);
				//HACK HACK: So when we try all surfaces in the leafs we know the number of the curve
				//we added here...
				out->visframe = loadmodel->nummeshes;
				loadmodel->nummeshes++;
			} else Con_Printf("Warning: MAX_MAP_MESHES exceeded");
			in->numvertices = 0;
			in->nummeshvertices = 0;
			out->polys = NULL;
		} else if (in->type != 1) {
			in->numvertices = 0;
			in->nummeshvertices = 0;
			out->polys = NULL;
		}
	}
}


/*
=================
ModQ3_SetupFaces

Does some post load face setup, this is calculating tangent space and doing neighbour finding
as they require everything to be loaded.
=================
*/
void ModQ3_SetupFaces (void)
{
	int surfnum;	
	msurface_t 	*s;
	glpoly_t	*poly;
	char	cache[MAX_QPATH], fullpath[MAX_OSPATH];
	int mod = loadmodel->numsurfaces / 10;
	int progress = 0;
	int fh;
	qboolean recalculate = false;

	//
	//Try to load cached connectivity, if anything goes wrong recalculate
	//
	sprintf(cache,"%s/cache/%s.con", com_gamedir, loadname);
	if (Sys_FileOpenRead (cache, &fh) >= 0) {
		char magic[4];
		int numSurf;
		int numNeigh;

		Sys_FileRead(fh, magic, 4);
		if (strcmp(magic,"TCON"))  {
			recalculate = true;
			goto close;
		}

		Sys_FileRead(fh, &numSurf, sizeof(int));
		if (numSurf != loadmodel->numsurfaces)  {
			recalculate = true;
			goto close;
		}

		//Write it out
		for (surfnum=0; surfnum<loadmodel->numsurfaces; surfnum++)
		{
			s = &loadmodel->surfaces[surfnum];
			poly = s->polys;

			if (!poly) continue;
			if(s->shader->shader->flags & SURF_NOSHADOW) continue;
			Sys_FileRead(fh, &numNeigh, sizeof(int));
			if (numNeigh != poly->numneighbours)  {
				recalculate = true;
				break;
			}
			Sys_FileRead(fh, poly->neighbours, poly->numneighbours*sizeof(mneighbour_t));
		}
	close:
		Sys_FileClose(fh);
	} else {
		recalculate = true;
	}

	//
	//Calculate tangent space and connectivity(if no cache was found)
	//
	if (mod < 1) mod = 1;
	numnaughty = 0;
	numnormal = 0;
	noneighbour = 0;
	for (surfnum=0; surfnum<loadmodel->numsurfaces; surfnum++)
	{
		if ((surfnum % mod) == 0) {
			if (progress < 11)
				progress++;
			Con_Printf("  %i%%\n",(progress-1)*10);
		}

		s = &loadmodel->surfaces[surfnum];
		poly = s->polys;

		if (!poly)
			continue;
		
		//checkHasCenterSplit(poly);

		//fill in the glpoly
		TangentForPoly(poly->indecies,&tempVertices[0],s->tangent,s->binormal);

		if(s->shader->shader->flags & SURF_NOSHADOW) continue;
		if (recalculate)
			findNeighbours(poly);
	}
	if (recalculate) {
		Con_Printf("  Checked %i edges\n",numnormal);
		Con_Printf("  %i single edges\n",noneighbour);
		Con_Printf("  Ignored %i manyfold edges\n",numnaughty);
	} else {
		Con_Printf("  Using cached connectivity\n");
	}

	//
	//Save the connectivity if we just calculated it
	//
	if (recalculate) {
		sprintf(cache,"%s/cache", com_gamedir);
		Sys_mkdir (cache);
		sprintf(cache,"%s/cache/%s.con", com_gamedir, loadname);
		fh = Sys_FileOpenWrite(cache);	
		Sys_FileWrite(fh, "TCON", 4);
		Sys_FileWrite(fh, &loadmodel->numsurfaces, sizeof(int));
		//Write it out
		for (surfnum=0; surfnum<loadmodel->numsurfaces; surfnum++)
		{
			s = &loadmodel->surfaces[surfnum];
			poly = s->polys;

			if (!poly) continue;
			if(s->shader->shader->flags & SURF_NOSHADOW) continue;
			Sys_FileWrite(fh, &poly->numneighbours, sizeof(int));
			Sys_FileWrite(fh, poly->neighbours, poly->numneighbours*sizeof(mneighbour_t));
		}
		Sys_FileClose(fh);
	}

/*
	// look for a cached version of the neighbour pointers
	sprintf(cache,"cache/%s.con",loadname);
	COM_FOpenFile (cache, &f);	
	if (f)
	{
		uLong adler, original_adler;

		fread(&original_adler,4,1,f);
		adler = adler32(0L, Z_NULL, 0);
		adler = adler32(adler, loadmodel->vertices, (loadmodel->numvertexes*sizeof(mmvertex_t)));
		if (adler != original_adler) error();

		Con_Printf("Loading edges...\n");
		for (surfnum=0; surfnum<loadmodel->numsurfaces; surfnum++)
		{
			int i;
			s = &loadmodel->surfaces[surfnum];
			poly = s->polys;

			if (poly->numverts == 0)
				continue;

			for (i=0; i<poly->numverts; i++) {
				int neigh;
				fread(&neigh,4,1,f);
				if (neigh == -1)
					poly->neighbours[i] = NULL;
				else
					poly->neighbours[i] = (loadmodel->surfaces+neigh)->polys;
			}
		}
		fclose (f);
	// none calculate and save it
	} else {

		Con_Printf("Calculating edges...\n");
		for (surfnum=0; surfnum<loadmodel->numsurfaces; surfnum++)
		{
			s = &loadmodel->surfaces[surfnum];
			poly = s->polys;

			if (poly->numverts == 0)
				continue;

			findNeighbours(poly);
		}

		// save out the cached version
		Con_Printf("Saving edges...\n");
		sprintf (fullpath, "%s/%s", com_gamedir, cache);
		f = fopen (fullpath, "wb");
		if (f)
		{
			for (surfnum=0; surfnum<loadmodel->numsurfaces; surfnum++)
			{
				int i;
				s = &loadmodel->surfaces[surfnum];
				poly = s->polys;

				if (poly->numverts == 0)
					continue;

				for (i=0; i<poly->numverts; i++) {
					int index;
					if (poly->neighbours[i] == NULL) {
						index = -1;
						fwrite (&index, 4, 1, f);
					} else {
						int k;
						for (k=0; k<loadmodel->numsurfaces; k++)
						{
							if (loadmodel->surfaces[k].polys == poly)
								break;
						}
						index = k;
						fwrite (&index, 4, 1, f);
					}
				}
			}
		}
		fclose (f);
	}
*/

}

/*
=================
ModQ3_SetParent
=================
*/
void ModQ3_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents & CONTENTS_LEAF)
		return;
	ModQ3_SetParent (node->children[0], node);
	ModQ3_SetParent (node->children[1], node);
}

/*
=================
ModQ3_LoadNodes

Small changes...
=================
*/
void ModQ3_LoadNodes (lump_t *l)
{
	int			i, j, count, p;
	dq3node_t		*in;
	mnode_t 	*out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = (l->filelen / sizeof(*in)) + 1;
	out = Hunk_AllocName ( (count+6)*sizeof(*out), loadname); //add the const to allocate space for the bounding box
	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleLong (in->mins[j]);
			out->minmaxs[3+j] = LittleLong (in->maxs[j]);
		}
	
		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		//where did this go?
		out->firstsurface = -1;
		out->numsurfaces = 0;
		out->contents = 0;
		
		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0) {
				out->children[j] = loadmodel->nodes + p;
				out->ichildren[j] = p;
			} else {
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
				out->ichildren[j] = p;
			}
		}
		out->parent = NULL;
	}
	
	ModQ3_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
ModQ3_LoadLeafs

  needs the surface lump as second item...
  needs the leafsurface lump as the third item
=================
*/
void ModQ3_LoadLeafs (lump_t *l, lump_t *sl, lump_t *ss)
{
	dq3leaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;
	dq3face_t * faces;
	int			*leaffaces;
	int			newnumsurf;
	msurface_t  *surf;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName (count*sizeof(*out), loadname);	
	faces = (void *)(mod_base + sl->fileofs);
	leaffaces = (void *)(mod_base + ss->fileofs);
	//Leaf 0 is a "solid" dummy leaf
/*	
	out->nummarksurfaces = 0;
	for (j=0 ; j<4 ; j++)
		out->ambient_sound_level[j] = 0;
	out->compressed_vis = 0;
	out->contents = -2;
	out->efrags = NULL;
	out->key = 0;
	out->parent = NULL;
	out->visframe = 0;
	out++;*/

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleLong (in->mins[j]);
			out->minmaxs[3+j] = LittleLong (in->maxs[j]);
		}

		//if (i == 0)
		//	out->contents = -2;
		//else
		out->contents = CONTENTS_LEAF; //not determined per leaf anymore, use leaf brushes instead...

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleLong(in->firstmarksurface);
		out->nummarksurfaces = LittleLong(in->nummarksurfaces);

		out->firstbrush = LittleLong(in->firstmarkbrush);
		out->numbrushes = LittleLong(in->nummakbrushes);
		out->area = LittleLong(in->areaportal) + 1;

		if ( out->area >= loadmodel->numareas ) {
			loadmodel->numareas = out->area + 1;
		}

		out->cluster = LittleLong(in->cluster);

		//Create the leafmeshes by going over all surfaces in this leaf
		//and move the meshes to a separate list
		out->firstmesh = loadmodel->numleafmeshes;
		out->nummeshes = 0;
		newnumsurf = 0;
		for (j = 0; j<out->nummarksurfaces; j++) {
			surf = loadmodel->marksurfaces[LittleLong(in->firstmarksurface)+j];
			if (surf->polys) {
				loadmodel->marksurfaces[LittleLong(in->firstmarksurface)+newnumsurf] = surf;
				newnumsurf++;
			} else if (surf->visframe > -1) {
				if (loadmodel->numleafmeshes < MAX_MAP_LEAFMESHES) {
					loadmodel->leafmeshes[loadmodel->numleafmeshes] = surf->visframe;
					loadmodel->numleafmeshes++;
					out->nummeshes++;
					//Con_Printf("Curve in leaf %i\n",surf->patchOrder[0]);
				} else Con_Printf("Warning: MAX_MAP_LEAFMESHES exceeded");
			}
		}
		out->nummarksurfaces = newnumsurf;
		
		p = LittleLong(in->cluster);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata;
		out->efrags = NULL;
		
		//No more abient q1 sounds in q3 maps...
		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = 0;

		//Index of this leaf... Spectacular huh?
		out->index = i+1;
		out->parent = NULL;
	}	
}


/*
=================
ModQ3_MakeHull0

Duplicate the drawing hull structure as a clipping hull
=================
*/
void ModQ3_MakeHull0(void)
{
	mnode_t		*in, *child;
	dclipnode_t *out;
	int			i, j, count;
	hull_t		*hull;
	
	hull = &loadmodel->hulls[0];	
	
	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents & CONTENTS_LEAF)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
ModQ3_MakeHull0

Deplicate the fine hull as coarse hulls for quake3 maps
=================
*/
void ModQ3_DuplicateHulls(void)
{
	hull_t		*hull;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = loadmodel->hulls[0].clipnodes;
	hull->firstclipnode = loadmodel->hulls[0].firstclipnode;
	hull->lastclipnode = loadmodel->hulls[0].lastclipnode;
	hull->planes = loadmodel->planes;
/*	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;*/
	
	hull = &loadmodel->hulls[2];
	hull->clipnodes = loadmodel->hulls[0].clipnodes;
	hull->firstclipnode = loadmodel->hulls[0].firstclipnode;
	hull->lastclipnode = loadmodel->hulls[0].lastclipnode;
	hull->planes = loadmodel->planes;
/*	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;	*/	
}
/*
=================
ModQ3_LoadMarksurfaces
=================
*/
void ModQ3_LoadMarksurfaces (lump_t *l)
{	
	int		i, j, count;
	long		*in;
	msurface_t **out;
	
	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong(in[i]);
		if (j >= loadmodel->numsurfaces)
			Sys_Error ("ModQ3_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

//Extra planes since some polys don't seem to have a plane stored in the planes list for them
//darn q3 map 
#define EXTRA_PLANES 240

int	calcPlaneType(vec3_t normal) {
	vec_t	ax, ay, az;
		
	if (normal[0] >= 1.0f)
		return PLANE_X;
	if (normal[1] >= 1.0f)
		return PLANE_Y;
	if (normal[2] >= 1.0f)
		return PLANE_Z;
		
	ax = fabs(normal[0]);
	ay = fabs(normal[1]);
	az = fabs(normal[2]);

	if (ax >= ay && ax >= az)
		return PLANE_ANYX;
	if (ay >= ax && ay >= az)
		return PLANE_ANYY;
	return PLANE_ANYZ;
}


/*
=================
ModQ3_LoadPlanes
=================
*/
void ModQ3_LoadPlanes (lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dq3plane_t 	*in;
	int			count;
	int			bits;
	
	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( (count+12+EXTRA_PLANES)*2*sizeof(*out), loadname); //add the const to allocate space for the bounding box	
	ExtraPlanes = EXTRA_PLANES;

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = calcPlaneType(out->normal);
		out->signbits = bits;
	}
}

/*
=================
ModQ3_LoadBrushSides
For collision detection
=================
*/
void ModQ3_LoadBrushSides (lump_t *l)
{
	dq3brushside_t	*in;
	mbrushside_t	*out;
	int				count, i;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( (count+6)*sizeof(*out), loadname); //add the const to allocate space for the bounding box
	
	loadmodel->brushsides = out;
	loadmodel->numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->plane = loadmodel->planes + LittleLong(in->plane);
		out->shader = &loadmodel->mapshaders[LittleLong(in->texture)];
	}
}

/*
=================
ModQ3_LoadBrushes
For collision detection
=================
*/
void ModQ3_LoadBrushes (lump_t *l, lump_t *texturelump)
{
	dq3brush_t		*in;
	dq3texture_t	*textures;
	mbrush_t		*out;
	int				count, i;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( (count+1)*sizeof(*out), loadname); //add the const to allocate space for the bounding box
	textures = (void *)(mod_base + texturelump->fileofs);	

	loadmodel->brushes = out;
	loadmodel->numbrushes = count;
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->firstbrushside = LittleLong(in->firstbrushside);
		out->numsides = LittleLong(in->numbrushsides);
		out->checkcount = 0;
		out->contents = textures[LittleLong(in->texture)].contents;
		//out->contents = //0; //ContentsForTexture(loadmodel->textures+in->texture);
	}
}

/*
=================
ModQ3_LoadLeafBrushes
For collision detection
=================
*/
void ModQ3_LoadLeafBrushes (lump_t *l)
{
	int		*in;
	int		*out;
	int		count, i;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( (count+1)*sizeof(*out), loadname); //add the const to allocate space for the bounding box
	
	loadmodel->leafbrushes = out;
	loadmodel->numleafbrushes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		*out = LittleLong(*in);
	}
}

void ModQ3_AllocLeafCurves (void)
{
	loadmodel->numleafmeshes = 0;
	loadmodel->leafmeshes = Hunk_Alloc(MAX_MAP_LEAFMESHES*sizeof(int));
}

void ModQ3_AllocCurves (void)
{
	loadmodel->nummeshes = 0;
	loadmodel->meshes = Hunk_Alloc(MAX_MAP_MESHES*sizeof(mesh_t));
}

void ModQ3_InitAreas(void) {
	loadmodel->numareas = 1;
	loadmodel->areas = &map_areas[0];
	memset(map_areas,0,sizeof(map_areas));
}

extern int numTempVertices;

void ModQ3_LoadBrushModel (model_t *mod, void *buffer)
{
	int			i, j;
	dq3header_t	*header;
	dmodel_t 	*bm;
	model_t		*submodel;
	char		name[5];

	loadmodel = mod;
	loadmodel->type = mod_brush;
	
	header = (dq3header_t *)buffer;

	i = LittleLong (header->version);
	if (i != Q3_BSPVERSION)
		Sys_Error ("ModQ3_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);

// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	ModQ3_AllocLeafCurves ();
	ModQ3_AllocCurves ();
	ModQ3_InitAreas ();

	Con_Printf("Loading vertices...\n");
	ModQ3_LoadVertexes (&header->lumps[Q3_LUMP_VERTEXES]);
	Con_Printf("Loading textures...\n");
	ModQ3_LoadTextures (&header->lumps[Q3_LUMP_TEXTURES]);
	Con_Printf("Loading lighting...\n");
	ModQ3_LoadLighting (&header->lumps[Q3_LUMP_LIGHTING]);
	Con_Printf("Loading planes...\n");
	ModQ3_LoadPlanes (&header->lumps[Q3_LUMP_PLANES]);
	Con_Printf("Loading MeshVerts...\n");
	ModQ3_LoadMeshVerts (&header->lumps[Q3_LUMP_MESHVERTEXES]);
	Con_Printf("Loading faces...\n");
	ModQ3_LoadFaces (&header->lumps[Q3_LUMP_FACES]);
	Con_Printf("Loading surface lists...\n");
	ModQ3_LoadMarksurfaces (&header->lumps[Q3_LUMP_MARKSURFACES]);
	Con_Printf("Loading visibility...\n");
	ModQ3_LoadVisibility (&header->lumps[Q3_LUMP_VISIBILITY]);
	Con_Printf("Loading leafs...\n");
	ModQ3_LoadLeafs (&header->lumps[Q3_LUMP_LEAFS],&header->lumps[Q3_LUMP_FACES],&header->lumps[Q3_LUMP_MARKSURFACES]);
	Con_Printf("Loading nodes...\n");
	ModQ3_LoadNodes (&header->lumps[Q3_LUMP_NODES]);
	Con_Printf("Loading entities...\n");
	ModQ3_LoadEntities (&header->lumps[Q3_LUMP_ENTITIES]);
	Con_Printf("Loading submodels...\n");
	ModQ3_LoadSubmodels (&header->lumps[Q3_LUMP_MODELS]);
	Con_Printf("Loading brushsides...\n");
	ModQ3_LoadBrushSides (&header->lumps[Q3_LUMP_BRUSHSIDES]);
	Con_Printf("Loading brushes...\n");
	ModQ3_LoadBrushes (&header->lumps[Q3_LUMP_BRUSHES], &header->lumps[Q3_LUMP_TEXTURES]);
	Con_Printf("Loading leafbrushes...\n");
	ModQ3_LoadLeafBrushes (&header->lumps[Q3_LUMP_LEAFBRUSHES]);

	ModQ3_MakeHull0 ();
	ModQ3_DuplicateHulls();

	Con_Printf("Configuring faces...\n");
	ModQ3_SetupFaces ();
	Con_Printf("%ikb of vertex data (%i vertices)\n", (numTempVertices*sizeof(mmvertex_t))/1024, numTempVertices);

	mod->numframes = 2;		// regular and alternate animation

	for (i=1 ; i<mod->numsubmodels ; i++)
	{
		//get a model
		sprintf (name, "*%i", i);
		submodel = Mod_FindName (name);
		//copy all stuff to the new model
		*submodel = *mod;
		strcpy (submodel->name, name);
		//copy submodel specific data to the new model
		bm = &mod->submodels[i];
		submodel->firstmodelsurface = bm->firstface;
		submodel->nummodelsurfaces = bm->numfaces;
		submodel->firstmodelbrush = bm->firstbrush;
		submodel->nummodelbrushes = bm->numbrushes;
		VectorCopy (bm->maxs, submodel->maxs);
		VectorCopy (bm->mins, submodel->mins);
		submodel->radius = Mod_RadiusFromBounds (submodel->mins, submodel->maxs);
		submodel->numleafs = bm->visleafs;
		submodel->hulls[0].firstclipnode = 0;//bm->headnode[0];
		for (j=1 ; j<MAX_MAP_HULLS ; j++)
		{
			submodel->hulls[j].firstclipnode = 0;//bm->headnode[j];
			submodel->hulls[j].lastclipnode = 0;//mod->numclipnodes-1;
		}
	}

//
// set up the submodels (FIXME: this is confusing)
//
/*
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = 0;//bm->headnode[0];
		for (j=1 ; j<MAX_MAP_HULLS ; j++)
		{
			mod->hulls[j].firstclipnode = 0;//bm->headnode[j];
			mod->hulls[j].lastclipnode = 0;//mod->numclipnodes-1;
		}
		
		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;
		
		VectorCopy (bm->maxs, mod->maxs);
		VectorCopy (bm->mins, mod->mins);

		mod->radius = RadiusFromBounds (mod->mins, mod->maxs);

		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels-1)
		{	// duplicate the basic information
			char	name[10];

			sprintf (name, "*%i", i+1);
			loadmodel = Mod_FindName (name);
			*loadmodel = *mod;
			strcpy (loadmodel->name, name);
			mod = loadmodel;
		}
	}
*/
//	Sys_Error("Finished loading q3 bsp\n");
}