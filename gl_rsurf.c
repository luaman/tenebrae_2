/*
Copyright (C) 1996-1997 Id Software, Inc.

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

*/
// r_surf.c: surface-related refresh code

#include "quakedef.h"

#ifndef GL_RGBA4
#define	GL_RGBA4	0
#endif


int		lightmap_bytes;		// 1, 2, or 4

int		lightmap_textures = 0;

/*
====================================================
Global vertex table stuff:

The global vertex table contains world+brush model+static models+curve vertices.

	We first add vertices to a dynamically allocated buffer (malloc)
	at the end of level loading we copy that to a "quake" allocated
	buffer (on the Hunk)
====================================================
*/

mmvertex_t *tempVertices = NULL;
int	tempVerticesSize = 0;
int	numTempVertices = 0;
int	numVertices = 0;

int R_GetNextVertexIndex(void) {
	return numTempVertices;
}
/*
Returns the index of the vertex the data was copied to...
*/
int R_AllocateVertexInTemp(vec3_t pos, float texture [2], float lightmap[2], byte color[4]) {

	int i;
	mmvertex_t *temp;


	if (!tempVertices) {
		tempVerticesSize = 512;
		tempVertices = malloc(tempVerticesSize*sizeof(mmvertex_t));
		numTempVertices = 0;
	}

	if (numTempVertices >= tempVerticesSize) {

		tempVerticesSize+=512;
		temp = malloc(tempVerticesSize*sizeof(mmvertex_t));
		if (!temp) Sys_Error("R_AllocateVertexInTemp: malloc failed\n");
		Q_memcpy(temp,tempVertices,(tempVerticesSize-512)*sizeof(mmvertex_t));
		free(tempVertices);
		tempVertices = temp;
	}

	VectorCopy(pos,tempVertices[numTempVertices].position);
	for (i=0; i<2; i++) {
		tempVertices[numTempVertices].texture[i] = texture[i];
		tempVertices[numTempVertices].lightmap[i] = lightmap[i];
	}

	for (i=0; i<4; i++) {
		tempVertices[numTempVertices].color[i] = color[i];
	}

	numTempVertices++;
	return numTempVertices-1;
}

vertexdef_t worldVertexDef;

void R_CopyVerticesToHunk(void)
{
	vec3_t *vecs;
	int i,j;
	//Position, texture coords and color
	//Setup the vertexdef for the world
	Q_memset(&worldVertexDef, 0, sizeof(vertexdef_t));
	worldVertexDef.vertices = GL_StaticAlloc(numTempVertices*sizeof(mmvertex_t), tempVertices);
	worldVertexDef.vertexstride = sizeof(mmvertex_t);
	worldVertexDef.texcoords = GL_OffsetDriverPtr(worldVertexDef.vertices, 12);
	worldVertexDef.texcoordstride = sizeof(mmvertex_t);
	worldVertexDef.lightmapcoords = GL_OffsetDriverPtr(worldVertexDef.vertices, 20);
	worldVertexDef.lightmapstride = sizeof(mmvertex_t);
	worldVertexDef.colors = GL_OffsetDriverPtr(worldVertexDef.vertices, 28);
	worldVertexDef.colorstride = sizeof(mmvertex_t);

	free(tempVertices);

	//Tangent space basis
	vecs = malloc(numTempVertices*sizeof(vec3_t));

	//tangents
	for (i=0; i<cl.worldmodel->numsurfaces; i++) {
		msurface_t *s = &cl.worldmodel->surfaces[i];
		glpoly_t *p = s->polys;
		if (!p) continue;
		for (j=0; j<p->numverts; j++) {
			VectorCopy(s->tangent,vecs[p->firstvertex+j]);
		}
	}
	worldVertexDef.tangents = GL_StaticAlloc(numTempVertices*sizeof(vec3_t), vecs);

	//binormals
	for (i=0; i<cl.worldmodel->numsurfaces; i++) {
		msurface_t *s = &cl.worldmodel->surfaces[i];
		glpoly_t *p = s->polys;
		if (!p) continue;
		for (j=0; j<p->numverts; j++) {
			VectorCopy(s->binormal,vecs[p->firstvertex+j]);
		}
	}
	worldVertexDef.binormals = GL_StaticAlloc(numTempVertices*sizeof(vec3_t), vecs);

	//normals
	for (i=0; i<cl.worldmodel->numsurfaces; i++) {
		msurface_t *s = &cl.worldmodel->surfaces[i];
		glpoly_t *p = s->polys;
		if (!p) continue;
		for (j=0; j<p->numverts; j++) {
			VectorCopy(s->plane->normal,vecs[p->firstvertex+j]);
		}
	}
	worldVertexDef.normals = GL_StaticAlloc(numTempVertices*sizeof(vec3_t), vecs);

	Con_Printf("Copied %i vertices to hunk\n",numTempVertices);

	numVertices = numTempVertices;
	tempVertices = NULL;
	tempVerticesSize = 0;
	numTempVertices = 0;
}

/*
void R_EnableVertexTable(int fields) {

	glVertexPointer(3, GL_FLOAT, VERTEXSIZE*sizeof(float), globalVertexTable);
	glEnableClientState(GL_VERTEX_ARRAY);

	if (fields & VERTEX_TEXTURE) {
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexCoordPointer(2, GL_FLOAT, VERTEXSIZE*sizeof(float), (float *)(globalVertexTable)+3);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (fields & VERTEX_LIGHTMAP) {
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		glTexCoordPointer(2, GL_FLOAT, VERTEXSIZE*sizeof(float), (float *)(globalVertexTable)+5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
}

void R_DisableVertexTable(int fields) {

	glVertexPointer(3, GL_FLOAT, 0, globalVertexTable);
	glDisableClientState(GL_VERTEX_ARRAY);

	if (fields & VERTEX_TEXTURE) {
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (fields & VERTEX_LIGHTMAP) {
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
}
*/
void R_RenderDynamicLightmaps (msurface_t *fa);

/*
=============================================================

	BRUSH MODELS

=============================================================
*/


extern	int		solidskytexture;
extern	int		alphaskytexture;
extern	float	speedscale;		// for top sky and bottom sky

void DrawGLWaterPoly (glpoly_t *p);
void DrawGLWaterPolyLightmap (glpoly_t *p);

/* NV_register_combiners command function pointers
PENTA: I put them here because mtex functions are above it, and I hadent't any ispiration for some other place.
*/
PFNGLCOMBINERPARAMETERFVNVPROC qglCombinerParameterfvNV = NULL;
PFNGLCOMBINERPARAMETERIVNVPROC qglCombinerParameterivNV = NULL;
PFNGLCOMBINERPARAMETERFNVPROC qglCombinerParameterfNV = NULL;
PFNGLCOMBINERPARAMETERINVPROC qglCombinerParameteriNV = NULL;
PFNGLCOMBINERINPUTNVPROC qglCombinerInputNV = NULL;
PFNGLCOMBINEROUTPUTNVPROC qglCombinerOutputNV = NULL;
PFNGLFINALCOMBINERINPUTNVPROC qglFinalCombinerInputNV = NULL;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC qglGetCombinerInputParameterfvNV = NULL;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC qglGetCombinerInputParameterivNV = NULL;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC qglGetCombinerOutputParameterfvNV = NULL;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC qglGetCombinerOutputParameterivNV = NULL;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC qglGetFinalCombinerInputParameterfvNV = NULL;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC qglGetFinalCombinerInputParameterivNV = NULL;
PFNGLCOMBINERSTAGEPARAMETERFVNVPROC qglCombinerStageParameterfvNV = NULL;
PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC qglGetCombinerStageParameterfvNV = NULL;

PFNGLTEXIMAGE3DEXT qglTexImage3DEXT = NULL;

PFNGLACTIVETEXTUREARBPROC qglActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC qglClientActiveTextureARB = NULL;
PFNGLMULTITEXCOORD1FARBPROC qglMultiTexCoord1fARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC qglMultiTexCoord2fARB = NULL;
PFNGLMULTITEXCOORD2FVARBPROC qglMultiTexCoord2fvARB = NULL;
PFNGLMULTITEXCOORD3FARBPROC qglMultiTexCoord3fARB = NULL;
PFNGLMULTITEXCOORD3FVARBPROC qglMultiTexCoord3fvARB = NULL;

/*
ARB_vertex_program
PFNGLBINDPROGRAMARBPROC qglBindProgramARB = NULL;
PFNGLDELETEPROGRAMSARBPROC qglDeleteProgramsARB = NULL;
PFNGLGENPROGRAMSARBPROC qglGenProgramsARB = NULL;
PFNGLGETPROGRAMIVARBPROC qglGetProgramivARB = NULL;
PFNGLGETPROGRAMSTRINGARBPROC qglGetProgramStringARB = NULL;
PFNGLGETVERTEXATTRIBDVARBPROC qglGetVertexAttribdvARB = NULL;
PFNGLGETVERTEXATTRIBFVARBPROC qglGetVertexAttribfvARB = NULL;
PFNGLGETVERTEXATTRIBIVARBPROC qglGetVertexAttribivARB = NULL;
PFNGLGETVERTEXATTRIBPOINTERVARBPROC qglGetVertexAttribPointervARB = NULL;
PFNGLISPROGRAMARBPROC qglIsProgramARB = NULL;
PFNGPROGRAMSTRINGARBPROC qglProgramStringARB = NULL;
PFNGLTRACKMATRIXNVPROC glTrackMatrixNV = NULL;
PFNGLVERTEXATTRIB1DARBPROC qglVertexAttrib1dARB = NULL;
PFNGLVERTEXATTRIB1DVARBPROC qglVertexAttrib1dvARB = NULL;
PFNGLVERTEXATTRIB1FARBPROC qglVertexAttrib1fARB = NULL;
PFNGLVERTEXATTRIB1FVARBPROC qglVertexAttrib1fvARB = NULL;
PFNGLVERTEXATTRIB1SARBPROC qglVertexAttrib1sARB = NULL;
PFNGLVERTEXATTRIB1SVARBPROC qglVertexAttrib1svARB = NULL;
PFNGLVERTEXATTRIB2DARBPROC qglVertexAttrib2dARB = NULL;
PFNGLVERTEXATTRIB2DVARBPROC qglVertexAttrib2dvARB = NULL;
PFNGLVERTEXATTRIB2FARBPROC qglVertexAttrib2fARB = NULL;
PFNGLVERTEXATTRIB2FVARBPROC qglVertexAttrib2fvARB = NULL;
PFNGLVERTEXATTRIB2SARBPROC qglVertexAttrib2sARB = NULL;
PFNGLVERTEXATTRIB2SVARBPROC qglVertexAttrib2svARB = NULL;
PFNGLVERTEXATTRIB3DARBPROC qglVertexAttrib3dARB = NULL;
PFNGLVERTEXATTRIB3DVARBPROC qglVertexAttrib3dvARB = NULL;
PFNGLVERTEXATTRIB3FARBPROC qglVertexAttrib3fARB = NULL;
PFNGLVERTEXATTRIB3FVARBPROC qglVertexAttrib3fvARB = NULL;
PFNGLVERTEXATTRIB3SARBPROC qglVertexAttrib3sARB = NULL;
PFNGLVERTEXATTRIB3SVARBPROC qglVertexAttrib3svARB = NULL;
PFNGLVERTEXATTRIB4DARBPROC qglVertexAttrib4dARB = NULL;
PFNGLVERTEXATTRIB4DVARBPROC qglVertexAttrib4dvARB = NULL;
PFNGLVERTEXATTRIB4FARBPROC qglVertexAttrib4fARB = NULL;
PFNGLVERTEXATTRIB4FVARBPROC qglVertexAttrib4fvARB = NULL;
PFNGLVERTEXATTRIB4SARBPROC qglVertexAttrib4sARB = NULL;
PFNGLVERTEXATTRIB4SVARBPROC qglVertexAttrib4svARB = NULL;
PFNGLVERTEXATTRIB4UBVARBPROC qglVertexAttrib4ubvARB = NULL;
*/

PFNGLAREPROGRAMSRESIDENTNVPROC qglAreProgramsResidentNV = NULL;
PFNGLBINDPROGRAMNVPROC qglBindProgramNV = NULL;
PFNGLDELETEPROGRAMSNVPROC qglDeleteProgramsNV = NULL;
PFNGLEXECUTEPROGRAMNVPROC qglExecuteProgramNV = NULL;
PFNGLGENPROGRAMSNVPROC qglGenProgramsNV = NULL;
PFNGLGETPROGRAMPARAMETERDVNVPROC qglGetProgramParameterdvNV = NULL;
PFNGLGETPROGRAMPARAMETERFVNVPROC qglGetProgramParameterfvNV = NULL;
PFNGLGETPROGRAMIVNVPROC qglGetProgramivNV = NULL;
PFNGLGETPROGRAMSTRINGNVPROC qglGetProgramStringNV = NULL;
PFNGLGETTRACKMATRIXIVNVPROC qglGetTrackMatrixivNV = NULL;
PFNGLGETVERTEXATTRIBDVNVPROC qglGetVertexAttribdvNV = NULL;
PFNGLGETVERTEXATTRIBFVNVPROC qglGetVertexAttribfvNV = NULL;
PFNGLGETVERTEXATTRIBIVNVPROC qglGetVertexAttribivNV = NULL;
PFNGLGETVERTEXATTRIBPOINTERVNVPROC qglGetVertexAttribPointervNV = NULL;
PFNGLISPROGRAMNVPROC qglIsProgramNV = NULL;
PFNGLLOADPROGRAMNVPROC qglLoadProgramNV = NULL;
PFNGLPROGRAMPARAMETER4DNVPROC qglProgramParameter4dNV = NULL;
PFNGLPROGRAMPARAMETER4DVNVPROC qglProgramParameter4dvNV = NULL;
PFNGLPROGRAMPARAMETER4FNVPROC qglProgramParameter4fNV = NULL;
PFNGLPROGRAMPARAMETER4FVNVPROC qglProgramParameter4fvNV = NULL;
PFNGLPROGRAMPARAMETERS4DVNVPROC qglProgramParameters4dvNV = NULL;
PFNGLPROGRAMPARAMETERS4FVNVPROC qglProgramParameters4fvNV = NULL;
PFNGLREQUESTRESIDENTPROGRAMSNVPROC qglRequestResidentProgramsNV = NULL;
PFNGLTRACKMATRIXNVPROC qglTrackMatrixNV = NULL;
PFNGLVERTEXATTRIBPOINTERNVPROC qglVertexAttribPointerNV = NULL;
PFNGLVERTEXATTRIB1DNVPROC qglVertexAttrib1dNV = NULL;
PFNGLVERTEXATTRIB1DVNVPROC qglVertexAttrib1dvNV = NULL;
PFNGLVERTEXATTRIB1FNVPROC qglVertexAttrib1fNV = NULL;
PFNGLVERTEXATTRIB1FVNVPROC qglVertexAttrib1fvNV = NULL;
PFNGLVERTEXATTRIB1SNVPROC qglVertexAttrib1sNV = NULL;
PFNGLVERTEXATTRIB1SVNVPROC qglVertexAttrib1svNV = NULL;
PFNGLVERTEXATTRIB2DNVPROC qglVertexAttrib2dNV = NULL;
PFNGLVERTEXATTRIB2DVNVPROC qglVertexAttrib2dvNV = NULL;
PFNGLVERTEXATTRIB2FNVPROC qglVertexAttrib2fNV = NULL;
PFNGLVERTEXATTRIB2FVNVPROC qglVertexAttrib2fvNV = NULL;
PFNGLVERTEXATTRIB2SNVPROC qglVertexAttrib2sNV = NULL;
PFNGLVERTEXATTRIB2SVNVPROC qglVertexAttrib2svNV = NULL;
PFNGLVERTEXATTRIB3DNVPROC qglVertexAttrib3dNV = NULL;
PFNGLVERTEXATTRIB3DVNVPROC qglVertexAttrib3dvNV = NULL;
PFNGLVERTEXATTRIB3FNVPROC qglVertexAttrib3fNV = NULL;
PFNGLVERTEXATTRIB3FVNVPROC qglVertexAttrib3fvNV = NULL;
PFNGLVERTEXATTRIB3SNVPROC qglVertexAttrib3sNV = NULL;
PFNGLVERTEXATTRIB3SVNVPROC qglVertexAttrib3svNV = NULL;
PFNGLVERTEXATTRIB4DNVPROC qglVertexAttrib4dNV = NULL;
PFNGLVERTEXATTRIB4DVNVPROC qglVertexAttrib4dvNV = NULL;
PFNGLVERTEXATTRIB4FNVPROC qglVertexAttrib4fNV = NULL;
PFNGLVERTEXATTRIB4FVNVPROC qglVertexAttrib4fvNV = NULL;
PFNGLVERTEXATTRIB4SNVPROC qglVertexAttrib4sNV = NULL;
PFNGLVERTEXATTRIB4SVNVPROC qglVertexAttrib4svNV = NULL;
PFNGLVERTEXATTRIB4UBVNVPROC qglVertexAttrib4ubvNV = NULL;
PFNGLVERTEXATTRIBS1DVNVPROC qglVertexAttribs1dvNV = NULL;
PFNGLVERTEXATTRIBS1FVNVPROC qglVertexAttribs1fvNV = NULL;
PFNGLVERTEXATTRIBS1SVNVPROC qglVertexAttribs1svNV = NULL;
PFNGLVERTEXATTRIBS2DVNVPROC qglVertexAttribs2dvNV = NULL;
PFNGLVERTEXATTRIBS2FVNVPROC qglVertexAttribs2fvNV = NULL;
PFNGLVERTEXATTRIBS2SVNVPROC qglVertexAttribs2svNV = NULL;
PFNGLVERTEXATTRIBS3DVNVPROC qglVertexAttribs3dvNV = NULL;
PFNGLVERTEXATTRIBS3FVNVPROC qglVertexAttribs3fvNV = NULL;
PFNGLVERTEXATTRIBS3SVNVPROC qglVertexAttribs3svNV = NULL;
PFNGLVERTEXATTRIBS4DVNVPROC qglVertexAttribs4dvNV = NULL;
PFNGLVERTEXATTRIBS4FVNVPROC qglVertexAttribs4fvNV = NULL;
PFNGLVERTEXATTRIBS4SVNVPROC qglVertexAttribs4svNV = NULL;
PFNGLVERTEXATTRIBS4UBVNVPROC qglVertexAttribs4ubvNV = NULL; 

PFNGLBINDBUFFERARBPROC qglBindBufferARB = NULL; 
PFNGLDELETEBUFFERSARBPROC qglDeleteBuffersARB = NULL; 
PFNGLGENBUFFERSARBPROC qglGenBuffersARB = NULL; 
PFNGLISBUFFERARBPROC qglIsBufferARB = NULL; 
PFNGLBUFFERDATAARBPROC qglBufferDataARB = NULL; 
PFNGLBUFFERSUBDATAARBPROC qglBufferSubDataARB = NULL; 
PFNGLGETBUFFERSUBDATAARBPROC qglGetBufferSubDataARB = NULL; 
PFNGLMAPBUFFERARBPROC qglMapBufferARB = NULL; 
PFNGLUNMAPBUFFERARBPROC qglUnmapBufferARB = NULL; 
PFNGLGETBUFFERPARAMETERIVARBPROC qglGetBufferParameterivARB = NULL; 
PFNGLGETBUFFERPOINTERVARBPROC qglGetBufferPointervARB = NULL; 

qboolean mtexenabled = false;

void GL_SelectTexture (GLenum target);

void GL_DisableMultitexture(void) 
{
	if (mtexenabled) {
		glDisable(GL_TEXTURE_2D);
		GL_SelectTexture(GL_TEXTURE0_ARB);
		mtexenabled = false;
	}
}

void GL_EnableMultitexture(void) 
{
	if (gl_mtexable) {
		GL_SelectTexture(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		mtexenabled = true;
	}
}

/*
void R_RenderBrushPolyCaustics (msurface_t *fa)
{	int		i;
	float	*v;
	glpoly_t *p;

	glBegin (GL_TRIANGLE_FAN);
	p = fa->polys;
	//v = p->verts[0];
	v = (float *)(&globalVertexTable[p->firstvertex]);
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f (v[5], v[6]);
		glVertex3fv (v);
	}

	glEnd ();
}
*/
/*
================
DrawTextureChains
PENTA: Modifications

We fill the causistics chain here!
================
*/
void DrawTextureChains (void)
{
	int		i;
	msurface_t	*s;
	mapshader_t	*sh;
	shader_t	*shani;
	qboolean	found = false;
	mesh_t		*mesh;

	//glBlendFunc(GL_ZERO,GL_SRC_COLOR);
	//glEnable(GL_BLEND);
	glDepthMask (1);
	GL_DisableMultitexture();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor3f(sh_lightmapbright.value,sh_lightmapbright.value,sh_lightmapbright.value);

/*	GL_EnableMultitexture();
	if (sh_colormaps.value) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	} else {
		//No colormaps: Color maps are bound on tmu 0 so disable it
		//and let tu1 modulate itself with the light map brightness
		glDisable(GL_REGISTER_COMBINERS_NV);
		GL_SelectTexture(GL_TEXTURE0_ARB);		
		glDisable(GL_TEXTURE_2D);
		GL_SelectTexture(GL_TEXTURE1_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	}
*/


	if (gl_wireframe.value) {
		GL_SelectTexture(GL_TEXTURE0_ARB);
		glDisable(GL_TEXTURE_2D);
		GL_SelectTexture(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
	}

	causticschain = NULL; //clear chain here

	//R_EnableVertexTable(VERTEX_TEXTURE | VERTEX_LIGHTMAP);

	for (i=0 ; i<cl.worldmodel->nummapshaders ; i++)
	{
		sh = &cl.worldmodel->mapshaders[i];
		if (!sh)
			continue;

		s = sh->texturechain;
		if (!s)
			continue;
		if (!IsShaderBlended(sh->shader) || gl_wireframe.value) {
			R_DrawWorldAmbientChain(sh->texturechain);
			sh->texturechain = NULL;
		}
	}


	for (i=0 ; i<cl.worldmodel->nummapshaders ; i++)
	{
		sh = &cl.worldmodel->mapshaders[i];
		mesh = sh->meshchain;

		if (!mesh)
			continue;

		if (!IsShaderBlended(mesh->shader->shader) || gl_wireframe.value) {
			while (mesh) {
				R_DrawMeshAmbient(mesh);
				mesh = mesh->next;
			}
			sh->meshchain = NULL;
		}
	}

	if (gl_wireframe.value) {
		for (i=0 ; i<cl.worldmodel->nummapshaders ; i++)
		{
			cl.worldmodel->mapshaders[i].meshchain = NULL;
			cl.worldmodel->mapshaders[i].texturechain = NULL;
		}
		GL_SelectTexture(GL_TEXTURE0_ARB);
	}


	//R_DisableVertexTable(VERTEX_TEXTURE | VERTEX_LIGHTMAP);
	GL_SelectTexture(GL_TEXTURE1_ARB);
	GL_DisableMultitexture();
	//glDisable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (1);
}

void DrawBlendedTextureChains (void)
{
	int		i;
	msurface_t	*s;
	mapshader_t	*sh;
	shader_t	*shani;
	qboolean	found = false;
	mesh_t		*mesh;

	glDepthMask (1);
	//GL_DisableMultitexture();
	/*glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor3f(sh_lightmapbright.value,sh_lightmapbright.value,sh_lightmapbright.value);

	if (gl_wireframe.value) {
		GL_SelectTexture(GL_TEXTURE0_ARB);
		glDisable(GL_TEXTURE_2D);
		GL_SelectTexture(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
	}
	*/

	causticschain = NULL; //clear chain here

	//R_EnableVertexTable(VERTEX_TEXTURE | VERTEX_LIGHTMAP);

	for (i=0 ; i<cl.worldmodel->nummapshaders ; i++)
	{
		sh = &cl.worldmodel->mapshaders[i];
		if (!sh)
			continue;

		s = sh->texturechain;
		if (!s)
			continue;

		
		if (IsShaderBlended(sh->shader)) {
			R_DrawWorldAmbientChain(sh->texturechain);
			sh->texturechain = NULL;
		}
	}

	for (i=0 ; i<cl.worldmodel->nummapshaders ; i++)
	{
		sh = &cl.worldmodel->mapshaders[i];
		mesh = sh->meshchain;

		if (!mesh)
			continue;

		if (IsShaderBlended(mesh->shader->shader)) {
			while (mesh) {
				R_DrawMeshAmbient(mesh);
				mesh = mesh->next;
			}
			sh->meshchain = NULL;
		}
	}
	//R_DisableVertexTable(VERTEX_TEXTURE | VERTEX_LIGHTMAP);
	//GL_SelectTexture(GL_TEXTURE1_ARB);
	//GL_DisableMultitexture();
	glDisable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (1);
}

//void R_DrawBrushModelCaustics (entity_t *e);

/*
=============
R_DrawCaustics

Tenebrae does "real cauistics", projected textures on all things including ents
(Other mods seem to just add an extra, not properly projected layer to the polygons in the world.)
=============
*/
/*
void R_DrawCaustics(void) {

	msurface_t *s;
	int			i;
	vec3_t		mins, maxs;

	GLfloat sPlane[4] = {0.01f, 0.005f, 0.0f, 0.0f };
	GLfloat tPlane[4] = {0.0f, 0.01f, 0.005f, 0.0f };

	if (!gl_caustics.value) return;

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, sPlane);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tPlane);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_ONE);

	GL_Bind(caustics_textures[(int)(cl.time*16)&7]);
	busy_caustics = true;
	s = causticschain;

	R_EnableVertexTable(0);
	while (s) {
		//R_RenderBrushPolyCaustics (s);
		glDrawArrays(GL_TRIANGLE_FAN,s->polys->firstvertex,s->polys->numverts);			
		s = s->texturechain;
	}
	R_DisableVertexTable(0);

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
		{
			int i;
			for (i=0 ; i<3 ; i++)
			{
				mins[i] = currententity->origin[i] - currententity->model->radius;
				maxs[i] = currententity->origin[i] + currententity->model->radius;
			}
		} else {
			VectorAdd (currententity->origin,currententity->model->mins, mins);
			VectorAdd (currententity->origin,currententity->model->maxs, maxs);
		}

		if (R_CullBox (mins, maxs))
			continue;

		//quick hack! check if ent is below water
		if ((!(CL_PointContents(mins) & CONTENTS_WATER)) && (!(CL_PointContents(maxs) & CONTENTS_WATER)))
			continue;

		if (mirror) {
			if (mirror_clipside == BoxOnPlaneSide(mins, maxs, mirror_plane)) {
				continue;
			}


			if ( BoxOnPlaneSide(mins, maxs, &mirror_far_plane) == 1) {
				return;
			}
		}

		switch (currententity->model->type)
		{
		case mod_alias:
			R_DrawAliasModel (1.0);
			break;

		case mod_brush:
			R_DrawBrushModelCaustics(currententity);
			break;

		default:
			break;
		}
	}

	busy_caustics = false;
	glDisable(GL_BLEND);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);	
}
*/
/*
=================
R_DrawBrushModel
=================
*//*
void R_DrawBrushModelCaustics (entity_t *e)
{
	vec3_t		mins, maxs;
	int			i;
	msurface_t	*psurf;
	float		dot;
	mplane_t	*pplane;
	model_t		*clmodel;
	qboolean	rotated;

	//bright = 1;

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

	if (R_CullBox (mins, maxs))
		return;

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

    glPushMatrix ();
e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
e->angles[0] = -e->angles[0];	// stupid quake bug

	//Draw model with specified ambient color
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//
	// draw texture
	//
	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
				R_RenderBrushPolyCaustics (psurf);
		}
	}

	//R_BlendLightmaps (); nope no lightmaps 
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glPopMatrix ();
}
*/
/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModelAmbient (entity_t *e)
{
	vec3_t		mins, maxs;
	int			i;
	msurface_t	*psurf;
	float		dot;
	mplane_t	*pplane;
	model_t		*clmodel;
	qboolean	rotated;

	//bright = 1;

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

	if (R_CullBox (mins, maxs))
		return;

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

    glPushMatrix ();
e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
e->angles[0] = -e->angles[0];	// stupid quake bug

	R_DrawBrushAmbient(e);

/*	//Draw model with specified ambient color
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//
	// draw texture
	//
	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
				R_RenderBrushPolyLightmap (psurf);
		}
	}

	//R_BlendLightmaps (); nope no lightmaps 
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
*/
	glPopMatrix ();
}


/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode

PENTA: Modifications
================
*/
qboolean didnode;
aabox_t worldbox;

void R_RecursiveWorldNode (mnode_t *node)
{
	int			c, side, i;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	mesh_t		*mesh;
	double		dot;
	aabox_t		leafbox;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (R_CullBox (node->minmaxs, node->minmaxs+3))
		return;

		if (mirror) {

			int side = BoxOnPlaneSide(node->minmaxs, node->minmaxs+3, mirror_plane); 
			if ((mirror_clipside == side)) {
				return;
			}

			if ( BoxOnPlaneSide(node->minmaxs, node->minmaxs+3, &mirror_far_plane) == 1) {
				return;
			}
		}


// if a leaf node, draw stuff
	if (node->contents & CONTENTS_LEAF)
	{
/*		
		if (mirror) {
			if (mirror_clipside == BoxOnPlaneSide(node->minmaxs, node->minmaxs+3, mirror_plane)) {
				return;
			}
		}
	
*/
		pleaf = (mleaf_t *)node;


		leafbox = constructBox(pleaf->minmaxs,pleaf->minmaxs+3);
		worldbox = addBoxes(&worldbox,&leafbox);		

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			//Con_Printf("Leaf with %i surfaces\n",c);
			do
			{


/*
				if ((*mark)->flags & SURF_MIRROR) {
					if (!mirror)
						R_AllocateMirror((*mark));	
					mark++;
					continue;
				}*/

				/*
				if ((*mark)->flags & SURF_MIRROR) {

					//PENTA the  SURF_UNDERWATER check is needed so that we dont draw glass
					//twice since we design it as a water block we would render two poly's with the
					//mirror otherwice
					if(  (! (((*mark)->flags & SURF_UNDERWATER) && ((*mark)->flags & SURF_GLASS)) )|| (!((*mark)->flags & SURF_DRAWTURB)))

						if (!mirror) R_AllocateMirror((*mark));
					//destroy visframe
					(*mark)->visframe = 0;
					continue;
				}
				*/
				if ((*mark)->visframe == r_framecount) {
					mark++;
					continue;
				}
/*
				if (((*mark)->flags & SURF_DRAWTURB) && ((*mark)->flags & SURF_MIRROR)) {
					mark++;
					continue;
				}
				
				if (((*mark)->flags & SURF_DRAWTURB) && (mirror)) {
					mark++;
					continue;
				}
*/
				(*mark)->texturechain = (*mark)->shader->texturechain;
				(*mark)->shader->texturechain = (*mark);
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		c = pleaf->firstmesh;
		//if (pleaf->numcurves) Con_Printf("Numcurves %i\n",pleaf->numcurves);
		for (i=0; i<pleaf->nummeshes; i++) {
			mesh = &cl.worldmodel->meshes[cl.worldmodel->leafmeshes[c+i]];

			if (mesh->visframe == r_framecount) continue;
			mesh->visframe = r_framecount;

			if (R_CullBox(mesh->mins, mesh->maxs)) continue;

			mesh->next = mesh->shader->meshchain;
			mesh->shader->meshchain = mesh; 
			//Con_Printf("AddCurve\n");
		}

	// deal with model fragments in this leaf
	// steek de statishe moddellen voor dit leafy in de lijst
	// waarom zo ingewikkeld doen via efrags is nog niet helemaal duidelijk
		if (pleaf->efrags)
			R_StoreEfrags (&pleaf->efrags);

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side]);
	R_RecursiveWorldNode (node->children[!side]);

/*
// draw stuff
	c = node->numsurfaces;

	if (c)
	{
		surf = cl.worldmodel->surfaces + node->firstsurface;

		if (dot < 0 -BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;
		{
			if (c) Con_Printf("Node with %i surf\n",c);

			for ( ; c ; c--, surf++)
			{
				if (surf->visframe != r_framecount)
					continue;

				// don't backface underwater surfaces, because they warp
				
					if ( (dot < 0) ^ !!(surf->flags & SURF_PLANEBACK))
						continue;		// wrong side
				


				
				if (surf->flags & SURF_MIRROR) {

					//PENTA the  SURF_UNDERWATER check is needed so that we dont draw glass
					//twice since we design it as a water block we would render two poly's with the
					//mirror otherwice
					if(  (! ((surf->flags & SURF_UNDERWATER) && (surf->flags & SURF_GLASS)) )|| (!(surf->flags & SURF_DRAWTURB)))

						if (!mirror) R_AllocateMirror(surf);
					//destroy visframe
					surf->visframe = 0;
					continue;
				}

				if ((surf->flags & SURF_DRAWTURB) && (surf->flags & SURF_MIRROR)) {
					continue;
				}
				
				if ((surf->flags & SURF_DRAWTURB) && (mirror)) {
					continue;
				}

				// if sorting by texture, just store it out
				//if (gl_texsort.value)
				//{

					// add the surface to the proper texture chain 
					//if (!mirror
					//|| surf->texinfo->texture != cl.worldmodel->textures[mirrortexturenum])
					//{
						surf->texturechain = surf->shader->texturechain;
						surf->shader->texturechain = surf;
					//}
					

					// add the poly to the proper lightmap chain 
					//steek hem in de lijst en vermenigvuldig de lightmaps er achteraf over via
					//r_blendlightmaps
					//we steken geen lucht/water polys in de lijst omdat deze geen
					//lightmaps hebben
					if (!(surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))) {
						surf->polys->chain = lightmap_polys[surf->lightmaptexturenum];
						lightmap_polys[surf->lightmaptexturenum] = surf->polys;
					}

				
				//} else if (surf->flags & SURF_DRAWSKY) {
				//	surf->texturechain = skychain;
				//	skychain = surf;
				//} else if (surf->flags & SURF_DRAWTURB) {
				//	surf->texturechain = waterchain;
				//	waterchain = surf;
				//} else
				//	R_DrawSequentialPoly (surf);
				//

			}
		}

	}
*/
// recurse down the back side
	//R_RecursiveWorldNode (node->children[!side]);
}



/*
=============
R_InitDrawWorld

Make sure everyting is set up correctly to draw
the world model.

PENTA: Modifications
=============
*/
void R_InitDrawWorld (void)
{
	entity_t	ent;

	memset (&ent, 0, sizeof(ent));
	ent.model = cl.worldmodel;

	VectorCopy (r_refdef.vieworg, modelorg);

	currententity = &ent;
	currenttexture = -1;

	glColor3f (1,1,1);
	worldbox = emptyBox();
	R_RecursiveWorldNode (cl.worldmodel->nodes);

	if (gl_wireframe.value) return;
	R_DrawSkyBox ();
}

/*
=============
R_WorldMultiplyTextures

Once we have drawn the lightmap in the depth buffer multiply the textures over it
PENTA: New
=============
*/
void R_WorldMultiplyTextures(void) {
	DrawTextureChains ();
	//R_RenderDlights (); //render "fake" dynamic lights als die aan staan
}

/*
===============
R_MarkLeaves


Markeer visible leafs voor dit frame
aangeroepen vanuit render scene voor alle 
ander render code
(hmm handig hiernaa kunnen we dus al van de vis voor de camera
gebruik maken en toch nog stuff doen)

PENTA: Modifications

===============
*/

void R_MarkLeaves (void)
{
	byte	*vis;
	mnode_t	*node;
	mleaf_t	*leaf;
	int		i;
	byte	solid[4096];

	//zelfde leaf als vorige frame vermijd van vis opnieuw te calculaten
//	if (r_oldviewleaf == r_viewleaf && !r_novis.value)
//		return;
	
	//	return;

	r_visframecount++;//begin nieuwe timestamp
	r_oldviewleaf = r_viewleaf;

	if (r_novis.value)
	{
		vis = solid;
		memset (solid, 0xff, (cl.worldmodel->numleafs+7)>>3);
	}
	else
		vis = Mod_LeafPVS (r_viewleaf, cl.worldmodel);//vraag visible leafs vanuit dit leaf aan model
		
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)//overloop alle leafs
	{

		leaf = cl.worldmodel->leafs+i;
		if (vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))) // staat leaf i's vis bit op true ?
		{
			// check area portals
			if (! (r_refdef.areabits[leaf->area>>3] & (1<<(leaf->area&7)) ) && !r_noareaportal.value ) {
				continue;		// not visible
			}

			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount) //node al op true gezet dit frame, exit
					break;
				node->visframe = r_visframecount;//zet node zijn timestamp voor dit frame
				node = node->parent;//mark parents as vis
			} while (node);
		}
	}
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

void SaveTGA (const char *filename, unsigned char *pixels, int width, int height) {
	TargaHeader		targa_header;
	FILE			*fout;
	int i;

	fout = fopen(filename, "wb");
        if (!fout)
        {
	    Con_Printf("Could not create file %s", filename);
	    return;
        }
   
	//fwrite(pixels, 1, width*height*4, fout);
	for (i=0; i<width*height; i++) {
		int ind = i*3;
		fputc(pixels[ind+0],fout);
		fputc(pixels[ind+1],fout);
		fputc(pixels[ind+2],fout);
	}
	fclose(fout);
}


/*
==================
GL_BuildLightmaps

Lightmaps are much easyer with q3, we just upload all the lightmaps here,
and he surface then binds the correct lightmap...
Of course we can't have dynamic lights with this but we don't need those anyway...
==================
*/
void GL_BuildLightmaps (void)
{
	int		i, j;
	model_t	*m;

	r_framecount = 1;		// no dlightcache

	if (!lightmap_textures)
	{
		lightmap_textures = texture_extension_number;
		texture_extension_number += MAX_LIGHTMAPS;
	}

	gl_lightmap_format = GL_RGB;
	lightmap_bytes = 4;

	if (!COM_CheckParm("-externallight")) {
		byte *packed, *deluxPack;
		int lmIndex;
		char name[512];
	//no lightmaps in bsp make everything black...
		if (!cl.worldmodel->numlightmaps) {
			int black = 0;
			Con_Printf("No lightmaps in map, defaulting to black.\n");

			GL_Bind(lightmap_textures);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D (GL_TEXTURE_2D, 0, 3, 1, 1, 0, gl_lightmap_format, GL_UNSIGNED_BYTE, &black);

			for (i=0; i<cl.worldmodel->numsurfaces; i++) {
				cl.worldmodel->surfaces[i].lightmaptexturenum = 0;
			}
			cl.worldmodel->numlightmaps = 1;

			Cvar_Set("sh_delux","0");
			return;
		}
	//Load lightmaps stored in the bsp lump
		packed = malloc(PACKED_LIGHTMAP_WIDTH*PACKED_LIGHTMAP_WIDTH*3);
		deluxPack = malloc(PACKED_LIGHTMAP_WIDTH*PACKED_LIGHTMAP_WIDTH*3);
		for (i=0 ; i<cl.worldmodel->numlightmaps ; i+=2)
		{
			int lmIndex = i/2; //Delux not counted
			int ofsX = LIGHTMAP_COLUMN(lmIndex)*FILE_LIGHTMAP_WIDTH;
			int ofsY = LIGHTMAP_ROW(lmIndex)*FILE_LIGHTMAP_WIDTH;
			int x,y,z;

			//Copy light color
			byte *lmap = cl.worldmodel->lightdata+(lmIndex*2*FILE_LIGHTMAP_WIDTH*FILE_LIGHTMAP_WIDTH*3);
			for (x=0; x<FILE_LIGHTMAP_WIDTH; x++) {
				for (y=0; y<FILE_LIGHTMAP_WIDTH; y++) {
					int pInd = ((ofsY+y)*PACKED_LIGHTMAP_WIDTH+ofsX+x)*3;
					int fInd = (y*FILE_LIGHTMAP_WIDTH+x)*3;
					packed[pInd+0] = lmap[fInd+0];
					packed[pInd+1] = lmap[fInd+1];
					packed[pInd+2] = lmap[fInd+2];
				}
			}

			//Delux
			lmap = cl.worldmodel->lightdata+((lmIndex*2+1)*FILE_LIGHTMAP_WIDTH*FILE_LIGHTMAP_WIDTH*3);
			for (x=0; x<FILE_LIGHTMAP_WIDTH; x++) {
				for (y=0; y<FILE_LIGHTMAP_WIDTH; y++) {
					int pInd = ((ofsY+y)*PACKED_LIGHTMAP_WIDTH+ofsX+x)*3;
					int fInd = (y*FILE_LIGHTMAP_WIDTH+x)*3;
					deluxPack[pInd+0] = lmap[fInd+0];
					deluxPack[pInd+1] = lmap[fInd+1];
					deluxPack[pInd+2] = lmap[fInd+2];
				}
			}

			if ((lmIndex%PACKED_LIGHTMAP_COUNT) == PACKED_LIGHTMAP_COUNT-1) {
				
				sprintf(name,"PackLight%i.raw",(lmIndex/PACKED_LIGHTMAP_COUNT)/2);
				SaveTGA(name, packed, PACKED_LIGHTMAP_WIDTH, PACKED_LIGHTMAP_WIDTH);

				sprintf(name,"PackDelux%i.raw",(lmIndex/PACKED_LIGHTMAP_COUNT)/2+1);
				SaveTGA(name, deluxPack, PACKED_LIGHTMAP_WIDTH, PACKED_LIGHTMAP_WIDTH);

				//Color
				GL_Bind(lightmap_textures + ((lmIndex/PACKED_LIGHTMAP_COUNT)*2));
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexImage2D (GL_TEXTURE_2D, 0, 3
				, PACKED_LIGHTMAP_WIDTH, PACKED_LIGHTMAP_WIDTH, 0, 
				gl_lightmap_format, GL_UNSIGNED_BYTE,packed);
				//Delux
				GL_Bind(lightmap_textures + ((lmIndex/PACKED_LIGHTMAP_COUNT)*2+1));
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexImage2D (GL_TEXTURE_2D, 0, 3
					, PACKED_LIGHTMAP_WIDTH, PACKED_LIGHTMAP_WIDTH, 0, 
					gl_lightmap_format, GL_UNSIGNED_BYTE,deluxPack);
			}
		}

		//Upload an eventual incomplete last one
		lmIndex = cl.worldmodel->numlightmaps/2;
		if (((lmIndex)%PACKED_LIGHTMAP_COUNT) != PACKED_LIGHTMAP_COUNT-1) {

			sprintf(name,"PackLight%i.raw",(lmIndex/PACKED_LIGHTMAP_COUNT)*2);
			SaveTGA(name, packed, 512, 512);

			sprintf(name,"PackDelux%i.raw",(lmIndex/PACKED_LIGHTMAP_COUNT)*2+1);
			SaveTGA(name, deluxPack, 512, 512);

			//Color
			GL_Bind(lightmap_textures + ((lmIndex*2)/PACKED_LIGHTMAP_COUNT));
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D (GL_TEXTURE_2D, 0, 3
				, PACKED_LIGHTMAP_WIDTH, PACKED_LIGHTMAP_WIDTH, 0, 
				gl_lightmap_format, GL_UNSIGNED_BYTE,packed);
			//Delux
			GL_Bind(lightmap_textures + ((lmIndex*2+1)/PACKED_LIGHTMAP_COUNT));
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D (GL_TEXTURE_2D, 0, 3
				, PACKED_LIGHTMAP_WIDTH, PACKED_LIGHTMAP_WIDTH, 0, 
				gl_lightmap_format, GL_UNSIGNED_BYTE,deluxPack);
		}

		cl.worldmodel->numlightmaps = (cl.worldmodel->numlightmaps/PACKED_LIGHTMAP_COUNT)+1;
		Con_Printf("GL_BuildLightmaps: %i unused lightmap slots\n", cl.worldmodel->numlightmaps%PACKED_LIGHTMAP_COUNT);
		free(packed);
		free(deluxPack);
	} else {
	//Load externally stored lightmaps
		int old_tex_ext;
		char basename[1024];
		char filename[1024];
		
		COM_StripExtension(cl.worldmodel->name, basename);
		Con_Printf("Loading external lightmaps from %s\n", basename);

		//Hack: so we can use easy tga load
		old_tex_ext = texture_extension_number;
		texture_extension_number = lightmap_textures;
		
		i = 0;
		while (true)
		{
			FILE *f;
			sprintf(filename,"%s/lm_%04d.tga",basename,i);
			Con_Printf("  Trying: %s\n", filename);
			COM_FOpenFile(filename, &f);
			if (!f) break;
			fclose(f);

			Con_Printf("  Lightmap: %s\n", filename);
			EasyTgaLoad(filename);
			i++;
		}
		texture_extension_number = old_tex_ext;
		cl.worldmodel->numlightmaps = i;
	}

}

