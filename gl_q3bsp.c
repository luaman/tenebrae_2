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

Penta, load the quake3 textures supposing they will get overriden, we pass a
4 pixel texture to the loading routine...
=================
*/
void ModQ3_LoadTextures (lump_t *l)
{
	int		i, j, pixels, num, max, altmax;
	miptex_t	*mt;
	texture_t	*tx, *tx2;
	texture_t	*anims[10];
	texture_t	*altanims[10];
	dq3texture_t 	*in;

	if (!l->filelen)
	{
		loadmodel->textures = NULL;
		return;
	}
	
	if (l->filelen % sizeof(dq3texture_t))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
		
	loadmodel->numtextures = l->filelen/sizeof(dq3texture_t);
	loadmodel->textures = Hunk_AllocName (loadmodel->numtextures * sizeof(*loadmodel->textures) , loadname);

	in = (dq3texture_t *)(mod_base + l->fileofs);
	for (i=0 ; i<loadmodel->numtextures ; i++, in++)
	{
		tx = Hunk_AllocName (sizeof(texture_t) +4, loadname ); //allocate 4 bytes for a fake pixel map
		loadmodel->textures[i] = tx;
		
		memcpy (tx->name, in->name, sizeof(in->name));	
		tx->width = 2;
		tx->height = 2;
		for (j=0 ; j<MIPLEVELS ; j++)
			tx->offsets[j] = sizeof(texture_t); //point to fake pix map
		Con_Printf("Loading texture %s\n",tx->name);
		//let the shaders handle loading it further based on the texture's name...
		tx->gl_texturenum = GL_LoadTexture (tx->name, tx->width, tx->height, (byte *)(tx+1), true, false, true);
		tx->anim_total = 0;
		tx->anim_min = 0;
		tx->anim_max = 0;
		tx->anim_next = NULL;
	}
}

/*
=================
ModQ3_LoadLighting

No changes with quake1
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

	loadmodel->lightdata = Hunk_AllocName ( l->filelen, loadname);	
	memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
	loadmodel->numlightmaps =  l->filelen/sizeof(dq3lightmap_t);


}


/*
=================
ModQ3_LoadVisibility

No changes with quake1
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
	mvertex_t	*out;
	int			i, count;
	vec3_t		pos;
	float		tex[2], lightmap[2];
	byte		color[4];

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	
	/*
	Not needed all vertices are fetched from global vertex table
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;
	*/
	
	for ( i=0 ; i<count ; i++, in++, out++)
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
* Find a polygon who shares and edge with the given edge.
* Note: Q3 maps seem to share edges between 3 poly's sometimes, we use the same
* solution as with meshes and let noone have the edge.
*/
void findNeighbourForEdge(glpoly_t *poly, int eindex) {
	int i,j, eindex2;
	glpoly_t *p, *found = NULL;
	int foundj;
	qboolean dup = false;

	if (poly->numverts == 0) return;
	eindex2 = (eindex + 1) % poly->numverts;

	if (poly->neighbours[eindex] != NULL) return;
	for (i=0; i<loadmodel->numsurfaces; i++) {
		p = loadmodel->surfaces[i].polys;

		//check if it has a shared vertex
		for (j=0; j<p->numverts; j++) {
			if ((vertexEqual(&tempVertices[p->firstvertex+j],&tempVertices[poly->firstvertex+eindex]) 
				&& vertexEqual(&tempVertices[p->firstvertex+(j+1)%p->numverts],&tempVertices[poly->firstvertex+eindex2])
				)
				||
			   (vertexEqual(&tempVertices[p->firstvertex+(j+1)%p->numverts],&tempVertices[poly->firstvertex+eindex])
			   && vertexEqual(&tempVertices[p->firstvertex+j],&tempVertices[poly->firstvertex+eindex2])))
			{

				if (poly == p) {
					if (j != eindex) {
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
			found->neighbours[foundj] = poly;
		} else {
			noneighbour++;
		}
		poly->neighbours[eindex] = found;

	} else {
		//naughty egde let no-one have the neighbour
		poly->neighbours[eindex] = NULL;
		numnaughty++;
	}
	
}

void findNeighbours(glpoly_t *poly) {
	int j;

	if (poly->numverts == 0) return;

	for (j=0; j<poly->numverts; j++) {
		findNeighbourForEdge(poly,j);
	}

}

void nullNeighbours(glpoly_t *poly) {
	int i;
	for (i=0; i<poly->numverts; i++) {
		poly->neighbours[i] = NULL;
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
=================
*/
void ModQ3_LoadMeshVerts (lump_t *l)
{
	meshVerts = (int *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*meshVerts))
		Sys_Error ("MOD_LoadMeshVerts: funny lump size in %s",loadmodel->name);	
}

int FindPlane(vec3_t normal, vec3_t vertex) {
	int i;
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
	Con_Printf("Plane added: %f %f %f %f\n",normal[0],normal[1],normal[2],dist);
	VectorCopy(normal,loadmodel->planes[loadmodel->numplanes].normal);
	loadmodel->planes[loadmodel->numplanes].dist = dist;
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
	mtexinfo_t	*texout;
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

	//penta duplicate the s/t vecs as texinfo records...
	texout = Hunk_AllocName ( count*sizeof(*texout), loadname);	
	
	loadmodel->texinfo = texout;
	loadmodel->numtexinfo = count;
	
	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++, texout++)
	{
		out->firstedge = -1;
		out->numedges = 0;
		out->flags = 0;

		planenum = FindPlane(in->normal,tempVertices[in->firstvertex].position); //ENDIAN bug!! flip in->normal	
		out->plane = loadmodel->planes + planenum;
		out->texinfo = loadmodel->texinfo + surfnum;
				
	// lighting info
		out->samples = NULL;
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->lightmaptexturenum = 0;
		else
			out->lightmaptexturenum = i;
			
	//fill in the texinfo fields
		for (i=0; i<3; i++) {
			out->texinfo->vecs[0][i] = LittleFloat(in->vecs[0][i]);	
			out->texinfo->vecs[1][i] = LittleFloat(in->vecs[1][i]);	
			VectorInverse(out->texinfo->vecs[1]);
		}

		out->texinfo->vecs[0][3] = 1;
		out->texinfo->vecs[1][3] = 1;
		out->texinfo->mipadjust = 1; //Used still?
		out->texinfo->flags = 0;//Used??
		out->texinfo->texture = loadmodel->textures[LittleLong(in->texinfo)];
		
	// set the drawing flags flag
		if (!Q_strncmp(out->texinfo->texture->name,"sky",3))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			//GL_SubdivideSurface (out);	// cut up polygon for warps => Don't cut up it distorts the global vertex table vertex order
			continue;
		}
		
		if (!Q_strncmp(out->texinfo->texture->name,"*",1))		// turbulent
		{
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			//GL_SubdivideSurface (out);	// cut up polygon for warps => Don't cut up it distorts the global vertex table vertex order
			continue;
		}
		

		//Con_Printf("Surface Type %i NumVert: %i NumIndex: %i\n",in->type,in->numvertices,in->nummeshvertices);
		
		//fill in the glpoly
		poly = Hunk_Alloc (sizeof(glpoly_t)+(LittleLong(in->nummeshvertices)-1)*sizeof(int));
		poly->numverts = in->numvertices;
		poly->numindecies = in->nummeshvertices;
		poly->firstvertex = in->firstvertex;
		poly->neighbours = (glpoly_t **)Hunk_Alloc (poly->numverts*sizeof(glpoly_t *));
		poly->next = NULL;
		poly->flags = out->flags;

		if (in->type == 2) {
			if (loadmodel->numcurves < MAX_MAP_CURVES) {
				CS_Create(in,&loadmodel->curves[loadmodel->numcurves],out->texinfo->texture);
				//HACK HACK: So when we try all surfaces in the leafs we now the number of the curve
				//we added here...
				in->patchOrder[0] = loadmodel->numcurves;
				loadmodel->numcurves++;
			} else Con_Printf("Warning: MAX_MAP_CURVES exceeded");
			in->numvertices = 0;
			poly->numverts = 0;
		}

		//fill in the index lists
		for (i=0; i<LittleLong(in->nummeshvertices); i++) {
			poly->indecies[i] = poly->firstvertex+meshVerts[i+LittleLong(in->firstmeshvertex)];
		}

		out->polys = poly;
		nullNeighbours(poly);
		out->numedges = poly->numverts;
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
	mtexinfo_t	*info;
	glpoly_t	*poly;
	FILE	*f;
	char	cache[MAX_QPATH], fullpath[MAX_OSPATH];
	int mod = loadmodel->numsurfaces / 10;
	int progress = 0;
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
		info = s->texinfo;
		poly = s->polys;

		if (poly->numverts == 0)
			continue;
		
		//checkHasCenterSplit(poly);

		//fill in the glpoly
		TangentForPoly(poly->indecies,&tempVertices[0],info->vecs[0],info->vecs[1]);
		//Scale up the tangent space a bit while this is a bit hacky it makes the bump look
		//much better, "more deep".
		VectorScale(info->vecs[0],r_tangentscale.value,info->vecs[0]);
		VectorScale(info->vecs[1],r_tangentscale.value,info->vecs[1]);
		findNeighbours(poly);

	}
	Con_Printf("  Checked %i edges\n",numnormal);
	Con_Printf("  %i single edges\n",noneighbour);
	Con_Printf("  Ignored %i manyfold edges\n",numnaughty);
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

		//Create the leafcurves by going over all surfaces in this leaf
		out->firstcurve = loadmodel->numleafcurves;
		out->numcurves = 0;
		for (j = 0; j<out->nummarksurfaces; j++) {
			dq3face_t *surf =  faces + leaffaces[LittleLong(in->firstmarksurface)+j];
			if (surf->type == 2) {
				if (loadmodel->numleafcurves < MAX_MAP_LEAFCURVES) {
					loadmodel->leafcurves[loadmodel->numleafcurves] = surf->patchOrder[0];
					loadmodel->numleafcurves++;
					out->numcurves++;
					//Con_Printf("Curve in leaf %i\n",surf->patchOrder[0]);
				} else Con_Printf("Warning: MAX_MAP_LEAFCURVES exceeded");
			}
		}
		
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
		out->texture = loadmodel->textures[LittleLong(in->texture)];
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
	loadmodel->numleafcurves = 0;
	loadmodel->leafcurves = Hunk_Alloc(MAX_MAP_LEAFCURVES*sizeof(int));
}

void ModQ3_AllocCurves (void)
{
	loadmodel->numcurves = 0;
	loadmodel->curves = Hunk_Alloc(MAX_MAP_CURVES*sizeof(mcurve_t));
}

void ModQ3_InitAreas(void) {
	loadmodel->numareas = 1;
	loadmodel->areas = &map_areas[0];
	memset(map_areas,0,sizeof(map_areas));
}

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
		submodel->radius = RadiusFromBounds (submodel->mins, submodel->maxs);
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