/*
Copyright (C) 2001-2002 Charles Hollemeersch
Copyright (C) 2003 Jarno Paananen

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

Generic bumpmapping with some Geforce optimizations
*/

#include "quakedef.h"

static cvar_t sh_fakespecular = {"sh_fakespecular","1", true};

//#define GENDEBUG

#if defined(GENDEBUG) && defined(_WIN32)
static void generic_checkerror()
{
    GLuint error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        _asm { int 3 };
    }
}
#else

#define generic_checkerror() do { } while(0)

#endif

/************************

	Generic "fragment programs"

************************/

/**
	Light attenuation shader
*/
void GL_EnableGenericAttShader() {
	
	GL_Bind(glow_texture_object);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	GL_EnableMultitexture();
	GL_Bind(glow_texture_object);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

}

void GL_DisableGenericAttShader() {
	GL_DisableMultitexture();
}

/**
	Color map modulation shader
	primary color = light color
	tu0 = light filter cube map 
	tu1 = material color map
*/
void GL_EnableGenericColorShader (qboolean specular) {

	GL_SelectTexture(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);

	GL_SelectTexture(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->filtercube);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	
	if (!specular) {
		GL_EnableMultitexture();
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	} else {
		GL_EnableMultitexture();
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	}
	
}

void GL_DisableGenericColorShader (qboolean specular) {

	if (!specular) {
		GL_DisableMultitexture();
	} else {
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		GL_DisableMultitexture();
	}

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);
}

/**
	Diffuse dot product shader

	texture coords for unit 0: Tangent space light vector
	texture coords for unit 1: Normal map texture coords
*/
void GL_EnableGenericDiffuseShader () {

	GL_SelectTexture(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	
	GL_EnableMultitexture();
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);

}

void GL_DisableGenericDiffuseShader () {

	GL_DisableMultitexture();
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

}

/************************

Shader utility routines

*************************/

void Generic_SetupTcMod(tcmod_t *tc)
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


void Generic_SetupSimpleStage(stage_t *s)
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
		Generic_SetupTcMod(&s->tcmods[i]);	
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

/************************

  Generic triangle list routines
  
*************************/

void FormatError(); // In gl_bumpgf.c

/*
void Generic_drawTriangleListBump (const vertexdef_t *verts, int *indecies,
								   int numIndecies, shader_t *shader,
								   const transform_t *tr, const lightobject_t *lo)
{
    return;
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	
    GL_AddColor();
    glColor3fv(&currentshadowlight->color[0]);
	
    Generic_EnableBumpShader(tr,currentshadowlight->origin,true, shader);
	
    //bind the correct textures
    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    if (shader->numbumpstages > 0)
		GL_BindAdvanced(shader->bumpstages[0].texture[0]);
    if (!verts->texcoords)
		FormatError();
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
	
    GL_SelectTexture(GL_TEXTURE1_ARB);
    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    if (shader->numcolorstages > 0)
		GL_BindAdvanced(shader->colorstages[0].texture[0]);
    if (!verts->tangents)
		FormatError();
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(3, GL_FLOAT, verts->tangentstride, verts->tangents);
	
    if (!verts->binormals)
		FormatError();
    GL_SelectTexture(GL_TEXTURE2_ARB);
    qglClientActiveTextureARB(GL_TEXTURE2_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(3, GL_FLOAT, verts->binormalstride, verts->binormals);
	
    if (!verts->normals)
		FormatError();
    GL_SelectTexture(GL_TEXTURE3_ARB);
    qglClientActiveTextureARB(GL_TEXTURE3_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(3, GL_FLOAT, verts->normalstride, verts->normals);
	
    glDrawElements(GL_TRIANGLES, numIndecies, GL_UNSIGNED_INT, indecies);
	
    qglClientActiveTextureARB(GL_TEXTURE3_ARB);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
    qglClientActiveTextureARB(GL_TEXTURE2_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    Generic_DisableBumpShader(shader);
}
*/

static const lightobject_t *currentLo;

void VertWV(float *pos, float *tex, float *lm, float *tg, float *bn, float*nr) {
	glTexCoord3fv(pos);
	qglMultiTexCoord2fvARB(GL_TEXTURE1_ARB, tex);
	glVertex3fv(pos);
}

void VertTex(float *pos, float *tex, float *lm, float *tg, float *bn, float*nr) {
	glTexCoord2fv(tex);
	glVertex3fv(pos);
}

void VertFakeSpecular(float *pos, float *tex, float *lm, float *tg, float *bn, float*nr) {
	glNormal3fv(nr);
	glTexCoord3fv(pos);
	qglMultiTexCoord2fvARB(GL_TEXTURE1_ARB, tex);
	glVertex3fv(pos);
}


void VertAtt(float *pos, float *tex, float *lm, float *tg, float *bn, float*nr) {

	vec3_t nearToVert;
	float tu, tv, tw;

	VectorSubtract (pos, currentLo->objectorigin, nearToVert);

	tu = (nearToVert[0]/currentshadowlight->radiusv[0])*0.5+0.5;
	tv = (nearToVert[1]/currentshadowlight->radiusv[1])*0.5+0.5;
	tw = (nearToVert[2]/currentshadowlight->radiusv[2])*0.5+0.5;

	glTexCoord2f(tu, tv);
	qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tw, 0.5);
	glVertex3fv(pos);
}

void VertDiffuse(float *pos, float *tex, float *lm, float *tg, float *bn, float*nr) {

	vec3_t lightDir, tsLightDir;

	VectorSubtract(currentLo->objectorigin, pos, lightDir);

	tsLightDir[0] = DotProduct(lightDir,tg);	
	tsLightDir[1] = -DotProduct(lightDir,bn);
	tsLightDir[2] = DotProduct(lightDir,nr);
			
	qglMultiTexCoord3fvARB(GL_TEXTURE0_ARB, &tsLightDir[0]);
	qglMultiTexCoord2fvARB(GL_TEXTURE1_ARB, tex);
	glVertex3fv(pos);
}

void Generic_DrawTriWithFunc(const vertexdef_t *verts, int *indecies,
								int numIndecies, void (*VertFunc) (float *, float *, float *, float *, float *, float*))
{
	int i;
	vertexdef_t v = *verts;

	if (!v.vertexstride) v.vertexstride = 12;
	if (!v.texcoordstride) v.texcoordstride = 8;
	if (!v.lightmapstride) v.lightmapstride = 8;
	if (!v.tangentstride) v.lightmapstride = 12;
	if (!v.binormalstride) v.binormalstride = 12;
	if (!v.normalstride) v.normalstride = 12;

	glBegin(GL_TRIANGLES);
	for (i=0; i<numIndecies; i++) {
		int ind = indecies[i];

		VertFunc(	(float *)((byte *)v.vertices + ind*v.vertexstride),
					(float *)((byte *)v.texcoords + ind*v.texcoordstride),
					(float *)((byte *)v.lightmapcoords + ind*v.lightmapstride),
					(float *)((byte *)v.tangents + ind*v.tangentstride),
					(float *)((byte *)v.binormals + ind*v.binormalstride),
					(float *)((byte *)v.normals + ind*v.normalstride)
				);
	}
	glEnd();
}


void Generic_drawTriangleListBump (const vertexdef_t *verts, int *indecies,
								   int numIndecies, shader_t *shader,
								   const transform_t *tr, const lightobject_t *lo)
{
    glDepthMask (0);
	
	currentLo = lo;

	//attenuate with projected 2D attenuation map
    GL_DrawAlpha();
    GL_EnableGenericAttShader();
	Generic_DrawTriWithFunc(verts, indecies, numIndecies, VertAtt);
    GL_DisableGenericAttShader();	

    // modulate diffuse bump (n dot l) to alpha
    GL_ModulateAlphaDrawAlpha();
    GL_EnableGenericDiffuseShader();
	if (shader->numbumpstages > 0) {
		GL_BindAdvanced(shader->bumpstages[0].texture[0]);
	}
	Generic_DrawTriWithFunc(verts, indecies, numIndecies, VertDiffuse);
    GL_DisableGenericDiffuseShader ();

	//Do fake specular
	if (sh_fakespecular.value) {
		float black[4] = {0.0, 0.0, 0.0, 1.0};
		float white[4] = {1.0, 1.0, 1.0, 1.0};
		float pos[4];

		GL_ModulateAlphaDrawColor();
		glColorMask(true, true, true, false);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		VectorCopy(lo->objectorigin, pos);
		pos[3] = 1.0;

		glLightfv(GL_LIGHT0, GL_POSITION, pos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, black);
		glLightfv(GL_LIGHT0, GL_AMBIENT, black);
		glLightfv(GL_LIGHT0, GL_SPECULAR, &currentshadowlight->color[0]);

		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16);

		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

		glEnable(GL_COLOR_MATERIAL);
		glColor4fv(white);

		GL_EnableGenericColorShader (true);
		if (shader->numbumpstages > 0) {
			GL_BindAdvanced(shader->bumpstages[0].texture[0]);
		}
		Generic_DrawTriWithFunc(verts, indecies, numIndecies, VertFakeSpecular);
		GL_DisableGenericColorShader (true);

		glDisable(GL_LIGHTING);
	}

    //	Then modulate Color map and light filter with alpha and add to color
	GL_ModulateAlphaDrawColor();
    glColor3fv(&currentshadowlight->color[0]);
    if ( currentshadowlight->filtercube )
    {
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		
		GL_SetupCubeMapMatrix(tr);
		GL_EnableGenericColorShader (false);
		if (shader->numcolorstages > 0) {
			GL_BindAdvanced(shader->colorstages[0].texture[0]);
		}
		Generic_DrawTriWithFunc(verts, indecies, numIndecies, VertWV);
		GL_DisableGenericColorShader (false);
		
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
    }
    else
    {
		// Just alpha * light color
		GL_DisableMultitexture();
		if (shader->numcolorstages > 0) {
			GL_BindAdvanced(shader->colorstages[0].texture[0]);
		}
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		Generic_DrawTriWithFunc(verts, indecies, numIndecies, VertTex);
    }

    GL_DrawColor();
    glColor3f (1,1,1);
    glDisable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask (1);
}

void Generic_drawTriangleListBase (vertexdef_t *verts, int *indecies,
								   int numIndecies, shader_t *shader,
								   int lightmapIndex)
{
    int i;
	
    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
	
    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
    if (!shader->cull)
    { 
		glDisable(GL_CULL_FACE); 
    } 
	
    for ( i = 0; i < shader->numstages; i++)
    {
		Generic_SetupSimpleStage(&shader->stages[i]);
		glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
		glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);
	
    if (verts->lightmapcoords && (lightmapIndex >= 0) && (shader->flags & SURF_PPLIGHT)) 
    {
		if (shader->numstages && shader->numcolorstages)
		{
			if (shader->colorstages[0].src_blend >= 0)
			{
				glEnable(GL_BLEND);
				glBlendFunc(shader->colorstages[0].src_blend, shader->colorstages[0].dst_blend);
			}
			else
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
			}
		}
		else
		{
			glDisable(GL_BLEND);
		}
		
		if (shader->numcolorstages)
		{
			if (shader->colorstages[0].numtextures)
				GL_BindAdvanced(shader->colorstages[0].texture[0]);
			
			if (shader->colorstages[0].alphatresh > 0)
			{
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
			}	
		}
		
		GL_SelectTexture(GL_TEXTURE0_ARB);
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		
		GL_SelectTexture(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		GL_Bind(lightmap_textures+lightmapIndex);
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		glTexCoordPointer(2, GL_FLOAT, verts->lightmapstride, verts->lightmapcoords);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		
		glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
		
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
		GL_SelectTexture(GL_TEXTURE0_ARB);
    }
    else
    {
		if (verts->colors && (shader->flags & SURF_PPLIGHT))
		{
			glColorPointer(3, GL_UNSIGNED_BYTE, verts->colorstride, verts->colors);
			glEnableClientState(GL_COLOR_ARRAY);
			glShadeModel(GL_SMOOTH);
			
			if (shader->numstages && shader->numcolorstages)
			{
				if (shader->colorstages[0].src_blend >= 0)
				{
					glEnable(GL_BLEND);
					glBlendFunc(shader->colorstages[0].src_blend, shader->colorstages[0].dst_blend);
				}
				else
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE);
				}
			}
			else 
			{
				glDisable(GL_BLEND);
			}
			
			if (shader->numcolorstages)
			{
				if (shader->colorstages[0].numtextures)
					GL_BindAdvanced(shader->colorstages[0].texture[0]);
				
				if (shader->colorstages[0].alphatresh > 0)
				{
					glEnable(GL_ALPHA_TEST);
					glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
				}	
			}
			
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			
			glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
			
			glDisableClientState(GL_COLOR_ARRAY);
		}
		else
		{
			if (shader->flags & SURF_PPLIGHT)
			{
				glColor3f(0,0,0);
				glDisable(GL_TEXTURE_2D);
				glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
				glEnable(GL_TEXTURE_2D);
			}
		}
    }
	
    if (!shader->cull)
    {
		glEnable(GL_CULL_FACE);
		//Con_Printf("Cullstuff %s\n",shader->name);
    }
	
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
}



/*************************

Generic world surfaces routines

**************************/

void Generic_sendSurfacesBase(msurface_t **surfs, int numSurfaces,
			      qboolean bindLightmap)
{
    int i, j;
    glpoly_t *p;
    msurface_t *surf;
    float *v;
	
    for ( i = 0; i < numSurfaces; i++)
	{
		surf = surfs[i];
		if (surf->visframe != r_framecount)
			continue;
		p = surf->polys;
		if (bindLightmap)
		{
			if (surf->lightmaptexturenum < 0)
				continue;
			GL_Bind(lightmap_textures+surf->lightmaptexturenum);
		}
		
		glBegin(GL_TRIANGLES);
		for (j=0; j<surf->polys->numindecies; j++) {
			v = (float *)(&cl.worldmodel->userVerts[surf->polys->indecies[j]]);
			qglMultiTexCoord2fvARB(GL_TEXTURE0_ARB, &v[3]);
			qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB, &v[5]);
			glVertex3fv(&v[0]);
		}
		glEnd();

    }
}

void Generic_drawSurfaceListBase (vertexdef_t* verts, msurface_t** surfs,
								  int numSurfaces, shader_t* shader)
{
    int i;
	
    GL_SelectTexture(GL_TEXTURE0_ARB);
    glColor3ub(255,255,255);
	
    if (!shader->cull)
    {
		glDisable(GL_CULL_FACE);
		//Con_Printf("Cullstuff %s\n",shader->name);
    }
	
    for (i = 0; i < shader->numstages; i++)
    {
		Generic_SetupSimpleStage(&shader->stages[i]);
		Generic_sendSurfacesBase(surfs, numSurfaces, false);
		glPopMatrix();
    }
	
    if (verts->lightmapcoords && (shader->flags & SURF_PPLIGHT))
    {
		GL_SelectTexture(GL_TEXTURE1_ARB);
		
		// Regular lightmapping
		if (shader->numstages && shader->numcolorstages)
		{
			if (shader->colorstages[0].src_blend >= 0)
			{
				glEnable(GL_BLEND);
				glBlendFunc(shader->colorstages[0].src_blend,
					shader->colorstages[0].dst_blend);
			}
			else
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
			}
		}
		else 
		{
			glDisable(GL_BLEND);
		}
		
		GL_SelectTexture(GL_TEXTURE0_ARB);
		if (shader->numcolorstages)
		{
			if (shader->colorstages[0].numtextures)
				GL_BindAdvanced(shader->colorstages[0].texture[0]);
			
			if (shader->colorstages[0].alphatresh > 0)
			{
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
			}	
		}
		
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		GL_EnableMultitexture();
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor3f(sh_lightmapbright.value, sh_lightmapbright.value,
			sh_lightmapbright.value);
		
		Generic_sendSurfacesBase(surfs, numSurfaces, true);
		
		GL_DisableMultitexture();
		GL_SelectTexture(GL_TEXTURE0_ARB);
    }
	
    if (!shader->cull)
    {
		glEnable(GL_CULL_FACE);
    }
	
    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND);
}


void Generic_sendSurfacesDiffuse(msurface_t **surfs, int numSurfaces)
{
    int i,j;
    glpoly_t *p;
    msurface_t *surf;
    shader_t *shader, *lastshader;
    float *v;
    qboolean cull;
    vec3_t lightDir;
    float tsLightDir[3];
	
    cull = true;
    lastshader = NULL;
	
    for (i=0; i<numSurfaces; i++)
    {
		surf = surfs[i];
		
		if (surf->visframe != r_framecount)
			continue;
		
		if (!(surf->flags & SURF_PPLIGHT))
			continue;
		
		shader = surf->shader->shader;
		
		if (shader->numbumpstages > 0)
			GL_BindAdvanced(shader->bumpstages[0].texture[0]);
		else
			continue;
		
		p = surf->polys;
		
		//less state changes
		if (lastshader != shader)
		{
			if (!shader->cull)
			{
				glDisable(GL_CULL_FACE);
				cull = false;
			}
			else
			{
				if (!cull)
					glEnable(GL_CULL_FACE);
				cull = true;
			}
			lastshader = shader;
		}
		
		glBegin(GL_TRIANGLES);
		for (j=0; j<surf->polys->numindecies; j++) {
			v = (float *)(&cl.worldmodel->userVerts[surf->polys->indecies[j]]);

			VectorSubtract(currentshadowlight->origin, (&v[0]), lightDir);
			if (surf->flags & SURF_PLANEBACK)	{
				tsLightDir[2] = -DotProduct(lightDir,surf->plane->normal);
			} else {
				tsLightDir[2] = DotProduct(lightDir,surf->plane->normal);
			}
			
			tsLightDir[1] = -DotProduct(lightDir,surf->binormal);
			tsLightDir[0] = DotProduct(lightDir,surf->tangent);
			
			qglMultiTexCoord3fvARB(GL_TEXTURE0_ARB, &tsLightDir[0]);
			qglMultiTexCoord2fvARB(GL_TEXTURE1_ARB, &v[3]);
			glVertex3fv(&v[0]);
		}
		glEnd();
    }
	
    if (!cull)
		glEnable(GL_CULL_FACE);
}


void Generic_sendSurfacesATT(msurface_t **surfs, int numSurfaces)
{
    int i, j;
    glpoly_t *p;
    msurface_t *surf;
    shader_t *shader, *lastshader;
    qboolean cull;
    float tu, tv, tw;
    vec3_t nearToVert;
    float *v;
	
    cull = true;
    lastshader = NULL;
	
    for ( i = 0; i < numSurfaces; i++)
    {
		surf = surfs[i];
		if (surf->visframe != r_framecount)
			continue;
		if (!(surf->flags & SURF_PPLIGHT))
			continue;
		
		shader = surf->shader->shader;
		if (shader->numbumpstages == 0)
			continue;
		
		p = surf->polys;
		
		if (lastshader != shader)
		{
			if (!shader->cull)
			{
				glDisable(GL_CULL_FACE);
				cull = false;
			}
			else
			{
				if (!cull)
					glEnable(GL_CULL_FACE);
				cull = true;
			}
			lastshader = shader;
		}
		
		glColor4f(1, 1, 1, 1);
	
		glBegin(GL_TRIANGLES);
		for (j=0; j<surf->polys->numindecies; j++) {
			v = (float *)(&cl.worldmodel->userVerts[surf->polys->indecies[j]]);

			VectorSubtract (v, currentshadowlight->origin, nearToVert);

			tu = (nearToVert[0]/currentshadowlight->radiusv[0])*0.5+0.5;
			tv = (nearToVert[1]/currentshadowlight->radiusv[1])*0.5+0.5;
			tw = (nearToVert[2]/currentshadowlight->radiusv[2])*0.5+0.5;
			
			glTexCoord2f(tu, tv);
			qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tw, 0.5);
			glVertex3fv(&v[0]);
		}
		glEnd();
    }
    if (!cull)
		glEnable(GL_CULL_FACE);
}

void Generic_sendSurfacesWV(msurface_t **surfs, int numSurfaces, qboolean specular)
{
    int i, j;
    glpoly_t *p;
    msurface_t *surf;
    shader_t *shader, *lastshader;
    qboolean cull;
    float *v;
    cull = true;
    lastshader = NULL;
	
    for ( i = 0; i < numSurfaces; i++)
    {
		surf = surfs[i];
		if (surf->visframe != r_framecount)
			continue;
		if (!(surf->flags & SURF_PPLIGHT))
			continue;
		
		shader = surf->shader->shader;
		if (shader->numbumpstages == 0)
			continue;
		
		p = surf->polys;
		
		if (lastshader != shader)
		{
			if (!shader->cull)
			{
				glDisable(GL_CULL_FACE);
				cull = false;
			}
			else
			{
				if (!cull)
					glEnable(GL_CULL_FACE);
				cull = true;
			}
			lastshader = shader;
			GL_SelectTexture(GL_TEXTURE1_ARB);
			if (!specular) {
				if (shader->numcolorstages > 0)
					GL_BindAdvanced(shader->colorstages[0].texture[0]);
			} else {
				if (shader->numbumpstages > 0)
					GL_BindAdvanced(shader->bumpstages[0].texture[0]);
			}
		}

		glNormal3fv(surf->plane->normal);

		glBegin(GL_TRIANGLES);
		for (j=0; j<surf->polys->numindecies; j++) {
			v = (float *)(&cl.worldmodel->userVerts[surf->polys->indecies[j]]);
			glTexCoord3fv(&v[0]);
			qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[3], v[4]);
			glVertex3fv(&v[0]);
		}
		glEnd();
    }
    if (!cull)
		glEnable(GL_CULL_FACE);
}

void Generic_sendSurfacesTex(msurface_t **surfs, int numSurfaces)
{
    int i, j;
    glpoly_t *p;
    msurface_t *surf;
    shader_t *shader, *lastshader;
    qboolean cull;
    float *v;
	
    cull = true;
    lastshader = NULL;
	
    for ( i = 0; i < numSurfaces; i++)
    {
		surf = surfs[i];
		if (surf->visframe != r_framecount)
			continue;
		if (!(surf->flags & SURF_PPLIGHT))
			continue;
		
		shader = surf->shader->shader;
		if (shader->numbumpstages == 0)
			continue;
		
		p = surf->polys;
		
		if (lastshader != shader)
		{
			if (!shader->cull)
			{
				glDisable(GL_CULL_FACE);
				cull = false;
			}
			else
			{
				if (!cull)
					glEnable(GL_CULL_FACE);
				cull = true;
			}
			lastshader = shader;
			
			GL_SelectTexture(GL_TEXTURE0_ARB);
			if (shader->numcolorstages > 0)
				GL_BindAdvanced(shader->colorstages[0].texture[0]);
		}
		
		/*
		glBegin(GL_TRIANGLE_FAN);
		v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
		for (j = 0; j < p->numverts; j++, v+= VERTEXSIZE)
		{
			glTexCoord2fv(&v[3]);
			glVertex3fv(&v[0]);
		}
		glEnd();
		*/

		glBegin(GL_TRIANGLES);
		for (j=0; j<surf->polys->numindecies; j++) {
			v = (float *)(&cl.worldmodel->userVerts[surf->polys->indecies[j]]);
			glTexCoord2fv(&v[3]);
			glVertex3fv(&v[0]);
		}
		glEnd();
    }
    if (!cull)
		glEnable(GL_CULL_FACE);
}

void Generic_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs,
								  int numSurfaces,const transform_t *tr, const lightobject_t *lo)
{
    glDepthMask (0);
	
	//attenuate with projected 2D attenuation map
    GL_DrawAlpha();
    GL_EnableGenericAttShader();
    Generic_sendSurfacesATT(surfs, numSurfaces);
    GL_DisableGenericAttShader();	

    // modulate diffuse bump (n dot l) to alpha
    GL_ModulateAlphaDrawAlpha();
    GL_EnableGenericDiffuseShader();
    Generic_sendSurfacesDiffuse(surfs, numSurfaces);
    GL_DisableGenericDiffuseShader ();

	//Do fake specular
	if (sh_fakespecular.value) {

		//This is so fake...
		//-it gets attenuated with the diffuse dot product and the light attenuation
		//-The specular gets caclulated per vertex
		//-But it remotely looks like specular at the cost of one extra pass

		float black[4] = {0.0, 0.0, 0.0, 1.0};
		float white[4] = {1.0, 1.0, 1.0, 1.0};
		float pos[4];

		GL_ModulateAlphaDrawColor();
		glColorMask(true, true, true, false);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		VectorCopy(lo->objectorigin, pos);
		pos[3] = 1.0;

		glLightfv(GL_LIGHT0, GL_POSITION, pos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, black);
		glLightfv(GL_LIGHT0, GL_AMBIENT, black);
		glLightfv(GL_LIGHT0, GL_SPECULAR, &currentshadowlight->color[0]);

		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16);

		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

		glEnable(GL_COLOR_MATERIAL);
		glColor4fv(white);

		GL_EnableGenericColorShader (true);
		Generic_sendSurfacesWV(surfs, numSurfaces, true);
		GL_DisableGenericColorShader (true);

		glDisable(GL_LIGHTING);
	}

    //	Then modulate Color map and light filter with alpha and add to color
	GL_ModulateAlphaDrawColor();
    glColor3fv(&currentshadowlight->baseColor[0]);
    if ( currentshadowlight->filtercube )
    {
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		
		GL_SetupCubeMapMatrix(tr);
		GL_EnableGenericColorShader (false);
		Generic_sendSurfacesWV(surfs, numSurfaces, false);
		GL_DisableGenericColorShader (false);
		
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
    }
    else
    {
		// Just alpha * light color
        GL_SelectTexture(GL_TEXTURE1_ARB);
		
        glDisable(GL_TEXTURE_2D);
		
        GL_SelectTexture(GL_TEXTURE0_ARB);
		
        glEnable(GL_TEXTURE_2D);
		
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		
        glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
		
        glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
		
        glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		

		//glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		Generic_sendSurfacesTex(surfs, numSurfaces);
    }

    GL_DrawColor();
    glColor3f (1,1,1);
    glDisable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask (1);
}


void Generic_freeDriver(void)
{
    //nothing here...
}



void BUMP_InitGeneric(void)
{
    if ( gl_cardtype != GENERIC && gl_cardtype != GEFORCE )
		return;
	
	Cvar_RegisterVariable(&sh_fakespecular);
    //bind the correct stuff to the bump mapping driver
    gl_bumpdriver.drawSurfaceListBase = Generic_drawSurfaceListBase;
    gl_bumpdriver.drawSurfaceListBump = Generic_drawSurfaceListBump;
    gl_bumpdriver.drawTriangleListBase = Generic_drawTriangleListBase;
    gl_bumpdriver.drawTriangleListBump = Generic_drawTriangleListBump;
    gl_bumpdriver.freeDriver = Generic_freeDriver;
}
