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

*/

// gl_shadow.c stencil shadow support for quake

#include "quakedef.h"

int numShadowLights;
int numStaticShadowLights;
int numUsedShadowLights; //number of shadow lights acutally drawn this frame

shadowlight_t shadowlights[MAXSHADOWLIGHTS];
shadowlight_t *usedshadowlights[MAXUSEDSHADOWLIGHS];
shadowlight_t *currentshadowlight;


int volumeCmdsBuff[MAX_VOLUME_COMMANDS+128]; //Hack protect against slight overflows
float volumeVertsBuff[MAX_VOLUME_VERTS+128];
lightcmd_t	lightCmdsBuff[MAX_LIGHT_COMMANDS+128];
lightcmd_t	lightCmdsBuffMesh[MAX_LIGHT_COMMANDS+128];
int numVolumeCmds;
int numLightCmds;
int numLightCmdsMesh;
int numVolumeVerts;

msurface_t *shadowchain; //linked list of polygons that are shadowed
mesh_t *meshshadowchain;

byte *lightvis;
byte worldvis[MAX_MAP_LEAFS/8];

extern aabox_t worldbox;

/* -DC- isn't that volumeVertsBuff ?
vec3_t volumevertices[MAX_VOLUME_VERTICES];//buffer for the vertices of the shadow volume
int usedvolumevertices;
*/

void DrawVolumeFromCmds(int *volumeCmds, lightcmd_t *lightCmds, float *volumeVerts);
void DrawAttentFromCmds(lightcmd_t *lightCmds);
void DrawBumpFromCmds(lightcmd_t *lightCmds);
void DrawSpecularBumpFromCmds(lightcmd_t *lightCmds);
void PrecalcVolumesForLight(model_t *model);
int getVertexIndexFromSurf(msurface_t *surf, int index, model_t *model);
qboolean R_ContributeFrame(shadowlight_t *light);

/*
=============
AllocShadowLight
=============
*/
shadowlight_t* AllocShadowLight(void) {

	if (numShadowLights >= MAXSHADOWLIGHTS) {
		return NULL;
	}

	shadowlights[numShadowLights].owner = NULL;
	numShadowLights++;
	return &shadowlights[numShadowLights-1];
}

void DrawLightVolumeInfo(void) {

	int i;
	aabox_t b;
	if (!sh_showlightvolume.value) return;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glColor3f(0.0,0.0,1.0);
	for (i=0; i<numUsedShadowLights; i++) {
		shadowlight_t *l = usedshadowlights[i];
		if (!l->shadowchainfilled) continue;

		b = intersectBoxes(&l->box, &worldbox);
		drawBoxWireframe(&b);
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
}

void BoxForRadius(shadowlight_t *l) {
	l->box.mins[0] = l->origin[0] - l->radius;
	l->box.mins[1] = l->origin[1] - l->radius;
	l->box.mins[2] = l->origin[2] - l->radius;

	l->box.maxs[0] = l->origin[0] + l->radius;
	l->box.maxs[1] = l->origin[1] + l->radius;
	l->box.maxs[2] = l->origin[2] + l->radius;

	l->radiusv[0] = l->radius;
	l->radiusv[1] = l->radius;
	l->radiusv[2] = l->radius;

	//l->filtercube = 0;
}

/*
=============
R_ShadowFromDlight
=============
*/
void R_ShadowFromDlight(dlight_t *light) {

	shadowlight_t *l;

	l = AllocShadowLight();
	if (!l) return;

	VectorCopy(light->origin,l->origin);
	l->radius = light->radius;
	l->color[0] = light->color[0];
	l->color[1] = light->color[1];
	l->color[2] = light->color[2];
	l->baseColor[0] = light->color[0];
	l->baseColor[1] = light->color[1];
	l->baseColor[2] = light->color[2];
	l->style = 0;
	l->brightness = 1;
	l->isStatic = false;
	l->numVisSurf = 0;
	l->visSurf = NULL;
	l->style = light->style;
	l->owner = light->owner;

	//VectorCopy(light->angles,l->angles);
	
	//We use some different angle convention
	l->angles[1] = light->angles[0];
	l->angles[0] = light->angles[2];
	l->angles[2] = light->angles[1];

	l->filtercube = light->filtercube;
	l->rspeed = 0;
	l->cubescale = 1;
	l->castShadow = true;

	/*PENTA: This crashes with explosions, wich are sprite models :D
	if (l->owner->model && (l->owner->model->type == mod_sprite)) {
		msprite_t *psprite = l->owner->model->cache.data;
		l->shader = psprite->frames[0].shader;
	} else {*/
		l->shader = GL_ShaderForName("textures/lights/default");
	/*}*/

	//Some people will be instulted by the mere existence of this flag.
	if (light->pflags & PFLAG_NOSHADOW) {
		l->castShadow = false;
	} else {
		l->castShadow = true;
	}

	if (light->pflags & PFLAG_HALO) {
		l->halo = true;
	} else {
		l->halo = false;
	}

	BoxForRadius(l);
}

/*
=============
R_MarkDLights

Adds dynamic lights to the shadow light list
=============
*/
void R_MarkDlights (void)
{
	int		i;
	dlight_t	*l;

	l = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, l++)
	{
		if (l->die < cl.time || !l->radius)
			continue;
		R_ShadowFromDlight(l);
	}
}

/*
=============
R_ShadowFromEntity
=============
*/
void R_ShadowFromEntity(entity_t *ent) {

	shadowlight_t *l;

        l = AllocShadowLight();
	if (!l) return;

	VectorCopy(ent->origin,l->origin);
	l->radius = 350;
	l->color[0] = 1;
	l->color[1] = 1;
	l->color[2] = 1;
	l->style = 0;
	l->brightness = 1;
	l->isStatic = false;
	l->numVisSurf = 0;
	l->visSurf = NULL;
	l->style = 0;
	l->owner = ent;

	l->style = ent->style;

	if (ent->light_lev != 0) {
		l->radius = ent->light_lev;
	} else {
		l->radius = 350;
	}

	VectorCopy(ent->color,l->baseColor);

	if ((l->baseColor[0] == 0) && (l->baseColor[1] == 0) && (l->baseColor[2] == 0)) {
		l->baseColor[0] = 1;
		l->baseColor[1] = 1;
		l->baseColor[2] = 1;
	}

	l->filtercube = 0;

	//We use some different angle convention
	l->angles[1] = ent->angles[0];
	l->angles[0] = ent->angles[2];
	l->angles[2] = ent->angles[1];


	l->rspeed = ent->alpha*512;
	l->cubescale = 1;
	l->shader = GL_ShaderForName("textures/lights/default");

	//Some people will be instulted by the mere existence of this flag.
	if (ent->pflags & PFLAG_NOSHADOW) {
		l->castShadow = false;
	} else {
		l->castShadow = true;
	}

	if (ent->pflags & PFLAG_HALO) {
		l->halo = true;
	} else {
		l->halo = false;
	}

        BoxForRadius(l);
}

int cut_ent;

/*
=============
R_InitShadowsForFrame

Do per frame intitialization for the shadows
=============
*/
void R_InitShadowsForFrame(void) {

	byte *vis;
	int i;

	numShadowLights = numStaticShadowLights;

	R_MarkDlights (); //add dynamic lights to the list
//	R_ShadowFromPlayer();//give the player some sort of torch

	numUsedShadowLights = 0;

	//if (cut_ent) Con_Printf("cut ents: %i\n",cut_ent);
	cut_ent = 0;
	Q_memset (&worldvis, 0, MAX_MAP_LEAFS/8); //all invisible

	vis = Mod_LeafPVS (r_viewleaf, cl.worldmodel);
	Q_memcpy(&worldvis, vis, MAX_MAP_LEAFS/8);

	for (i=0; i<numShadowLights; i++) {
		currentshadowlight = &shadowlights[i];
		if (R_ContributeFrame(currentshadowlight)) {
			currentshadowlight->visible = true;
			if (numUsedShadowLights < MAXUSEDSHADOWLIGHS) {
				usedshadowlights[numUsedShadowLights] = currentshadowlight;
				numUsedShadowLights++;
			} else {
				Con_Printf("R_InitShadowsForFrame: More than MAXUSEDSHADOWLIGHS lights for frame\n");
			}
		} else {
			currentshadowlight->visible = false;
		}
	}
}

#define MAX_LEAF_LIST 32
mleaf_t *leafList[MAX_LEAF_LIST];
int numLeafList;
extern vec3_t		r_emins, r_emaxs;	// <AWE> added "extern".

/*
=============
R_ProjectSphere

Returns the rectangle the sphere will be in when it is drawn.
FIXME: This is crappy code we draw a "sprite" and project those points
it should be possible to analytically derive a eq.

=============
*/

/*
void R_ProjectSphere (shadowlight_t *light, int *rect)
{
	int		i, j;
	float	a;
	vec3_t	v, vp2;
	float	rad;
	double minx, maxx, miny, maxy;
	double px, py, pz;

	rad = light->radius;
	
	//rect[0] = 100;
	//rect[1] = 100;
	//rect[2] = 300;
	//rect[3] = 300;

	//return;
	
	VectorSubtract (light->origin, r_origin, v);

	//dave - slight fix
	VectorSubtract (light->origin, r_origin, vp2);
	VectorNormalize(vp2);

	minx = 1000000;
	miny = 1000000;
	maxx = -1000000;
	maxy = -1000000;

	for (i=32 ; i>=0 ; i--)
	{
		a = i/32.0 * M_PI*2;
		for (j=0 ; j<3 ; j++)
			v[j] = light->origin[j] + vright[j]*cos(a)*rad + vup[j]*sin(a)*rad;

		gluProject(v[0], v[1], v[2], r_Dworld_matrix, r_Dproject_matrix,
                           (GLint *) r_Iviewport, &px, &py, &pz);			// <AWE> added cast.

		if (px > maxx) maxx = px;
		if (px < minx) minx = px;
		if (py > maxy) maxy = py;
		if (py < miny) miny = py;

	}

	rect[0] = (int)minx;
	rect[1] = (int)miny;
	rect[2] = (int)maxx;
	rect[3] = (int)maxy;	
}
*/

/*
=============

R_ProjectBoundingBox

	Returns the screen rectangle this light's bounding box will be in when it is drawn.

=============
*/
void R_ProjectBoundingBox (shadowlight_t *light, int *rect) {

	vec3_t dest;
	double minx, maxx, miny, maxy;
	double px, py, pz;
	int i, j, k;
	static float verts[9*3];
	matrix_4x4 modelviewproj, world, proj;
	matrix_1x4 point, res;
	aabox_t box;
	minx = 100000000;
	miny = 100000000;
	maxx = -100000000;
	maxy = -100000000;

	box = intersectBoxes(&light->box, &worldbox);

	i=0;
	VectorConstruct(box.maxs[0],box.maxs[1],box.maxs[2],&verts[i]);
	i+=3;
	VectorConstruct(box.mins[0],box.maxs[1],box.maxs[2],&verts[i]);
	i+=3;
	VectorConstruct(box.maxs[0],box.mins[1],box.maxs[2],&verts[i]);
	i+=3;
	VectorConstruct(box.mins[0],box.mins[1],box.maxs[2],&verts[i]);
	i+=3;
	VectorConstruct(box.mins[0],box.mins[1],box.mins[2],&verts[i]);
	i+=3;
	VectorConstruct(box.maxs[0],box.mins[1],box.mins[2],&verts[i]);
	i+=3;
	VectorConstruct(box.maxs[0],box.maxs[1],box.mins[2],&verts[i]);
	i+=3;
	VectorConstruct(box.mins[0],box.maxs[1],box.mins[2],&verts[i]);
	i+=3;

	glGetFloatv (GL_MODELVIEW_MATRIX, &world[0][0]);
//	glMatrixMode(GL_PROJECTION);
//	glPushMatrix();
	//gluPerspective(90,  r_Iviewport[0]/ (float)r_Iviewport[1], 0.1, 2000.0);
	glGetFloatv (GL_PROJECTION_MATRIX, &proj[0][0]);
//	glGetDoublev (GL_PROJECTION_MATRIX, &r_Dproject_matrix[0]);

	Mat_Mul_4x4_4x4(world, proj, modelviewproj);

	for (i=0; i<8; i++) {
		//float s;
		//float *v = &verts[i*3];
		
		point[0] = verts[i*3+0];
		point[1] = verts[i*3+1];
		point[2] = verts[i*3+2];
		point[3] = 1.0f;
		Mat_Mul_1x4_4x4(point, modelviewproj, res);
		
		//gluProject(v[0], v[1], v[2], r_Dworld_matrix, r_Dproject_matrix,
         //    (GLint *) r_Iviewport, &px, &py, &pz);

		//if (res[3] < 0) {
		//Con_Printf("%f %f\n", (float)px, (float)py);
		//s = (pz < 0) ? -1 : 1;


		px = (res[0]*(1/res[3])+1.0) * 0.5;
		py = (res[1]*(1/res[3])+1.0) * 0.5;

		px = px * r_Iviewport[2] + r_Iviewport[0];
		py = py * r_Iviewport[3] + r_Iviewport[1];

		//Con_Printf("%f %f\n", (float)px, (float)py);

			//continue;
		//}
		
		/*
		if (px < r_Iviewport[0]) px = r_Iviewport[0];
		if (py < r_Iviewport[1]) py = r_Iviewport[1];
		if (px > r_Iviewport[0]+r_Iviewport[2]) px = r_Iviewport[0]+r_Iviewport[2];
		if (py > r_Iviewport[1]+r_Iviewport[3]) py = r_Iviewport[1]+r_Iviewport[3];
		*/

		if (px > maxx) maxx = px;
		if (px < minx) minx = px;
		if (py > maxy) maxy = py;
		if (py < miny) miny = py;
	}

/*	glPopMatrix();
	glGetDoublev (GL_PROJECTION_MATRIX, &r_Dproject_matrix[0]);
	glMatrixMode(GL_MODELVIEW);*/
	/*
	for (i=-1; i<=1; i+=2) {
		for (j=-1; j<=1; j+=2) {
			for (k=-1; k<=1; k+=2) {
				MakeVertex(i*light->radius, j*light->radius, k*light->radius, light->origin, dest);
				Con_Printf("%f %f %f\n",dest[0],dest[1],dest[2]);
				gluProject(dest[0], dest[1], dest[2], r_Dworld_matrix, r_Dproject_matrix,
                           (GLint *) r_Iviewport, &px, &py, &pz);


				if (px > maxx) maxx = px;
				if (px < minx) minx = px;
				if (py > maxy) maxy = py;
				if (py < miny) miny = py;
			}
		}
	}
	*/
	
	/*
	if (minx < (double)r_Iviewport[0]) minx = (double)r_Iviewport[0];
	if (miny < (double)r_Iviewport[1]) miny = (double)r_Iviewport[1];
	if (maxx > (double)r_Iviewport[0]+r_Iviewport[2]) maxx = (double)r_Iviewport[0]+r_Iviewport[2];
	if (maxy > (double)r_Iviewport[1]+r_Iviewport[3]) maxy = (double)r_Iviewport[1]+r_Iviewport[3];
	*/

	rect[0] = (int)minx;
	rect[1] = (int)miny;
	rect[2] = (int)maxx;
	rect[3] = (int)maxy;
}

/*
=============

HasSharedLeaves

	Returns true if both vis arrays have shared leafs visible.
	FIXME: compare bytes at a time (what does quake fill the unused bits with??)

=============
*/
qboolean HasSharedLeafs(byte *v1, byte *v2) {

	int i;
	for (i=0 ; i<cl.worldmodel->numclusters; i++)
	{
		if (v1[i>>3] & (1<<(i&7)))
		{
			if (v2[i>>3] & (1<<(i&7)))
				return true;

		}
	}
	return false;
}

/*
=============

HasVisibleClusters

	Returns true if the light has leaves shared with the clusters in v2.

=============
*/
qboolean HasVisibleClusters(shadowlight_t *l, byte *v2) {

	int i;
	for (i=0 ; i<cl.worldmodel->numleafs; i++)
	{
		int cluster = cl.worldmodel->leafs[i].cluster;
		//leaf is visible from the light?
		if (l->leafvis[i>>3] & (1<<(i&7)))
		{
			//cluster is visible from the camera?
			if (v2[cluster>>3] & (1<<(cluster&7)))
				return true;

		}
	}
	return false;
}


/**************************************************************


	Code to determine what stuff is visible to the light.


***************************************************************/


/*
=============

CheckSurfInLight

  Returns true if the surface is lit by the light. (Of course approximately)

=============
*/
qboolean CheckSurfInLight(msurface_t *surf, shadowlight_t *light) 
{
	mplane_t	*plane;
	float		dist;
	glpoly_t	*poly;

	//return true;

	//Doesn't recieve per pixel light or cast shadows...
	if (!(surf->flags & SURF_PPLIGHT) && (surf->flags & SURF_NOSHADOW)) {
		return false;
	}

	plane = surf->plane;

	poly = surf->polys;

	if (poly->lightTimestamp == r_lightTimestamp) {
		return false;
	}

	switch (plane->type)
	{
	case PLANE_X:
		dist = light->origin[0] - plane->dist;
		break;
	case PLANE_Y:
		dist = light->origin[1] - plane->dist;
		break;
	case PLANE_Z:
		dist = light->origin[2] - plane->dist;
		break;
	default:
		dist = DotProduct (light->origin, plane->normal) - plane->dist;
		break;
	}

	//the normals are flipped when surf_planeback is 1
	if (((surf->flags & SURF_PLANEBACK) && (dist > 0)) ||
		(!(surf->flags & SURF_PLANEBACK) && (dist < 0)))
	{
		return false;
	}

	//the normals are flipped when surf_planeback is 1
	if ( abs(dist) > light->radius)
	{
		return false;
	}

	//Doesn't intersect light volume
	if (light->isStatic)
	{
		vec3_t mins = {10e10f,10e10f,10e10f};
		vec3_t maxs = {-10e10f,-10e10f,-10e10f};
		int i;
		for (i=0; i<poly->numindecies; i++) {
			VectorMax(maxs,cl.worldmodel->userVerts[poly->indecies[i]],maxs);
			VectorMin(mins,cl.worldmodel->userVerts[poly->indecies[i]],mins);
		}

		if (!intersectsMinsMaxs(&light->box, mins, maxs)) {
			return false;
		}
	}

	poly->lightTimestamp = r_lightTimestamp;

	return true;
}

qboolean CheckMeshInLight(mesh_t *mesh, shadowlight_t *light) {

	//Doesn't recieve per pixel light or cast shadows...
	if (!(mesh->shader->shader->flags & SURF_PPLIGHT) && (mesh->shader->shader->flags & SURF_NOSHADOW)) {
		return false;
	}

	//Already flagged
	if (mesh->lightTimestamp == r_lightTimestamp) {
		return false;
	}

	//Doesn't intersect light volume
	if (!intersectsMinsMaxs(&light->box, mesh->mins, mesh->maxs)) {
		return false;
	}

	mesh->lightTimestamp = r_lightTimestamp;
	return true;
}

/*
=============

R_MarkLightSurfaces

	Fills the shadow chain with polygons we should consider.

	Polygons that will be added are:
	1. In the light volume. (sphere)
	2. "Visible" to the light.

	Visible is:
	a. facing the light (dotprod > 0)
	b. in a leaf that is visible from the light's leaf. (based on vis data)

	This is crude for satic lights we use extra tricks (svbsp / revis) to
	reduce the number of polyons.

=============
*/
void R_MarkLightSurfaces (shadowlight_t *light, mnode_t *node)
{
	mplane_t	*plane;
	float		dist;
	msurface_t	**surf;
	mleaf_t		*leaf;
	int			c,leafindex;
	mesh_t		*mesh;

	//Not in the current light's bouding box
	if (!intersectsMinsMaxs(&light->box,node->minmaxs,node->minmaxs+3))
		return;

	if (node->contents & CONTENTS_LEAF) {
		//we are in a leaf
		leaf = (mleaf_t *)node;
		leafindex = leaf->cluster;

		//is this leaf visible from the light
		if (!(lightvis[leafindex>>3] & (1<<(leafindex&7)))) {
			return;
		}

		c = leaf->nummarksurfaces;
		surf = leaf->firstmarksurface;

		for (c=0; c<leaf->nummarksurfaces; c++, surf++) {

			if (CheckSurfInLight ((*surf), light)) {
				(*surf)->shadowchain = shadowchain;
				shadowchain = (*surf);				
				//svBsp_NumKeptPolys++;
			}
		}


		c = leaf->nummeshes;
		for (c=0; c<leaf->nummeshes; c++) {
			mesh = &cl.worldmodel->meshes[cl.worldmodel->leafmeshes[leaf->firstmesh+c]];
			if (CheckMeshInLight(mesh, light)) {
				mesh->shadowchain = meshshadowchain;
				meshshadowchain = mesh;
			}
		}
		return;
	}
/*
	plane = node->plane;
	dist = DotProduct (light->origin, plane->normal) - plane->dist;


	if (dist > light->radius)
	{
		R_MarkShadowCasting (light, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		R_MarkShadowCasting (light, node->children[1]);
		return;
	}
*/
	R_MarkLightSurfaces (light, node->children[0]);
	R_MarkLightSurfaces (light, node->children[1]);
}

/*
=============

 CheckEntityInLight

	Checks if Entity is in the light's volume and visible to the light (using vis)

=============
*/

qboolean CheckEntityInLight(entity_t *ent) {
	
	int i, leafindex;

	model_t	*entmodel = ent->model;
	mleaf_t *leaf;
	vec3_t mins, maxs;

	VectorAdd(ent->origin, entmodel->mins, mins);
	VectorAdd(ent->origin, entmodel->maxs, maxs);

	if (intersectsMinsMaxs(&currentshadowlight->box,mins,maxs)) {
		
		if (sh_noefrags.value) return true;
		
		for (i=0; i<ent->numleafs;i++) {
			leafindex = ent->leafnums[i];
			//leaf ent is in is visible from light
			leaf = cl.worldmodel->leafs+leafindex;
			if (currentshadowlight->entclustervis[leaf->cluster>>3] & (1<<(leaf->cluster&7)))
			{
				return true;
			}
		}
		if (ent->numleafs == 0) {
			//Con_Printf("Ent with no leafs");
			return true;
		}
		return false;
	}
	return false;
}

/*
=============

R_MarkLightEntities

	Adds shadow casting ents to the list

=============
*/
void R_MarkLightEntities() {

	int		i;
	vec3_t	mins, maxs;

	if (!cg_showentities.value)
		return;

	cl_numlightvisedicts = 0;
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		if ((currententity->model->flags & EF_NOSHADOW) && (currententity->model->flags & EF_FULLBRIGHT)) {
			continue;
		}

		if (mirror) {
			VectorAdd (currententity->origin,currententity->model->mins, mins);
			VectorAdd (currententity->origin,currententity->model->maxs, maxs);
			if (mirror_clipside == BoxOnPlaneSide(mins, maxs, mirror_plane)) {
				continue;
			}

			if ( BoxOnPlaneSide(mins, maxs, &mirror_far_plane) == 1) {
				return;
			}
		}

		//Dont cast shadows with the ent this light is attached to because
		//when the light is partially in the model shadows will look weird.
		//FIXME: Model is not lit by its own light.
		if (currententity != currentshadowlight->owner)
		{
			if (CheckEntityInLight(currententity)) {
				currententity->lightTimestamp = r_lightTimestamp;
				cl_lightvisedicts[cl_numlightvisedicts] = currententity;
				cl_numlightvisedicts++;
			} else cut_ent++;
		}
	}
}

/*
=============
R_ContributeFrame

	Returns true if the light should be rendered.
	We try to throw away as much ligts as possible here.  Most of the lights are culled here
	but some are culled further down the pipeline (but rarely).

=============
*/
void boxScreenSpaceRect(aabox_t *b, int *rect);

qboolean R_ContributeFrame (shadowlight_t *light)
{
	mleaf_t *lightleaf;
	float dist;
	aabox_t lightbox;

	float b = d_lightstylevalue[light->style]/255.0;

	light->color[0] = light->baseColor[0] * b;
	light->color[1] = light->baseColor[1] * b;
	light->color[2] = light->baseColor[2] * b;

	//verry soft light, don't bother.
	if (b < 0.1) {
		//Con_Printf("Culled: Weak light\n");
		return false;
	}

	//frustum scissor testing
	/*
	dist = SphereInFrustum(light->origin, light->radius);
	if (dist == 0)  {
		//whole sphere is out ouf frustum so cut it.
		return false;
	}
	*/

	if (R_CullBox(light->box.mins, light->box.maxs)) {
		//Con_Printf("Culled: Box outside frustum\n");
		return false;
	}

	lightbox.mins[0] = light->origin[0] - light->radius;
	lightbox.mins[1] = light->origin[1] - light->radius;
	lightbox.mins[2] = light->origin[2] - light->radius;

	lightbox.maxs[0] = light->origin[0] + light->radius;
	lightbox.maxs[1] = light->origin[1] + light->radius;
	lightbox.maxs[2] = light->origin[2] + light->radius;

	//fully/partially in frustum
	
	if (!sh_noscissor.value) {
		//vec3_t dst;
		//float d;
		//VectorSubtract (light->origin, r_refdef.vieworg, dst);
		//d = Length (dst);
		
		if (/*d > light->radius*/!intersectsBoxPoint(&light->box, r_refdef.vieworg)) {
			
			//R_ProjectBoundingBox (light, light->scizz.coords);
			boxScreenSpaceRect(&light->box, light->scizz.coords);
			//glScissor(light->scizz.coords[0], light->scizz.coords[1],
			//		light->scizz.coords[2]-light->scizz.coords[0],  light->scizz.coords[3]-light->scizz.coords[1]);
		} else {
			//viewport is ofs/width based
			light->scizz.coords[0] = r_Iviewport[0];
			light->scizz.coords[1] = r_Iviewport[1];
			light->scizz.coords[2] = r_Iviewport[0]+r_Iviewport[2];
			light->scizz.coords[3] = r_Iviewport[1]+r_Iviewport[3];
			//glScissor(r_Iviewport[0], r_Iviewport[1], r_Iviewport[2], r_Iviewport[3]);
		}
	}

	//r_lightTimestamp++;

	shadowchain = NULL;
	meshshadowchain = NULL;
	if (light->isStatic) {
		lightvis = &light->leafvis[0];
	} else {
		lightleaf = Mod_PointInLeaf (light->origin, cl.worldmodel);
		lightvis = Mod_LeafPVS (lightleaf, cl.worldmodel);
		Q_memcpy(&light->leafvis,lightvis,MAX_MAP_LEAFS/8);
		Q_memcpy(&light->entclustervis, lightvis, MAX_MAP_LEAFS/8);
		light->area = lightleaf->area;
	}

	//not in a visible area => skip it
	if (!(r_refdef.areabits[light->area>>3] & (1<<(light->area&7)))) {
		//Con_Printf("Culled: Not in same area\n");
		return false;
	}

	/*
	if (!intersectsBox(&worldbox, &lightbox)) {
		Con_Printf("Culled: Box outside the visible world\n");
		return false;
	}
	*/

	if (light->isStatic) {
		qboolean b = HasVisibleClusters(light,&worldvis[0]);
		//if (!b) Con_Printf("Culled: No visible clusters\n");
		return b;		
	} else {
		qboolean b = HasSharedLeafs(lightvis,&worldvis[0]);
		//if (!b) Con_Printf("Culled: No shared leafs\n");
		return b;	
	}

/*
	if (HasSharedLeafs(lightvis,&worldvis[0])) {

		//light->visible = true;
		//numUsedShadowLights++;

		//mark shadow casting ents
		//MarkShadowEntities();

		//mark shadow casting polygons
		//if (!light->isStatic) {
		//	R_MarkShadowCasting ( light, cl.worldmodel->nodes);
		//} else {
		//	return true;
		//}
		//return (shadowchain) ? true : false;
		return true;
	} else {
		Con_Printf("No shared leafs\n");
		return false;
	}

*/
}

/*
=============
R_FillLightChains 

	Returns true if the light should be rendered.
	It's here lights are seldomly removed further down the pipeline.

=============
*/
qboolean R_FillLightChains (shadowlight_t *light)
{

	r_lightTimestamp++;

	shadowchain = NULL;
	meshshadowchain = NULL;

	//lightvis = &light->vis[0];
	//numUsedShadowLights++;
	
	//mark shadow casting ents
	R_MarkLightEntities();
	
	//mark shadow casting polygons
	if (!light->isStatic) {
	       R_MarkLightSurfaces ( light, cl.worldmodel->nodes);
	} else {
	       return true;
	}
	
	return (shadowchain || meshshadowchain) ? true : false;
}

/**************************************************************


	Shadow volume calculations / drawing


***************************************************************/

void *VolumeVertsPointer;

/*
=============
R_ConstructShadowVolume

	Calculate the shadow volume commands for the light.
	(only if dynamic)
=============
*/
void R_ConstructShadowVolume(shadowlight_t *light) {

	if (!light->isStatic) {
		PrecalcVolumesForLight(cl.worldmodel);
		light->volumeCmds = &volumeCmdsBuff[0];
		light->volumeVerts = &volumeVertsBuff[0];
		light->lightCmds = &lightCmdsBuff[0];
		light->lightCmdsMesh = &lightCmdsBuffMesh[0];
		light->numlightcmdsmesh = numLightCmdsMesh;
		light->numlightcmds = numLightCmds;
	}

	VolumeVertsPointer = light->volumeVerts;

}

/*
=============
R_DrawShadowVolume

Draws the shadow volume, for statics this is the precalc one
for dynamics this is the R_ConstructShadowVolume one.

=============
*/
void R_DrawShadowVolume(shadowlight_t *light) {


	GL_SelectTexture(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
	GL_SelectTexture(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	GL_SelectTexture(GL_TEXTURE2_ARB);
	glDisable(GL_TEXTURE_2D);
	GL_SelectTexture(GL_TEXTURE3_ARB);
	glDisable(GL_TEXTURE_2D);

	
/*	if (!light->isStatic) {
		glColor3f(0,1,0);
		DrawVolumeFromCmds (&volumeCmdsBuff[0], &lightCmdsBuff[0]);
	} else {
*/
		glColor3f(1,0,0);
		DrawVolumeFromCmds (light->volumeCmds, light->lightCmds, VolumeVertsPointer);
//	}

	glColor3f(1,1,1);
	GL_SelectTexture(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
}


/**************************************************************


	Brush model shadow volume support


***************************************************************/

/*
=============
R_MarkBrushModelSurfaces

	Set the light timestamps of the brush model.
=============
*/
void R_MarkBrushModelSurfaces(entity_t *e) {

	vec3_t		mins, maxs;
	int			i;
	msurface_t	*psurf;
	model_t		*clmodel;
	qboolean	rotated;

	vec3_t oldlightorigin;
	//backup light origin since we will have to translate
	//light into model space
	VectorCopy (currentshadowlight->origin, oldlightorigin);

	currententity = e;
	currenttexture = -1;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, clmodel->mins, mins);
		VectorAdd (e->origin, clmodel->maxs, maxs);
	}

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	VectorSubtract (currentshadowlight->origin, e->origin, currentshadowlight->origin);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);

		VectorCopy (currentshadowlight->origin, temp);
		currentshadowlight->origin[0] = DotProduct (temp, forward);
		currentshadowlight->origin[1] = -DotProduct (temp, right);
		currentshadowlight->origin[2] = DotProduct (temp, up);

	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

    glPushMatrix ();
e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
e->angles[0] = -e->angles[0];	// stupid quake bug


	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
		CheckSurfInLight(psurf, currentshadowlight);
	}

	VectorCopy(oldlightorigin,currentshadowlight->origin);
	glPopMatrix ();
}


/*
=============
R_DrawBrushModelVolumes

	Draw the shadow volumes of the brush model.
	They are dynamically calculated.
=============
*/
/*
void R_DrawBrushModelVolumes(entity_t *e) {

	int			j, k;
	vec3_t		mins, maxs;
	int			i, numsurfaces;
	msurface_t	*surf;
	float		dot;
	mplane_t	*pplane;
	model_t		*clmodel;
	qboolean	rotated;
	glpoly_t	*poly;
	vec3_t v1,*v2;
	float scale;
	qboolean shadow;
	vec3_t		temp[32];


	vec3_t oldlightorigin;

	//backup light origin since we will have to translate
	//light into model space
	VectorCopy (currentshadowlight->origin, oldlightorigin);

	currententity = e;
	currenttexture = -1;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, clmodel->mins, mins);
		VectorAdd (e->origin, clmodel->maxs, maxs);
	}

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	VectorSubtract (currentshadowlight->origin, e->origin, currentshadowlight->origin);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);

		VectorCopy (currentshadowlight->origin, temp);
		currentshadowlight->origin[0] = DotProduct (temp, forward);
		currentshadowlight->origin[1] = -DotProduct (temp, right);
		currentshadowlight->origin[2] = DotProduct (temp, up);

	}

	surf = &clmodel->surfaces[clmodel->firstmodelsurface];

    glPushMatrix ();
e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
e->angles[0] = -e->angles[0];	// stupid quake bug

	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, surf++)
	{
		if (surf->polys->lightTimestamp != r_lightTimestamp) continue;

		poly = surf->polys;

		for (j=0 ; j<surf->numedges ; j++)
		{
			v2 = (vec3_t *)&poly->verts[j];
			VectorSubtract ( (*v2) ,currentshadowlight->origin, v1);
			scale = Length (v1);

			if (sh_visiblevolumes.value)
				VectorScale (v1, (1/scale)*50, v1);
			else
				VectorScale (v1, (1/scale)*currentshadowlight->radius*2, v1);

			VectorAdd (v1, (*v2), temp[j]);
		}

		//check if neighbouring polygons are shadowed
		for (j=0 ; j<surf->numedges ; j++)
		{
			shadow = false;

			if (poly->neighbours[j] != NULL) {
				if ( poly->neighbours[j]->lightTimestamp != poly->lightTimestamp) {
					shadow = true;
				}
			} else {
				shadow = true;
			}

			if (shadow) {
				
				//we extend the shadow volumes by projecting them on the
				//light's sphere.
				//This sometimes gives problems when the light is verry close to a big
				//polygon.  But further extending the volume wastes fill rate.
				//So ill have to fix it.

				glBegin(GL_QUAD_STRIP);
					glVertex3fv(&poly->verts[j][0]);
					glVertex3fv(&temp[j][0]);
					glVertex3fv(&poly->verts[((j+1)% poly->numverts)][0]);
					glVertex3fv(&temp[((j+1)% poly->numverts)][0]);
				glEnd();
			}
		}

		//Draw near light cap
		glBegin(GL_POLYGON);
		for (j=0; j<surf->numedges ; j++)
		{
				glVertex3fv(&poly->verts[j][0]);
		}
		glEnd();

		//Draw extruded cap
		glBegin(GL_POLYGON);
		for (j=surf->numedges-1; j>=0 ; j--)
		{
				glVertex3fv(&temp[j][0]);
		}
		glEnd();
	}

	//PrecalcVolumesForLight(clmodel);

	VectorCopy(oldlightorigin,currentshadowlight->origin);
	glPopMatrix ();
}

*/
/*
=============
R_DrawBrushModelVolumes

	Draw the shadow volumes of the brush model.
	They are dynamically calculated.
=============
*/

void R_DrawBrushModelVolumes(entity_t *e) {

	model_t	*model = e->model;
	msurface_t *surf;
	glpoly_t	*poly;
	int		i, j, count;
	brushlightinstant_t *ins = e->brushlightinstant;
	float *v;
	count = 0;


    glPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
	e->angles[0] = -e->angles[0];	// stupid quake bug


	surf = &model->surfaces[model->firstmodelsurface];
	for (i=0; i<model->nummodelsurfaces; i++, surf++)
	{
		if (!ins->polygonVis[i]) continue;

		if (surf->flags & SURF_NOSHADOW)
			continue;

		poly = surf->polys;
		//extrude edges
		for (j=0 ; j<poly->numneighbours ; j++)
		{
			mneighbour_t *neigh = &poly->neighbours[j];
			if (ins->neighbourVis[count+j]) {
				glBegin(GL_QUAD_STRIP);

					//Note: The neighbour p1,p2 are absolute vertex indecies, for the extruded ones
					//we want polygon relative indecies.

					//glVertex3fv(&poly->verts[j][0]);
					glVertex3fv((float *)(&cl.worldmodel->userVerts[neigh->p1]));
					glVertex3fv(&ins->extvertices[count+neigh->p1-poly->firstvertex][0]);
					//glVertex3fv(&poly->verts[((j+1)% poly->numverts)][0]);
					glVertex3fv((float *)(&cl.worldmodel->userVerts[neigh->p2]));
					glVertex3fv(&ins->extvertices[count+neigh->p2-poly->firstvertex][0]);
				glEnd();			
			}
		}

		//Draw near light cap
		/*
		glBegin(GL_TRIANGLE_FAN);
		for (j=0; j<surf->numedges ; j++)
		{
			//glVertex3fv(&poly->verts[j][0]);
			glVertex3fv((float *)(&globalVertexTable[surf->polys->firstvertex+j]));
		}
		glEnd();
		*/
		glBegin(GL_TRIANGLES);
		//v = surf->polys->verts[0];
		for (j=0; j<surf->polys->numindecies; j++) {
			v = (float *)(&cl.worldmodel->userVerts[surf->polys->indecies[j]]);
			glVertex3fv(&v[0]);
		}
		glEnd();


		//Draw extruded cap
		/*
		glBegin(GL_TRIANGLE_FAN);
		for (j=surf->numedges-1; j>=0 ; j--)
		{
			glVertex3fv(&ins->extvertices[count+j][0]);
		}
		glEnd();
		*/
		glBegin(GL_TRIANGLES);
		//v = surf->polys->verts[0];
		for (j=surf->polys->numindecies-1; j>=0; j--) {
			v = (float *)(&ins->extvertices[count+surf->polys->indecies[j]-surf->polys->firstvertex]);
			glVertex3fv(&v[0]);
		}
		glEnd();

		count+=surf->numedges;
	}	

	glPopMatrix ();
}


/**************************************************************

	Shadow volume precalculation & storing

***************************************************************/


/*
=============

getVertexIndexFromSurf

Gets index of the i'th vertex of the surface in the models vertex array
=============
*/
int getVertexIndexFromSurf(msurface_t *surf, int index, model_t *model) {

		int lindex = model->surfedges[surf->firstedge + index];
		medge_t	*r_pedge;

		if (lindex > 0)
		{
			r_pedge = &model->edges[lindex];
			//if (r_pedge->v[0] == 0) Con_Printf("moord en brand");
			return r_pedge->v[0];
		}
		else
		{
			r_pedge = &model->edges[-lindex];
			//if (r_pedge->v[1] == 0) Con_Printf("moord en brand");
			return r_pedge->v[1];
		}
}

/*
=============

PrecalcVolumesForLight

This will create arrays with gl-commands that define the shadow volumes
(something similar to the mesh arrays that are created.)
They are stored for static lights and recalculated every frame for dynamic ones.
Non calculated vertices are not saved in the list but the index in the vertex array
of the model is saved.

We store them in volumeCmdsBuff and lightCmdsBuff

=============
*/
void PrecalcVolumesForLight(model_t *model) {

	msurface_t *surf;

	int *volumeCmds = &volumeCmdsBuff[0];
	lightcmd_t *lightCmds = &lightCmdsBuff[0];
	lightcmd_t *lightCmdsMesh = &lightCmdsBuffMesh[0];
	float *volumeVerts = &volumeVertsBuff[0];
	int volumePos = 0;
	int lightPos = 0;
	int lightPosMesh = 0;
	int vertPos = 0;
	int startVerts, startNearVerts, numPos = 0, stripLen = 0, i;
	glpoly_t *poly;
	qboolean lastshadow;
	vec3_t v1, *v2, vert1;
	float scale;
	qboolean shadow;

	vec3_t *s, *t, nearPt, nearToVert;
	float dist, colorscale;
	mplane_t *splitplane;
	int		j;
	float	*v;
	mesh_t	*mesh;

	surf = shadowchain;

	//1. Calculate shadow volumes

	while (surf)
	{

		poly = surf->polys;

		/*
		if (( surf->flags & SURF_DRAWTURB ) || ( surf->flags & SURF_DRAWSKY )) {
			surf = surf->shadowchain;
			Con_Printf ("Water/Sky in shadow chain!!");
			continue;
		}
		*/
	
		if (surf->shader->shader->flags & SURF_NOSHADOW)
			goto noshadow;
		
		//a. far cap
//		volumeCmds[volumePos++] = GL_POLYGON;
		volumeCmds[volumePos++] = GL_TRIANGLES;
		volumeCmds[volumePos++] = poly->numindecies;

		//extrude verts
		startVerts = (int)vertPos/3;
		for (i=0 ; i<surf->numedges ; i++)
		{
			//v2 = (vec3_t *)&poly->verts[i];
			v2 = (vec3_t *)(&cl.worldmodel->userVerts[surf->polys->firstvertex+i]);
			VectorSubtract ( (*v2), currentshadowlight->origin, v1);

			scale = Length (v1);
			if (sh_visiblevolumes.value) {
				//make them short so that we see them
				VectorScale (v1, (1/scale)*50, v1);
			} else {
				//we don't have to be afraid they will clip with the far plane
				//since we use the infinite matrix trick
				VectorScale (v1, (1/scale)* currentshadowlight->radius*10, v1);
			}


			VectorAdd (v1, (*v2) ,vert1);
			VectorCopy (vert1, ((vec3_t *)(volumeVerts+vertPos))[0]);
			vertPos+=3;
		}

		//create indexes
		for (i=poly->numindecies-1; i>=0; i--)
		{
			volumeCmds[volumePos++] = startVerts+poly->indecies[i]-poly->firstvertex;
		}

		//copy vertices
		startNearVerts = (int)vertPos/3;
		for (i=0 ; i<surf->numedges ; i++)
		{
			//v2 = (vec3_t *)&poly->verts[i];
			v2 = (vec3_t *)(&cl.worldmodel->userVerts[surf->polys->firstvertex+i]);
			/*(float)*/volumeVerts[vertPos++] = (*v2)[0];	// <AWE> lvalue cast. what da...?
			/*(float)*/volumeVerts[vertPos++] = (*v2)[1];	// <AWE> a float is a float is a...
			/*(float)*/volumeVerts[vertPos++] = (*v2)[2];
		}


		if (vertPos >  MAX_VOLUME_VERTS) {
			Con_Printf ("More than MAX_VOLUME_VERTS vetices! %i\n", volumePos);
			break;
		}
		
		
		//b. borders of volume
		//we make quad strips if we have continuous borders
		lastshadow = false;

		for (i=0 ; i<poly->numneighbours ; i++)
		{
			mneighbour_t *neigh = &poly->neighbours[i];
			shadow = false;

			if (neigh->n != NULL) {
				if ( neigh->n->lightTimestamp != poly->lightTimestamp) {
					shadow = true;
				}
			} else {
				shadow = true;
			}

			//if (shadow) {

			//	if (!lastshadow) {
					//begin new strip
					volumeCmds[volumePos++] = GL_QUAD_STRIP;
					numPos = volumePos;
					volumeCmds[volumePos++] = 4;
					stripLen = 2;

					//copy vertices
					volumeCmds[volumePos++] = startNearVerts+neigh->p1-poly->firstvertex;//-getVertexIndexFromSurf(surf, i, model);
					volumeCmds[volumePos++] = startVerts+neigh->p1-poly->firstvertex;

			//	}

				volumeCmds[volumePos++] = startNearVerts+neigh->p2-poly->firstvertex;//-getVertexIndexFromSurf(surf, (i+1)%poly->numverts, model);
				volumeCmds[volumePos++] = startVerts+neigh->p2-poly->firstvertex;

				stripLen+=2;
				
		//	} else {
		//		if (lastshadow) {
					//close list up
					volumeCmds[numPos] = stripLen;
			//	}
		//	}
		//	lastshadow = shadow;
		}
		
		if (lastshadow) {
			//close list up
			volumeCmds[numPos] = stripLen;
		}
		
		if (volumePos >  MAX_VOLUME_COMMANDS) {
			Con_Printf ("More than MAX_VOLUME_COMMANDS commands! %i\n", volumePos);
			break;
		}
		
noshadow:
		lightCmds[lightPos++].asVoid = surf;

		//c. glow surfaces/texture coordinates
			//leftright vectors of plane
			
		
			/*
			s = (vec3_t *)&surf->tangent;
			t = (vec3_t *)&surf->binormal;

			
			//VectorNormalize(*s);
			//VectorNormalize(*t);
			

			splitplane = surf->plane;

			dist = DotProduct (currentshadowlight->origin, splitplane->normal) - splitplane->dist;


			dist = abs(dist);

			if (dist > currentshadowlight->radius) Con_Printf("Polygon to far\n");
			ProjectPlane (currentshadowlight->origin, (*s), (*t), nearPt);

			scale = 1 /((2 * currentshadowlight->radius) - dist);
			colorscale = (1 - (dist / currentshadowlight->radius));

			if (colorscale <0) colorscale = 0;

			lightCmds[lightPos++].asInt = GL_TRIANGLE_FAN;

			lightCmds[lightPos++].asVoid = surf;
			lightCmds[lightPos++].asFloat = currentshadowlight->color[0]*colorscale; 
			lightCmds[lightPos++].asFloat = currentshadowlight->color[1]*colorscale;
			lightCmds[lightPos++].asFloat = currentshadowlight->color[2]*colorscale;
			lightCmds[lightPos++].asFloat = colorscale;

			//v = poly->verts[0];
			v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
			for (j=0 ; j<poly->numverts ; j++, v+= VERTEXSIZE)
			{
				// Project the light image onto the face
				VectorSubtract (v, nearPt, nearToVert);

				// Get our texture coordinates, transform into tangent plane
				lightCmds[lightPos++].asVec = DotProduct (nearToVert, (*s)) * scale + 0.5;
				lightCmds[lightPos++].asVec = DotProduct (nearToVert, (*t)) * scale + 0.5;
				
				//calculate local light vector and put it into tangent space
				{
					vec3_t lightDir, tsLightDir;

					VectorSubtract( currentshadowlight->origin,v,lightDir);

					if (surf->flags & SURF_PLANEBACK)	{
						tsLightDir[2] = -DotProduct(lightDir,surf->plane->normal);
					} else {
						tsLightDir[2] = DotProduct(lightDir,surf->plane->normal);
					}

					tsLightDir[1] = -DotProduct(lightDir,(*t));
					tsLightDir[0] = DotProduct(lightDir,(*s));
					lightCmds[lightPos++].asVec = tsLightDir[0];
					lightCmds[lightPos++].asVec = tsLightDir[1];
					lightCmds[lightPos++].asVec = tsLightDir[2];
				}
			}
			*/
		if (lightPos >  MAX_LIGHT_COMMANDS) {
			Con_Printf ("More than MAX_LIGHT_COMMANDS commands %i\n", lightPos);
			break;
		}
		
		surf = surf->shadowchain;
	}

	lightPosMesh = 0;
	mesh = meshshadowchain;
	while (mesh) {
		lightCmdsMesh[lightPosMesh++].asVoid = mesh;
		mesh = mesh->shadowchain;
	}

	//Con_Printf("used %i\n",volumePos);
	//finish them off with 0
	lightCmds[lightPos++].asVoid = NULL;
	lightCmdsMesh[lightPosMesh++].asVoid = NULL;
	volumeCmds[volumePos++] = 0;

	numLightCmds = lightPos;
	numLightCmdsMesh = lightPosMesh;
	numVolumeCmds = volumePos;
	numVolumeVerts = vertPos;
}
/*
=============
DrawVolumeFromCmds

Draws the generated commands as shadow volumes
=============
*/
void DrawVolumeFromCmds(int *volumeCmds, lightcmd_t *lightCmds, float *volumeVerts) {

	int command, num, i;
	int volumePos = 0;
	int lightPos = 0;
//	int count = 0;						// <AWE> no longer required.
	msurface_t *surf;
	float		*v;

	glVertexPointer(3, GL_FLOAT, 0, volumeVerts);
	glEnableClientState(GL_VERTEX_ARRAY);

	while (1) {
		command = volumeCmds[volumePos++];
		if (command == 0) break; //end of list
		num = volumeCmds[volumePos++];
			
		glDrawElements(command,num,GL_UNSIGNED_INT,&volumeCmds[volumePos]);
		volumePos+=num;
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	//Con_Printf("%i objects drawn\n",count);
	if (sh_visiblevolumes.value) return;

	while (1) {
		
		/*
		command = lightCmds[lightPos++].asInt;
		if (command == 0) break; //end of list

		surf = lightCmds[lightPos++].asVoid;
		lightPos+=4;  //skip color
		num = surf->polys->numverts; 
		*/
		surf = lightCmds[lightPos++].asVoid;
		if (surf == NULL)
			break;
		
		if (surf->shader->shader->flags & SURF_NOSHADOW) continue;

		glBegin(GL_TRIANGLES);
		//v = surf->polys->verts[0];
		for (i=0; i<surf->polys->numindecies; i++) {
			v = (float *)(&cl.worldmodel->userVerts[surf->polys->indecies[i]]);
			glVertex3fv(&v[0]);
		}
		glEnd();

		/*
		glBegin(command);
		//v = surf->polys->verts[0];
		v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
		for (i=0; i<num; i++, v+= VERTEXSIZE) {
			//skip attent texture coord.
			lightPos+=2;
			//skip tangent space light vector
			lightPos+=3; 
			glVertex3fv(&v[0]);
		}
		glEnd();
		*/
	}
	
}

/**************************************************************


	Mesh shadow volume code (Curves/Inlines)


***************************************************************/

#define MAXMESHVERTS MAXALIASVERTS*4
#define MAXMESHTRIS MAXALIASTRIS*2*4

int		extrudeTimeStamp = 0;
vec3_t	extrudedVerts[MAXMESHVERTS];	//PENTA: Temp buffer for extruded vertices
int		extrudedTimestamp[MAXMESHVERTS];	//PENTA: Temp buffer for extruded vertices
qboolean	triangleVis[MAXMESHTRIS];	//PENTA: Temp buffer for light facingness of triangles

void SetupMeshToLightVisibility(shadowlight_t *light, mesh_t *mesh) {

	float d, scale;
	int i, j;
	vec3_t v2, *v1;	
	
	if (mesh->numtriangles > MAXMESHTRIS) {
		Con_Printf("Mesh model has %i tris (max %i), no shadows will be cast", mesh->numtriangles, MAXMESHTRIS);
		return;
	}

	if (mesh->numvertices > MAXMESHVERTS) {
		Con_Printf("Mesh model has %i verts (max %i), no shadows will be cast", mesh->numvertices, MAXMESHVERTS);
		return;
	}

	extrudeTimeStamp++;
	//calculate visibility
	for (i=0; i<mesh->numtriangles; i++) {
		d = DotProduct(mesh->triplanes[i].normal, light->origin) - mesh->triplanes[i].dist;
		if (d > 0) 
			triangleVis[i] = true;
		else
			triangleVis[i] = false;
	}

	//extude vertices
	for (i=0; i<mesh->numtriangles; i++) {

		if (triangleVis[i]) {//backfacing extrude it!
			
			for (j=0; j<3; j++) {

				int index = mesh->indecies[i*3+j];
				if (extrudedTimestamp[index] == extrudeTimeStamp) continue;
				extrudedTimestamp[index] = extrudeTimeStamp;

				v1 = &extrudedVerts[index];
				VectorCopy(mesh->userVerts[index],v2);

				VectorSubtract (v2, light->origin, (*v1));
				scale = Length ((*v1));

				if (sh_visiblevolumes.value) {
					//make them short so that we see them
					VectorScale ((*v1), (1/scale)* 70, (*v1));
				} else {
					//we don't have to be afraid they will clip with the far plane
					//since we use the infinite matrix trick
					VectorScale ((*v1), (1/scale)* light->radius*10, (*v1));
				}
				VectorAdd ((*v1), v2 ,(*v1));
			}
		}
	}
}

void DrawMeshVolume(mesh_t *mesh) {

	int i, j;

	if (mesh->numtriangles > MAXMESHTRIS) return;
	if (mesh->numvertices > MAXMESHVERTS) return;


	for (i=0; i<mesh->numtriangles; i++) {

		if (triangleVis[i]) {

			for (j=0; j<3; j++) {

				qboolean shadow = false;
				if (mesh->neighbours[i*3+j] == -1) {
					shadow = true;
				} else if (!triangleVis[mesh->neighbours[i*3+j]]) {
					shadow = true;
				}

				if (shadow) {
					int index = mesh->indecies[i*3+j];
					glBegin(GL_QUAD_STRIP);
					glVertex3fv(mesh->userVerts[index]);
					glVertex3fv(&extrudedVerts[index][0]);

					index = mesh->indecies[i*3+(j+1)%3];
					glVertex3fv(mesh->userVerts[index]);
					glVertex3fv(&extrudedVerts[index][0]);
					glEnd();
				}
			}
/*		}
	}

	glBegin(GL_TRIANGLES);
	for (i=0; i<mesh->numtriangles; i++) {

		if (triangleVis[i]) {
*/	
			glBegin(GL_TRIANGLES);
			for (j=0; j<3; j++) {
				int index = mesh->indecies[i*3+j];
				glVertex3fv(mesh->userVerts[index]);
			}
			glEnd();
	
			glBegin(GL_TRIANGLES);			
			for (j=2; j>=0; j--) {
				int index = mesh->indecies[i*3+j];
				glVertex3fv(&extrudedVerts[index][0]);
			}	
			glEnd();
		}
	}
	//glEnd();
	
}

void StencilMeshVolume(mesh_t *mesh) {
	if (mesh->shader->shader->flags & SURF_NOSHADOW) return;

	SetupMeshToLightVisibility(currentshadowlight, mesh);

	//
	//Pass 1 increase
	//
	glCullFace(GL_BACK);
	glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);

	DrawMeshVolume(mesh);

	//
	// Second Pass. Decrease Stencil Value In The Shadow
	//
	glCullFace(GL_FRONT);
	glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
	
	DrawMeshVolume(mesh);
	glCullFace(GL_BACK);
}

void StencilMeshVolume2(mesh_t *mesh)
{
    if (mesh->shader->shader->flags & SURF_NOSHADOW) return;
    SetupMeshToLightVisibility(currentshadowlight, mesh);
    DrawMeshVolume(mesh);
}

void StencilMeshVolumes()
{
    int i;
    switch(gl_twosidedstencil)
    {
    case 0:
	for (i=0; i<currentshadowlight->numlightcmdsmesh-1; i++)
	    StencilMeshVolume((mesh_t *)currentshadowlight->lightCmdsMesh[i].asVoid);
	break;
    case 1:
	// EXT_stencil_two_side
	glDisable(GL_CULL_FACE);
        glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
        qglActiveStencilFaceEXT(GL_BACK);
	glStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, ~0);
	qglActiveStencilFaceEXT(GL_FRONT);
	glStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	for (i=0; i<currentshadowlight->numlightcmdsmesh-1; i++)
	    StencilMeshVolume2((mesh_t *)currentshadowlight->lightCmdsMesh[i].asVoid);
	glEnable(GL_CULL_FACE);
        glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	break;

    case 2:
	// ATI_separate_stencil
	glDisable(GL_CULL_FACE);
	glStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
	qglStencilFuncSeparateATI(GL_ALWAYS, GL_ALWAYS, 0, ~0);
	qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
	qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

	for (i=0; i<currentshadowlight->numlightcmdsmesh-1; i++)
	    StencilMeshVolume2((mesh_t *)currentshadowlight->lightCmdsMesh[i].asVoid);
	glEnable(GL_CULL_FACE);
	break;
    }
}


/**************************************************************


	Stuff that should be elsewhere


***************************************************************/


/*
=============
R_RenderGlow

Render a halo around a light.
The idea is similar to the UT2003 lensflares.
=============
*/

//speed at wich halo grows fainter when it gets occluded
#define HALO_FALLOF 2
//the scale the alpa is multipied with (none is verry bright)
#define HALO_ALPHA_SCALE 0.7
//the maximum radius (in pixels) of the halo
#define HALO_SIZE 30
//the distance of the eye at wich the halo stops shrinking
#define HALO_MIN_DIST 300
void R_RenderGlow (shadowlight_t *light)
{
	vec3_t	hit = {0, 0, 0};
	double	x,y,z;
	float	fdist,realz;
	int		ofsx, ofsy;
	qboolean hitWorld;

	if (!light->halo || gl_wireframe.value) 
		return;

	//trace a from the eye to the light source
	TraceLine (r_refdef.vieworg, light->origin, hit);

	//if it's not visible anymore slowly fade it
	if (Length(hit) != 0) {
		light->haloalpha = light->haloalpha - (cl.time - cl.oldtime)*HALO_FALLOF;
		if (light->haloalpha < 0) return;
		hitWorld = true;
	} else
		hitWorld = false;

	/*
	VectorSubtract(r_refdef.vieworg, light->origin, dist);
	fdist = Length(dist);
	if (fdist < HALO_MIN_DIST) fdist = HALO_MIN_DIST;
	fdist = (1-1/fdist)*HALO_SIZE;
	*/
	fdist = HALO_SIZE;
	
	gluProject(light->origin[0], light->origin[1], light->origin[2], r_Dworld_matrix,
                   r_Dproject_matrix, (GLint *) r_Iviewport, &x, &y, &z); // <AWE> added cast.

	//Con_Printf("Viewp %i %i %i %i\n",r_Iviewport[0],r_Iviewport[1],r_Iviewport[2],r_Iviewport[3]);
	if (!hitWorld) {
		//we didn't hit any bsp try to read the z buffer (wich is totally utterly freakingly
		// über evil!)
		glReadPixels((int)x,(int)y,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&realz);
		if (realz < z) {
			light->haloalpha = light->haloalpha - (cl.time - cl.oldtime)*HALO_FALLOF;
			if (light->haloalpha < 0) return;	
		} else {
			//nothing in the way make it fully bright
			light->haloalpha = 1.0;
		}
	}

	ofsx = r_Iviewport[0];
	ofsy = r_Iviewport[1];
	x = x;
	y = glheight-y;

	x = (x/(float)glwidth)*vid.width;
	y = (y/(float)glheight)*vid.height;

	//glDisable (GL_TEXTURE_2D)
	GL_Bind(halo_texture_object);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_ALPHA_TEST);



	glBegin (GL_QUADS);

	glColor4f(light->color[0],light->color[1],light->color[2],light->haloalpha*HALO_ALPHA_SCALE);
	//glColor4f(1,1,1,light->haloalpha);
	glTexCoord2f (0, 0);
	glVertex2f (x-fdist, y-fdist);
	glTexCoord2f (1, 0);
	glVertex2f (x+fdist, y-fdist);
	glTexCoord2f (1, 1);
	glVertex2f (x+fdist, y+fdist);
	glTexCoord2f (0, 1);
	glVertex2f (x-fdist, y+fdist);
	
	glEnd ();
/*
	rad = 40;//light->radius * 0.35;

	VectorSubtract (light->origin, r_origin, v);

	//dave - slight fix
	VectorSubtract (light->origin, r_origin, vp2);
	VectorNormalize(vp2);


	glBegin (GL_TRIANGLE_FAN);
	glColor3f (light->color[0]*0.2,light->color[1]*0.2,light->color[2]*0.2);
	for (i=0 ; i<3 ; i++)
		v[i] = light->origin[i] - vp2[i]*rad;
	glVertex3fv (v);
	glColor3f (0,0,0);
	for (i=16 ; i>=0 ; i--)
	{
		a = i/16.0 * M_PI*2;
		for (j=0 ; j<3 ; j++)
			v[j] = light->origin[j] + vright[j]*cos(a)*rad
				+ vup[j]*sin(a)*rad;
		glVertex3fv (v);
	}
	glEnd ();
*/
	glEnable(GL_ALPHA_TEST);
	glColor3f (1,1,1);
	//glDisable (GL_BLEND);
	//glEnable (GL_TEXTURE_2D);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (1);
	glColor3f (1,1,1);
}

/**************************************************************

	Shadow volume bsp's

	Some of this suff should be put in gl_svbsp.c

***************************************************************/

svnode_t *currentlightroot;
vec3_t testvect = {10,10,10};

/*
================
  CutLeafs

  Removes leaves that were cut by using the svbsp from the light's
  visibility list.
  This gains some speed in certain cases.
================
*/
void CutLeafs(byte *vis) {

	int c, i;
	msurface_t **surf;
	mleaf_t *leaf;
	qboolean found;
	int removed = 0;
	aabox_t leafbox;
	aabox_t visbox = emptyBox();

	if (sh_norevis.value) return;
	
	Q_memset(currentshadowlight->leafvis,0,MAX_MAP_LEAFS/8);

	for (i=0 ; i<cl.worldmodel->numleafs ; i++)//overloop alle leafs
	{
		leaf = &cl.worldmodel->leafs[i];
		if (vis[leaf->cluster>>3] & (1<<(leaf->cluster&7)))
		{
			leafbox = constructBox(leaf->minmaxs,leaf->minmaxs+3);
			if (!intersectsBox(&currentshadowlight->box,&leafbox)) continue;
			
			visbox = addBoxes(&visbox,&leafbox);	

			//this leaf is visible from the leaf the current light (in a brute force way)
			//now check if we entirely cut this leaf by means of the svbsp
			c = leaf->nummarksurfaces;
			surf = leaf->firstmarksurface;
			
//			if (leaf->index != i) Con_Printf("Weird leaf index %i, %i\n",i,leaf->index);
			found = false;
			for (c=0; c<leaf->nummarksurfaces; c++, surf++) {			
				if ((*surf)->polys->lightTimestamp == r_lightTimestamp) {
					found = true;
					break;
				}
			}

			if (found) {
				currentshadowlight->leafvis[i>>3] |= (1<<(i&7));
				removed++;
			}
/*
			if (!found) {
				//set vis bit on false
				vis[i>>3] &= ~(1<<(i&7));	
				removed++;
			}
*/
		}
	}
	leafbox = intersectBoxes(&currentshadowlight->box,&visbox);
	currentshadowlight->box = leafbox;
	//Con_Printf("  Removed leafs: %i\n", removed);
}

/*
================
AddToShadowBsp

Add a surface as potential shadow caster to the svbsp, it can be
cut when it is occluded by other surfaces.
================
*/
void AddToShadowBsp(msurface_t *surf) {
	vec3_t surfvects[32];
	int numsurfvects;
	int i;
	float *v;

	/*
	PENTA: removed we checked this twice

	//we don't cast shadows with water
	if (( surf->flags & SURF_DRAWTURB ) || ( surf->flags & SURF_DRAWSKY )) {
		return;
	}

	plane = surf->plane;

	switch (plane->type)
	{
		case PLANE_X:
			dist = currentshadowlight->origin[0] - plane->dist;
			break;
		case PLANE_Y:
			dist = currentshadowlight->origin[1] - plane->dist;
			break;
		case PLANE_Z:
			dist = currentshadowlight->origin[2] - plane->dist;
			break;
		default:
			dist = DotProduct (currentshadowlight->origin, plane->normal) - plane->dist;
			break;
	}

	//the normals are flipped when surf_planeback is 1
	if (((surf->flags & SURF_PLANEBACK) && (dist > 0)) ||
		(!(surf->flags & SURF_PLANEBACK) && (dist < 0)))
	{
		return;
	}
	
	//the normals are flipped when surf_planeback is 1
	if ( abs(dist) > currentshadowlight->radius)
	{
		return;
	}
	
	*/

	//FIXME: use constant instead of 32
	if (surf->numedges > 32) {
		Con_Printf("Error: to many edges");
		return;
	}

	surf->visframe = 0;
	//Make temp copy of suface polygon
	numsurfvects = surf->numedges;
	for (i=0, v=(float *)(&cl.worldmodel->userVerts[surf->polys->firstvertex]); i<numsurfvects; i++, v+=VERTEXSIZE) {
		VectorCopy(v,surfvects[i]);
	}

	if (!sh_nosvbsp.value) {
		svBsp_NumAddedPolys++;
		R_AddShadowCaster(currentlightroot,surfvects,numsurfvects,surf,0);
	} else {
		surf->shadowchain = shadowchain;
		surf->polys->lightTimestamp = r_lightTimestamp;
		shadowchain = surf;
		svBsp_NumKeptPolys++;
	}
}

/*
================
R_RecursiveShadowAdd

Add surfaces front to back to the shadow bsp.
================
*/
void R_RecursiveShadowAdd(mnode_t *node)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf;
	double		dot;

	if (node->contents == CONTENTS_SOLID) {
		return;		// solid
	}

	if (node->contents & CONTENTS_LEAF) {
		return;		// leaf
	}

	//find which side of the node we are on

	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = currentshadowlight->origin[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = currentshadowlight->origin[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = currentshadowlight->origin[2] - plane->dist;
		break;
	default:
		dot = DotProduct (currentshadowlight->origin, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

	//recurse down the children, front side first
	R_RecursiveShadowAdd (node->children[side]);


	//draw stuff
	c = node->numsurfaces;

	if (c)
	{
		
		surf = cl.worldmodel->surfaces + node->firstsurface;
		do
		{
			if (surf->polys) {
				if ((surf->polys->lightTimestamp == r_lightTimestamp))
				{
					AddToShadowBsp (surf);
				}
			}
			surf++;
		} while (--c);
	}

	//recurse down the back side
	R_RecursiveShadowAdd (node->children[!side]);
}

/*
===============
R_MarkLightLeaves

Marks nodes from the light, this is used for
gross culling during svbsp creation.
===============
*/

void R_MarkLightLeaves (void)
{
	byte	solid[4096];
	mleaf_t	*lightleaf;

	//we use the same timestamp as for rendering (may cause errors maybe)
	r_visframecount++;
	lightleaf = Mod_PointInLeaf (currentshadowlight->origin, cl.worldmodel);

	if (r_novis.value)
	{
		lightvis = solid;
		memset (solid, 0xff, (cl.worldmodel->numleafs+7)>>3);
	}
	else
		lightvis = Mod_LeafPVS (lightleaf, cl.worldmodel);
}


/*
================
ShadowVolumeBsp

Create the shadow volume bsp for currentshadowlight
================
*/
void ShadowVolumeBsp() {
	msurface_t *n;

	currentlightroot = R_CreateEmptyTree();
	R_MarkLightLeaves();
	R_MarkLightSurfaces (currentshadowlight,cl.worldmodel->nodes);

	//PENTA: Q3 hack until svbsp's are fixed again
	svBsp_NumKeptPolys = 0;
	n = shadowchain;
	while (n) {
		svBsp_NumKeptPolys++;
		n = n->shadowchain;
	}

	//shadowchain = NULL;
	//svBsp_NumKeptPolys = 0;
	//R_RecursiveShadowAdd(cl.worldmodel->nodes);

}

int done = 0;

/*
================
R_StaticLightFromEnt

Spawn a static lights for the given entity.
================
*/
void R_StaticLightFromEnt(entity_t *ent) {
	
	int i;
	msurface_t *surf;
	msurface_t	*s;
	mapshader_t	*t;

        //Con_Printf("Shadow volumes start\n");
	
	if (!ent->model) {
		Con_Printf("Light with null model");
		return;
	}
		
	shadowchain = NULL;
	meshshadowchain = NULL;
	done++;
	
	//Con_Printf("->Light %i\n",done);
	
	
	r_lightTimestamp++;
	r_framecount++;
	
	svBsp_NumCutPolys = 0;
	svBsp_NumKeptPolys = 0;
	svBsp_NumAddedPolys = 0;
	
	//Create a light and make it static
	R_ShadowFromEntity(ent);
	
	numStaticShadowLights++;
	
	if (numShadowLights >= MAXSHADOWLIGHTS)  {
		Con_Printf("R_StaticLightFromEnt: More than MAXSHADOWLIGHTS lights");
		return;
	}
	
	currentshadowlight = &shadowlights[numShadowLights-1];
	currentshadowlight->isStatic = true;
	
	//Get origin / box from the model	
	VectorAdd(ent->model->mins, ent->model-> maxs, currentshadowlight->origin);
	VectorScale(currentshadowlight->origin, 0.5f, currentshadowlight->origin);
	VectorCopy(ent->angles, currentshadowlight->angles);
	VectorCopy(ent->model->mins, currentshadowlight->box.mins);
	VectorCopy(ent->model->maxs, currentshadowlight->box.maxs);
	VectorSubtract(ent->model->maxs,ent->model->mins,currentshadowlight->radiusv);
	VectorScale(currentshadowlight->radiusv, 0.5f, currentshadowlight->radiusv);
	currentshadowlight->radius = max(Length(ent->model->mins),Length(ent->model->maxs));

	//Shader to use on light == shader on the ent's model
	if ((ent->model->type == mod_brush) && (ent->model->nummodelbrushes)) {
		model_t *mod = ent->model;
		mbrush_t *b = &mod->brushes[mod->firstmodelbrush];
		mbrushside_t *bs = &mod->brushsides[b->firstbrushside];
		currentshadowlight->shader = bs->shader->shader;
	} else if ((ent->model->type == mod_sprite)) {
		msprite_t *psprite = ent->model->cache.data;
		currentshadowlight->shader = psprite->frames[0].shader;
	} else {
		currentshadowlight->shader = GL_ShaderForName("textures/lights/default");
	}
	
	//Calculate visible polygons
	ShadowVolumeBsp();
	
	//Print stats
	/*
	Con_Printf("  Thrown away: %i\n",svBsp_NumAddedPolys-svBsp_NumKeptPolys);
	Con_Printf("  Total in volume: %i\n",svBsp_NumKeptPolys);
	*/
	
	currentshadowlight->visSurf = Hunk_Alloc(4*svBsp_NumKeptPolys);
	currentshadowlight->numVisSurf = svBsp_NumKeptPolys;
	surf = shadowchain;
	
	//Clear texture chains
	for (i=0 ; i<cl.worldmodel->nummapshaders; i++)
	{
		cl.worldmodel->mapshaders[i].texturechain = NULL;
	}
	
	//Remark polys since polygons may have been removed since the last time stamp
	r_lightTimestamp++;
	for (i=0; i<svBsp_NumKeptPolys; i++,surf = surf->shadowchain) {
		surf->polys->lightTimestamp = r_lightTimestamp;
		currentshadowlight->visSurf[i] = surf;
		//put it in the correct texture chain
		surf->texturechain = surf->shader->texturechain;
		surf->shader->texturechain = surf;
	}
	
	//Sort surfs in our list per texture
	shadowchain = NULL;
	//meshshadowchain = NULL;
	for (i=0 ; i<cl.worldmodel->nummapshaders ; i++)
	{
		t = &cl.worldmodel->mapshaders[i];
		if (!t)
			continue;
		s = t->texturechain;
		if (!s)
			continue;
		else
		{
			for ( ; s ; s=s->texturechain) {
				s->shadowchain = shadowchain;
				shadowchain = s;
			}
		}
		t->texturechain = NULL;
	}
	
	//Recalculate vis for this light
	currentshadowlight->leaf = Mod_PointInLeaf (currentshadowlight->origin, cl.worldmodel);
	lightvis = Mod_LeafPVS (currentshadowlight->leaf, cl.worldmodel);
	currentshadowlight->area = currentshadowlight->leaf->area;
	//Q_memcpy(&currentshadowlight->vis[0], lightvis, MAX_MAP_LEAFS/8);
	Q_memcpy(&currentshadowlight->entclustervis[0], lightvis, MAX_MAP_LEAFS/8);
	CutLeafs(lightvis);
	
	//Precalculate the shadow volume / glow-texcoords
	PrecalcVolumesForLight(cl.worldmodel);
	currentshadowlight->volumeCmds = Hunk_Alloc(4*numVolumeCmds);
	Q_memcpy(currentshadowlight->volumeCmds, &volumeCmdsBuff, 4*numVolumeCmds);
	
	currentshadowlight->volumeVerts = Hunk_Alloc(4*numVolumeVerts);
	currentshadowlight->numVolumeVerts = numVolumeVerts;
	Q_memcpy(currentshadowlight->volumeVerts, &volumeVertsBuff, 4*numVolumeVerts);
	
	currentshadowlight->lightCmds = Hunk_Alloc(4*numLightCmds);
	Q_memcpy(currentshadowlight->lightCmds, &lightCmdsBuff, 4*numLightCmds);
	currentshadowlight->numlightcmds = numLightCmds;
	
	currentshadowlight->lightCmdsMesh = Hunk_Alloc(4*numLightCmdsMesh);
	Q_memcpy(currentshadowlight->lightCmdsMesh, &lightCmdsBuffMesh, 4*numLightCmdsMesh);
	currentshadowlight->numlightcmdsmesh = numLightCmdsMesh;

}

/**************************************************************


	Client side light entity loading code


***************************************************************/

/**

Hacked entitiy loading code, this parses the entities client side to find lights in it

**/

void LightFromFile(entity_t *fakeEnt)
{
	R_StaticLightFromEnt(fakeEnt);
}


void ParseVector (char *s, float *d)
{
	int		i;
	char	string[128];
	char	*v, *w;
		
	strncpy (string, s,sizeof(string));
	v = string;
	w = string;
	for (i=0 ; i<3 ; i++)
	{
		while (*v && *v != ' ')
			v++;
		*v = 0;
		d[i] = atof (w);
		w = v = v+1;
	}
}



char *ParseEnt (char *data, qboolean *isLight, entity_t *ent)
{
	qboolean	init;
	char		keyname[256];
	qboolean	foundworld = false;
	init = false;

// go through all the dictionary pairs
	while (1)
	{	
	// parse key
		data = COM_Parse (data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ParseEnt: EOF without closing brace");

		strncpy (keyname, com_token,sizeof(keyname));

	// parse value	
		data = COM_Parse (data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		init = true;	


		if (!strcmp(keyname, "classname")) {
			if (!strcmp(com_token, "rt_light")) {
				*isLight = true;
			} else {
				*isLight = false;
			}
		} else if (!strcmp(keyname, "_color"))  {
			ParseVector(com_token, ent->color);	
		} else if (!strcmp(keyname, "angles"))  {
			ParseVector(com_token, ent->angles);	
		} else if (!strcmp(keyname, "origin"))  {
			ParseVector(com_token, ent->origin);	
		} else if (!strcmp(keyname, "light"))  {
			ent->light_lev = atof(com_token);
		} else if (!strcmp(keyname, "model"))  {
			ent->model = Mod_ForName(com_token, true);
		} else if (!strcmp(keyname, "skin"))  {
			ent->skinnum = atoi(com_token);
		} else if (!strcmp(keyname, "style"))  {
			ent->style = atoi(com_token);
		} else if (!strcmp(keyname, "_noautolight")) {
			Con_Printf("Automatic light gen disabled\n");//XYW \n
			foundworld = true;
		} else if (!strcmp(keyname, "_skybox")) {
			strncpy(skybox_name,com_token,sizeof(skybox_name));
		} else if (!strcmp(keyname, "_cloudspeed")) {
			skybox_cloudspeed = atof(com_token);
		} else if (!strcmp(keyname, "_lightmapbright")) {
			Cvar_Set("sh_lightmapbright",com_token);
			Con_Printf("Lightmap brightness set to %f\n",sh_lightmapbright.value);
		} else if (!strcmp(keyname, "_fog_color")) {
			vec3_t temp;
			ParseVector(com_token, temp);	
			Cvar_SetValue("fog_r",temp[0]);
			Cvar_SetValue("fog_g",temp[1]);
			Cvar_SetValue("fog_b",temp[2]);
		} else if (!strcmp(keyname, "_fog_start")) {
			Cvar_Set("fog_start",com_token);
		} else if (!strcmp(keyname, "_fog_end")) {
			Cvar_Set("fog_end",com_token);
		} else {
			//just do nothing
		}
	}



	if (foundworld) return NULL;
	return data;
}

void LoadLightsFromFile (char *data)
{	
	qboolean	isLight;
	entity_t	fakeEnt;

	Cvar_SetValue ("fog_start",0.0);
	Cvar_SetValue ("fog_end",0.0);

// parse ents
	while (1)
	{
// parse the opening brace	
		data = COM_Parse (data);
		if (!data)
			break;
		if (com_token[0] != '{')
			Sys_Error ("LoadLightsFromFile: found %s when expecting {",com_token);

		isLight = false;
		Q_memset(&fakeEnt,0,sizeof(entity_t));
		data = ParseEnt (data, &isLight, &fakeEnt);
		if (isLight) {
			LightFromFile(&fakeEnt);
			//Con_Printf("found light in file");
		}
	}
	
	if ((!fog_start.value) && (!fog_end.value)) {
		Cvar_SetValue ("gl_fog",0.0);
	}
}

void R_AutomaticLightPos() {

/*
	for (i=0; i<m->numsurfaces; i++) {
		surf = &m->surfaces[i];
		if (strstr(surf->texinfo->texture->name,"light")) {
			LightFromSurface(surf);
		}
	}*/

	LoadLightsFromFile(cl.worldmodel->entities);
}
