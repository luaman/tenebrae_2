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

Same as gl_bumpmap.c but geforce3&4 optimized 
These routines require 4 texture units an some need nvidia shaders

Most lights reqire 2 passes this way
1 diffuse
2 specular

If a light has a cubemap filter it requires 3 passes
1 attenuation
2 diffuse
3 specular
*/

#include "quakedef.h"
#include "nvparse/nvparse.h"

//<AWE> "diffuse_program_object" has to be defined static. Otherwise nameclash with "gl_bumpradeon.c".
static GLuint diffuse_program_object;
static GLuint specularalias_program_object; //He he nice name to type a lot

/*
Pixel shader for diffuse bump mapping does diffuse bumpmapping with norm cube, self shadowing & dist attent in
1 pass (thanx to the 4 texture units on a gf4)
*/
void GL_EnableDiffuseShaderGF3(const transform_t *tr, vec3_t lightOrig) {

	float invrad = 1/currentshadowlight->radius;

	//tex 0 = normal map
	//tex 1 = nomalization cube map (tangent space light vector)
	//tex 2 = color map
	//tex 3 = (attenuation or light filter, depends on light settings but the actual
	//			register combiner setup does not change only the bound texture)

	GL_SelectTexture(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

	GL_SelectTexture(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);

	GL_SelectTexture(GL_TEXTURE3_ARB);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	if (currentshadowlight->filtercube) {
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->filtercube);
		GL_SetupCubeMapMatrix(tr);
	} else {
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, atten3d_texture_object);

		glTranslatef(0.5,0.5,0.5);
		glScalef(0.5,0.5,0.5);
		glScalef(invrad, invrad, invrad);
		glTranslatef(-lightOrig[0], -lightOrig[1], -lightOrig[2]);
	}

	GL_SelectTexture(GL_TEXTURE0_ARB);

	//combiner0 RGB: calculate 
	//	(normal map = A) dot (norm cubemap = B) save in Spare0 RGB
	//	(color map = C) mul (light filter = D) save in Spare1 RGB
	qglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_EXPAND_NORMAL_NV, GL_RGB);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV, GL_RGB);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE2_ARB, GL_UNSIGNED_IDENTITY_NV , GL_RGB);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV, GL_TEXTURE3_ARB, GL_UNSIGNED_IDENTITY_NV , GL_RGB);
	qglCombinerOutputNV(GL_COMBINER0_NV, GL_RGB, GL_SPARE0_NV, GL_SPARE1_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_TRUE, GL_FALSE, GL_FALSE);


	//combiner0 Alpha: store 8*expand(tang space light vect z comp) into Spare0 Alpha (this is the selfshadow term)
	qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV, GL_BLUE);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_BLUE);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
	qglCombinerOutputNV(GL_COMBINER0_NV, GL_ALPHA, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

	//Only if the light is not white we use a second combiner
	//this is when the light is at its full brightness (for flickering lights)
	//and doesn't have any color (other than white)
	
	if ((currentshadowlight->color[0] != 1) || (currentshadowlight->color[1] != 1) || (currentshadowlight->color[2] != 1)) {
		qglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 2);

		qglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
		qglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_B_NV, GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
		qglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_C_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV , GL_RGB);
		qglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV , GL_RGB);
		qglCombinerOutputNV(GL_COMBINER1_NV, GL_RGB, GL_SPARE1_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);
		//alpha out = nothing
		qglCombinerOutputNV(GL_COMBINER1_NV, GL_ALPHA, GL_DISCARD_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);
	} else {
		qglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);
	}
	
	//final combiner: final RGB = (Spare 0 Alpha) * ( (Spare 0 RGB) * (Spare 1 RGB) )
	qglFinalCombinerInputNV(GL_VARIABLE_A_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
	//qglFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_TEXTURE2_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_E_TIMES_F_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	//qglFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_C_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_E_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_F_NV, GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);

	//final cominer alpha doesn't really matter we use A dot B
	qglFinalCombinerInputNV(GL_VARIABLE_G_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_BLUE);

	glEnable(GL_REGISTER_COMBINERS_NV);

    // Enable the vertex program.
    //qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, diffuse_program_object );
    //glEnable( GL_VERTEX_PROGRAM_ARB );
	qglBindProgramNV( GL_VERTEX_PROGRAM_NV, diffuse_program_object );
	qglProgramParameter4fNV( GL_VERTEX_PROGRAM_NV, 24, currentshadowlight->origin[0],
		 currentshadowlight->origin[1],  currentshadowlight->origin[2], 1.0);
	qglProgramParameter4fNV( GL_VERTEX_PROGRAM_NV, 25, r_refdef.vieworg[0],
		 r_refdef.vieworg[1],  r_refdef.vieworg[2], 1.0);
    glEnable( GL_VERTEX_PROGRAM_NV );

}

void GL_DisableDiffuseShaderGF3() {

	qglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

	//tex 0 = normal map
	//tex 1 = nomalization cube map (tangent space light vector)
	//tex 2 = color map
	//tex 3 = (attenuation or light filter, depends on light settings)

	GL_SelectTexture(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	GL_SelectTexture(GL_TEXTURE2_ARB);
	glDisable(GL_TEXTURE_2D);

	GL_SelectTexture(GL_TEXTURE3_ARB);
	if (currentshadowlight->filtercube) {
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
	} else {
		glDisable(GL_TEXTURE_3D);
	}
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	GL_SelectTexture(GL_TEXTURE0_ARB);
	glDisable(GL_REGISTER_COMBINERS_NV);

//    glDisable( GL_VERTEX_PROGRAM_ARB );
    glDisable( GL_VERTEX_PROGRAM_NV );
}

void GL_EnableSpecularShaderGF3(const transform_t *tr, vec3_t lightOrig, qboolean alias) {

	vec3_t scaler = {0.5f, 0.5f, 0.5f};
	float invrad = 1/currentshadowlight->radius;

	//tex 0 = normal map
	//tex 1 = nomalization cube map (tangent space half angle)
	//tex 2 = color map
	//tex 3 = (attenuation or light filter, depends on light settings but the actual
	//			register combiner setup does not change only the bound texture)

	GL_SelectTexture(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

	GL_SelectTexture(GL_TEXTURE2_ARB);

        glEnable(GL_TEXTURE_2D);

	GL_SelectTexture(GL_TEXTURE3_ARB);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	if (currentshadowlight->filtercube) {
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->filtercube);
		GL_SetupCubeMapMatrix(tr);
	} else {
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, atten3d_texture_object);

		glTranslatef(0.5,0.5,0.5);
		glScalef(0.5,0.5,0.5);
		glScalef(invrad, invrad, invrad);
		glTranslatef(-lightOrig[0], -lightOrig[1], -lightOrig[2]);


	}

	GL_SelectTexture(GL_TEXTURE0_ARB);

	qglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 4);
	qglCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, &scaler[0]);

	//combiner0 RGB: calculate 
	//	(normal map = A) dot (norm cubemap = B) save in Spare0 RGB
	//	(gloss map = C) mul (light filter = D) save in Spare1 RGB
	qglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_EXPAND_NORMAL_NV, GL_RGB);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV, GL_RGB);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV , GL_ALPHA);
	qglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV, GL_TEXTURE3_ARB, GL_UNSIGNED_IDENTITY_NV , GL_RGB);
	qglCombinerOutputNV(GL_COMBINER0_NV, GL_RGB, GL_SPARE0_NV, GL_SPARE1_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_TRUE, GL_FALSE, GL_FALSE);

	//combiner0 Alpha: store 8*expand(tang space light vect z comp) into Spare1 Alpha (this is the selfshadow term)
	if (alias) {
		qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_SECONDARY_COLOR_NV, GL_EXPAND_NORMAL_NV, GL_BLUE);
		qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
		qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV, GL_SECONDARY_COLOR_NV, GL_EXPAND_NORMAL_NV, GL_BLUE);
		qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
	} else {
		qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_EXPAND_NORMAL_NV, GL_BLUE);
		qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
		qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV, GL_TEXTURE0_ARB, GL_EXPAND_NORMAL_NV, GL_BLUE);
		qglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
	}

	qglCombinerOutputNV(GL_COMBINER0_NV, GL_ALPHA, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE1_NV, GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

	//rgb = multipy light with color
	qglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_B_NV, GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_C_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV , GL_RGB);
	qglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV , GL_RGB);
	qglCombinerOutputNV(GL_COMBINER1_NV, GL_RGB, GL_SPARE1_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

	//combiner1 Alpha: calculate 2*((N'dotH)^2 - 0.5f) -> store in Spare0 Alpha ("raise" to an exponent)
	qglCombinerInputNV(GL_COMBINER1_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_BLUE);
	qglCombinerInputNV(GL_COMBINER1_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_BLUE);
	qglCombinerInputNV(GL_COMBINER1_NV, GL_ALPHA, GL_VARIABLE_C_NV, GL_CONSTANT_COLOR0_NV, GL_SIGNED_NEGATE_NV, GL_BLUE);
	qglCombinerInputNV(GL_COMBINER1_NV, GL_ALPHA, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
	qglCombinerOutputNV(GL_COMBINER1_NV, GL_ALPHA, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV, GL_SCALE_BY_TWO_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

	//combiner2 Alpha: Raise specular further
	qglCombinerInputNV(GL_COMBINER2_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
	qglCombinerInputNV(GL_COMBINER2_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
	qglCombinerOutputNV(GL_COMBINER2_NV, GL_ALPHA, GL_SPARE0_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);
	//combiner2 rgb: Do nothing
//	qglCombinerInputNV(GL_COMBINER2_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
//	qglCombinerInputNV(GL_COMBINER2_NV, GL_RGB, GL_VARIABLE_B_NV, GL_TEXTURE2_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
//	qglCombinerOutputNV(GL_COMBINER2_NV, GL_RGB, GL_SPARE1_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);
	qglCombinerOutputNV(GL_COMBINER2_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

	//combiner3 Alpha: Raise specular further
	qglCombinerInputNV(GL_COMBINER3_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
	qglCombinerInputNV(GL_COMBINER3_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
	qglCombinerOutputNV(GL_COMBINER3_NV, GL_ALPHA, GL_SPARE0_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);
	//combiner3 rgb: Do nothing
//	qglCombinerOutputNV(GL_COMBINER2_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);
	qglCombinerInputNV(GL_COMBINER3_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
	qglCombinerInputNV(GL_COMBINER3_NV, GL_RGB, GL_VARIABLE_B_NV, GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglCombinerInputNV(GL_COMBINER3_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV , GL_ALPHA);
	qglCombinerInputNV(GL_COMBINER3_NV, GL_RGB, GL_VARIABLE_D_NV, GL_TEXTURE3_ARB, GL_UNSIGNED_IDENTITY_NV , GL_RGB);
	qglCombinerOutputNV(GL_COMBINER3_NV, GL_RGB, GL_SPARE1_NV, GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);


	//final combiner: final RGB = (Spare 0 Alpha) * ( (Spare 0 RGB) * (Spare 1 RGB) )
	qglFinalCombinerInputNV(GL_VARIABLE_A_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
	qglFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_E_TIMES_F_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_C_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_E_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	qglFinalCombinerInputNV(GL_VARIABLE_F_NV, GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);

	//final cominer alpha doesn't really matter we use A dot B
	qglFinalCombinerInputNV(GL_VARIABLE_G_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_BLUE);

	glEnable(GL_REGISTER_COMBINERS_NV);


    // Enable the vertex program.
//    qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, diffuse_program_object );
//    glEnable( GL_VERTEX_PROGRAM_ARB );
	//if (alias)
		qglBindProgramNV( GL_VERTEX_PROGRAM_NV, specularalias_program_object );
	//else
	//	qglBindProgramNV( GL_VERTEX_PROGRAM_NV, diffuse_program_object );

	qglProgramParameter4fNV( GL_VERTEX_PROGRAM_NV, 24, currentshadowlight->origin[0],
		 currentshadowlight->origin[1],  currentshadowlight->origin[2], 1.0);
	qglProgramParameter4fNV( GL_VERTEX_PROGRAM_NV, 25,r_refdef.vieworg[0], r_refdef.vieworg[1],  r_refdef.vieworg[2], 1.0);

    glEnable( GL_VERTEX_PROGRAM_NV );

}

/*
GL_DisableSpecularShaderGF3() ??
Same as GL_DisableDiffuseShaderGF3()
*/

void GL_EnableAttentShaderGF3(vec3_t lightOrig) {

	float invrad = 1/currentshadowlight->radius;
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0.5,0.5,0.5);
	glScalef(0.5,0.5,0.5);
	glScalef(invrad, invrad, invrad);
	glTranslatef(-lightOrig[0],
				 -lightOrig[1],
				 -lightOrig[2]);
	
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, atten3d_texture_object);
}

void GL_DisableAttentShaderGF3() {

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_TEXTURE_3D);
	glEnable(GL_TEXTURE_2D);
}

/*
typedef struct {

	float *vertices;
	int vertexstride;

	float *texcoords;
	int texcoordstride;

	float *lightmapcoords;
	int lightmapstride;

	float *tangents;
	int tangentstride;

	float *binormals;
	int binormalstride;

	float *normals;
	int normalstride;

	unsigned char *colors;
	int colorstride;

} vertexdef_t;

typedef struct {
	//system code
	void (*initDriver) (void);
	void (*freeDriver) (void);
	void *(*getDriverMem) (size_t size, drivermem_t hint);
	void (*freeAllDriverMem) (void);
	
	//FIXME: Do we need fence like support?

	//drawing code
	void (*drawTriangleListBase) (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader);
	void (*drawTriangleListBump) (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader);
	void (*drawTriangleListSys) (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader);
	void (*drawSurfaceListBase) (msurface_t **surfs, int numSurfaces);
	void (*drawSurfaceListBump) (msurface_t **surfs, int numSurfaces);

} bumpdriver_t;

*/

/************************

Shader utitlity routines

*************************/

void GF3_SetupTcMod(tcmod_t *tc) {

	switch (tc->type) {
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


void GF3_SetupSimpleStage(stage_t *s) {
	tcmod_t *tc;
	int i;

	if (s->type != STAGE_SIMPLE) {
		Con_Printf("Non simple stage, in simple stage list");
		return;
	}

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();

	for (i=0; i<s->numtcmods; i++) {
		GF3_SetupTcMod(&s->tcmods[i]);	
	}

	if (s->src_blend > -1) {
		glBlendFunc(s->src_blend, s->dst_blend);
		glEnable(GL_BLEND);
	}

	if (s->alphatresh > 0) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, s->alphatresh);
	}

	if ((s->numtextures > 0) && (s->texture[0]))
		GL_Bind(s->texture[0]->texnum);
}

/************************

Generic triangle list routines

*************************/

void FormatError () {
	Sys_Error("Invalid vertexdef_t\n");
}

void GF3_sendTriangleListWV(const vertexdef_t *verts, int *indecies, int numIndecies) {

	glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	glTexCoordPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//draw them
	glDrawElements(GL_TRIANGLES, numIndecies, GL_UNSIGNED_INT, indecies);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void GF3_sendTriangleListTA(const vertexdef_t *verts, int *indecies, int numIndecies) {

	glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

	if (!verts->texcoords) FormatError();
	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (!verts->tangents) FormatError();
	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(3, GL_FLOAT, verts->tangentstride, verts->tangents);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (!verts->binormals) FormatError();
	qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	glTexCoordPointer(3, GL_FLOAT, verts->binormalstride, verts->binormals);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (!verts->normals) FormatError();
	qglClientActiveTextureARB(GL_TEXTURE3_ARB);
	glTexCoordPointer(3, GL_FLOAT, verts->normalstride, verts->normals);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//draw them
	glDrawElements(GL_TRIANGLES, numIndecies, GL_UNSIGNED_INT, indecies);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	GL_SelectTexture(GL_TEXTURE0_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

}

void GF3_drawTriangleListBump (const vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader, const transform_t *tr) {

	if (currentshadowlight->filtercube) {
		//draw attent into dest alpha
		GL_DrawAlpha();
		GL_EnableAttentShaderGF3(currentshadowlight->origin);
		GF3_sendTriangleListWV(verts,indecies,numIndecies);
		GL_DisableAttentShaderGF3();
		GL_ModulateAlphaDrawColor();
	} else {
		GL_AddColor();
	}
	glColor3fv(&currentshadowlight->color[0]);

	GL_EnableSpecularShaderGF3(tr,currentshadowlight->origin,true);
	//bind the correct texture
	GL_SelectTexture(GL_TEXTURE0_ARB);
		if (shader->numbumpstages > 0)
			GL_Bind(shader->bumpstages[0].texture[0]->texnum);
	GL_SelectTexture(GL_TEXTURE2_ARB);
		if (shader->numcolorstages > 0)
			GL_Bind(shader->colorstages[0].texture[0]->texnum);

	GF3_sendTriangleListTA(verts,indecies,numIndecies);
	GL_DisableDiffuseShaderGF3();

	GL_EnableDiffuseShaderGF3(tr,currentshadowlight->origin);
	//bind the correct texture
	GL_SelectTexture(GL_TEXTURE0_ARB);
		if (shader->numbumpstages > 0)
			GL_Bind(shader->bumpstages[0].texture[0]->texnum);
	GL_SelectTexture(GL_TEXTURE2_ARB);
		if (shader->numcolorstages > 0)
			GL_Bind(shader->colorstages[0].texture[0]->texnum);

	GF3_sendTriangleListTA(verts,indecies,numIndecies);
	GL_DisableDiffuseShaderGF3();
}

void GF3_drawTriangleListBase (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader) {

	int i;

	glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (!shader->cull) {
		glDisable(GL_CULL_FACE);
		//Con_Printf("Cullstuff %s\n",shader->name);
	}

	for (i=0; i<shader->numstages; i++) {
		GF3_SetupSimpleStage(&shader->stages[i]);
		glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
		glPopMatrix();
	}
	glMatrixMode(GL_MODELVIEW);

	if (verts->colors && (shader->flags & SURF_PPLIGHT)) {

		glColorPointer(3, GL_UNSIGNED_BYTE, verts->colorstride, verts->colors);
		glEnableClientState(GL_COLOR_ARRAY);
		glShadeModel(GL_SMOOTH);

		if (shader->numstages && shader->numcolorstages)
			if (shader->colorstages[0].src_blend >= 0) {
				glEnable(GL_BLEND);
				glBlendFunc(shader->colorstages[0].src_blend, shader->colorstages[0].dst_blend);
			} else {
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
			}
		else 
			glDisable(GL_BLEND);

		if (shader->numcolorstages) {
			if (shader->colorstages[0].numtextures)
				GL_Bind(shader->colorstages[0].texture[0]->texnum);

			if (shader->colorstages[0].alphatresh > 0) {
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
			}	
		}

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);

		glDisableClientState(GL_COLOR_ARRAY);
	} else if (shader->flags & SURF_PPLIGHT) {
		glColor3f(0,0,0);
		glDisable(GL_TEXTURE_2D);
		glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
		glEnable(GL_TEXTURE_2D);
	}

	if (!shader->cull) {
		glEnable(GL_CULL_FACE);
		//Con_Printf("Cullstuff %s\n",shader->name);
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
}

/*************************

Generic world surfaces routines

**************************/

void GF3_sendSurfacesBase(msurface_t **surfs, int numSurfaces, qboolean bindLightmap) {
	int i;
	glpoly_t *p;
	msurface_t *surf;

	for (i=0; i<numSurfaces; i++) {
		surf = surfs[i];
		if (surf->visframe != r_framecount)
			continue;
		p = surf->polys;
		if (bindLightmap) {
			if (surf->lightmaptexturenum < 0)
				continue;
			GL_Bind(lightmap_textures+surf->lightmaptexturenum);
		}
		glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT, &p->indecies[0]);
	}
}

void GF3_drawSurfaceListBase (vertexdef_t *verts, msurface_t **surfs, int numSurfaces, shader_t *shader) {
	
	int i;

	glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glColor3ub(255,255,255);

	if (!shader->cull) {
		glDisable(GL_CULL_FACE);
		//Con_Printf("Cullstuff %s\n",shader->name);
	}

	for (i=0; i<shader->numstages; i++) {
		GF3_SetupSimpleStage(&shader->stages[i]);
		GF3_sendSurfacesBase(surfs, numSurfaces, false);
		glPopMatrix();
	}

	if (verts->lightmapcoords && (shader->flags & SURF_PPLIGHT)) {
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		glTexCoordPointer(2, GL_FLOAT, verts->lightmapstride, verts->lightmapcoords);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		if (shader->numstages && shader->numcolorstages)
			if (shader->colorstages[0].src_blend >= 0) {
				glEnable(GL_BLEND);
				glBlendFunc(shader->colorstages[0].src_blend, shader->colorstages[0].dst_blend);
			} else {
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
			}
		else 
			glDisable(GL_BLEND);

		if (shader->numcolorstages) {
			if (shader->colorstages[0].numtextures)
				GL_Bind(shader->colorstages[0].texture[0]->texnum);

			if (shader->colorstages[0].alphatresh > 0) {
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
			}	
		}

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		GL_EnableMultitexture();
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor3f(sh_lightmapbright.value,sh_lightmapbright.value,sh_lightmapbright.value);

		GF3_sendSurfacesBase(surfs, numSurfaces, true);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		GL_DisableMultitexture();
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	}

	if (!shader->cull) {
		glEnable(GL_CULL_FACE);
	}

	glDisable(GL_ALPHA_TEST);
	glMatrixMode(GL_MODELVIEW);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_BLEND);

}

void GF3_sendSurfacesTA(msurface_t **surfs, int numSurfaces) {
	int i,j;
	glpoly_t *p;
	msurface_t *surf;
	shader_t *shader, *lastshader;
	float *v;
	qboolean cull;
	lastshader = NULL;

	cull = true;
	for (i=0; i<numSurfaces; i++) {
		surf = surfs[i];
		if (surf->visframe != r_framecount)
			continue;

		if (!(surf->flags & SURF_PPLIGHT))
			continue;

		p = surf->polys;
		
		shader = surfs[i]->shader->shader;

		//less state changes
		if (lastshader != shader) {

			if (!shader->cull) {
				glDisable(GL_CULL_FACE);
				cull = false;
			} else {
				if (!cull)
				glEnable(GL_CULL_FACE);
				cull = true;
			}
			//bind the correct texture
			GL_SelectTexture(GL_TEXTURE0_ARB);
				if (shader->numbumpstages > 0)
					GL_Bind(shader->bumpstages[0].texture[0]->texnum);
			GL_SelectTexture(GL_TEXTURE2_ARB);
				if (shader->numcolorstages > 0)
					GL_Bind(shader->colorstages[0].texture[0]->texnum);
			lastshader = shader;
		}

		//Note: texture coords out of begin-end are not a problem...
		qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB, &surf->tangent[0]);
		qglMultiTexCoord3fvARB(GL_TEXTURE2_ARB, &surf->binormal[0]);
		qglMultiTexCoord3fvARB(GL_TEXTURE3_ARB, &surf->plane->normal[0]);
		/*
		glBegin(GL_POLYGON);
		v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
		for (j=0; j<p->numverts; j++, v+= VERTEXSIZE) {
			qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
			glVertex3fv(&v[0]);
		}
		glEnd();
		*/
		glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT, &p->indecies[0]);
	}

	if (!cull)
	 glEnable(GL_CULL_FACE);
}


void GF3_sendSurfacesPlain(msurface_t **surfs, int numSurfaces) {
	int i,j;
	glpoly_t *p;
	msurface_t *surf;
	shader_t *shader, *lastshader;
	float *v;
	qboolean cull;
	lastshader = NULL;

	cull = true;

	for (i=0; i<numSurfaces; i++) {
		surf = surfs[i];
		
		if (surf->visframe != r_framecount)
			continue;

		if (!(surf->flags & SURF_PPLIGHT))
			continue;

		p = surf->polys;

		shader = surf->shader->shader;

		//less state changes
		if (lastshader != shader) {
			if (!shader->cull) {
				glDisable(GL_CULL_FACE);
				cull = false;
			} else {
				if (!cull)
				glEnable(GL_CULL_FACE);
				cull = true;
			}
			lastshader = shader;
		}
		/*		
		glBegin(GL_POLYGON);
		v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
		for (j=0; j<p->numverts; j++, v+= VERTEXSIZE) {
			//qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
			glVertex3fv(&v[0]);
		}
		glEnd();
		*/
		glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT, &p->indecies[0]);
	}

	if (!cull)
	 glEnable(GL_CULL_FACE);
}

void GF3_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs, int numSurfaces,const transform_t *tr) {

	glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (currentshadowlight->filtercube) {
		//draw attent into dest alpha
		GL_DrawAlpha();
		GL_EnableAttentShaderGF3(currentshadowlight->origin);
		
		glTexCoordPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
		GF3_sendSurfacesPlain(surfs,numSurfaces);

		GL_DisableAttentShaderGF3();
		GL_ModulateAlphaDrawColor();
	} else {
		GL_AddColor();
	}
	glColor3fv(&currentshadowlight->color[0]);

	GL_EnableSpecularShaderGF3(tr,currentshadowlight->origin,true);

	glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
	GF3_sendSurfacesTA(surfs,numSurfaces);
	GL_DisableDiffuseShaderGF3();

	GL_EnableDiffuseShaderGF3(tr,currentshadowlight->origin);
	GF3_sendSurfacesTA(surfs,numSurfaces);
	GL_DisableDiffuseShaderGF3();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

/*
void R_DrawWorldBumpedGF3() {

	vertexdef_t def;
	int numsurf;
	msurface_t *s;
	def.vertices = &globalVertexTable[0].position[0];
	def.vertexstride = sizeof(mmvertex_t);
	def.texcoords = &globalVertexTable[0].texture[0];
	def.texcoordstride = sizeof(mmvertex_t);
	def.tangents = NULL;
	def.tangentstride = 0;
	def.binormals = NULL;
	def.binormalstride = 0;
	def.normals = NULL;
	def.normalstride = 0;
	def.lightmapcoords = &globalVertexTable[0].lightmap[0];
	def.lightmapstride = sizeof(mmvertex_t);

	GF3_drawSurfaceListBump(&def, (msurface_t **)(&currentshadowlight->lightCmds[0]), currentshadowlight->numlightcmds-1);
/*
	numsurf = 0;
	s = first;
	while (s) {
		if (numsurf < 1024) {
			surfArray[numsurf] = s;
			numsurf++;
		} else {
			GF3_drawSurfaceListBase(&def, surfArray, numsurf, first->shader->shader);
			numsurf = 0;
		}
		s = s->texturechain;
	}

	if (numsurf) {
		GF3_drawSurfaceListBase(&def, surfArray, numsurf, first->shader->shader);
		numsurf = 0;
	}

	Con_Printf("Draw world ambient chain\n");*/
//}

/*
void R_DrawWorldGF3Diffuse(lightcmd_t *lightCmds) {

	int command, num, i;
	int lightPos = 0;
	vec3_t lightOr;
	msurface_t *surf;
	float		*v;
	shader_t	*s;//XYZ

	//support flickering lights
	VectorCopy(currentshadowlight->origin,lightOr);

	while (1) {
		
		command = lightCmds[lightPos++].asInt;
		if (command == 0) break; //end of list

		surf = lightCmds[lightPos++].asVoid;

		if (surf->visframe != r_framecount) {
			lightPos+=(4+surf->polys->numverts*(2+3));
			continue;
		}


		num = surf->polys->numverts;
		lightPos+=4;//skip color

		//XYZ
		s = surf->shader->shader;

		GL_SelectTexture(GL_TEXTURE0_ARB);
		if (s->numbumpstages > 0)
			GL_Bind(s->bumpstages[0].texture[0]->texnum);
		GL_SelectTexture(GL_TEXTURE2_ARB);
		if (s->numcolorstages > 0)
			GL_Bind(s->colorstages[0].texture[0]->texnum);

		glBegin(command);

		
		qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB, &surf->tangent[0]);
		qglMultiTexCoord3fvARB(GL_TEXTURE2_ARB, &surf->binormal[0]);
		qglMultiTexCoord3fvARB(GL_TEXTURE3_ARB, &surf->plane->normal[0]);
		
		//v = surf->polys->verts[0];
		v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
		for (i=0; i<num; i++, v+= VERTEXSIZE) {
			//skip attent texture coord.
			lightPos+=2;

			qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
			//qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB,
			//		      &lightCmds[lightPos].asFloat);
			lightPos+=3;
			glVertex3fv(&v[0]);
		}
		glEnd();
	}

	GL_SelectTexture(GL_TEXTURE0_ARB);
}

void R_DrawWorldGF3Specular(lightcmd_t *lightCmds) {

	int command, num, i;
	int lightPos = 0;
	vec3_t tsH,H;
	float* lightP;
	msurface_t *surf;
	float		*v;
	vec3_t lightDir;
	shader_t	*s;//XYZ

	//support flickering lights
	//VectorCopy(currentshadowlight->origin,lightOr);

	while (1) {
		
		command = lightCmds[lightPos++].asInt;
		if (command == 0) break; //end of list

		surf = lightCmds[lightPos++].asVoid;

		if (surf->visframe != r_framecount) {
			lightPos+=(4+surf->polys->numverts*(2+3));
			continue;
		}

		num = surf->polys->numverts;
		lightPos+=4;//skip color

		//XYZ
		s = surf->shader->shader;

		GL_SelectTexture(GL_TEXTURE0_ARB);
		if (s->numbumpstages > 0)
			GL_Bind(s->bumpstages[0].texture[0]->texnum);
		GL_SelectTexture(GL_TEXTURE2_ARB);
		if (s->numcolorstages > 0)
			GL_Bind(s->colorstages[0].texture[0]->texnum);

		glBegin(command);

		qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB, &surf->tangent[0]);
		qglMultiTexCoord3fvARB(GL_TEXTURE2_ARB, &surf->binormal[0]);
		qglMultiTexCoord3fvARB(GL_TEXTURE3_ARB, &surf->plane->normal[0]);

		//v = surf->polys->verts[0];
		v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
		for (i=0; i<num; i++, v+= VERTEXSIZE) {
			lightPos+=2;//skip texcoords
			lightP = &lightCmds[lightPos].asFloat;

                        VectorCopy(lightP, lightDir);
			VectorNormalize(lightDir);
			lightPos+=3;

			//calculate local H vector and put it into tangent space

			//r_origin = camera position
			//VectorSubtract(r_refdef.vieworg,v,H);
			//VectorNormalize(H);

			//put H in tangent space firste since lightDir (precalc) is already in tang space
//			if (surf->flags & SURF_PLANEBACK)	{
//				tsH[2] = -DotProduct(H,surf->plane->normal);
//			} else {
//				tsH[2] = DotProduct(H,surf->plane->normal);
//			}

//			tsH[1] = -DotProduct(H,surf->binormal);
//			tsH[0] = DotProduct(H,surf->tangent);

			//
			VectorAdd(lightDir,tsH,tsH);

			qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
			//qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB,&tsH[0]);
			glVertex3fv(&v[0]);
		}
		glEnd();
	}

	GL_SelectTexture(GL_TEXTURE0_ARB);
}
*//*
void R_DrawBrushGF3Diffuse(entity_t *e) {

	model_t	*model = e->model;
	msurface_t *surf;
	glpoly_t	*poly;
	int		i, j, count;
	brushlightinstant_t *ins = e->brushlightinstant;
	float	*v;
	shader_t *s; //XYZ

	count = 0;

	surf = &model->surfaces[model->firstmodelsurface];
	for (i=0; i<model->nummodelsurfaces; i++, surf++)
	{
		if (!ins->polygonVis[i]) continue;

		poly = surf->polys;

		//XYZ
		s = surf->shader->shader;

		GL_SelectTexture(GL_TEXTURE0_ARB);
		if (s->numbumpstages > 0)
			GL_Bind(s->bumpstages[0].texture[0]->texnum);
		GL_SelectTexture(GL_TEXTURE2_ARB);
		if (s->numcolorstages > 0)
			GL_Bind(s->colorstages[0].texture[0]->texnum);

		glBegin(GL_TRIANGLE_FAN);
		//v = poly->verts[0];
		v = (float *)(&globalVertexTable[poly->firstvertex]);
		for (j=0 ; j<poly->numverts ; j++, v+= VERTEXSIZE)
		{	
			qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
			qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB,&ins->tslights[count+j][0]);
			glVertex3fv(v);
		}
		glEnd();
		count+=surf->numedges;
	}	
}*/
/*
void R_DrawBrushGF3Specular(entity_t *e) {

	model_t	*model = e->model;
	msurface_t *surf;
	glpoly_t	*poly;
	int		i, j, count;
	brushlightinstant_t *ins = e->brushlightinstant;
	float	*v;
	shader_t	*s;//XYZ

	count = 0;

	surf = &model->surfaces[model->firstmodelsurface];
	for (i=0; i<model->nummodelsurfaces; i++, surf++)
	{
		if (!ins->polygonVis[i]) continue;

		poly = surf->polys;

		//XYZ
		s = surf->shader->shader;

		GL_SelectTexture(GL_TEXTURE0_ARB);
		if (s->numbumpstages > 0)
			GL_Bind(s->bumpstages[0].texture[0]->texnum);
		GL_SelectTexture(GL_TEXTURE2_ARB);
		if (s->numcolorstages > 0)
			GL_Bind(s->colorstages[0].texture[0]->texnum);

		glBegin(GL_TRIANGLE_FAN);
		//v = poly->verts[0];
		v = (float *)(&globalVertexTable[poly->firstvertex]);
		for (j=0 ; j<poly->numverts ; j++, v+= VERTEXSIZE)
		{	
			qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
			qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB,&ins->tshalfangles[count+j][0]);
			glVertex3fv(v);
		}
		glEnd();
		count+=surf->numedges;
	}	
}*/
/*
void R_DrawAliasBumpedGF3(aliashdr_t *paliashdr, aliasframeinstant_t *instant) {

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

	VectorCopy(currententity->origin,trans.origin);
	VectorCopy(currententity->angles,trans.angles);
	trans.scale[0] = trans.scale[1] = trans.scale[2] = 1.0f;
	GF3_drawTriangleListBump(&def,&linstant->indecies[0],linstant->numtris*3,paliashdr->shader, &trans);
	/*
	if (currentshadowlight->filtercube) {
		//draw attent into dest alpha
		GL_DrawAlpha();
		GL_EnableAttentShaderGF3(instant->lightinstant->lightpos);
		R_DrawAliasFrameWV(paliashdr,instant, false);
		GL_DisableAttentShaderGF3();
		GL_ModulateAlphaDrawColor();
	} else {
		GL_AddColor();
	}
	glColor3fv(&currentshadowlight->color[0]);

	GL_EnableSpecularShaderGF3(false,instant->lightinstant->lightpos,true);
	R_DrawAliasFrameGF3Specular(paliashdr,instant);
	GL_DisableDiffuseShaderGF3();

        GL_EnableDiffuseShaderGF3(false,instant->lightinstant->lightpos);
	R_DrawAliasFrameGF3Diffuse(paliashdr,instant);
	GL_DisableDiffuseShaderGF3();
	*/
//}

/*
void R_DrawAliasFrameGF3Diffuse (aliashdr_t *paliashdr, aliasframeinstant_t *instant)
{

	mtriangle_t *tris;
	fstvert_t	*texcoords;
	int anim;
	int			*indecies;
	shader_t	*s;
	aliaslightinstant_t *linstant = instant->lightinstant;

	tris = (mtriangle_t *)((byte *)paliashdr + paliashdr->triangles);
	texcoords = (fstvert_t *)((byte *)paliashdr + paliashdr->texcoords);

	//bind normal map
	anim = (int)(cl.time*10) & 3;
	s = paliashdr->shader;
	GL_SelectTexture(GL_TEXTURE0_ARB);
		if (s->numbumpstages > 0)
			GL_Bind(s->bumpstages[0].texture[0]->texnum);
	GL_SelectTexture(GL_TEXTURE2_ARB);
		if (s->numcolorstages > 0)
			GL_Bind(s->colorstages[0].texture[0]->texnum);
	
	indecies = (int *)((byte *)paliashdr + paliashdr->indecies);

	glVertexPointer(3, GL_FLOAT, 0, instant->vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	
	//qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	//glTexCoordPointer(3, GL_FLOAT, 0, linstant->tslights);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(3, GL_FLOAT, 0, instant->tangents);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	glTexCoordPointer(3, GL_FLOAT, 0, instant->binomials);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE3_ARB);
	glTexCoordPointer(3, GL_FLOAT, 0, instant->normals);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//glDrawElements(GL_TRIANGLES,paliashdr->numtris*3,GL_UNSIGNED_INT,indecies);
	glDrawElements(GL_TRIANGLES,linstant->numtris*3,GL_UNSIGNED_INT,&linstant->indecies[0]);

	if (sh_noshadowpopping.value) {
		glStencilFunc(GL_LEQUAL, 1, 0xffffffff);
		glDrawElements(GL_TRIANGLES,(paliashdr->numtris*3)-(linstant->numtris*3),GL_UNSIGNED_INT,&linstant->indecies[linstant->numtris*3]);
		glStencilFunc(GL_EQUAL, 0, 0xffffffff);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	GL_SelectTexture(GL_TEXTURE0_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}
*//*
void R_DrawAliasFrameGF3Specular (aliashdr_t *paliashdr, aliasframeinstant_t *instant)
{

	mtriangle_t *tris;
	fstvert_t	*texcoords;
	vec3_t		lightOr;
	int anim;
	int *indecies;
	shader_t	*s;
	aliaslightinstant_t *linstant = instant->lightinstant;

	tris = (mtriangle_t *)((byte *)paliashdr + paliashdr->triangles);
	texcoords = (fstvert_t *)((byte *)paliashdr + paliashdr->texcoords);

	VectorCopy(currentshadowlight->origin,lightOr);

	//bind normal map
	anim = (int)(cl.time*10) & 3;
	s = paliashdr->shader;
	GL_SelectTexture(GL_TEXTURE0_ARB);
		if (s->numbumpstages > 0)
			GL_Bind(s->bumpstages[0].texture[0]->texnum);
	GL_SelectTexture(GL_TEXTURE2_ARB);
		if (s->numcolorstages > 0)
			GL_Bind(s->colorstages[0].texture[0]->texnum);

	indecies = (int *)((byte *)paliashdr + paliashdr->indecies);

	glVertexPointer(3, GL_FLOAT, 0, instant->vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	//qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	//glTexCoordPointer(3, GL_FLOAT, 0, linstant->tshalfangles);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//to to correct self shadowing on alias models send the light vectors an extra time...
	
	//qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	//glTexCoordPointer(3, GL_FLOAT, 0, linstant->tslights);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	

	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(3, GL_FLOAT, 0, instant->tangents);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	glTexCoordPointer(3, GL_FLOAT, 0, instant->binomials);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE3_ARB);
	glTexCoordPointer(3, GL_FLOAT, 0, instant->normals);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);


	glDrawElements(GL_TRIANGLES,linstant->numtris*3,GL_UNSIGNED_INT,&linstant->indecies[0]);

	if (sh_noshadowpopping.value) {
		glStencilFunc(GL_LEQUAL, 1, 0xffffffff);
		//Con_Printf("%i backfacing tris\n",(paliashdr->numtris*3)-(linstant->numtris*3));
		glDrawElements(GL_TRIANGLES,(paliashdr->numtris*3)-(linstant->numtris*3),GL_UNSIGNED_INT,&linstant->indecies[linstant->numtris*3]);
		glStencilFunc(GL_EQUAL, 0, 0xffffffff);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(GL_TEXTURE0_ARB);
	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}
*/
/*
void R_DrawWorldBumpedGF3() {

	if (!currentshadowlight->visible)
		return;

	glDepthMask (0);
	glShadeModel (GL_SMOOTH);

	if (currentshadowlight->filtercube) {
		//draw attent into dest alpha
		GL_DrawAlpha();
		GL_EnableAttentShaderGF3(currentshadowlight->origin);
		R_DrawWorldWV(currentshadowlight->lightCmds, false);
		GL_DisableAttentShaderGF3();
		GL_ModulateAlphaDrawColor();
	} else {
		GL_AddColor();
	}
	glColor3fv(&currentshadowlight->color[0]);

	GL_EnableSpecularShaderGF3(true,currentshadowlight->origin,false);
	R_DrawWorldGF3Specular(currentshadowlight->lightCmds);
	GL_DisableDiffuseShaderGF3();

	GL_EnableDiffuseShaderGF3(true,currentshadowlight->origin);
	R_DrawWorldGF3Diffuse(currentshadowlight->lightCmds);
	GL_DisableDiffuseShaderGF3();

	glColor3f (1,1,1);
	glDisable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (1);
}
*//*
void R_DrawBrushBumpedGF3(entity_t *e) {

	if (currentshadowlight->filtercube) {
		//draw attent into dest alpha
		GL_DrawAlpha();
		GL_EnableAttentShaderGF3(((brushlightinstant_t *)e->brushlightinstant)->lightpos);
		R_DrawBrushWV(e, false);
		GL_DisableAttentShaderGF3();
		GL_ModulateAlphaDrawColor();
	} else {
		GL_AddColor();
	}
	glColor3fv(&currentshadowlight->color[0]);

	GL_EnableSpecularShaderGF3(false,((brushlightinstant_t *)e->brushlightinstant)->lightpos,false);
	R_DrawBrushGF3Specular(e);
	GL_DisableDiffuseShaderGF3();

        GL_EnableDiffuseShaderGF3(false,((brushlightinstant_t *)e->brushlightinstant)->lightpos);
	R_DrawBrushGF3Diffuse(e);
	GL_DisableDiffuseShaderGF3();
}
*/
/*
	Vertex programs 
*/


/*
  Thisone does not do anything too usefull, it just copyies some things around
  instead of sending the save coordinates for unit0/2 we send them once and copy
  them here, this saves some bandwith and is slightly faster.
*/ 
  char vpDiffuseGF3 [] = 
      "!!VP1.1 # Diffuse bumpmapping vetex program.\n"
	  "OPTION NV_position_invariant;"
      // Generates a necessary input for the diffuse bumpmapping registers
      // 
      // c[0]...c[3]      contains the modelview projection composite matrix
      // c[4]...c[7]      contains the texture matrix of unit 3
      // v[OPOS]          contains the per-vertex position
      // v[TEX1]          contains the per-vertex tangent space light vector
      // v[TEX0]          contains the per-vertex texture coordinate 0

      // o[HPOS]          output register for homogeneous position
      // o[TEX0]          output register for texture coordinate 0
      // o[TEX1]          output register for texture coordinate 1
      // o[TEX2]          output register for texture coordinate 2
      // o[TEX3]          output register for texture coordinate 3

      // Transform vertex to view-space

	  //dynamic calculation of light vector
	  "ADD R1, -v[OPOS], c[24];"
	  "DP4 R0.x, R1, R1;"
	  "RSQ R0.x, R0.x;"
	  "MUL R0, R0.x, R1;"

	  //convert to tangent space
	  "DP4 o[TEX1].x, R0, v[TEX1];"
	  "DP4 o[TEX1].y, R0, -v[TEX2];"
      "DP4 o[TEX1].z, R0, v[TEX3];"

	  // move light vector out
	  //"MOV   o[TEX1], v[TEX1];"

	  //copy tex coords of unit 0 to unit 2
	  "MOV   o[TEX0], v[TEX0];"
	  "MOV   o[TEX2], v[TEX0];"
	  "MOV   o[COL0], v[COL0];"

      // Transform vertex by texture matrix and copy to output
      "DP4   o[TEX3].x, v[OPOS], c[4];"
      "DP4   o[TEX3].y, v[OPOS], c[5];"
      "DP4   o[TEX3].z, v[OPOS], c[6];"
      "DP4   o[TEX3].w, v[OPOS], c[7];"

      "END";

  char vpSpecularAliasGF3 [] = 
      "!!VP1.1 # Diffuse bumpmapping vetex program.\n"
	  "OPTION NV_position_invariant;"
      // Generates a necessary input for the diffuse bumpmapping registers
      // 
      // c[0]...c[3]      contains the modelview projection composite matrix
      // c[4]...c[7]      contains the texture matrix of unit 3
      // v[OPOS]          contains the per-vertex position
      // v[TEX1]          contains the per-vertex tangent space light vector
      // v[TEX0]          contains the per-vertex texture coordinate 0

      // o[HPOS]          output register for homogeneous position
      // o[TEX0]          output register for texture coordinate 0
      // o[TEX1]          output register for texture coordinate 1
      // o[TEX2]          output register for texture coordinate 2
      // o[TEX3]          output register for texture coordinate 3

      // Transform vertex to view-space

	  //dynamic calculation of light vector
	  "ADD R1, -v[OPOS], c[24];"
	  "DP4 R0.x, R1, R1;"
	  "RSQ R0.x, R0.x;"
	  "MUL R0, R0.x, R1;"

	  //dynamic calculation of half angle vector
	  "ADD R2, -v[OPOS], c[25];"
	  "DP4 R3.x, R2, R2;"
	  "RSQ R3.x, R3.x;"
	  "MUL R3, R3.x, R2;"
	  "ADD R3, R0, R3;"

	  //put into tangent space
	  "DP4 o[TEX1].x, R3, v[TEX1];"
	  "DP4 o[TEX1].y, R3, -v[TEX2];"
      "DP4 o[TEX1].z, R3, v[TEX3];"

	  //copy tangent space light vector to color
	  //but we only convert z to tangent space as it's the only component we use
	  "DP4 R0.z, R0, v[TEX3];"
	  "MAD o[COL1], R0, c[20], c[20];"

	  //copy tex coords of unit 0 to unit 2
	  "MOV   o[TEX0], v[TEX0];"
	  "MOV   o[TEX2], v[TEX0];"
	  "MOV   o[COL0], v[COL0];"

      // Transform vertex by texture matrix and copy to output
      "DP4   o[TEX3].x, v[OPOS], c[4];"
      "DP4   o[TEX3].y, v[OPOS], c[5];"
      "DP4   o[TEX3].z, v[OPOS], c[6];"
      "DP4   o[TEX3].w, v[OPOS], c[7];"

      "END";
/*
	VectorSubtract(linstant->vieworg, instant->vertices[i], H);
	VectorNormalize(H);
	VectorAdd(lightDir,H,H);
*/
/*
	Only used for specular on alias models when noshadowpopping is enabled...
*//* 
  char vpSpecularAliasGF3 [] = 
      "!!VP1.1 # Diffuse bumpmapping vetex program.\n"
	  "OPTION NV_position_invariant;"
      // Generates a necessary input for the diffuse bumpmapping registers
      // 
      // c[0]...c[3]      contains the modelview projection composite matrix
      // c[4]...c[7]      contains the texture matrix of unit 3
      // v[OPOS]          contains the per-vertex position
      // v[TEX1]          contains the per-vertex tangent space light vector
      // v[TEX0]          contains the per-vertex texture coordinate 0
	  // v[TEX2]		  contains the per-vertex light vector

      // o[HPOS]          output register for homogeneous position
      // o[TEX0]          output register for texture coordinate 0
      // o[TEX1]          output register for texture coordinate 1
      // o[TEX2]          output register for texture coordinate 2
      // o[TEX3]          output register for texture coordinate 3

      // Transform vertex to view-space

      // Transform vertex by texture matrix and copy to output
      "DP4   o[TEX3].x, v[OPOS], c[4];"
      "DP4   o[TEX3].y, v[OPOS], c[5];"
      "DP4   o[TEX3].z, v[OPOS], c[6];"
      "DP4   o[TEX3].w, v[OPOS], c[7];"


	  "MOV   o[TEX0], v[TEX0];"

	  //range compress into secondary color
	  "MAD   o[COL1], v[TEX2], c[20], c[20];"

	  //copy tex coords of unit 0 to unit 2
	  "MOV   o[TEX2], v[TEX0];"

	  "MOV   o[TEX1], v[TEX1];"

	  "MOV   o[COL0], v[COL0];"

      "END";
*/

typedef struct allocchain_s {
	struct allocchain_s *next;
	char data[1];//variable sized
} allocchain_t;

static allocchain_t *allocChain = NULL;

void *GF3_getDriverMem(size_t size, drivermem_t hint) {
	allocchain_t *r = (allocchain_t *)malloc(size+sizeof(void *));
	r->next = allocChain;
	allocChain = r;
	return &r->data[0];
}

void GF3_freeAllDriverMem(void) {

	allocchain_t *r = allocChain;
	allocchain_t *next;

	while (r) {
		next = r->next;
		free(r);
		r = next;
	}
}

void GF3_freeDriver(void) {
	//nothing here...
}


void BUMP_InitGeforce3(void) {

	GLint errPos, errCode;
	const GLubyte *errString;

	if ( gl_cardtype != GEFORCE3 ) return;

	// Create the vertex program.
	qglGenProgramsNV( 1, &diffuse_program_object);

	qglLoadProgramNV( GL_VERTEX_PROGRAM_NV, diffuse_program_object,
					strlen(vpDiffuseGF3), (const GLubyte *) vpDiffuseGF3);

	qglGenProgramsNV( 1, &specularalias_program_object);

	qglLoadProgramNV( GL_VERTEX_PROGRAM_NV, specularalias_program_object,
					strlen(vpSpecularAliasGF3), (const GLubyte *) vpSpecularAliasGF3);

	if ( (errCode = glGetError()) != GL_NO_ERROR ) {
		errString = gluErrorString( errCode );
//		glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
		glGetIntegerv( GL_PROGRAM_ERROR_POSITION_NV, &errPos);
		Con_Printf("LoadVertexProgram: %s\n", errString);
		Con_Printf("error is located at line: %d\n", errPos);
		exit( -1 );
	} else {
		Con_Printf("VertexProgram loaded\n");
	}


    // Track the concatenation of the modelview and projection matrix in registers 0-3.
    qglTrackMatrixNV( GL_VERTEX_PROGRAM_NV, 0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV );

    // Track the texture unit 3 maxtix in registers 4-7
    qglTrackMatrixNV( GL_VERTEX_PROGRAM_NV, 4, GL_TEXTURE3_ARB, GL_IDENTITY_NV );

	//store 0.5 0.5 0.5 0.5 in register 8
	qglProgramParameter4fNV( GL_VERTEX_PROGRAM_NV, 20, 0.5, 0.5, 0.5, 0.5);

	//bind the correct stuff to the bump mapping driver
	gl_bumpdriver.drawSurfaceListBase = GF3_drawSurfaceListBase;
	gl_bumpdriver.drawSurfaceListBump = GF3_drawSurfaceListBump;
	gl_bumpdriver.drawTriangleListBase = GF3_drawTriangleListBase;
	gl_bumpdriver.drawTriangleListBump = GF3_drawTriangleListBump;
	gl_bumpdriver.getDriverMem = GF3_getDriverMem;
	gl_bumpdriver.freeAllDriverMem = GF3_freeAllDriverMem;
	gl_bumpdriver.freeDriver = GF3_freeDriver;
}
