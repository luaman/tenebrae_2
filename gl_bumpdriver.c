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
float gl_Light_Ambience2[4] = {0.03,0.03,0.03,0.03};
float gl_Light_Diffuse2[4] = {0.03,0.03,0.03,0.03};
float gl_Light_Specular2[4] = {0,0,0,0};
float gl_Material_Color2[4] = {0.9, 0.9, 0.9, 0.9};

void R_DrawLightEntitiesGF3 (shadowlight_t *l);
void R_DrawLightEntitiesGF (shadowlight_t *l);
void R_DrawLightEntitiesGEN (shadowlight_t *l);
void R_DrawLightEntitiesRadeon (shadowlight_t *l); //PA:
void R_DrawLightEntitiesParhelia (shadowlight_t *l); //PA:
void R_DrawLightEntitiesARB (shadowlight_t *l); //PA:

/*************************

Temp backwards compatibility

**************************/

void R_DrawMeshAmbient(mesh_t *mesh) {

	vertexdef_t def;
	transform_t trans;
	int	res;

	def.vertices = &globalVertexTable[mesh->firstvertex].position[0];
	def.vertexstride = sizeof(mmvertex_t);

	def.texcoords = &globalVertexTable[mesh->firstvertex].texture[0];
	def.texcoordstride = sizeof(mmvertex_t);

	def.tangents = &mesh->tangents[0][0];
	def.tangentstride = 0;

	def.binormals = &mesh->binormals[0][0];
	def.binormalstride = 0;

	def.normals = &mesh->normals[0][0];
	def.normalstride = 0;

	def.lightmapcoords = &globalVertexTable[mesh->firstvertex].lightmap[0]; // no lightmaps on aliasses
	def.lightmapstride = sizeof(mmvertex_t);

	def.colors = &globalVertexTable[mesh->firstvertex].color[0];
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
}

void R_DrawMeshBumped(mesh_t *mesh) {

	vertexdef_t def;

	if (mesh->visframe != r_framecount)
		return;

	def.vertices = &globalVertexTable[mesh->firstvertex].position[0];
	def.vertexstride = sizeof(mmvertex_t);

	def.texcoords = &globalVertexTable[mesh->firstvertex].texture[0];
	def.texcoordstride = sizeof(mmvertex_t);

	def.tangents = &mesh->tangents[0][0];
	def.tangentstride = 0;

	def.binormals = &mesh->binormals[0][0];
	def.binormalstride = 0;

	def.normals = &mesh->normals[0][0];
	def.normalstride = 0;

	def.lightmapcoords = NULL; // no lightmaps on aliasses

	c_alias_polys += mesh->numtriangles;

	gl_bumpdriver.drawTriangleListBump(&def, mesh->indecies, mesh->numindecies, mesh->shader->shader, &mesh->trans);
}

void R_DrawAliasAmbient(aliashdr_t *paliashdr, aliasframeinstant_t *instant) {

	vertexdef_t def;
	transform_t trans;

	def.vertices = &instant->vertices[0][0];
	def.vertexstride = 0;
	def.texcoords = (float *)((byte *)paliashdr + paliashdr->texcoords);
	def.texcoordstride = 0;
	def.tangents = &instant->tangents[0][0];
	def.tangentstride = 0;
	def.binormals = &instant->binomials[0][0];
	def.binormalstride = 0;
	def.normals = &instant->normals[0][0];
	def.normalstride = 0;
	def.lightmapcoords = NULL; // no lightmaps on aliasses
	def.colors = NULL;
	def.colorstride = 0;

	c_alias_polys += paliashdr->numtris;

	gl_bumpdriver.drawTriangleListBase(&def, (int *)((byte *)paliashdr + paliashdr->indecies),paliashdr->numtris*3,paliashdr->shader, -1);
}

void R_DrawAliasBumped(aliashdr_t *paliashdr, aliasframeinstant_t *instant) {

	vertexdef_t def;
	transform_t trans;
	aliaslightinstant_t *linstant = instant->lightinstant;

	def.vertices = &instant->vertices[0][0];
	def.vertexstride = 0;
	def.texcoords = (float *)((byte *)paliashdr + paliashdr->texcoords);
	def.texcoordstride = 0;
	def.tangents = &instant->tangents[0][0];
	def.tangentstride = 0;
	def.binormals = &instant->binomials[0][0];
	def.binormalstride = 0;
	def.normals = &instant->normals[0][0];
	def.normalstride = 0;
	def.colors = NULL;
	def.colorstride = 0;

	VectorCopy(currententity->origin,trans.origin);
	VectorCopy(currententity->angles,trans.angles);
	trans.scale[0] = trans.scale[1] = trans.scale[2] = 1.0f;

	c_alias_polys += paliashdr->numtris;

	gl_bumpdriver.drawTriangleListBump(&def,&linstant->indecies[0],linstant->numtris*3,paliashdr->shader, &trans);
}

void R_SetupWorldVertexDef(vertexdef_t *def) {

	def->vertices = &globalVertexTable[0].position[0];
	def->vertexstride = sizeof(mmvertex_t);
	def->texcoords = &globalVertexTable[0].texture[0];
	def->texcoordstride = sizeof(mmvertex_t);
	def->tangents = NULL;
	def->tangentstride = 0;
	def->binormals = NULL;
	def->binormalstride = 0;
	def->normals = NULL;
	def->normalstride = 0;
	def->lightmapcoords = &globalVertexTable[0].lightmap[0];
	def->lightmapstride = sizeof(mmvertex_t);

}

msurface_t *surfArray[1024];

void R_DrawBrushAmbient (entity_t *e) {

	int runlength, i;
	msurface_t *surf;
	vertexdef_t def;
	shader_t *runshader , *s;
	model_t	*model;

	model = e->model;
	R_SetupWorldVertexDef(&def);

	runshader = NULL;
	runlength = 0;
	surf = &model->surfaces[model->firstmodelsurface];
	glColor3f(sh_lightmapbright.value, sh_lightmapbright.value, sh_lightmapbright.value);
	for (i=0; i<model->nummodelsurfaces; i++, surf++)
	{
		s = surf->shader->shader;
		surf->visframe = r_framecount;
		if (s != runshader) {
			//a run has finished, draw it
			if (runshader && runlength)
				gl_bumpdriver.drawSurfaceListBase(&def, surfArray, runlength, runshader);	
			
			//start a new one
			runshader = s;
			runlength = 1;
			surfArray[0] = surf;
		} else {
			if (runlength < 1024) {
				surfArray[runlength] = surf;
				runlength++;
			}
		}
	}
	if (runshader && runlength) {
		c_brush_polys += runlength;
		gl_bumpdriver.drawSurfaceListBase(&def, surfArray, runlength, runshader);
	}
}

void R_DrawBrushBumped (entity_t *e) {

	int runlength, i;
	msurface_t *surf;
	vertexdef_t def;
	shader_t *runshader , *s;
	model_t	*model;
	transform_t trans;

	model = e->model;
	R_SetupWorldVertexDef(&def);

	runshader = NULL;
	runlength = 0;
	surf = &model->surfaces[model->firstmodelsurface];
	for (i=0; i<model->nummodelsurfaces; i++, surf++)
	{
		if (runlength < 1024) {
			surfArray[runlength] = surf;
			runlength++;
		}
	}

	VectorCopy(e->origin,trans.origin);
	VectorCopy(e->angles,trans.angles);
	trans.scale[0] = trans.scale[1] = trans.scale[2] = 1.0f;
	c_brush_polys += runlength;
	gl_bumpdriver.drawSurfaceListBump(&def, surfArray, runlength, &trans);
}

void R_DrawWorldAmbientChain(msurface_t *first) {

	vertexdef_t def;
	int numsurf;
	msurface_t *s;
	R_SetupWorldVertexDef(&def);

	numsurf = 0;
	s = first;
	while (s) {
		if (numsurf < 1024) {
			surfArray[numsurf] = s;
			numsurf++;
		} else {
			gl_bumpdriver.drawSurfaceListBase(&def, surfArray, numsurf, first->shader->shader);
			numsurf = 0;
		}
		s = s->texturechain;
	}

	if (numsurf) {
		c_brush_polys += numsurf;
		gl_bumpdriver.drawSurfaceListBase(&def, surfArray, numsurf, first->shader->shader);
		numsurf = 0;
	}
}

void R_DrawWorldBumped() {

	vertexdef_t def;
	transform_t trans;
	int i;

	if (!currentshadowlight->visible)
		return;

	R_SetupWorldVertexDef(&def);

	glDepthMask (0);
	glShadeModel (GL_SMOOTH);
	glDepthFunc(GL_EQUAL);

	trans.angles[0] = trans.angles[1]  = trans.angles[2] = 0.0f;
	trans.origin[0] = trans.origin[1]  = trans.origin[2] = 0.0f;
	trans.scale[0] = trans.scale[1] = trans.scale[2] = 1.0f;

	c_brush_polys += (currentshadowlight->numlightcmds-1);
	gl_bumpdriver.drawSurfaceListBump(&def, (msurface_t **)(&currentshadowlight->lightCmds[0]), currentshadowlight->numlightcmds-1, &trans);

	for (i=0; i<currentshadowlight->numlightcmdsmesh-1; i++) {
		R_DrawMeshBumped((mesh_t *)currentshadowlight->lightCmdsMesh[i].asVoid);
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
/*
//PA:
void R_DrawLightEntitiesRadeon (shadowlight_t *l)
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

	    R_DrawAliasObjectLight(currententity, R_DrawAliasBumpedRadeon);
	}
    }

    if (R_ShouldDrawViewModel()) {
	R_DrawAliasObjectLight(&cl.viewent, R_DrawAliasBumpedRadeon);
    }

    //Brush models
    for (i=0 ; i<cl_numlightvisedicts ; i++)
    {
	currententity = cl_lightvisedicts[i];

	if (currententity->model->type == mod_brush)
	{
	    if (!currententity->brushlightinstant) continue;
	    if ( ((brushlightinstant_t *)currententity->brushlightinstant)->shadowonly) continue;
	    R_DrawBrushObjectLight(currententity, R_DrawBrushBumpedRadeon);
	}

    }

    //Cleanup state
    glColor3f (1,1,1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask (1);
}

#ifndef __glx__

//PA:
void R_DrawLightEntitiesParhelia (shadowlight_t *l)
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

	    R_DrawAliasObjectLight(currententity, R_DrawAliasBumpedParhelia);
	}
    }

    if (R_ShouldDrawViewModel())
    {
	R_DrawAliasObjectLight(&cl.viewent, R_DrawAliasBumpedParhelia);
    }

    //Brush models
    for (i=0 ; i<cl_numlightvisedicts ; i++)
    {
	currententity = cl_lightvisedicts[i];

	if (currententity->model->type == mod_brush)
	{
	    if (!currententity->brushlightinstant) continue;
	    if ( ((brushlightinstant_t *)currententity->brushlightinstant)->shadowonly) continue;
	    R_DrawBrushObjectLight(currententity, R_DrawBrushBumpedParhelia);
	}

    }

    //Cleanup state
    glColor3f (1,1,1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask (1);
}

#endif

void R_DrawLightEntitiesARB (shadowlight_t *l)
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

	    R_DrawAliasObjectLight(currententity, R_DrawAliasBumpedARB);
	}
    }

    if (R_ShouldDrawViewModel())
    {
	R_DrawAliasObjectLight(&cl.viewent, R_DrawAliasBumpedARB);
    }

    //Brush models
    for (i=0 ; i<cl_numlightvisedicts ; i++)
    {
	currententity = cl_lightvisedicts[i];

	if (currententity->model->type == mod_brush)
	{
	    if (!currententity->brushlightinstant) continue;
	    if ( ((brushlightinstant_t *)currententity->brushlightinstant)->shadowonly) continue;
	    R_DrawBrushObjectLight(currententity, R_DrawBrushBumpedARB);
	}

    }

    //Cleanup state
    glColor3f (1,1,1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask (1);
}
*/