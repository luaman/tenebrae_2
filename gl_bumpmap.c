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

	This is mostly leftover code from tenebrae1 the bumpmaps are done in the drivers now.
*/

#include "quakedef.h"

void GL_EnableColorShader (qboolean specular) {
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	

	//primary color = light color
	//tu0 = light filter cube map 
	//tu1 = material color map
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

void GL_DisableColorShader (qboolean specular) {
	if (!specular) {
		GL_DisableMultitexture();
	} else {
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		GL_DisableMultitexture();
	}

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);
}

/*
Draw incoming fragment to destination alpha.

Note: A fragment in the next comment means a pixel that will be drawn on the screen
(gl has some specific definition of it)
*/
void GL_DrawAlpha() {
	glColorMask(false, false, false, true);
	glDisable(GL_BLEND);
}

/*
Draw incoming fragment by modulating it with destination alpha
*/
void GL_ModulateAlphaDrawAlpha() {
	glColorMask(false, false, false, true);
	glBlendFunc (GL_DST_ALPHA, GL_ZERO);
	glEnable (GL_BLEND);
}

/*
Draw incoming fragment by adding it to destination alpha
*/
void GL_AddAlphaDrawAlpha() {
	glColorMask(false, false, false, true);
	glBlendFunc (GL_ONE, GL_ONE);
	glEnable (GL_BLEND);
}

/*
Draw incoming fragment by modulation it with destination alpha
*/
void GL_ModulateAlphaDrawColor() {
	glColorMask(true, true, true, true);
	glBlendFunc (GL_DST_ALPHA, GL_ONE);
	glEnable (GL_BLEND);
}

/*
Draw incoming alpha squared to destination alpha
*/
void GL_DrawSquareAlpha() {
	glColorMask(false, false, false, true);
	glBlendFunc (GL_SRC_ALPHA, GL_ZERO);
	glEnable (GL_BLEND);
}

/*
Square the destination alpha
*/
void GL_SquareAlpha() {
	glColorMask(false, false, false, true);
	glBlendFunc (GL_ZERO, GL_DST_ALPHA);
	glEnable (GL_BLEND);
}

/*
Add incoming fragment to destination color
*/
void GL_AddColor() {
	glColorMask(true, true, true, true);
	glBlendFunc (GL_ONE, GL_ONE);
	glEnable (GL_BLEND);
}

/*
Ovewrite destination color with incoming fragment
*/
void GL_DrawColor() {
	glColorMask(true, true, true, true);
	glDisable (GL_BLEND);
}

/*
=================
R_WorldToObjectMatrix

Returns a world to object coordinate transformation matrix.
This is crap and I know it ;)
The problem is that quake does strange stuff with its angles
(look at R_RotateForEntity) so inverting the matrix will
certainly give the desired result.
Why I use the gl matrix? Well quake doesn't have build in matix
routines an I don't have any lying around in c (Lot's in Pascal
of course i'm a Delphi man)

=================
*/

void R_WorldToObjectMatrix(entity_t *e, matrix_4x4 result)
{
	matrix_4x4 world;

	glPushMatrix();
	glLoadIdentity();
	R_RotateForEntity (e);
	glGetFloatv (GL_MODELVIEW_MATRIX, &world[0][0]);
	glPopMatrix();
	MatrixAffineInverse(world,result);
}

/*
=============
GL_SetupCubeMapMatrix

Loads the current matrix with a tranformation used for light filters
-Put object space into world space
-Then put this worldspace into lightspace
=============
*/
void GL_SetupCubeMapMatrix(const transform_t *tr) {

	glRotatef (-currentshadowlight->angles[0],  1, 0, 0);
    glRotatef (-currentshadowlight->angles[1],  0, 1, 0);
    glRotatef (-currentshadowlight->angles[2],  0, 0, 1);

	glTranslatef(-currentshadowlight->origin[0],
				 -currentshadowlight->origin[1],
				 -currentshadowlight->origin[2]);

	glTranslatef (tr->origin[0],  tr->origin[1],  tr->origin[2]);

    glRotatef (tr->angles[1],  0, 0, 1);
    glRotatef (-tr->angles[0], 0, 1, 0);
    glRotatef (tr->angles[2],  1, 0, 0);

}

/*
=============
GL_SetupAttenMatrix

Loads the current matrix with a tranformation used for light filters
-Put object space into world space
-Then put this worldspace into lightspace
	(Ignore light angles as the boxes don't rotate with the light)
=============
*/
void GL_SetupAttenMatrix(const transform_t *tr) {

	glTranslatef(-currentshadowlight->origin[0],
				 -currentshadowlight->origin[1],
				 -currentshadowlight->origin[2]);

	glTranslatef (tr->origin[0],  tr->origin[1],  tr->origin[2]);

    glRotatef (tr->angles[1],  0, 0, 1);
    glRotatef (-tr->angles[0], 0, 1, 0);
    glRotatef (tr->angles[2],  1, 0, 0);

}


void PrintScreenPos(vec3_t v) {

	double	Dproject_matrix[16];
	double	Dworld_matrix[16];
	GLint Iviewport[4];		// <AWE> changed from int to GLint.
	double px, py, pz;

	glGetDoublev (GL_MODELVIEW_MATRIX, Dworld_matrix);
	glGetDoublev (GL_PROJECTION_MATRIX, Dproject_matrix);
	glGetIntegerv (GL_VIEWPORT, Iviewport);
	gluProject(v[0], v[1], v[2],
				   Dworld_matrix, Dproject_matrix, Iviewport, &px, &py, &pz);
	Con_Printf("Pos: %f %f %f\n", px, py, pz);
}

/*
=============
R_DrawBrushObjectLight

Idea: Creepy object oriented programming by using function pointers.
Function: Puts the light into object space, adapts the world->eye matrix
and calls BrushGeoSender if all that has been done.
Cleans up afterwards so nothing has changed.
=============
*/
void R_DrawBrushObjectLight(entity_t *e,void (*BrushGeoSender) (entity_t *e)) {

	model_t		*clmodel;

	currententity = e;
	currenttexture = -1;

	clmodel = e->model;

	glPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
	e->angles[0] = -e->angles[0];	// stupid quake bug

	BrushGeoSender(e);

	glPopMatrix ();
}

/*
=================
R_DrawAliasObjectLight

Same as R_DrawBrushObjectLight but with alias models

=================
*/
void R_DrawAliasObjectLight(entity_t *e,void (*AliasGeoSender) (aliashdr_t *paliashdr, aliasframeinstant_t* instant))
{
	aliashdr_t	*paliashdr;
	alias3data_t    *data;
	vec3_t		oldlightpos, oldvieworg;
	aliasframeinstant_t *aliasframeinstant;
	int i,maxnumsurf;

	currententity = e;

	glPushMatrix ();
	R_RotateForEntity (e);

	//
	// locate the proper data
	//
	data = (alias3data_t *)Mod_Extradata (e->model);
	maxnumsurf = data->numSurfaces;
	aliasframeinstant = e->aliasframeinstant;
	for (i=0;i<maxnumsurf;++i){
		paliashdr = (aliashdr_t *)((char*)data + data->ofsSurfaces[i]);
		if (!aliasframeinstant) {
			glPopMatrix();
			Con_Printf("R_DrawAliasObjectLight: missing instant for ent %s\n", e->model->name);
			return;
		}

		if (aliasframeinstant->shadowonly) continue;

		if ((e->frame >= paliashdr->numframes) || (e->frame < 0))
		{
			glPopMatrix();
			return;
		}

		//Draw it!
		AliasGeoSender(paliashdr,aliasframeinstant);
		aliasframeinstant = aliasframeinstant->_next;
	} /* for paliashdr */

	glPopMatrix();
}

void SetColorForAtten(vec3_t point) {
	vec3_t dist;
	float colorscale;

	VectorSubtract(point, currentshadowlight->origin, dist);
	colorscale = 1 - (Length(dist) / currentshadowlight->radius);

	glColor3f(currentshadowlight->color[0]*colorscale,
		currentshadowlight->color[1]*colorscale,
		currentshadowlight->color[2]*colorscale);

}


/*
=================
R_DrawSpriteModelWV

Draw a sprite texture with as tex0 coordinates the world space position of it's vertexes.
tex1 is the sprites texture coordinates.

=================
*/
void R_DrawSpriteModelLight (entity_t *e)
{
	mspriteframe_t	*frame;
	msprite_t		*psprite;
	float		*up, *right;
	vec3_t		point;

	frame = R_GetSpriteFrame (e);
	psprite = currententity->model->cache.data;

	up = vup;
	right = vright;

	if (frame->shader->flags & SURF_NODRAW) return;

	//We don't do full bumpapping on sprites only additive cube*attenuation*colormap
	if (!frame->shader->numcolorstages) return;

	GL_BindAdvanced(frame->shader->colorstages[0].texture[0]);

	glBegin (GL_QUADS);

	qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	SetColorForAtten(point);
	glVertex3fv (point);

	qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	SetColorForAtten(point);
	glVertex3fv (point);

	qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, 1, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	SetColorForAtten(point);
	glVertex3fv (point);

	qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, 1, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	SetColorForAtten(point);
	glVertex3fv (point);

	glEnd ();
}

void R_DrawSpriteModelLightWV (entity_t *e)
{
	mspriteframe_t	*frame;
	msprite_t		*psprite;
	float		*up, *right;
	vec3_t		point;

	frame = R_GetSpriteFrame (e);
	psprite = currententity->model->cache.data;

	up = vup;
	right = vright;

	if (frame->shader->flags & SURF_NODRAW) return;

	//We don't do full bumpapping on sprites only additive cube*attenuation*colormap
	if (!frame->shader->numcolorstages) return;
	GL_BindAdvanced(frame->shader->colorstages[0].texture[0]);

	glBegin (GL_QUADS);

	qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	glTexCoord3fv(point);
	glVertex3fv (point);

	qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	glTexCoord3fv(point);
	SetColorForAtten(point);
	glVertex3fv (point);

	qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	glTexCoord3fv(point);
	SetColorForAtten(point);
	glVertex3fv (point);

	qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	glTexCoord3fv(point);
	SetColorForAtten(point);
	glVertex3fv (point);

	glEnd ();
}


