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

Shader utility routines

*************************/

void Generic_SetupTcMod(tcmod_t *tc)
{
    switch (tc->type)
    {
    case TCMOD_ROTATE:
	glTranslatef(0.5,0.5,0.0);
	glRotatef(cl.time * tc->params[0],0,0,1);
	glTranslatef(-0.5, -0.5, 0.0);
	break;
    case TCMOD_SCROLL:
	glTranslatef(cl.time * tc->params[0], cl.time * tc->params[1], 0.0);
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

void Generic_drawTriangleListBump (const vertexdef_t *verts, int *indecies,
			       int numIndecies, shader_t *shader,
			       const transform_t *tr)
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

        glBegin(GL_TRIANGLE_FAN);
	v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
	for (j = 0; j < p->numverts; j++, v+= VERTEXSIZE)
	{
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

	glBegin(GL_TRIANGLE_FAN);
	v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
	for (j = 0; j < p->numverts; j++, v+= VERTEXSIZE)
	{
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
    float tu, tv;
    vec3_t *s, *t, nearPt, nearToVert;
    float dist, scale;
    mplane_t *splitplane;
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
	v = (float *)(&globalVertexTable[surf->polys->firstvertex]);

	// Calculate attenuation
	splitplane = surf->plane;
	s = (vec3_t *)&surf->tangent;
	t = (vec3_t *)&surf->binormal;
	dist = DotProduct (currentshadowlight->origin, splitplane->normal) - splitplane->dist;
	dist = abs(dist);
	ProjectPlane (currentshadowlight->origin, (*s), (*t), nearPt);

        // FIXME! BBLIGHTS
//	scale = 1 /((2 * currentshadowlight->radius) - dist);
	scale = 1 /((2 * 400) - dist);

	glBegin(GL_TRIANGLE_FAN);
	for (j = 0; j < p->numverts; j++, v+= VERTEXSIZE)
	{
	    VectorSubtract (v, nearPt, nearToVert);

	    // Get our texture coordinates, transform into tangent plane
	    tu = DotProduct (nearToVert, (*s)) * scale + 0.5;
	    tv = DotProduct (nearToVert, (*t)) * scale + 0.5;
	    
	    glTexCoord2f(tu, tv);
	    glVertex3fv(&v[0]);
	}
	glEnd();
    }
    if (!cull)
	glEnable(GL_CULL_FACE);
}

void Generic_sendSurfacesWV(msurface_t **surfs, int numSurfaces)
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
 
	glBegin(GL_TRIANGLE_FAN);
	v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
	for (j = 0; j < p->numverts; j++, v+= VERTEXSIZE)
	{
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

	glBegin(GL_TRIANGLE_FAN);
	v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
	for (j = 0; j < p->numverts; j++, v+= VERTEXSIZE)
	{
	    glTexCoord2fv(&v[3]);
	    glVertex3fv(&v[0]);
	}
	glEnd();
    }
    if (!cull)
	glEnable(GL_CULL_FACE);
}

void Generic_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs,
			      int numSurfaces,const transform_t *tr)
{
    glDepthMask (0);

    // First draw diffuse bump (n dot l) to alpha
    GL_DrawAlpha();
    GL_EnableDiffuseShader();
    Generic_sendSurfacesDiffuse(surfs, numSurfaces);
    GL_DisableDiffuseShader ();

    // Then attenuate with projected 2D attenuation map
    GL_ModulateAlphaDrawAlpha();
    GL_EnableAttShader();
    Generic_sendSurfacesATT(surfs, numSurfaces);
    GL_DisableAttShader();

    //	Then modulate Color map and light filter with alpha and add to color
    GL_ModulateAlphaDrawColor();
    glColor3fv(&currentshadowlight->baseColor[0]);
    if ( currentshadowlight->filtercube )
    {
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();

	GL_SetupCubeMapMatrix(tr);
	GL_EnableColorShader (false);
	Generic_sendSurfacesWV(surfs, numSurfaces);
	GL_DisableColorShader (false);
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

	Generic_sendSurfacesTex(surfs, numSurfaces);
    }
    GL_DrawColor();
    glColor3f (1,1,1);
    glDisable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask (1);
}


typedef struct allocchain_s
{
    struct allocchain_s* next;
    char data[1];//variable sized
} allocchain_t;

static allocchain_t* allocChain = NULL;

void* Generic_getDriverMem(size_t size, drivermem_t hint)
{
    allocchain_t *r = (allocchain_t *)malloc(size+sizeof(void *));
    r->next = allocChain;
    allocChain = r;
    return &r->data[0];
}

void Generic_freeAllDriverMem(void)
{
    allocchain_t *r = allocChain;
    allocchain_t *next;

    while (r)
    {
	next = r->next;
	free(r);
	r = next;
    }
}

void Generic_freeDriver(void)
{
    //nothing here...
}



void BUMP_InitGeneric(void)
{
    if ( gl_cardtype != GENERIC && gl_cardtype != GEFORCE )
	return;

    //bind the correct stuff to the bump mapping driver
    gl_bumpdriver.drawSurfaceListBase = Generic_drawSurfaceListBase;
    gl_bumpdriver.drawSurfaceListBump = Generic_drawSurfaceListBump;
    gl_bumpdriver.drawTriangleListBase = Generic_drawTriangleListBase;
    gl_bumpdriver.drawTriangleListBump = Generic_drawTriangleListBump;
    gl_bumpdriver.getDriverMem = Generic_getDriverMem;
    gl_bumpdriver.freeAllDriverMem = Generic_freeAllDriverMem;
    gl_bumpdriver.freeDriver = Generic_freeDriver;
}
