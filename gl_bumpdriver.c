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

Abstracts the different paths that exist for different 3d-cards.

  Idea: Write a new R_DrawLightEntitys* and do whatever you want in it.
	adapt R_DrawLightEntitys to call your new code if the requirements are met.
*/

/*
DrawLightEntities, draws lit bumpmapped entities, calls apropriate function for the user's 3d card.

NOTE:  This should not draw sprites, sprites are drawn separately.

*/
#include "quakedef.h"

bumpdriver_t gl_bumpdriver;

/* Some material definitions. */
float gl_Light_Ambience2[4] = {0.03f,0.03f,0.03f,0.03f};
float gl_Light_Diffuse2[4] = {0.03f,0.03f,0.03f,0.03f};
float gl_Light_Specular2[4] = {0,0,0,0};
float gl_Material_Color2[4] = {0.9f, 0.9f, 0.9f, 0.9f};

void R_DrawLightEntitiesGF3 (shadowlight_t *l);
void R_DrawLightEntitiesGF (shadowlight_t *l);
void R_DrawLightEntitiesGEN (shadowlight_t *l);
void R_DrawLightEntitiesRadeon (shadowlight_t *l); //PA:
void R_DrawLightEntitiesParhelia (shadowlight_t *l); //PA:
void R_DrawLightEntitiesARB (shadowlight_t *l); //PA:

/************************

Shader utility routines: For use by drivers

*************************/

void SH_SetupTcMod(tcmod_t *tc)
{
	switch (tc->type)
	{
	case TCMOD_ROTATE:
		glTranslatef(0.5,0.5,0.0);
		glRotatef(realtime * tc->params[0],0,0,1);
		glTranslatef(-0.5, -0.5, 0.0);
		break;
	case TCMOD_SCROLL:
		glTranslatef(realtime * tc->params[0], realtime * tc->params[1], 0.0);
		break;
	case TCMOD_SCALE:
		glScalef(tc->params[0],tc->params[1],1.0);
		break;
	case TCMOD_STRETCH:
		//PENTA: fixme
		glScalef(1.0, 1.0, 1.0);
		break;
	}
}

void SH_SetupTcMods(stage_t *s)
{
	int i;
	for (i = 0; i < s->numtcmods; i++)
		SH_SetupTcMod(&s->tcmods[i]);	
}


void SH_SetupSimpleStage(stage_t *s)
{
	tcmod_t *tc;
	int i;

	if (s->type != STAGE_SIMPLE)
	{
		Con_Printf("Non simple stage, in simple stage list");
		return;
	}

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();

	for (i=0; i<s->numtcmods; i++)
	{
		SH_SetupTcMod(&s->tcmods[i]);	
	}

	if (s->src_blend > -1)
	{
		glBlendFunc(s->src_blend, s->dst_blend);
		glEnable(GL_BLEND);
	}

	if (s->alphatresh > 0)
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, s->alphatresh);
	}

	if ((s->numtextures > 0) && (s->texture[0]))
		GL_BindAdvanced(s->texture[0]);
}

void SH_BindBumpmap(shader_t *shader, int index) {
	if (shader->numbumpstages) {
		if (shader->bumpstages[index].numtextures)
			GL_BindAdvanced(shader->bumpstages[index].texture[0]);
	}
}

void SH_BindColormap(shader_t *shader, int index) {
	if (shader->numbumpstages) {
		if (shader->colorstages[index].numtextures)
			GL_BindAdvanced(shader->colorstages[index].texture[0]);
	}
}

void SH_SetupAlphaTest(shader_t *shader) {
	if (shader->numcolorstages)
	{
		if (shader->colorstages[0].alphatresh > 0)
		{
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
		} else {
			glDisable(GL_ALPHA_TEST);
		}
	}
}

/**

	Draw tangent space for a vertexdef

*/
void R_DrawTangents (vertexdef_t *def, int num)
{
	float*			vert;
	float*			normal;
	float*			binormal;
	float*			tangent;
	int				vstr, nstr, tstr, bstr;
	vec3_t			extr;
	int				i;
/*
	vert = def->vertices;
	normal = def->normals;
	binormal = def->binormals;
	tangent = def->tangents;

	vstr = (def->vertexstride == 0) ? 3 : def->vertexstride/4;
	nstr = (def->normalstride == 0) ? 3 : def->normalstride/4;
	tstr = (def->tangentstride == 0) ? 3 : def->tangentstride/4;
	bstr = (def->binormalstride == 0) ? 3 : def->binormalstride/4;

	for (i=0; i<num; i++, vert+=vstr, normal+=nstr, binormal+=bstr, tangent+=tstr) {

		glColor3ub(255,0,0);
		glBegin(GL_LINES);
			glVertex3fv(&vert[0]);
			VectorMA(vert,1,normal,extr);
			glVertex3fv(&extr[0]);
		glEnd();

		glColor3ub(0,255,0);
		glBegin(GL_LINES);
			glVertex3fv(&vert[0]);
			VectorMA(vert,1,tangent,extr);
			glVertex3fv(&extr[0]);
		glEnd();

		glColor3ub(0,0,255);
		glBegin(GL_LINES);
			glVertex3fv(&vert[0]);
			VectorMA(vert,1,binormal,extr);
			glVertex3fv(&extr[0]);
		glEnd();
	}
*/
}


void R_DrawMeshAmbient(mesh_t *mesh) {

	vertexdef_t def;
	int	res;

	def.vertices = mesh->vertices;
	def.vertexstride = sizeof(mmvertex_t);

	def.texcoords = GL_OffsetDriverPtr(mesh->vertices, 12);
	def.texcoordstride = sizeof(mmvertex_t);

	def.tangents = mesh->tangents;
	def.tangentstride = 0;

	def.binormals = mesh->binormals;
	def.binormalstride = 0;

	def.normals = mesh->normals;
	def.normalstride = 0;

	def.lightmapcoords = GL_OffsetDriverPtr(mesh->vertices, 20);
	def.lightmapstride = sizeof(mmvertex_t);

	def.colors = GL_OffsetDriverPtr(mesh->vertices, 28);
	def.colorstride = sizeof(mmvertex_t);

	c_alias_polys += mesh->numtriangles;

	if (gl_occlusiontest && sh_occlusiontest.value) {
		glEnable(GL_OCCLUSION_TEST_HP);
		gl_bumpdriver.drawTriangleListBase(&def, mesh->indecies, mesh->numindecies, mesh->shader->shader, mesh->lightmapIndex);
		glDisable(GL_OCCLUSION_TEST_HP);
		glGetIntegerv(GL_OCCLUSION_TEST_RESULT_HP,&res);
		if (!res) {
			mesh->visframe = r_framecount - 10; //make it invisible this frame
			occlusion_cut_meshes++;
		}
	} else {
		gl_bumpdriver.drawTriangleListBase(&def, mesh->indecies, mesh->numindecies, mesh->shader->shader, mesh->lightmapIndex);
	}

	if (sh_showtangent.value) {
		 R_DrawTangents(&def, mesh->numvertices);
	}
}

void R_DrawMeshBumped(mesh_t *mesh) {

	vertexdef_t def;
	lightobject_t lo;

	if (mesh->visframe != r_framecount)
		return;

	def.vertices = mesh->vertices;
	def.vertexstride = sizeof(mmvertex_t);

	def.texcoords = GL_OffsetDriverPtr(mesh->vertices, 12);
	def.texcoordstride = sizeof(mmvertex_t);

	def.tangents = mesh->tangents;
	def.tangentstride = 0;

	def.binormals = mesh->binormals;
	def.binormalstride = 0;

	def.normals = mesh->normals;
	def.normalstride = 0;

	def.lightmapcoords = DRVNULL; // no lightmaps on aliasses

	VectorCopy(currentshadowlight->origin, lo.objectorigin);
	VectorCopy(r_refdef.vieworg, lo.objectvieworg);

	c_alias_polys += mesh->numtriangles;

	gl_bumpdriver.drawTriangleListBump(&def, mesh->indecies, mesh->numindecies, mesh->shader->shader, &mesh->trans, &lo);
}

void R_DrawAliasAmbient(aliashdr_t *paliashdr, aliasframeinstant_t *instant) {

	vertexdef_t def;

	def.vertices = GL_WrapUserPointer(&instant->vertices[0][0]);
	def.vertexstride = 0;
	def.texcoords = GL_WrapUserPointer((byte *)paliashdr + paliashdr->texcoords);
	def.texcoordstride = 0;
	def.tangents = GL_WrapUserPointer(&instant->tangents[0][0]);
	def.tangentstride = 0;
	def.binormals = GL_WrapUserPointer(&instant->binomials[0][0]);
	def.binormalstride = 0;
	def.normals = GL_WrapUserPointer(&instant->normals[0][0]);
	def.normalstride = 0;
	def.lightmapcoords = DRVNULL; // no lightmaps on aliasses
	def.colors = DRVNULL;
	def.colorstride = 0;

	c_alias_polys += paliashdr->numtris;

	gl_bumpdriver.drawTriangleListBase(&def, (int *)((byte *)paliashdr + paliashdr->indecies),paliashdr->numtris*3,paliashdr->shader, -1);

	if (sh_showtangent.value) {
		 R_DrawTangents(&def, paliashdr->numtris*3);
	}
}

void R_DrawAliasBumped(aliashdr_t *paliashdr, aliasframeinstant_t *instant) {

	vertexdef_t def;
	transform_t trans;
	aliaslightinstant_t *linstant = instant->lightinstant;
	lightobject_t lo;

	def.vertices = GL_WrapUserPointer(&instant->vertices[0][0]);
	def.vertexstride = 0;
	def.texcoords = GL_WrapUserPointer((byte *)paliashdr + paliashdr->texcoords);
	def.texcoordstride = 0;
	def.tangents = GL_WrapUserPointer(&instant->tangents[0][0]);
	def.tangentstride = 0;
	def.binormals = GL_WrapUserPointer(&instant->binomials[0][0]);
	def.binormalstride = 0;
	def.normals = GL_WrapUserPointer(&instant->normals[0][0]);
	def.normalstride = 0;
	def.colors = DRVNULL;
	def.colorstride = 0;

	VectorCopy(currententity->origin,trans.origin);
	VectorCopy(currententity->angles,trans.angles);
	trans.scale[0] = trans.scale[1] = trans.scale[2] = 1.0f;

	VectorCopy(linstant->lightpos, lo.objectorigin);
	VectorCopy(linstant->vieworg, lo.objectvieworg);
	c_alias_polys += paliashdr->numtris;

	gl_bumpdriver.drawTriangleListBump(&def,&linstant->indecies[0],linstant->numtris*3,paliashdr->shader, &trans, &lo);
}

msurface_t	*lightmapSurfs[MAX_LIGHTMAPS];
//Batches are almost never larger than about 300 indexes in a normal map
#define MAX_BATCH_SIZE 512
int indexList[MAX_BATCH_SIZE];

void R_DrawBrushAmbient (entity_t *e) {
	int numsurf;
	msurface_t *s;
	glpoly_t *p;
	int freeIndex, i;
	shader_t *lastShader;
	model_t	*model;

	model = e->model;
	memset(lightmapSurfs, 0, sizeof(msurface_t*)*cl.worldmodel->numlightmaps);
	//Sort per lightmap
	s =  &model->surfaces[model->firstmodelsurface];
	for (i=0; i<model->nummodelsurfaces; i++, s++) {
		if (!s->polys) { continue; }
		s->lightmapchain = lightmapSurfs[s->lightmaptexturenum];
		lightmapSurfs[s->lightmaptexturenum] = s;
	}

	//Dump to index array
	for (i=0; i<cl.worldmodel->numlightmaps; i++) {
		s = lightmapSurfs[i];
		freeIndex = 0;
		if (!s) continue;
		lastShader = s->shader->shader;
		while (s) {
			p = s->polys;
			//Space left in this batch?
			if (((freeIndex + p->numindecies) >= MAX_BATCH_SIZE) || (s->shader->shader != lastShader)) {
				gl_bumpdriver.drawTriangleListBase(&worldVertexDef, indexList, freeIndex, lastShader, i);
				freeIndex = 0;
			}
			memcpy(&indexList[freeIndex],p->indecies,sizeof(int)*p->numindecies);
			freeIndex += p->numindecies;
			lastShader = s->shader->shader;
			s = s->lightmapchain;
		}

		//Draw the last batch
		if (freeIndex) {
			gl_bumpdriver.drawTriangleListBase(&worldVertexDef, indexList, freeIndex, lastShader, i);
			//Con_Printf("DrawBatch %i\n", freeIndex/3);
		}
	}
}

void R_DrawBrushBumped (entity_t *e) {
	transform_t trans;
	int freeIndex, i;
	lightobject_t lo;
	shader_t *lastShader;
	msurface_t *s;
	glpoly_t *p;
	model_t	*model;

	VectorCopy(e->origin,trans.origin);
	VectorCopy(e->angles,trans.angles);
	trans.scale[0] = trans.scale[1] = trans.scale[2] = 1.0f;

	VectorCopy(((brushlightinstant_t *)e->brushlightinstant)->lightpos, lo.objectorigin);
	VectorCopy(((brushlightinstant_t *)e->brushlightinstant)->vieworg, lo.objectvieworg);

	model = e->model;
	s = &model->surfaces[model->firstmodelsurface];
	lastShader = s->shader->shader;
	freeIndex = 0;
	for (i=0; i<model->nummodelsurfaces; i++, s++) {
		p = s->polys;
		if (!p) continue;
		//Shader has changed, end this batch and start a new one
		if ((s->shader->shader != lastShader) || ((freeIndex + p->numindecies) >= MAX_BATCH_SIZE)) {
			gl_bumpdriver.drawTriangleListBump(&worldVertexDef, indexList, freeIndex, lastShader, &trans, &lo);	
			//Con_Printf("BumpDrawBatch %i\n", freeIndex/3);
			freeIndex = 0;
		}
		lastShader = s->shader->shader;
		memcpy(&indexList[freeIndex],p->indecies,sizeof(int)*p->numindecies);
		freeIndex += p->numindecies;
	}

	if (freeIndex) {
		gl_bumpdriver.drawTriangleListBump(&worldVertexDef, indexList, freeIndex, lastShader, &trans, &lo);	
		//Con_Printf("BumpDrawBatch %i\n", freeIndex/3);
	}
}

/**
	All surfaces have the same shader but may have different lightmaps
*/
void R_DrawWorldAmbientChain(msurface_t *first) {
	int numsurf;
	msurface_t *s;
	glpoly_t *p;
	int freeIndex, i;

	memset(lightmapSurfs, 0, sizeof(msurface_t*)*cl.worldmodel->numlightmaps);
	//Sort per lightmap
	s = first;
	while (s) {
		if (!s->polys) { s = s->texturechain; continue; }
		s->lightmapchain = lightmapSurfs[s->lightmaptexturenum];
		lightmapSurfs[s->lightmaptexturenum] = s;
		s = s->texturechain;
	}

    //Dump to index array
	for (i=0; i<cl.worldmodel->numlightmaps; i++) {
		s = lightmapSurfs[i];
		freeIndex = 0;
		while (s) {
			p = s->polys;
			//Space left in this batch?
			if ((freeIndex + p->numindecies) >= MAX_BATCH_SIZE) {
				gl_bumpdriver.drawTriangleListBase(&worldVertexDef, indexList, freeIndex, first->shader->shader, i);
				//Con_Printf("DrawBatch %i\n", freeIndex/3);
				freeIndex = 0;
			}
			memcpy(&indexList[freeIndex],p->indecies,sizeof(int)*p->numindecies);
			freeIndex += p->numindecies;
			s = s->lightmapchain;
		}

		//Draw the last batch
		if (freeIndex) {
			gl_bumpdriver.drawTriangleListBase(&worldVertexDef, indexList, freeIndex, first->shader->shader, i);
			//Con_Printf("DrawBatch %i\n", freeIndex/3);
		}
	}
}

void R_DrawWorldBumped() {

	transform_t trans;
	int freeIndex, i;
	lightobject_t lo;
	shader_t *lastShader;
	msurface_t *s;
	glpoly_t *p;

	if (!currentshadowlight->visible)
		return;


	glDepthMask (0);
	glShadeModel (GL_SMOOTH);
	glDepthFunc(GL_EQUAL);

	trans.angles[0] = trans.angles[1]  = trans.angles[2] = 0.0f;
	trans.origin[0] = trans.origin[1]  = trans.origin[2] = 0.0f;
	trans.scale[0] = trans.scale[1] = trans.scale[2] = 1.0f;

	VectorCopy(currentshadowlight->origin, lo.objectorigin);
	VectorCopy(r_refdef.vieworg, lo.objectvieworg);

	for (i=0; i<currentshadowlight->numlightcmdsmesh-1; i++) {
		R_DrawMeshBumped((mesh_t *)currentshadowlight->lightCmdsMesh[i].asVoid);
	}
	
        if ( (msurface_t *)currentshadowlight->lightCmds[0].asVoid)
	    lastShader = ((msurface_t *)currentshadowlight->lightCmds[0].asVoid)->shader->shader;
	freeIndex = 0;
	for (i=0; i<currentshadowlight->numlightcmds-1; i++) {
		s = (msurface_t *)currentshadowlight->lightCmds[i].asVoid;
		p = s->polys;
		if (!p) continue;
		//Shader has changed, end this batch and start a new one
		if ((s->shader->shader != lastShader) || ((freeIndex + p->numindecies) >= MAX_BATCH_SIZE)) {
			gl_bumpdriver.drawTriangleListBump(&worldVertexDef, indexList, freeIndex, lastShader, &trans, &lo);	
			//Con_Printf("BumpDrawBatch %i\n", freeIndex/3);
			freeIndex = 0;
		}
		lastShader = s->shader->shader;
		memcpy(&indexList[freeIndex],p->indecies,sizeof(int)*p->numindecies);
		freeIndex += p->numindecies;
	}

	if (freeIndex) {
		gl_bumpdriver.drawTriangleListBump(&worldVertexDef, indexList, freeIndex, lastShader, &trans, &lo);	
		//Con_Printf("BumpDrawBatch %i\n", freeIndex/3);
	}

	glColor3f (1,1,1);
	glDisable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glDepthMask (1);
}

/*
void R_DrawWorldBumped ()  // <AWE> Function should not have parameters.
{
    switch(gl_cardtype )
    {
    case GEFORCE3:
	R_DrawWorldBumpedGF3();	// <AWE> Function has no parameters.
	break;

    case GEFORCE:
	R_DrawWorldBumpedGF();	// <AWE> Function has no parameters.
	break;

    case RADEON:
	R_DrawWorldBumpedRadeon();
	break;
#ifndef __glx__
    case PARHELIA:
	R_DrawWorldBumpedParhelia();
	break;
#endif
    case ARB:
	R_DrawWorldBumpedARB();
	break;
    default:
	R_DrawWorldBumpedGEN();
	break;
    }
}
*//*
void R_DrawLightEntities (shadowlight_t *l)
{
    switch(gl_cardtype )
    {
    case GEFORCE3:
	R_DrawLightEntitiesGF3( l );
	break;

    case GEFORCE:
	R_DrawLightEntitiesGF( l );
	break;

    case RADEON:
	R_DrawLightEntitiesRadeon( l );
	break;
#ifndef __glx__
    case PARHELIA:
	R_DrawLightEntitiesParhelia( l );
	break;
#endif
    case ARB:
	R_DrawLightEntitiesARB( l );
	break;
    default:
	R_DrawLightEntitiesGEN( l );
	break;
    }
}
*/
/*
void R_DrawLightEntitiesGF (shadowlight_t *l)
{
    int		i;
    float	pos[4];
    if (!cg_showentities.value)
	return;

    if (!currentshadowlight->visible)
	return;

    glDepthMask (0);
    glShadeModel (GL_SMOOTH);

    //Meshes: Atten & selfshadow via vertex ligting

    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
	
    pos[0] = l->origin[0];
    pos[1] = l->origin[1];
    pos[2] = l->origin[2];
    pos[3] = 1;

    glLightfv(GL_LIGHT0, GL_POSITION,&pos[0]);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, &l->color[0]);
    glLightfv(GL_LIGHT0, GL_AMBIENT, &gl_Light_Ambience2[0]);
    glLightfv(GL_LIGHT0, GL_SPECULAR, &gl_Light_Specular2[0]);
    glEnable(GL_COLOR_MATERIAL);


    for (i=0 ; i<cl_numlightvisedicts ; i++)
    {
	currententity = cl_lightvisedicts[i];

	if (currententity->model->type == mod_alias)
	{
	    //these models are full bright 
	    if (currententity->model->flags & EF_FULLBRIGHT) continue;
	    if (!currententity->aliasframeinstant) continue;
	    if ( ((aliasframeinstant_t *)currententity->aliasframeinstant)->shadowonly) continue;

	    R_DrawAliasObjectLight(currententity, R_DrawAliasBumped);
	}
    }

    if (R_ShouldDrawViewModel())
    {
	R_DrawAliasObjectLight(&cl.viewent, R_DrawAliasBumped);
    }


    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);

    
    //  Brushes: we use the same thecnique as the world
    
	
    //glEnable(GL_TEXTURE_2D);
    //GL_Bind(glow_texture_object);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    //glShadeModel (GL_SMOOTH);
    //glEnable(GL_ALPHA_TEST);
    //glAlphaFunc(GL_GREATER,0.2);
    for (i=0 ; i<cl_numlightvisedicts ; i++)
    {
	currententity = cl_lightvisedicts[i];

	if (currententity->model->type == mod_brush)
	{
	    if (!currententity->brushlightinstant) continue;
	    if ( ((brushlightinstant_t *)currententity->brushlightinstant)->shadowonly) continue;
	    R_DrawBrushObjectLight(currententity, R_DrawBrushBumped);
	}
	
    }
	
    //reset gl state

    //glAlphaFunc(GL_GREATER,0.666);//Satan!
    //glDisable(GL_ALPHA_TEST);
    glColor3f (1,1,1);
    glDepthMask (1);	
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void R_DrawLightEntitiesGEN (shadowlight_t *l)
{
    //Currently this is merged with the geforce2 path
    R_DrawLightEntitiesGF(l);
}
*/
void R_DrawLightEntities (shadowlight_t *l)
{
    int		i;

    if (!cg_showentities.value)
	return;

    if (!currentshadowlight->visible)
	return;

    glDepthMask (0);
    glShadeModel (GL_SMOOTH);

    //Alias models

    for (i=0 ; i<cl_numlightvisedicts ; i++)
    {
		currententity = cl_lightvisedicts[i];

		if (currententity->model->type == mod_alias)
		{
			//these models are full bright 
			if (currententity->model->flags & EF_FULLBRIGHT) continue;
			if (!currententity->aliasframeinstant) continue;
			if ( ((aliasframeinstant_t *)currententity->aliasframeinstant)->shadowonly) continue;

			R_DrawAliasObjectLight(currententity, R_DrawAliasBumped);
		}
    }

    if (R_ShouldDrawViewModel()) {
		R_DrawAliasObjectLight(&cl.viewent, R_DrawAliasBumped);
    }

    //Brush models
    for (i=0 ; i<cl_numlightvisedicts ; i++)
    {
		currententity = cl_lightvisedicts[i];

		if (currententity->model->type == mod_brush)
		{
			if (!currententity->brushlightinstant) continue;
			if ( ((brushlightinstant_t *)currententity->brushlightinstant)->shadowonly) continue;
			R_DrawBrushObjectLight(currententity, R_DrawBrushBumped);
		}
    }

    //Cleanup state
    glColor3f (1,1,1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask (1);
}
